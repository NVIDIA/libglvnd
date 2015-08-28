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

#include "GLdispatchABI.h"

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
 * @{
 */

/*!
 * Current version of the ABI.
 */
#define GLX_VENDOR_ABI_VERSION 0


/*!
 * This opaque structure stores function pointers for GLX extension functions.
 * It is allocated at runtime by the API library. Vendor-provided dispatch
 * functions retrieve and operate on this structure using the API below.
 */
typedef struct __GLXvendorInfoRec __GLXvendorInfo;

/****************************************************************************
 * API library exports                                                      *
 ****************************************************************************/

typedef struct __GLXapiExportsRec {
    /************************************************************************
     * The following routines are used by vendor-implemented GLX dispatch
     * functions to lookup and call into the right vendor.
     ************************************************************************/

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
     */
    void (*addScreenContextMapping)(Display *dpy, GLXContext context, int screen, __GLXvendorInfo *vendor);

    /*!
     * Removes a mapping from context to vendor. The context must have been
     * added with \p addScreenContextMapping.
     */
    void (*removeScreenContextMapping)(Display *dpy, GLXContext context);

    /*!
     * Looks up the screen and vendor for a context.
     *
     * If no mapping is found, then \p retScreen and \p retVendor will be set
     * to -1 and NULL, respectively.
     *
     * Either of \p retScreen or \p retVendor may be NULL if the screen or
     * vendor are not required.
     *
     * \param dpy The display connection.
     * \param context The context to look up.
     * \param[out] retScreen Returns the screen number.
     * \param[out] retVendor Returns the vendor.
     * \return Zero if a match was found, or non-zero if it was not.
     */
    int (*vendorFromContext)(Display *dpy, GLXContext context, int *retScreen, __GLXvendorInfo **retVendor);

    void (*addScreenFBConfigMapping)(Display *dpy, GLXFBConfig config, int screen, __GLXvendorInfo *vendor);
    void (*removeScreenFBConfigMapping)(Display *dpy, GLXFBConfig config);
    int (*vendorFromFBConfig)(Display *dpy, GLXFBConfig config, int *retScreen, __GLXvendorInfo **retVendor);

    void (*addScreenVisualMapping)(Display *dpy, const XVisualInfo *visual, __GLXvendorInfo *vendor);
    void (*removeScreenVisualMapping)(Display *dpy, const XVisualInfo *visual);
    int (*vendorFromVisual)(Display *dpy, const XVisualInfo *visual, __GLXvendorInfo **retVendor);

    void (*addScreenDrawableMapping)(Display *dpy, GLXDrawable drawable, int screen, __GLXvendorInfo *vendor);
    void (*removeScreenDrawableMapping)(Display *dpy, GLXDrawable drawable);

    /*!
     * Looks up the screen and vendor for a drawable.
     *
     * If the server does not support the x11glvnd extension, then this
     * function may not be able to determine the screen number for a drawable.
     * In that case, it will return -1 for the screen number.
     *
     * Even without x11glvnd, this function will still return a vendor
     * suitable for indirect rendering.
     */
    int (*vendorFromDrawable)(Display *dpy, GLXDrawable drawable, int *retScreen, __GLXvendorInfo **retVendor);

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
     * This notifies the vendor library when an X error should be generated
     * due to a detected error in the GLX API stream.
     */
    void        (*notifyError)  (Display *dpy, char error,
                                 char opcode, XID resid);

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
