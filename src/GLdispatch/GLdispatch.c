/*
 * Copyright (c) 2013, NVIDIA CORPORATION.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and/or associated documentation files (the
 * "Materials"), to deal in the Materials without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sublicense, and/or sell copies of the Materials, and to
 * permit persons to whom the Materials are furnished to do so, subject to
 * the following conditions:
 *
 * The above copyright notice and this permission notice shall be included
 * unaltered in all copies or substantial portions of the Materials.
 * Any additions, deletions, or changes to the original source files
 * must be clearly indicated in accompanying documentation.
 *
 * If only executable code is distributed, then the accompanying
 * documentation must state that "this software is based in part on the
 * work of the Khronos Group."
 *
 * THE MATERIALS ARE PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 * IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
 * CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 * MATERIALS OR THE USE OR OTHER DEALINGS IN THE MATERIALS.
 */

#include <string.h>
#include <pthread.h>
#include <dlfcn.h>

#include "trace.h"
#include "glvnd_list.h"
#include "GLdispatch.h"
#include "GLdispatchPrivate.h"
#include "stub.h"
#include "glvnd_pthread.h"
#include "app_error_check.h"

/*
 * Global current dispatch table list. We need this to fix up all current
 * dispatch tables whenever GetProcAddress() is called on a new function.
 * Accesses to this need to be protected by the dispatch lock.
 */
static struct glvnd_list currentDispatchList;

/*
 * Number of clients using GLdispatch.
 */
static int clientRefcount;

/*
 * The number of current contexts that GLdispatch is aware of
 */
static int numCurrentContexts;

/**
 * Private data for each API state.
 */
typedef struct __GLdispatchThreadStatePrivateRec {
    /// A pointer back to the API state.
    __GLdispatchThreadState *threadState;

    /// ID of the current vendor for this state
    int vendorID;

    /// The current (high-level) __GLdispatch table
    __GLdispatchTable *dispatch;
} __GLdispatchThreadStatePrivate;

typedef struct __GLdispatchProcEntryRec {
    char *procName;

    // Cached offset of this dispatch entry, retrieved from
    // _glapi_get_proc_offset()
    int offset;

    // The generation in which this dispatch entry was defined.
    // Used to determine whether a given dispatch table needs to
    // be fixed up with the right function address at this offset.
    int generation;

    // List handle
    struct glvnd_list entry;
} __GLdispatchProcEntry;

/*
 * List of valid extension procs which have been assigned prototypes. At make
 * current time, if the new context's generation is out-of-date, we iterate
 * through this list and fix up the new context with entrypoints with a greater
 * generation number. Accesses to this need to be protected by the dispatch
 * lock.
 */
static struct glvnd_list extProcList;

/*
 * Monotonically increasing integer describing the most up-to-date "generation"
 * of the dispatch table. Used to determine if a given table needs fixup.
 * Accesses to this need to be protected by the dispatch lock.
 *
 * Note: wrapping is theoretically an issue here, but shouldn't happen in
 * practice as it requires calling GetProcAddress() on 2^31-1 unique functions.
 * We'll run out of dispatch stubs long before then.
 */
static int latestGeneration;

/*
 * Dispatch stub list for entrypoint rewriting.
 */
static struct glvnd_list dispatchStubList;
static int nextDispatchStubID = 1;
static int localDispatchStubId = -1;

/*
 * Track the latest generation of the dispatch stub list so that vendor
 * libraries can determine when their copies of the stub offsets need to
 * be updated.
 *
 * Note: wrapping is theoretically an issue here, but encountering this
 * situation would require loading and unloading an API library that registers
 * its entrypoints with GLdispatch 2^63-1 times, so it is unlikely to be an
 * issue in practice.
 */
static GLint64 dispatchStubListGeneration;

/*
 * Used when generating new vendor IDs for GLdispatch clients.  Valid vendor
 * IDs must be non-zero.
 */
static int firstUnusedVendorID = 1;

/**
 * The key used to store the __GLdispatchThreadState for the current thread.
 */
static glvnd_key_t threadContextKey;

