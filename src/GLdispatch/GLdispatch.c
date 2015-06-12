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

#define _GNU_SOURCE 1

#include <string.h>
#include <pthread.h>
#include <dlfcn.h>

#include "trace.h"
#include "glvnd_list.h"
#include "GLdispatch.h"
#include "GLdispatchPrivate.h"
#include "stub.h"

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
 * Threading imports used for locking.
 */
GLVNDPthreadFuncs pthreadFuncs;

/*
 * The number of current contexts that GLdispatch is aware of
 */
static int numCurrentContexts;

typedef struct __GLdispatchProcEntryRec {
    char *procName;

    // Cached offset of this dispatch entry, retrieved from
    // _glapi_add_dispatch()
    int offset;

    // The generation in which this dispatch entry was defined.
    // Used to determine whether a given dispatch table needs to
    // be fixed up with the right function address at this offset.
    int generation;

    // List handle
    struct glvnd_list entry;
} __GLdispatchProcEntry;

/*
 * List of new dispatch procs which need to be given prototypes at make current
 * time. Accesses to this need to be protected by the dispatch lock.
 */
static struct glvnd_list newProcList;

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

/*
 * Dispatch stub list for entrypoint rewriting.
 */
static struct glvnd_list dispatchStubList;

/**
 * The key used to store the __GLdispatchAPIState for the current thread.
 */
static glvnd_key_t threadContextKey;

static void SetCurrentAPIState(__GLdispatchAPIState *apiState);
static void ThreadDestroyed(void *data);

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
 * above global variables or makes calls to _glapi_add_dispatch() or
 * _glapi_get_proc_offset().
 */
struct {
    glvnd_mutex_t lock;
    int isLocked;
} dispatchLock = { GLVND_MUTEX_INITIALIZER, 0 };

static inline void LockDispatch(void)
{
    pthreadFuncs.mutex_lock(&dispatchLock.lock);
    dispatchLock.isLocked = 1;
}

static inline void UnlockDispatch(void)
{
    dispatchLock.isLocked = 0;
    pthreadFuncs.mutex_unlock(&dispatchLock.lock);
}

#define CheckDispatchLocked() assert(dispatchLock.isLocked)

