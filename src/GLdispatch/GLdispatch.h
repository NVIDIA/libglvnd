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

#if !defined(__GL_DISPATCH_H__)
#define __GL_DISPATCH_H__

#include "glheader.h"
#include "compiler.h"
#include "glvnd_pthread.h"
#include "GLdispatchABI.h"

/*!
 * \defgroup gldispatch core GL/GLES dispatch and TLS module
 *
 * GLdispatch is a thin wrapper around Mesa's mapi/glapi dispatch table
 * implementation which does some bookkeeping to simplify dispatch table
 * management. API libraries use this library to retrieve dispatch stubs and fix
 * up the dispatch table at make current time to point to the appropriate vendor
 * library entrypoints.
 */

/*
 * XXX: Kludge to export the get current function directly from glapi for
 * performance reasons.
 */
#define CURRENT_CONTEXT     1  // GLAPI_CURRENT_CONTEXT
#define CURRENT_API_STATE   2  // GLAPI_CURRENT_USER1

PUBLIC void *_glapi_get_current(int index);

/* Namespaces for API state */
enum {
    GLDISPATCH_API_GLX,
    GLDISPATCH_API_EGL
};

/*!
 * Generic API state structure. The window system binding API libraries subclass
 * from this structure to track API-library specific current state (e.g.
 * current drawables). There is one API state for each combination of (winsys
 * library, thread that has had a context current). The winsys library is
 * responsible for tracking, allocating, and freeing its API states. Though
 * each thread owns an API state within the winsys library, only one API state
 * may be "current" at a time (the API state of the winsys binding which has
 * a context current). This is done to conserve TLS space.
 */
typedef struct __GLdispatchAPIStateRec {
    /*************************************************************************
     * Winsys-managed variables: fixed for lifetime of state
     *************************************************************************/

    /*!
     * Namespace of the state: either API_GLX or API_EGL
     */
    int tag;

    /*!
     * Unique identifier for the state within the namespace. Usually (pointer
     * to) thread id
     */
    void *id;

    /*************************************************************************
     * GLdispatch-managed variables: Modified by MakeCurrent()
     *************************************************************************/

    /*!
     * ID of the current vendor for this state
     */
    int vendorID;

    /*!
     * The current (high-level) __GLdispatch table
     */
    __GLdispatchTable *dispatch;

    /*!
     * The current (vendor-specific) GL context
     */
    void *context;
} __GLdispatchAPIState;

/*!
 * Offset hook type used by GLdispatch wrapper libraries to implement entrypoint
 * rewriting.
 */
typedef void (*__GLdispatchGetOffsetHook)(void *(*lookupStubOffset)(const char *));

/*!
 * Initialize GLdispatch with pthreads functions needed for locking.
 */
PUBLIC void __glDispatchInit(GLVNDPthreadFuncs *funcs);

/*!
 * This returns a process-unique ID that is suitable for use with a new GL
 * vendor.
 */
PUBLIC int __glDispatchNewVendorID(void);

/*!
 * Get a dispatch stub suitable for returning to the application from
 * GetProcAddress().
 */
PUBLIC __GLdispatchProc __glDispatchGetProcAddress(const char *procName);

/*!
 * Create a new dispatch table in GLdispatch. This reference hangs off the
 * client GLX or EGL context, and is passed into GLdispatch during make current.
 * A dispatch table is owned by a particular vendor.
 *
 * \param [in] getProcAddress a vendor library callback GLdispatch can use to
 * query addresses of functions from the vendor. This callback also takes
 * a pointer to vendor-private data.
 * \param [in] destroyVendorData a vendor library callback to destroy private
 * data when the dispatch table is destroyed.
 * \param [in] vendorData a pointer to vendor library private data, which can
 * be used by the getProcAddress callback.
 */
PUBLIC __GLdispatchTable *__glDispatchCreateTable(
    __GLgetProcAddressCallback getProcAddress
);

/*!
 * Destroy a dispatch table in GLdispatch.
 */
PUBLIC void __glDispatchDestroyTable(__GLdispatchTable *dispatch);

/*!
 * This makes the given API state current, and assigns this API state
 * the passed-in current dispatch table, context, and vendor ID.
 *
 * This returns GL_FALSE if the make current operation failed, and GL_TRUE
 * if it succeeded.
 */
PUBLIC GLboolean __glDispatchMakeCurrent(__GLdispatchAPIState *apiState,
                                         __GLdispatchTable *dispatch,
                                         void *context,
                                         int vendorID);

/*!
 * This makes the NOP dispatch table current and sets the current context and
 * API state to NULL.
 */
PUBLIC void __glDispatchLoseCurrent(void);

/*!
 * This gets the current (opaque) API state pointer. If the pointer is
 * NULL, no context is current, otherwise the contents of the pointer depends on
 * which client API owns the context (EGL or GLX).
 *
 * This currently just hijacks _glapi_{set,get}_context() for this purpose.
 */
static inline __GLdispatchAPIState *__glDispatchGetCurrentAPIState(void)
{
    return _glapi_get_current(CURRENT_API_STATE);
}

/*!
 * This gets the current (opaque) vendor library context pointer. If the pointer
 * is NULL, no context is current, otherwsise the contents of the pointer
 * depends on the vendor whose context is current.
 */
static inline void *__glDispatchGetCurrentContext(void)
{
    return _glapi_get_current(CURRENT_CONTEXT);
}

/*!
 * This gets the "offset" of the given entrypoint in the GL dispatch table
 * structure, or -1 if there was an error.  This is technically an opaque handle
 * which can be passed into __glDispatchSetEntry() later, but in practice
 * describes a real offset.  If the call succeeds, the offset remains valid for
 * the lifetime of libglvnd for all GL dispatch tables used by libglvnd.
 */
PUBLIC GLint __glDispatchGetOffset(const GLubyte *procName);

/*!
 * This sets the dispatch table entry given by <offset> to the entrypoint
 * address given by <addr>.
 */
PUBLIC void __glDispatchSetEntry(__GLdispatchTable *dispatch,
                                 GLint offset, __GLdispatchProc addr);

/*!
 * This registers stubs with GLdispatch to be overwritten if a vendor library
 * explicitly requests custom entrypoint code.  This is used by the wrapper
 * interface libraries.
 */
PUBLIC void __glDispatchRegisterStubCallbacks(
    void (*get_offsets_func)(__GLdispatchGetOffsetHook func),
    void (*restore_func)(void)
);

/*!
 * This unregisters the GLdispatch stubs, and performs any necessary cleanup.
 */
PUBLIC void __glDispatchUnregisterStubCallbacks(
    void (*get_offsets_func)(__GLdispatchGetOffsetHook func),
    void (*restore_func)(void)
);

#endif