static void SetCurrentThreadState(__GLdispatchThreadState *threadState);
static void ThreadDestroyed(void *data);
static int RegisterStubCallbacks(const __GLdispatchStubPatchCallbacks *callbacks);


/*
 * The vendor ID of the current "owner" of the entrypoint code.  0 if
 * we are using the default libglvnd stubs.
 */
static int stubOwnerVendorID;

/*
 * The current set of patch callbacks being used, or NULL if using the
 * default libglvnd entrypoints.
 */
static const __GLdispatchPatchCallbacks *stubCurrentPatchCb;

static glvnd_thread_t firstThreadId = GLVND_THREAD_NULL_INIT;
static int isMultiThreaded = 0;

/*
 * The dispatch lock. This should be taken around any code that manipulates the
 * above global variables or makes calls to _glapi_get_proc_offset() or
 * _glapi_get_proc_offset().
 */
struct {
    glvnd_mutex_t lock;
    int isLocked;
} dispatchLock = { GLVND_MUTEX_INITIALIZER, 0 };

static inline void LockDispatch(void)
{
    __glvndPthreadFuncs.mutex_lock(&dispatchLock.lock);
    dispatchLock.isLocked = 1;
}

static inline void UnlockDispatch(void)
{
    dispatchLock.isLocked = 0;
    __glvndPthreadFuncs.mutex_unlock(&dispatchLock.lock);
}

#define CheckDispatchLocked() assert(dispatchLock.isLocked)

int __glDispatchGetABIVersion(void)
{
    return GLDISPATCH_ABI_VERSION;
}

#if defined(USE_ATTRIBUTE_CONSTRUCTOR)
void __attribute__ ((constructor)) __glDispatchOnLoadInit(void)
#else
void _init(void)
#endif
{
    // Here, we only initialize the pthreads imports. Everything else we'll
    // deal with in __glDispatchInit.
    glvndSetupPthreads();
    glvndAppErrorCheckInit();
}

void __glDispatchInit(void)
{
    LockDispatch();

    if (clientRefcount == 0) {
        // Initialize the GLAPI layer.
        _glapi_init();
        __glvndPthreadFuncs.key_create(&threadContextKey, ThreadDestroyed);

        glvnd_list_init(&extProcList);
        glvnd_list_init(&currentDispatchList);
        glvnd_list_init(&dispatchStubList);

        // Register GLdispatch's static entrypoints for rewriting
        localDispatchStubId = RegisterStubCallbacks(stub_get_patch_callbacks());
    }

    clientRefcount++;
    UnlockDispatch();
}

int __glDispatchNewVendorID(void)
{
    int vendorID;

    LockDispatch();
    vendorID = firstUnusedVendorID++;
    UnlockDispatch();

    return vendorID;
}

static void noop_func(void)
{
    // nop
}

/*!
 * Frees a dispatch table.
 */
static void FreeDispatchTable(__GLdispatchTable *dispatch)
{
    glvnd_list_del(&dispatch->entry);
    free(dispatch->table);
    free(dispatch);
}

/*!
 * Checks if a dispatch table is unreferenced, and frees it if it is.
 */
static void DispatchCheckDelete(__GLdispatchTable *dispatch)
{
    CheckDispatchLocked();
    if (dispatch->currentThreads <= 0 && dispatch->getProcAddress == NULL) {
        FreeDispatchTable(dispatch);
    }
}

/*!
 * Increments the refcount of a dispatch table.
 */
static void DispatchCurrentRef(__GLdispatchTable *dispatch)
{
    CheckDispatchLocked();
    dispatch->currentThreads++;
}

/*!
 * Decrements the refcount of a dispatch table, and then frees it if it's no
 * longer referenced.
 */
static void DispatchCurrentUnref(__GLdispatchTable *dispatch)
{
    CheckDispatchLocked();
    dispatch->currentThreads--;
    assert(dispatch->currentThreads >= 0);
    DispatchCheckDelete(dispatch);
}

/*
 * Fix up a dispatch table. Calls to this function must be protected by the
 * dispatch lock.
 */