void __glDispatchInit(GLVNDPthreadFuncs *funcs)
{
    if (clientRefcount == 0) {
        /* Initialize pthreads imports */
        glvndSetupPthreads(RTLD_DEFAULT, &pthreadFuncs);

        // Initialize the GLAPI layer.
        _glapi_init();
        pthreadFuncs.key_create(&threadContextKey, ThreadDestroyed);

        LockDispatch();
        glvnd_list_init(&newProcList);
        glvnd_list_init(&extProcList);
        glvnd_list_init(&currentDispatchList);
        glvnd_list_init(&dispatchStubList);
        UnlockDispatch();

        // Register GLdispatch's static entrypoints for rewriting
        __glDispatchRegisterStubCallbacks(stub_get_offsets,
                                          stub_restore);
    }

    clientRefcount++;

    if (funcs) {
        // If the client needs a copy of these funcs, assign them now
        // XXX: instead of copying, we should probably just provide a pointer.
        // However, that's a bigger change...
        *funcs = pthreadFuncs;
    }
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

static void DispatchCurrentRef(__GLdispatchTable *dispatch)
{
    CheckDispatchLocked();
    dispatch->currentThreads++;
    if (dispatch->currentThreads == 1) {
        glvnd_list_add(&dispatch->entry, &currentDispatchList);
    }
}

static void DispatchCurrentUnref(__GLdispatchTable *dispatch)
{
    CheckDispatchLocked();
    dispatch->currentThreads--;
    if (dispatch->currentThreads == 0) {
        glvnd_list_del(&dispatch->entry);
    }
    assert(dispatch->currentThreads >= 0);
}

/*
 * Fix up a dispatch table. Calls to this function must be protected by the
 * dispatch lock.
 */
static void FixupDispatchTable(__GLdispatchTable *dispatch)
{
    DBG_PRINTF(20, "dispatch=%p\n", dispatch);
    CheckDispatchLocked();

    __GLdispatchProcEntry *curProc, *tmpProc;
    const char *function_names[2];
    void *procAddr;
    void **tbl = (void **)dispatch->table;

    /*
     * For each proc in the newProcList, attempt to create a dispatch stub in
     * glapi, then move the proc from the newProcList to the extProcList and do
     * some cleanup.
     */
    glvnd_list_for_each_entry_safe(curProc, tmpProc, &newProcList, entry) {

        DBG_PRINTF(20, "newProc procName=%s\n", curProc->procName);

        /*
         * _glapi_add_dispatch() has support for mapping multiple "equivalent"
         * function names (for example, glUniform1fv() and glUniform1fvARB()) to
         * one dispatch stub.  However, this becomes problematic in a
         * multi-vendor scenario, as vendors may have differing viewpoints on
         * whether two entrypoints are identical.  The GLX_ARB_create_context
         * extension, for example, defines "forward-compatible" contexts in
         * which calling the ARB version of a function is an error if the
         * non-ARB version is available.  For vendors that support this
         * extension, the ARB and non-ARB entrypoints are not identical.
         *
         * Hence, we only pass one function name per entrypoint to glapi.
         */
        function_names[0] = curProc->procName;
        function_names[1] = NULL;

        curProc->offset =
            _glapi_add_dispatch((const char * const *)function_names);
        DBG_PRINTF(20, "newProc offset=%d\n", curProc->offset);

        assert(curProc->offset != -1);

        glvnd_list_del(&curProc->entry);
        glvnd_list_add(&curProc->entry, &extProcList);

    }

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

            procAddr = (void*)(*dispatch->getProcAddress)(
                (const GLubyte *)curProc->procName, GL_TRUE);

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
    GLint offset;
    _glapi_proc addr;

    /*
     * We need to lock the dispatch before calling into glapi in order to
     * prevent races when retrieving the entrypoint stub.
     */
    LockDispatch();

    addr = _glapi_get_proc_address(procName);

    DBG_PRINTF(20, "addr=%p\n", addr);
    if (addr) {
        /*
         * Newly-generated entrypoints receive a temporary dispatch offset of
         * -1 until they are given a real offset later by _glapi_add_dispatch().
         * If this entrypoint hasn't already been added to the newProcList,
         * do so now, and fix up tables accordingly. Any procs landed in the
         * extProcList should already have a valid offset assigned to them,
         * and hence we don't need to search that list as well.
         */
        offset = _glapi_get_proc_offset(procName);
        if ((offset == -1) &&
            !FindProcInList(procName, &newProcList)) {
            __GLdispatchTable *curDispatch;
            __GLdispatchProcEntry *pEntry = malloc(sizeof(*pEntry));
            pEntry->procName = strdup(procName);
            pEntry->offset = -1; // To be assigned later

            /*
             * Bump the latestGeneration, then assign it to this proc.
             */
            pEntry->generation = ++latestGeneration;

            glvnd_list_add(&pEntry->entry, &newProcList);

            /*
             * Fixup any current dispatch tables to contain the right pointer
             * to this proc.
             */
            glvnd_list_for_each_entry(curDispatch, &currentDispatchList, entry) {
                FixupDispatchTable(curDispatch);
            }
        }
    }
    UnlockDispatch();

    return addr;
}

PUBLIC void __glDispatchSetEntry(__GLdispatchTable *dispatch,
                                 GLint offset,
                                 __GLdispatchProc addr)
{
    void **tbl;

    LockDispatch();
    if (dispatch) {
        tbl = (void **)dispatch->table;
        if (tbl) {
            tbl[offset] = addr;
        }
    }
    UnlockDispatch();
}

GLint __glDispatchGetOffset(const GLubyte *procName)
{
    return _glapi_get_proc_offset((const char *)procName);
}

PUBLIC __GLdispatchTable *__glDispatchCreateTable(__GLgetProcAddressCallback getProcAddress)
{
    __GLdispatchTable *dispatch = malloc(sizeof(__GLdispatchTable));

    dispatch->generation = 0;
    dispatch->currentThreads = 0;
    dispatch->table = NULL;

    dispatch->getProcAddress = getProcAddress;

    return dispatch;
}

PUBLIC void __glDispatchDestroyTable(__GLdispatchTable *dispatch)
{
    /*
     * XXX: Technically, dispatch->currentThreads should be 0 if we're calling
     * into this function, but buggy apps may unload libGLX without losing
     * current, in which case this won't be true when the dispatch table
     * is destroyed.
     */
    LockDispatch();
    free(dispatch->table);
    free(dispatch);
    UnlockDispatch();
}

static struct _glapi_table
*CreateGLAPITable(__GLgetProcAddressCallback getProcAddress)
{
    size_t entries = _glapi_get_dispatch_table_size();
    struct _glapi_table *table = (struct _glapi_table *)
        calloc(1, entries * sizeof(void *));

    CheckDispatchLocked();

    if (table) {
        _glapi_init_table_from_callback(table,
                                        entries,
                                        getProcAddress);
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
        }
        inited = GL_TRUE;
    }

    return disallowPatch;
}

