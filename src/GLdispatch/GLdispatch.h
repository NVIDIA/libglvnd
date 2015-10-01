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
#include "GLdispatchABI.h"

/*!
 * The current version of the ABI between libGLdispatch and the window system
 * libraries.
 *
 * \see __glDispatchGetABIVersion
 */
#define GLDISPATCH_ABI_VERSION 0

/*!
 * \defgroup gldispatch core GL/GLES dispatch and TLS module
 *
 * GLdispatch is a thin wrapper around Mesa's mapi/glapi dispatch table
 * implementation which does some bookkeeping to simplify dispatch table
 * management. API libraries use this library to retrieve dispatch stubs and fix
 * up the dispatch table at make current time to point to the appropriate vendor
 * library entrypoints.
 */

/* Namespaces for API state */
enum {
    GLDISPATCH_API_GLX,
    GLDISPATCH_API_EGL
};

/*!
 * This opaque structure describes the core GL dispatch table.
 */
typedef struct __GLdispatchTableRec __GLdispatchTable;

typedef void (*__GLdispatchProc)(void);

typedef void *(*__GLgetProcAddressCallback)(const char *procName, void *param);

/**
 * An opaque structure used for internal API state data.
 */
struct __GLdispatchAPIStatePrivateRec;

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

    /**
     * A callback that is called when a thread that has a current context
     * terminates.
     *
     * This is called after libGLdispatch handles its cleanup, so
     * __glDispatchGetCurrentAPIState will return NULL. The API state is passed
     * as a parameter instead.
     *
     * The callback should not call __glDispatchMakeCurrent or
     * __glDispatchLoseCurrent.
     *
     * \param apiState The API state passed to __glDispatchMakeCurrent.
     */
    void (*threadDestroyedCallback)(struct __GLdispatchAPIStateRec *apiState);

    /*************************************************************************
     * GLdispatch-managed variables: Modified by MakeCurrent()
     *************************************************************************/

    /*!
     * Private data for this API state.
     *
     * This structure is assigned in \c __glDispatchMakeCurrent, and freed in
     * \c __glDispatchLoseCurrent.
     *
     * The value of this pointer, if any, is an internal detail of
     * libGLdispatch. The window system library should just ignore it.
     */
    struct __GLdispatchAPIStatePrivateRec *priv;
} __GLdispatchAPIState;

/*!
 * Gets the version number for the ABI between libGLdispatch and the
 * window-system libraries.
 *
 * The current version (which libGLX checks for) is \c GLDISPATCH_ABI_VERSION.
 *
 * Note that this only defines the interface between the libGLdispatch and a
 * window-system library such as libGLX. The interface between libGLX and a
 * vendor library still uses \c GLX_VENDOR_ABI_VERSION for its version number.
 *
 * This function can (and generally should) be called before
 * \c __glDispatchInit.
 */
PUBLIC int __glDispatchGetABIVersion(void);

/*!
 * Initialize GLdispatch with pthreads functions needed for locking.
 */
PUBLIC void __glDispatchInit(void);

/*!
 * Tears down GLdispatch state.
 */
PUBLIC void __glDispatchFini(void);

/*!
 * Called when the client library has detected a fork, and GLdispatch state
 * needs to be reset to handle the fork.
 */
PUBLIC void __glDispatchReset(void);

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
 * \param[in] getProcAddress a vendor library callback GLdispatch can use to
 * query addresses of functions from the vendor. This callback also takes
 * a pointer to caller-private data.
 * \param[in] param A pointer to pass to \p getProcAddress.
 */
PUBLIC __GLdispatchTable *__glDispatchCreateTable(
    __GLgetProcAddressCallback getProcAddress,
    void *param
);

/*!
 * Destroy a dispatch table in GLdispatch.
 */
PUBLIC void __glDispatchDestroyTable(__GLdispatchTable *dispatch);

/*!
 * This makes the given API state current, and assigns this API state
 * the passed-in current dispatch table and vendor ID.
 *
 * When this function is called, the current thread must not already have an
 * API state. To switch between two API states, first release the old API state
 * by calling \c __glDispatchLoseCurrent.
 *
 * If patchCb is not NULL, GLdispatch will attempt to overwrite its
 * entrypoints (and the entrypoints of any loaded interface libraries)
 * using the provided callbacks.  If patchCb is NULL and the entrypoints
 * have been previously overwritten, GLdispatch will attempt to restore
 * the default libglvnd entrypoints.
 *
 * This returns GL_FALSE if the make current operation failed, and GL_TRUE
 * if it succeeded.
 */
PUBLIC GLboolean __glDispatchMakeCurrent(__GLdispatchAPIState *apiState,
                                         __GLdispatchTable *dispatch,
                                         int vendorID,
                                         const __GLdispatchPatchCallbacks *patchCb);

/*!
 * This makes the NOP dispatch table current and sets the current API state to
 * NULL.
 *
 * A window system library should only call this if it created the current API
 * state. That is, if libGLX should not attempt to release an EGL context or
 * vice-versa.
 */
PUBLIC void __glDispatchLoseCurrent(void);

/*!
 * This gets the current (opaque) API state pointer. If the pointer is
 * NULL, no context is current, otherwise the contents of the pointer depends on
 * which client API owns the context (EGL or GLX).
 */
PUBLIC __GLdispatchAPIState *__glDispatchGetCurrentAPIState(void);

/**
 * Checks to see if multiple threads are being used. This should be called
 * periodically from places like glXMakeCurrent.
 */
PUBLIC void __glDispatchCheckMultithreaded(void);

#endif