static void FixupDispatchTable(__GLdispatchTable *dispatch)
{
    DBG_PRINTF(20, "dispatch=%p\n", dispatch);
    CheckDispatchLocked();

    __GLdispatchProcEntry *curProc;
    void *procAddr;
    void **tbl = (void **)dispatch->table;

    /*
     * For each proc in the extProcList, compare its gen# against that of
     * the context. If greater, then fix up the dispatch table to contain
     * the right entrypoint.
     * XXX optimization: could we assume that the list is sorted by generation
     * number and hence early out once we reach gen# <= the context's?
     */
    glvnd_list_for_each_entry(curProc, &extProcList, entry) {
        if (curProc->generation > dispatch->generation) {
            assert(curProc->offset != -1);
            assert(curProc->procName);

            if (dispatch->getProcAddress != NULL) {
                procAddr = (void*)(*dispatch->getProcAddress)(
                    curProc->procName,
                    dispatch->getProcAddressParam);
            } else {
                procAddr = NULL;
            }

            tbl[curProc->offset] = procAddr ? procAddr : (void *)noop_func;
            DBG_PRINTF(20, "extProc procName=%s, addr=%p, noop=%p\n",
                       curProc->procName, procAddr, noop_func);
        }
    }

    dispatch->generation = latestGeneration;
}

static __GLdispatchProcEntry *FindProcInList(const char *procName,
                                             struct glvnd_list *list)
{
    DBG_PRINTF(20, "%s\n", procName);
    __GLdispatchProcEntry *curProc;
    CheckDispatchLocked();
    glvnd_list_for_each_entry(curProc, list, entry) {
        if (!strcmp(curProc->procName, procName)) {
            DBG_PRINTF(20, "yes\n");
            return curProc;
        }
    }

    DBG_PRINTF(20, "no\n");
    return NULL;
}

PUBLIC __GLdispatchProc __glDispatchGetProcAddress(const char *procName)
{
    _glapi_proc addr;

    /*
     * We need to lock the dispatch before calling into glapi in order to
     * prevent races when retrieving the entrypoint stub.
     */
    LockDispatch();

    addr = _glapi_get_proc_address(procName);

    DBG_PRINTF(20, "addr=%p\n", addr);
    if (addr) {
        if (!FindProcInList(procName, &extProcList))
        {
            __GLdispatchTable *curDispatch;
            __GLdispatchProcEntry *pEntry = malloc(sizeof(*pEntry));
            pEntry->procName = strdup(procName);
            pEntry->offset = _glapi_get_proc_offset(procName);
            assert(pEntry->offset >= 0);

            /*
             * Bump the latestGeneration, then assign it to this proc.
             */
            pEntry->generation = ++latestGeneration;

            glvnd_list_add(&pEntry->entry, &extProcList);

            /*
             * Fixup any current dispatch tables to contain the right pointer
             * to this proc.
             */
            glvnd_list_for_each_entry(curDispatch, &currentDispatchList, entry) {
                if (curDispatch->currentThreads > 0) {
                    FixupDispatchTable(curDispatch);
                }
            }
        }
    }
    UnlockDispatch();

    return addr;
}

PUBLIC __GLdispatchTable *__glDispatchCreateTable(
        int vendorID,
        __GLdispatchGetProcAddressCallback getProcAddress, void *param)
{
    __GLdispatchTable *dispatch;

    if (getProcAddress == NULL) {
        assert(getProcAddress != NULL);
        return NULL;
    }

    dispatch = malloc(sizeof(__GLdispatchTable));
    if (dispatch == NULL) {
        return NULL;
    }

    dispatch->vendorID = vendorID;
    dispatch->generation = 0;
    dispatch->currentThreads = 0;
    dispatch->table = NULL;

    dispatch->getProcAddress = getProcAddress;
    dispatch->getProcAddressParam = param;

    LockDispatch();
    glvnd_list_add(&dispatch->entry, &currentDispatchList);
    UnlockDispatch();

    return dispatch;
}

