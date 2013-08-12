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

static GLVNDPthreadFuncs *pthreadFuncs;

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
    // TODO: Call into GLAPI to see if we are multithreaded
    // TODO: fix GLAPI to use the pthread funcs provided here?

}

PUBLIC __GLdispatchProc __glDispatchGetProcAddress(const char *procName)
{
    _glapi_proc addr;

    /*
     * We need to lock the dispatch before calling into glapi in order to
     * prevent races when retrieving the entrypoint stub.
     */
    LockDispatch();

    /* TODO: get the addr from glapi */
    addr = NULL;

    DBG_PRINTF(20, "addr=%p\n", addr);
    if (addr) {
        /*
         * TODO: fixup dispatch tables to contain the right vendor entrypoints
         * for this proc.
         */
    }
    UnlockDispatch();

    return addr;
}

PUBLIC void __glDispatchSetEntry(__GLdispatchTable *dispatch,
                                 GLint offset,
                                 __GLdispatchProc addr)
{
    /* TODO: set the dispatch table proc at <offset> to <addr> */
}

GLint __glDispatchGetOffset(const char *procName)
{
    /* TODO: get the offset from glapi */
    return -1;
}

PUBLIC __GLdispatchTable *__glDispatchCreateTable(__GLgetProcAddressCallback getProcAddress,
                                                  __GLgetDispatchProtoCallback getDispatchProto,
                                                  __GLdestroyVendorDataCallback destroyVendorData,
                                                  void *vendorData)
{
    __GLdispatchTable *dispatch = malloc(sizeof(__GLdispatchTable));

    dispatch->getProcAddress = getProcAddress;
    dispatch->getDispatchProto = getDispatchProto;
    dispatch->destroyVendorData = destroyVendorData;

    dispatch->vendorData = vendorData;

    return dispatch;
}

PUBLIC void __glDispatchDestroyTable(__GLdispatchTable *dispatch)
{
    // NOTE this assumes the table is not current!
    // TODO: this is currently unused...
    LockDispatch();
    free(dispatch);
    UnlockDispatch();
}

PUBLIC void __glDispatchMakeCurrent(__GLdispatchAPIState *apiState)
{
    // We need to fix up the dispatch table if it hasn't been
    // initialized, or there are new dynamic entries which were
    // added since the last time make current was called.
    LockDispatch();
    DBG_PRINTF(20, "dispatch=%p\n", dispatch);

    /*
     * TODO: fixup the dispatch table if necessary
     */

    /*
     * TODO: track this dispatch table in our global list of current tables
     */

    /*
     * TODO: Set the current __GLdispatchTable and _glapi_table in TLS
     */
    UnlockDispatch();

    /*
     * TODO: Set other current state in TLS
     */

}

PUBLIC void __glDispatchLoseCurrent(void)
{
    __GLdispatchAPIState *curApiState =
        (__GLdispatchAPIState *)_glapi_get_current(CURRENT_API_STATE);

    DBG_PRINTF(20, "\n");

    if (curApiState) {
        LockDispatch();
        /*
         * TODO: remove the dispatch table from our global list
         */
        UnlockDispatch();
    }

    /*
     * TODO: set TLS entries to NULL
     */
}
