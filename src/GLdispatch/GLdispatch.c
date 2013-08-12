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

#include "trace.h"
#include "glvnd_list.h"
#include "GLdispatch.h"
#include "GLdispatchPrivate.h"

/*
 * Global current dispatch table list. We need this to fix up all current
 * dispatch tables whenever GetProcAddress() is called on a new function.
 * Accesses to this need to be protected by the dispatch lock.
 */
static struct glvnd_list currentDispatchList;
static GLVNDPthreadFuncs *pthreadFuncs;

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
    pthreadFuncs->mutex_lock(&dispatchLock.lock);
    dispatchLock.isLocked = 1;
}

static inline void UnlockDispatch(void)
{
    dispatchLock.isLocked = 0;
    pthreadFuncs->mutex_unlock(&dispatchLock.lock);
}

#define CheckDispatchLocked() assert(dispatchLock.isLocked)

void __glDispatchInit(GLVNDPthreadFuncs *funcs)
{
    pthreadFuncs = funcs;
    // Call into GLAPI to see if we are multithreaded
    // TODO: fix GLAPI to use the pthread funcs provided here?
    _glapi_check_multithread();

    LockDispatch();
    glvnd_list_init(&newProcList);
    glvnd_list_init(&extProcList);
    glvnd_list_init(&currentDispatchList);
    UnlockDispatch();
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

    /*
     * TODO: For each proc in the newProcList, request a dispatch prototype from
     * the vendor library and plug it into glapi. If we succeed, move the proc
     * from the newProcList to the extProcList, and do some cleanup.
     */

    /*
     * TODO: For each proc in the extProcList, compare its gen# against that of
     * the context. If greater, then fix up the dispatch table to contain
     * the right entrypoint.
     */

    dispatch->generation = latestGeneration;
}

static __GLdispatchProcEntry *FindProcInList(const char *procName,
                                             struct glvnd_list *list)
{
    /* TODO */
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

GLint __glDispatchGetOffset(const char *procName)
{
    return _glapi_get_proc_offset(procName);
}

PUBLIC __GLdispatchTable *__glDispatchCreateTable(__GLgetProcAddressCallback getProcAddress,
                                                  __GLgetDispatchProtoCallback getDispatchProto,
                                                  __GLdestroyVendorDataCallback destroyVendorData,
                                                  void *vendorData)
{
    __GLdispatchTable *dispatch = malloc(sizeof(__GLdispatchTable));

    dispatch->generation = 0;
    dispatch->currentThreads = 0;
    dispatch->table = NULL;

    dispatch->getProcAddress = getProcAddress;
    dispatch->getDispatchProto = getDispatchProto;
    dispatch->destroyVendorData = destroyVendorData;

    dispatch->vendorData = vendorData;

    return dispatch;
}

PUBLIC void __glDispatchDestroyTable(__GLdispatchTable *dispatch)
{
    // NOTE this assumes the table is not current!
    // TODO: delete the global lists
    // TODO: this is currently unused...
    LockDispatch();
    dispatch->destroyVendorData(dispatch->vendorData);
    free(dispatch->table);
    free(dispatch);
    UnlockDispatch();
}

static struct _glapi_table
*CreateGLAPITable(__GLgetProcAddressCallback getProcAddress,
                  void *vendorData)
{
    size_t entries = _glapi_get_dispatch_table_size();
    struct _glapi_table *table = (struct _glapi_table *)
        calloc(1, entries * sizeof(void *));

    CheckDispatchLocked();

    if (table) {
        // TODO: call into glapi to initialize the table using our
        // getProcAddress callback
    }

    return table;
}

PUBLIC void __glDispatchMakeCurrent(__GLdispatchAPIState *apiState)
{
    __GLdispatchAPIState *curApiState = (__GLdispatchAPIState *)
        _glapi_get_current(CURRENT_API_STATE);
    __GLdispatchTable *dispatch = apiState->dispatch;
    __GLdispatchTable *curDispatch = curApiState ? curApiState->dispatch : NULL;

    // We need to fix up the dispatch table if it hasn't been
    // initialized, or there are new dynamic entries which were
    // added since the last time make current was called.
    LockDispatch();
    DBG_PRINTF(20, "dispatch=%p\n", dispatch);

    if (!dispatch->table ||
        (dispatch->generation < latestGeneration)) {

        // Lazily create the dispatch table if we haven't already
        if (!dispatch->table) {
            dispatch->table = CreateGLAPITable(dispatch->getProcAddress,
                                               dispatch->vendorData);
        }

        FixupDispatchTable(dispatch);
    }

    if (curDispatch != dispatch) {
        if (curDispatch) {
            DispatchCurrentUnref(curDispatch);
        }
        DispatchCurrentRef(dispatch);
    }

    /*
     * Set the current __GLdispatchTable and _glapi_table in TLS
     * we have to keep the dispatch lock until the _glapi_table
     * is set.
     * XXX: this would be cleaner if this used
     * _glapi_set_current(dispatch->table, CURRENT_DISPATCH)
     */
    _glapi_set_dispatch(dispatch->table);

    DBG_PRINTF(20, "done\n");
    UnlockDispatch();

    _glapi_set_current(apiState->context, CURRENT_CONTEXT);
    _glapi_set_current(apiState, CURRENT_API_STATE);
}

PUBLIC void __glDispatchLoseCurrent(void)
{
    __GLdispatchAPIState *curApiState =
        (__GLdispatchAPIState *)_glapi_get_current(CURRENT_API_STATE);

    DBG_PRINTF(20, "\n");

    if (curApiState) {
        LockDispatch();
        DispatchCurrentUnref(curApiState->dispatch);
        UnlockDispatch();
    }

    _glapi_set_current(NULL, CURRENT_API_STATE);
    _glapi_set_current(NULL, CURRENT_CONTEXT);
    _glapi_set_dispatch(NULL);
}