PUBLIC void __glDispatchDestroyTable(__GLdispatchTable *dispatch)
{
    /*
     * It's possible that this dispatch table is still current to some thread.
     * In that case, we'll stub out the getProcAddress callback, but wait until
     * it's no longer used before we free it.
     */
    LockDispatch();
    dispatch->getProcAddress = NULL;
    dispatch->getProcAddressParam = NULL;
    DispatchCheckDelete(dispatch);
    UnlockDispatch();
}

PUBLIC void __glDispatchDestroyVendorTables(int vendorID)
{
    __GLdispatchTable *dispatch, *tmp;

    LockDispatch();
    glvnd_list_for_each_entry_safe(dispatch, tmp, &currentDispatchList, entry) {
        if (dispatch->vendorID == vendorID) {
            dispatch->getProcAddress = NULL;
            dispatch->getProcAddressParam = NULL;
            DispatchCheckDelete(dispatch);
        }
    }
    UnlockDispatch();
}

static struct _glapi_table
*CreateGLAPITable(__GLdispatchGetProcAddressCallback getProcAddress, void *param)
{
    size_t entries = _glapi_get_dispatch_table_size();
    struct _glapi_table *table = (struct _glapi_table *)
        calloc(1, entries * sizeof(void *));

    CheckDispatchLocked();

    if (table) {
        _glapi_init_table_from_callback(table,
                                        entries,
                                        getProcAddress,
                                        param);
    }

    return table;
}

static int CurrentEntrypointsSafeToUse(int vendorID)
{
    CheckDispatchLocked();
    return !stubOwnerVendorID || (vendorID == stubOwnerVendorID);
}

static inline int PatchingIsDisabledByEnvVar(void)
{
    static GLboolean inited = GL_FALSE;
    static GLboolean disallowPatch = GL_FALSE;

    CheckDispatchLocked();

    if (!inited) {
        char *disallowPatchStr = getenv("__GLVND_DISALLOW_PATCHING");
        if (disallowPatchStr) {
            disallowPatch = atoi(disallowPatchStr);
        } else if (glvndAppErrorCheckGetEnabled()) {
            // Entrypoint rewriting means skipping the dispatch table in
            // libGLdispatch, which would disable checking for calling OpenGL
            // functions without a context.
            disallowPatch = GL_TRUE;
        }
        inited = GL_TRUE;
    }

    return disallowPatch;
}

static inline int ContextIsCurrentInAnyOtherThread(void)
{
    int thisThreadsContext = !!__glDispatchGetCurrentThreadState();
    int otherContexts;

    CheckDispatchLocked();

    otherContexts = (numCurrentContexts - thisThreadsContext);
    assert(otherContexts >= 0);

    return !!otherContexts;
}

static int PatchingIsSafe(void)
{
    CheckDispatchLocked();

    /*
     * Can only patch entrypoints on supported TLS access models
     */
    if (glvnd_list_is_empty(&dispatchStubList)) {
        return 0;
    }

    if (PatchingIsDisabledByEnvVar()) {
        return 0;
    }

    if (ContextIsCurrentInAnyOtherThread()) {
        return 0;
    }

    return 1;
}

typedef struct __GLdispatchStubCallbackRec {
    __GLdispatchStubPatchCallbacks callbacks;
    int id;
    GLboolean isPatched;

    struct glvnd_list entry;
} __GLdispatchStubCallback;

/**
 * Does the same thing as __glDispatchRegisterStubCallbacks, but requires the
 * caller to already be holding the dispatch lock.
 *
 * This is used in __glDispatchInit to register the libGLdispatch's own stub
 * functions.
 */
int RegisterStubCallbacks(const __GLdispatchStubPatchCallbacks *callbacks)
{
    if (callbacks == NULL) {
        return -1;
    }

    __GLdispatchStubCallback *stub = malloc(sizeof(*stub));
    if (stub == NULL) {
        return -1;
    }

    memcpy(&stub->callbacks, callbacks, sizeof(__GLdispatchStubPatchCallbacks));
    stub->isPatched = GL_FALSE;

    stub->id = nextDispatchStubID++;
    glvnd_list_add(&stub->entry, &dispatchStubList);
    dispatchStubListGeneration++;

    return stub->id;
}