static inline int ContextIsCurrentInAnyOtherThread(void)
{
    int thisThreadsContext = !!__glDispatchGetCurrentAPIState();
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
    if (!stub_allow_override()) {
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
    void (*get_offsets_func)(__GLdispatchGetOffsetHook func);
    void (*restore_func)(void);

    struct glvnd_list entry;
} __GLdispatchStubCallback;

void __glDispatchRegisterStubCallbacks(
    void (*get_offsets_func)(__GLdispatchGetOffsetHook func),
    void (*restore_func)(void)
)
{
    __GLdispatchStubCallback *stub = malloc(sizeof(*stub));
    stub->get_offsets_func = get_offsets_func;
    stub->restore_func = restore_func;

    LockDispatch();
    glvnd_list_add(&stub->entry, &dispatchStubList);
    dispatchStubListGeneration++;
    UnlockDispatch();
}

void __glDispatchUnregisterStubCallbacks(
    void (*get_offsets_func)(__GLdispatchGetOffsetHook func),
    void (*restore_func)(void)
)
{
    __GLdispatchStubCallback *curStub, *tmpStub;
    LockDispatch();

    glvnd_list_for_each_entry_safe(curStub, tmpStub, &dispatchStubList, entry) {
        if (get_offsets_func == curStub->get_offsets_func) {
            assert(restore_func == curStub->restore_func);

            glvnd_list_del(&curStub->entry);
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
        stubCurrentPatchCb->releasePatch();
    }

    if (patchCb) {
        GLboolean needOffsets;

        if (!patchCb->initiatePatch(entry_type,
                                    entry_stub_size,
                                    dispatchStubListGeneration,
                                    &needOffsets)) {
            // Patching unsupported on this platform
            return 0;
        }

        if (needOffsets) {
            // Fetch offsets for this vendor
            glvnd_list_for_each_entry(stub, &dispatchStubList, entry) {
                stub->get_offsets_func(patchCb->getOffsetHook);
            }
        }

        patchCb->finalizePatch();

        stubCurrentPatchCb = patchCb;
        stubOwnerVendorID = vendorID;
    } else {
        // Restore the stubs to the default implementation
        glvnd_list_for_each_entry(stub, &dispatchStubList, entry) {
            stub->restore_func();
        }

        stubCurrentPatchCb = NULL;
        stubOwnerVendorID = 0;
    }

    return 1;
}

PUBLIC GLboolean __glDispatchMakeCurrent(__GLdispatchAPIState *apiState,
                                         __GLdispatchTable *dispatch,
                                         int vendorID,
                                         const __GLdispatchPatchCallbacks *patchCb)
{
    __GLdispatchAPIState *curApiState = __glDispatchGetCurrentAPIState();
    __GLdispatchTable *curDispatch;

    curDispatch = curApiState ? curApiState->dispatch : NULL;

    // We need to fix up the dispatch table if it hasn't been
    // initialized, or there are new dynamic entries which were
    // added since the last time make current was called.
    LockDispatch();

    // Patch if necessary
    PatchEntrypoints(patchCb, vendorID);

    // If the current entrypoints are unsafe to use with this vendor, bail out.
    if (!CurrentEntrypointsSafeToUse(vendorID)) {
        UnlockDispatch();
        return GL_FALSE;
    }

    if (!dispatch->table ||
        (dispatch->generation < latestGeneration)) {

        // Lazily create the dispatch table if we haven't already
        if (!dispatch->table) {
            dispatch->table = CreateGLAPITable(dispatch->getProcAddress);
        }

        FixupDispatchTable(dispatch);
    }

    if (curDispatch != dispatch) {
        if (curDispatch) {
            DispatchCurrentUnref(curDispatch);
        }
        DispatchCurrentRef(dispatch);
    }

    if (!curApiState) {
        numCurrentContexts++;
    }

    UnlockDispatch();

    /*
     * Update the API state with the new values.
     */
    apiState->dispatch = dispatch;
    apiState->vendorID = vendorID;

    /*
     * Set the current state in TLS.
     */
    SetCurrentAPIState(apiState);
    _glapi_set_dispatch(dispatch->table);

    return GL_TRUE;
}

static void LoseCurrentInternal(__GLdispatchAPIState *curApiState,
        GLboolean threadDestroyed)
{
    LockDispatch();
    // Try to restore the libglvnd default stubs, if possible.
    PatchEntrypoints(NULL, 0);

    if (curApiState) {
        numCurrentContexts--;
    }
    UnlockDispatch();

    if (curApiState) {
        LockDispatch();
        DispatchCurrentUnref(curApiState->dispatch);
        UnlockDispatch();

        curApiState->dispatch = NULL;
        curApiState->vendorID = -1;
    }

    if (!threadDestroyed) {
        SetCurrentAPIState(NULL);
        _glapi_set_dispatch(NULL);
    }
}

PUBLIC void __glDispatchLoseCurrent(void)
{
    __GLdispatchAPIState *curApiState = __glDispatchGetCurrentAPIState();
    if (curApiState == NULL) {
        return;
    }
    LoseCurrentInternal(curApiState, GL_FALSE);
}

__GLdispatchAPIState *__glDispatchGetCurrentAPIState(void)
{
    return (__GLdispatchAPIState *) pthreadFuncs.getspecific(threadContextKey);
}

void SetCurrentAPIState(__GLdispatchAPIState *apiState)
{
    pthreadFuncs.setspecific(threadContextKey, apiState);
}

/*
 * Handles resetting GLdispatch state after a fork.
 */
void __glDispatchReset(void)
{
    __GLdispatchTable *cur, *tmp;

    /* Reset the dispatch lock */
    pthreadFuncs.mutex_init(&dispatchLock.lock, NULL);
    dispatchLock.isLocked = 0;

    LockDispatch();
    /*
     * Clear out the current dispatch list.
     */

    glvnd_list_for_each_entry_safe(cur, tmp, &currentDispatchList, entry) {
        cur->currentThreads = 0;
        glvnd_list_del(&cur->entry);
    }
    UnlockDispatch();

    /* Clear GLAPI TLS entries. */
    SetCurrentAPIState(NULL);
    _glapi_set_dispatch(NULL);
}

/*
 * Handles cleanup on library unload.
 */
void __glDispatchFini(void)
{
    __GLdispatchProcEntry *curProc, *tmpProc;
    assert(clientRefcount > 0);

    clientRefcount--;

    LockDispatch();
    /* This frees the dispatchStubList */
    UnregisterAllStubCallbacks();

    /* 
     * Before we get here, client libraries should
     * have cleared out the current dispatch list.
     */
    assert(glvnd_list_is_empty(&currentDispatchList));

    /*
     * Clear out the getProcAddress lists.
     */
    glvnd_list_for_each_entry_safe(curProc, tmpProc, &newProcList, entry) {
        glvnd_list_del(&curProc->entry);
        free(curProc->procName);
        free(curProc);
    }

    glvnd_list_for_each_entry_safe(curProc, tmpProc, &extProcList, entry) {
        // XXX: is there any glapi-specific cleanup that needs to happen
        // here?
        glvnd_list_del(&curProc->entry);
        free(curProc->procName);
        free(curProc);
    }

    UnlockDispatch();

    // Clean up GLAPI thread state
    _glapi_destroy();
}

void __glDispatchCheckMultithreaded(void)
{
    if (!pthreadFuncs.is_singlethreaded)
    {
        LockDispatch();
        if (!isMultiThreaded) {
            glvnd_thread_t tid = pthreadFuncs.self();
            if (pthreadFuncs.equal(firstThreadId, GLVND_THREAD_NULL)) {
                firstThreadId = tid;
            } else if (!pthreadFuncs.equal(firstThreadId, tid)) {
                isMultiThreaded = 1;
                _glapi_set_multithread();
            }
        }
        UnlockDispatch();
    }
}

void ThreadDestroyed(void *data)
{
    if (data != NULL) {
        __GLdispatchAPIState *apiState = (__GLdispatchAPIState *) data;
        LoseCurrentInternal(apiState, GL_TRUE);

        if (apiState->threadDestroyedCallback != NULL) {
            apiState->threadDestroyedCallback(apiState);
        }
    }
}

