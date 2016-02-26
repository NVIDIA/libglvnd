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

#if !defined(__LIB_GLX_ABI_H)
#define __LIB_GLX_ABI_H

#include <stdint.h>
#include <GL/glx.h>

#include "glvnd/GLdispatchABI.h"

#if defined(__cplusplus)
extern "C" {
#endif

/*!
 * \defgroup glxvendorabi GLX Vendor ABI
 *
 * Definition of ABI exported by libGLX.so to libGLX_VENDOR.so libraries.
 *
 * Each vendor is associated with three distinct dispatch table types:
 *
 * - static GLX dispatch table: this is the fixed list of GLX 1.4 entrypoints
 *   provided by the vendor at load time during the initial handshake.
 * - dynamic GLX dispatch table: this is a structure allocated by the API
 *   library at runtime used to manage GLX extension functions which are not
 *   present in the static table.
 * - core GL dispatch table: this is a structure maintained by the API library
 *   which contains both GL core (static) and GL extension (dynamic) functions.
 *
 * Note that while the implementations of most GLX functions in a vendor
 * library is mostly unchanged from a traditional, single-vendor driver, libGLX
 * has additional requirements for GLXContext and GLXFBConfig handle values.
 *
 * First, all GLXContext and GLXFBConfig handles have to be unique between
 * vendor libraries. That is, every GLXContext or GLXFBConfig handle must map
 * to exactly one vendor library, so that libGLX knows which library to dispatch
 * to.
 *
 * To do that, all GLXContext and GLXFBConfig handles *must* be a pointer to an
 * address that the vendor library somehow controls. The address doesn't need
 * to be readable or writable, but it must be an address that no other vendor
 * library would use.
 *
 * The address could be a pointer to a structure, or an address in a statically
 * or dynamically allocated array. It could even be a file mapping, or even an
 * offset into wherever the vendor library itself is mapped.
 *
 * A vendor library may not, however, use anything like an index or an XID for
 * a GLXContext or GLXFBConfig handle.
 *
 * GLXContext handles must also be globally unique across all display
 * connections in the entire process. That is, a vendor library may not return
 * the same GLXContext handle for two different contexts, even if they're on
 * different displays or different servers.
 *
 * GLXFBConfigs may be duplicated between multiple displays, as long as they
 * are still unique between vendors. Some applications even depend on this:
 * They will look up a GLXFBConfig handle with one connection, and then try to
 * use that config on another connection.
 *
 * @{
 */

/*!
 * Current version of the ABI.
 */
#define GLX_VENDOR_ABI_VERSION 1


/*!
 * This opaque structure stores function pointers for GLX extension functions.
 * It is allocated at runtime by the API library. Vendor-provided dispatch
 * functions retrieve and operate on this structure using the API below.
 */
typedef struct __GLXvendorInfoRec __GLXvendorInfo;

/****************************************************************************
 * API library exports                                                      *
 ****************************************************************************/

/*!
 * Functions exported by libGLX.so.
 *
 * These functions are exported by libGLX, and should be used by the
 * vendor-implemented dispatch functions to lookup and call into the right
 * vendor.
 *
 * These functions should only be called from the GLX dispatch functions, never
 * from the actual implementation of any function. libGLX.so may be holding a
 * non-recursive lock when it calls into the vendor library, so trying to call
 * back into libGLX could deadlock.
 */
typedef struct __GLXapiExportsRec {
    /*!
     * This fetches the appropriate dynamic GLX dispatch table given the display
     * and screen number.
     */
    __GLXvendorInfo *(*getDynDispatch)(Display *dpy,
                                                 const int screen);

    /*!
     * This function retrieves the appropriate current dynamic dispatch table,
     * if a GL context is current. Otherwise, this returns NULL.
     */
    __GLXvendorInfo *(*getCurrentDynDispatch)(void);

    /*!
     * This function retrieves an entry point from the dynamic dispatch table
     * given an index into the table.
     */
    __GLXextFuncPtr           (*fetchDispatchEntry)
        (__GLXvendorInfo *dynDispatch, int index);

    /************************************************************************
     * This routine is used by the vendor to lookup its context structure.
     * The contents of this structure are opaque to the API library and
     * vendor-dependent.
     ************************************************************************/

    /*!
     * This retrieves the current context for this thread.
     */
    GLXContext                (*getCurrentContext)(void);

    /************************************************************************
     * These routines are used by vendor dispatch functions to look up
     * and add mappings between various objects and screens.
     ************************************************************************/

    /*!
     * Records the screen number and vendor for a context. The screen and
     * vendor must be the ones returned for the XVisualInfo or GLXFBConfig that
     * the context is created from.
     *
     * \param dpy The display pointer.
     * \param context The context handle.
     * \param vendor The vendor that created the context.
     * \return Zero on success, non-zero on error.
     */
    int (*addVendorContextMapping)(Display *dpy, GLXContext context, __GLXvendorInfo *vendor);

    /*!
     * Removes a mapping from context to vendor. The context must have been
     * added with \p addVendorContextMapping.
     */
    void (*removeVendorContextMapping)(Display *dpy, GLXContext context);

    /*!
     * Looks up the screen and vendor for a context.
     *
     * If no mapping is found, then \p retScreen will be set to -1, and
     * \p retVendor and \p retDisplay will be set to NULL.
     *
     * \p retScreen, \p retVendor, and \p retDisplay may be NULL if the screen,
     * vendor, or display are not required.
     *
     * Note that this function does not take a display connection, since
     * there are cases (e.g., glXGetContextIDEXT) that take a GLXContext but
     * not a display.
     *
     * \param context The context to look up.
     * \param[out] retVendor Returns the vendor.
     * \return Zero if a match was found, or non-zero if it was not.
     */
    int (*vendorFromContext)(GLXContext context, __GLXvendorInfo **retVendor);

    int (*addVendorFBConfigMapping)(Display *dpy, GLXFBConfig config, __GLXvendorInfo *vendor);
    void (*removeVendorFBConfigMapping)(Display *dpy, GLXFBConfig config);
    int (*vendorFromFBConfig)(Display *dpy, GLXFBConfig config, __GLXvendorInfo **retVendor);

    void (*addScreenVisualMapping)(Display *dpy, const XVisualInfo *visual, __GLXvendorInfo *vendor);
    void (*removeScreenVisualMapping)(Display *dpy, const XVisualInfo *visual);
    int (*vendorFromVisual)(Display *dpy, const XVisualInfo *visual, __GLXvendorInfo **retVendor);

    int (*addVendorDrawableMapping)(Display *dpy, GLXDrawable drawable, __GLXvendorInfo *vendor);
    void (*removeVendorDrawableMapping)(Display *dpy, GLXDrawable drawable);

    /*!
     * Looks up the vendor for a drawable.
     *
     * If the drawable was created from another GLX function, then this will
     * return the same vendor library that was used to create it.
     *
     * If the drawable was not created from GLX (a regular X window, for
     * example), then libGLX.so will use the x11glvnd server extension to
     * figure out a vendor library.
     *
     * All of this should be opaque to a dispatch function, since the only
     * thing that matters is finding out which vendor to dispatch to.
     */
    int (*vendorFromDrawable)(Display *dpy, GLXDrawable drawable, __GLXvendorInfo **retVendor);

} __GLXapiExports;

/*****************************************************************************
 * API library imports                                                       *
 *****************************************************************************/

/*!
 * This structure stores required and optional vendor library callbacks.
 */
typedef struct __GLXapiImportsRec {
    /*!
     * Checks if the vendor library can support a given X screen. If this
     * returns false, then libGLX will fall back to the indirect rendering
     * library (if one exists).
     *
     * \param dpy The display connection.
     * \param screen The screen number.
     * \return True if the vendor library can support this screen.
     */
    Bool (* checkSupportsScreen) (Display *dpy, int screen);

    /*!
     * This retrieves the pointer to the real GLX or core GL function.
     *
     * \param procName The name of the function.
     * \return A pointer to a function, or \c NULL if the vendor does not
     * support the function.
     */
    void        *(*getProcAddress)        (const GLubyte *procName);

    /*!
     * This retrieves vendor-neutral functions which use the
     * __GLXdispatchTableDynamic API above to dispatch to the correct vendor.
     *
     * A vendor library must provide a dispatch function for all GLX functions
     * that it supports. If \c getDispatchAddress returns NULL, but
     * \c getProcAddress returns non-NULL, then libGLX will assume that the
     * function is a GL function, not GLX.
     *
     * That allows libGLX to dispatch GL and GLX functions correctly, even in
     * the case of a GL function that starts with "glX".
     *
     * \param procName The name of the function.
     * \return A pointer to a function, or \c NULL if the vendor does not
     * support the function or \p procName is not a GLX function.
     */
    void        *(*getDispatchAddress)    (const GLubyte *procName);

    /*!
     * This notifies the vendor library which dispatch table index is
     * assigned to a particular GLX extension function.
     */
    void        (*setDispatchIndex)      (const GLubyte *procName, int index);

    /*!
     * (OPTIONAL) This notifies the vendor library when an X error was
     * generated due to a detected error in the GLX API stream.
     *
     * This may be \c NULL, in which case the vendor library is not notified of
     * any errors.
     *
     * \note this is a notification only -- libGLX takes care of actually
     * reporting the error.
     *
     * \param dpy The display connection.
     * \param error The error code.
     * \param resid The XID associated with the error, if any.
     * \param opcode The minor opcode of the function that generated the error.
     * \param coreX11error True if the error code is a core X11 error, or False
     * if it's a GLX error code.
     *
     * \return True if libGLX should report the error to the application.
     */
    Bool        (*notifyError)  (Display *dpy, unsigned char error,
                                 XID resid, unsigned char opcode,
                                 Bool coreX11error);

    /*!
     * (OPTIONAL) Callbacks by which the vendor library may re-write libglvnd's
     * entrypoints at make current time, provided no other contexts are current
     * and the TLS model supports this functionality.  This is a performance
     * optimization that may not be available at runtime; the vendor library
     * must not depend on this functionality for correctness.  This should
     * point to a statically-allocated structure, or NULL if unimplemented.
     */
    const __GLdispatchPatchCallbacks *patchCallbacks;

} __GLXapiImports;

/*****************************************************************************/

/*!
 * Vendor libraries must export a function called __glx_Main() with the
 * following prototype. This function also performs a handshake based on the ABI
 * version number. This function receives a pointer to an exports table whose
 * lifetime is only guaranteed to be at a minimum that of the call to
 * __glx_Main(), in addition to the version number and a string identifying the
 * vendor. If there is an ABI version mismatch or some other error occurs, this
 * function returns NULL; otherwise this returns a pointer to a filled-in
 * dispatch table.
 */
#define __GLX_MAIN_PROTO_NAME "__glx_Main"
#define __GLX_MAIN_PROTO(version, exports, vendorName)                \
    const __GLXapiImports *__glx_Main(uint32_t version,               \
                                      const __GLXapiExports *exports, \
                                      const char *vendorName,         \
                                      int vendorID)

typedef const __GLXapiImports *(*__PFNGLXMAINPROC)
    (uint32_t, const __GLXapiExports *, const char *, int);

/*!
 * @}
 */

#if defined(__cplusplus)
}
#endif

#endif /* __LIB_GLX_ABI_H */