int __glDispatchRegisterStubCallbacks(const __GLdispatchStubPatchCallbacks *callbacks)
{
    int ret;
    LockDispatch();
    ret = RegisterStubCallbacks(callbacks);
    UnlockDispatch();
    return ret;
}

void __glDispatchUnregisterStubCallbacks(int stubId)
{
    __GLdispatchStubCallback *curStub, *tmpStub;
    if (stubId < 0) {
        return;
    }

    LockDispatch();

    glvnd_list_for_each_entry_safe(curStub, tmpStub, &dispatchStubList, entry) {
        if (curStub->id == stubId) {
            glvnd_list_del(&curStub->entry);
            free(curStub);
            break;
        }
    }

    dispatchStubListGeneration++;
    UnlockDispatch();
}

void UnregisterAllStubCallbacks(void)
{
    __GLdispatchStubCallback *curStub, *tmpStub;
    CheckDispatchLocked();

    glvnd_list_for_each_entry_safe(curStub, tmpStub, &dispatchStubList, entry) {
        glvnd_list_del(&curStub->entry);
        free(curStub);
    }

    dispatchStubListGeneration++;
}


/*
 * Attempt to patch entrypoints with the given patch function and vendor ID.
 * If the function pointers are NULL, then this attempts to restore the default
 * libglvnd entrypoints.
 *
 * Returns 1 on success, 0 on failure.
 */
static int PatchEntrypoints(
   const __GLdispatchPatchCallbacks *patchCb,
   int vendorID
)
{
    __GLdispatchStubCallback *stub;
    CheckDispatchLocked();

    if (!PatchingIsSafe()) {
        return 0;
    }

    if (patchCb == stubCurrentPatchCb) {
        // Entrypoints already using the requested patch; no need to do anything
        return 1;
    }

    if (stubCurrentPatchCb) {
        // Notify the previous vendor that it no longer owns these
        // entrypoints.
        if (stubCurrentPatchCb->releasePatch != NULL) {
            stubCurrentPatchCb->releasePatch();
        }
    }

    if (patchCb) {
        GLboolean anySuccess = GL_FALSE;

        glvnd_list_for_each_entry(stub, &dispatchStubList, entry) {
            if (patchCb->isPatchSupported(stub->callbacks.getStubType(),
                        stub->callbacks.getStubSize()))
            {
                if (stub->callbacks.startPatch()) {
                    if (patchCb->initiatePatch(stub->callbacks.getStubType(),
                                stub->callbacks.getStubSize(),
                                stub->callbacks.getPatchOffset)) {
                        stub->callbacks.finishPatch();
                        stub->isPatched = GL_TRUE;
                        anySuccess = GL_TRUE;
                    } else {
                        stub->callbacks.abortPatch();
                        stub->isPatched = GL_FALSE;
                    }
                }
            } else if (stub->isPatched) {
                // The vendor library can't patch these stubs, but they were
                // patched before. Restore them now.
                stub->callbacks.restoreFuncs();
                stub->isPatched = GL_FALSE;
            }
        }

        if (anySuccess) {
            stubCurrentPatchCb = patchCb;
            stubOwnerVendorID = vendorID;
        } else {
            stubCurrentPatchCb = NULL;
            stubOwnerVendorID = 0;
        }
    } else {
        // Restore the stubs to the default implementation
        glvnd_list_for_each_entry(stub, &dispatchStubList, entry) {
            if (stub->isPatched) {
                stub->callbacks.restoreFuncs();
                stub->isPatched = GL_FALSE;
            }
        }

        stubCurrentPatchCb = NULL;
        stubOwnerVendorID = 0;
    }

    return 1;
}

