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
#include "glvnd/GLdispatchABI.h"

/*!
 * \defgroup gldispatch core GL/GLES dispatch and TLS module
 *
 * libGLdispatch manages the dispatch table used to dispatch OpenGL and GLES
 * functions to the appropriate vendor library for the current context.
 *
 * Window system libraries (libGLX.so and eventually libEGL.so) use
 * libGLdispatch.so to set and keep track of the current rendering context.
 * Their respective MakeCurrent calls (glXMakeCurrent and eglMakeCurrent) pass
 * in a dispatch table for whatever vendor owns the current context.
 *
 * The entrypoint libraries (libOpenGL.so, libGL.so, libGLESv1_CM.so, and
 * libGLESv2.so) all use the dispatch table stored in libGLdispatch.so.
 *
 * The dispatch table and dispatch stubs are based on Mesa's mapi/glapi
 * library.
 */

/*!
 * The current version of the ABI between libGLdispatch and the window system
 * libraries.
 *
 * \see __glDispatchGetABIVersion
 */
#define GLDISPATCH_ABI_VERSION 1

/* Namespaces for thread state */
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
 * An opaque structure used for internal thread state data.
 */
struct __GLdispatchThreadStatePrivateRec;

/*!
 * Generic thread state structure. The window system binding API libraries
 * subclass from this structure to track API-library specific current state
 * (e.g. current context and drawables).
 *
 * A thread has a thread state structure if and only if it has a current
 * context. A thread can only have one thread state at a time, so there can't
 * be both a current GLX context and a current EGL context at the same time.
 *
 * The winsys library is responsible for tracking, allocating, and freeing the
 * thread state structures.
 */
typedef struct __GLdispatchThreadStateRec {
    /*************************************************************************
     * Winsys-managed variables: fixed for lifetime of state
     *************************************************************************/

    /*!
     * Specifies which window system library owns this state. It should be set
     * to either \c GLDISPATCH_API_GLX or \c GLDISPATCH_API_EGL.
     *
     * This is used to make sure that a GLX context doesn't clobber a current
     * EGL context or vice-versa.
     */
    int tag;

    /*!
     * A callback that is called when a thread that has a current context
     * terminates.
     *
     * This is called after libGLdispatch handles its cleanup, so
     * __glDispatchGetCurrentThreadState will return NULL. The thread state is
     * passed as a parameter instead.
     *
     * The callback should not call __glDispatchMakeCurrent or
     * __glDispatchLoseCurrent.
     *
     * \param threadState The thread state passed to __glDispatchMakeCurrent.
     */
    void (*threadDestroyedCallback)(struct __GLdispatchThreadStateRec *threadState);

    /*************************************************************************
     * GLdispatch-managed variables: Modified by MakeCurrent()
     *************************************************************************/

    /*!
     * Private data for this thread state.
     *
     * This structure is assigned in \c __glDispatchMakeCurrent, and freed in
     * \c __glDispatchLoseCurrent.
     *
     * The value of this pointer, if any, is an internal detail of
     * libGLdispatch. The window system library should just ignore it.
     */
    struct __GLdispatchThreadStatePrivateRec *priv;
} __GLdispatchThreadState;

typedef struct __GLdispatchPatchCallbacksRec {
    /*!
     * Checks to see if the vendor library supports patching the given stub
     * type and size.
     *
     * \param type The type of entrypoints. This will be a one of the
     * __GLDISPATCH_STUB_* values.
     * \param stubSize The maximum size of the stub that the vendor library can
     * write, in bytes.
     * \param lookupStubOffset A callback into libglvnd to look up the address
     * of each entrypoint.
     */
    GLboolean (* isPatchSupported)(int type, int stubSize);

    /*!
     * Called by libglvnd to request that a vendor library patch its top-level
     * entrypoints.
     *
     * The vendor library should use the \p lookupStubOffset callback to find
     * the addresses of each entrypoint.
     *
     * This function may be called more than once to patch multiple sets of
     * entrypoints. For example, depending on how they're built, libOpenGL.so
     * or libGL.so may have their own entrypoints that are separate functions
     * from the ones in libGLdispatch.
     *
     * Note that during this call is the only time that the entrypoints can be
     * modified. After the call to \c initiatePatch returns, the vendor library
     * should treat the entrypoints as read-only.
     *
     * \param type The type of entrypoints. This will be a one of the
     * __GLDISPATCH_STUB_* values.
     * \param stubSize The maximum size of the stub that the vendor library can
     * write, in bytes.
     * \param lookupStubOffset A callback into libglvnd to look up the address
     * of each entrypoint.
     *
     * \return GL_TRUE if the vendor library supports patching with this type
     * and size.
     */
    GLboolean (*initiatePatch)(int type,
                               int stubSize,
                               DispatchPatchLookupStubOffset lookupStubOffset);

    /*!
     * (OPTIONAL) Called by libglvnd to notify the current vendor that it no
     * longer owns the top-level entrypoints.
     *
     * Libglvnd will take care of the restoring the entrypoints back to their
     * original state. The vendor library must not try to modify them.
     */
    void (*releasePatch)(void);

    /*!
     * (OPTIONAL) Called at the start of window-system functions (GLX and EGL).
     * This callback allows vendor libraries to perform any per-thread
     * initialization.
     *
     * This is basically a workaround for broken applications. A lot of apps
     * will make one or more invalid GLX/EGL calls on a thread (often including
     * a MakeCurrent with invalid parameters), and then will try to call an
     * OpenGL function.
     *
     * A non-libglvnd-based driver would be able to initialize any thread state
     * even on a bogus GLX call, but with libglvnd, those calls wouldn't get
     * past libGLX.
     *
     * This function is optional. If it's \c NULL, then libGLdispatch will
     * simply ignore it.
     *
     * \note This function may be called concurrently from multiple threads.
     */
    void (*threadAttach)(void);
} __GLdispatchPatchCallbacks;

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
 * This makes the given thread state current, and assigns this thread state the
 * passed-in current dispatch table and vendor ID.
 *
 * When this function is called, the current thread must not already have a
 * thread state. To switch between two thread states, first release the old
 * thread state by calling \c __glDispatchLoseCurrent.
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
PUBLIC GLboolean __glDispatchMakeCurrent(__GLdispatchThreadState *threadState,
                                         __GLdispatchTable *dispatch,
                                         int vendorID,
                                         const __GLdispatchPatchCallbacks *patchCb);

/*!
 * This makes the NOP dispatch table current and sets the current thread state
 * to NULL.
 *
 * A window system library should only call this if it created the current API
 * state. That is, if libGLX should not attempt to release an EGL context or
 * vice-versa.
 */
PUBLIC void __glDispatchLoseCurrent(void);

/*!
 * This gets the current thread state pointer. If the pointer is \c NULL, no
 * context is current, otherwise the contents of the pointer depends on which
 * client API owns the context (EGL or GLX).
 */
PUBLIC __GLdispatchThreadState *__glDispatchGetCurrentThreadState(void);

/**
 * Checks to see if multiple threads are being used. This should be called
 * periodically from places like glXMakeCurrent.
 */
PUBLIC void __glDispatchCheckMultithreaded(void);

/**
 * Tells libGLdispatch to unpatch the OpenGL entrypoints, but only if they were
 * patched by the given vendor.
 *
 * This is called when libEGL or libGLX is unloaded, to remove any dangling
 * pointers to the vendor library's patch callbacks.
 */
PUBLIC GLboolean __glDispatchForceUnpatch(int vendorID);

#endif
