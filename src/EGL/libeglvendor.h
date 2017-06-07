#ifndef LIBEGLVENDOR_H
#define LIBEGLVENDOR_H

#include "libeglabipriv.h"
#include "GLdispatch.h"
#include "lkdhash.h"
#include "glvnd_list.h"
#include "winsys_dispatch.h"

extern const __EGLapiExports __eglExportsTable;

/*!
 * Structure containing relevant per-vendor information.
 */
struct __EGLvendorInfoRec {
    int vendorID; //< unique GLdispatch ID
    void *dlhandle; //< shared library handle
    __GLVNDwinsysVendorDispatch *dynDispatch;

    // TODO: Should this have a separate dispatch table for GL and GLES?
    __GLdispatchTable *glDispatch; //< GL dispatch table

    __EGLapiImports eglvc;
    __EGLdispatchTableStatic staticDispatch; //< static EGL dispatch table

    EGLBoolean patchSupported;
    __GLdispatchPatchCallbacks patchCallbacks;

    EGLBoolean supportsGL;
    EGLBoolean supportsGLES;

    EGLBoolean supportsDevice;
    EGLBoolean supportsPlatformDevice;
    EGLBoolean supportsPlatformGbm;
    EGLBoolean supportsPlatformX11;
    EGLBoolean supportsPlatformWayland;

    struct glvnd_list entry;
};

void __eglInitVendors(void);
void __eglTeardownVendors(void);

/**
 * Selects and loads the vendor libraries.
 *
 * \return A linked list of __EGLvendorInfo structs.
 */
struct glvnd_list *__eglLoadVendors(void);

#endif // LIBEGLVENDOR_H