PUBLIC GLboolean __glDispatchMakeCurrent(__GLdispatchThreadState *threadState,
                                         __GLdispatchTable *dispatch,
                                         int vendorID,
                                         const __GLdispatchPatchCallbacks *patchCb)
{
    __GLdispatchThreadStatePrivate *priv;

    if (__glDispatchGetCurrentThreadState() != NULL) {
        assert(!"__glDispatchMakeCurrent called with a current API state\n");
        return GL_FALSE;
    }

    priv = (__GLdispatchThreadStatePrivate *) malloc(sizeof(__GLdispatchThreadStatePrivate));
    if (priv == NULL) {
        return GL_FALSE;
    }

    // We need to fix up the dispatch table if it hasn't been
    // initialized, or there are new dynamic entries which were
    // added since the last time make current was called.
    LockDispatch();

    // Patch if necessary
    PatchEntrypoints(patchCb, vendorID);

    // If the current entrypoints are unsafe to use with this vendor, bail out.
    if (!CurrentEntrypointsSafeToUse(vendorID)) {
        UnlockDispatch();
        free(priv);
        return GL_FALSE;
    }

    numCurrentContexts++;

    UnlockDispatch();

    /*
     * Update the API state with the new values.
     */
    priv->dispatch = NULL;
    priv->vendorID = vendorID;
    priv->threadState = threadState;
    threadState->priv = priv;

    /*
     * Set the current state in TLS.
     */
    SetCurrentThreadState(threadState);
    _glapi_set_current(NULL);

    __glDispatchSetDispatch(dispatch);

    return GL_TRUE;
}

PUBLIC void __glDispatchSetDispatch(__GLdispatchTable *dispatch)
{
    __GLdispatchThreadState *threadState;
    __GLdispatchTable *prevDispatch;

    threadState = __glDispatchGetCurrentThreadState();
    if (threadState == NULL) {
        assert(threadState != NULL);
        return;
    }

    if (dispatch != NULL && dispatch->vendorID != threadState->priv->vendorID) {
        assert(dispatch->vendorID == threadState->priv->vendorID);
        return;
    }

    prevDispatch = threadState->priv->dispatch;
    if (prevDispatch == dispatch) {
        return;
    }

    LockDispatch();

    if (dispatch != NULL) {
        if (!dispatch->table ||
            (dispatch->generation < latestGeneration)) {

            // Lazily create the dispatch table if we haven't already
            if (!dispatch->table) {
                dispatch->table = CreateGLAPITable(dispatch->getProcAddress,
                        dispatch->getProcAddressParam);
            }

            FixupDispatchTable(dispatch);
        }
        DispatchCurrentRef(dispatch);
        _glapi_set_current(dispatch->table);
    } else {
        _glapi_set_current(NULL);
    }
    threadState->priv->dispatch = dispatch;

    if (prevDispatch != NULL) {
        DispatchCurrentUnref(prevDispatch);
    }

    UnlockDispatch();
    return;
}

PUBLIC __GLdispatchTable *__glDispatchGetCurrentDispatch(void)
{
    __GLdispatchThreadState *threadState = __glDispatchGetCurrentThreadState();
    if (threadState != NULL) {
        return threadState->priv->dispatch;
    } else {
        return NULL;
    }
}

static void LoseCurrentInternal(__GLdispatchThreadState *curThreadState,
        GLboolean threadDestroyed)
{
    LockDispatch();
    // Try to restore the libglvnd default stubs, if possible.
    PatchEntrypoints(NULL, 0);

    if (curThreadState) {
        numCurrentContexts--;
        if (curThreadState->priv != NULL) {
            if (curThreadState->priv->dispatch != NULL) {
                DispatchCurrentUnref(curThreadState->priv->dispatch);
            }

            free(curThreadState->priv);
            curThreadState->priv = NULL;
        }
    }
    UnlockDispatch();

    if (!threadDestroyed) {
        SetCurrentThreadState(NULL);
        _glapi_set_current(NULL);
    }
}

PUBLIC void __glDispatchLoseCurrent(void)
{
    __GLdispatchThreadState *curThreadState = __glDispatchGetCurrentThreadState();
    if (curThreadState == NULL) {
        return;
    }
    LoseCurrentInternal(curThreadState, GL_FALSE);
}

__GLdispatchThreadState *__glDispatchGetCurrentThreadState(void)
{
    return (__GLdispatchThreadState *) __glvndPthreadFuncs.getspecific(threadContextKey);
}

void SetCurrentThreadState(__GLdispatchThreadState *threadState)
{
    __glvndPthreadFuncs.setspecific(threadContextKey, threadState);
}

/*
 * Handles resetting GLdispatch state after a fork.
 */
void __glDispatchReset(void)
{
    __GLdispatchTable *cur, *tmp;

    /* Reset the dispatch lock */
    __glvndPthreadFuncs.mutex_init(&dispatchLock.lock, NULL);
    dispatchLock.isLocked = 0;

    LockDispatch();
    /*
     * Clear out the current dispatch list.
     */

    glvnd_list_for_each_entry_safe(cur, tmp, &currentDispatchList, entry) {
        cur->currentThreads = 0;
        DispatchCheckDelete(cur);
    }
    UnlockDispatch();

    /* Clear GLAPI TLS entries. */
    SetCurrentThreadState(NULL);
    _glapi_set_current(NULL);
}

/*
 * Handles cleanup on library unload.
 */
void __glDispatchFini(void)
{
    LockDispatch();

    if (clientRefcount <= 0) {
        assert(clientRefcount > 0);
        UnlockDispatch();
        return;
    }

    clientRefcount--;

    if (clientRefcount == 0) {
        __GLdispatchProcEntry *curProc, *tmpProc;
        __GLdispatchTable *curDispatch, *tmpDispatch;

        /* This frees the dispatchStubList */
        UnregisterAllStubCallbacks();

        /*
         * Free any remaining dispatch tables. There might still be some if the
         * app still had a current context on some thread.
         */
        glvnd_list_for_each_entry_safe(curDispatch, tmpDispatch, &currentDispatchList, entry) {
            FreeDispatchTable(curDispatch);
        }

        /*
         * Clear out the getProcAddress lists.
         */
        glvnd_list_for_each_entry_safe(curProc, tmpProc, &extProcList, entry) {
            glvnd_list_del(&curProc->entry);
            free(curProc->procName);
            free(curProc);
        }

        __glvndPthreadFuncs.key_delete(threadContextKey);

        // Clean up GLAPI thread state
        _glapi_destroy();
    }

    UnlockDispatch();
}

void __glDispatchCheckMultithreaded(void)
{
    if (!__glvndPthreadFuncs.is_singlethreaded)
    {
        // Check to see if the current thread has a dispatch table assigned to
        // it, and if it doesn't, then plug in the no-op table.
        // This is a partial workaround to broken applications that try to call
        // OpenGL functions without a current context, without adding any
        // additional overhead to the dispatch stubs themselves. As long as the
        // thread calls at least one GLX function first, any OpenGL calls will
        // go to the no-op stubs instead of crashing.
        if (_glapi_get_current() == NULL) {
            // Calling _glapi_set_current(NULL) will plug in the no-op table.
            _glapi_set_current(NULL);
        }

        LockDispatch();
        if (!isMultiThreaded) {
            glvnd_thread_t tid = __glvndPthreadFuncs.self();
            if (__glvndPthreadFuncs.equal(firstThreadId, GLVND_THREAD_NULL)) {
                firstThreadId = tid;
            } else if (!__glvndPthreadFuncs.equal(firstThreadId, tid)) {
                isMultiThreaded = 1;
                _glapi_set_multithread();
            }
        }
        UnlockDispatch();

        if (stubCurrentPatchCb != NULL && stubCurrentPatchCb->threadAttach != NULL) {
            stubCurrentPatchCb->threadAttach();
        }
    }
}

void ThreadDestroyed(void *data)
{
    if (data != NULL) {
        __GLdispatchThreadState *threadState = (__GLdispatchThreadState *) data;
        LoseCurrentInternal(threadState, GL_TRUE);

        if (threadState->threadDestroyedCallback != NULL) {
            threadState->threadDestroyedCallback(threadState);
        }
    }
}

