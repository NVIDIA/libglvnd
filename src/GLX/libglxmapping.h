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

#if !defined(__LIB_GLX_MAPPING_H)
#define __LIB_GLX_MAPPING_H

#include "libglxabipriv.h"
#include "GLdispatch.h"
#include "lkdhash.h"
#include "winsys_dispatch.h"

#define GLX_CLIENT_STRING_LAST_ATTRIB GLX_EXTENSIONS

/*!
 * Structure containing relevant per-vendor information.
 */
struct __GLXvendorInfoRec {
    int vendorID; //< unique GLdispatch ID
    char *name; //< name of the vendor
    void *dlhandle; //< shared library handle

    /// dynamic GLX dispatch table
    __GLVNDwinsysVendorDispatch *dynDispatch;

    __GLdispatchTable *glDispatch; //< GL dispatch table

    const __GLXapiImports *glxvc;
    const __GLdispatchPatchCallbacks *patchCallbacks;
    __GLXdispatchTableStatic staticDispatch; //< static GLX dispatch table
};

typedef struct __GLXvendorXIDMappingHashRec __GLXvendorXIDMappingHash;

/*!
 * Structure containing per-display information.
 */
typedef struct __GLXdisplayInfoRec {
    Display *dpy;

    char *clientStrings[GLX_CLIENT_STRING_LAST_ATTRIB];

    /**
     * An array of vendors for each screen.
     *
     * Do not access this directly. Instead, call \c __glXLookupVendorByScreen.
     */
    __GLXvendorInfo **vendors;
    glvnd_rwlock_t vendorLock;

    DEFINE_LKDHASH(__GLXvendorXIDMappingHash, xidVendorHash);

    /// True if the server supports the GLX extension.
    Bool glxSupported;

    /// The major opcode for GLX, if it's supported.
    int glxMajorOpcode;
    int glxFirstError;

    Bool libglvndExtensionSupported;
} __GLXdisplayInfo;

typedef struct __GLXlocalDispatchFunctionRec {
    const char *name;
    __GLXextFuncPtr addr;
} __GLXlocalDispatchFunction;

/*!
 * A NULL-termianted list of GLX dispatch functions that are implemented in
 * libGLX instead of in any vendor library.
 */
extern const __GLXlocalDispatchFunction LOCAL_GLX_DISPATCH_FUNCTIONS[];

/*!
 * Accessor functions used to retrieve the "current" dispatch table for each of
 * the three types of dispatch tables (see libglxabi.h for an explanation of
 * these types).
 */
__GLXvendorInfo *__glXGetDynDispatch(Display *dpy, const int screen);

/*!
 * Various functions to manage mappings used to determine the screen
 * of a particular GLX call.
 */
int __glXAddVendorContextMapping(Display *dpy, GLXContext context, __GLXvendorInfo *vendor);
void __glXRemoveVendorContextMapping(Display *dpy, GLXContext context);
__GLXvendorInfo *__glXVendorFromContext(GLXContext context);

int __glXAddVendorFBConfigMapping(Display *dpy, GLXFBConfig config, __GLXvendorInfo *vendor);
void __glXRemoveVendorFBConfigMapping(Display *dpy, GLXFBConfig config);
__GLXvendorInfo *__glXVendorFromFBConfig(Display *dpy, GLXFBConfig config);

int __glXAddVendorDrawableMapping(Display *dpy, GLXDrawable drawable, __GLXvendorInfo *vendor);
void __glXRemoveVendorDrawableMapping(Display *dpy, GLXDrawable drawable);
__GLXvendorInfo *__glXVendorFromDrawable(Display *dpy, GLXDrawable drawable);

__GLXextFuncPtr __glXGetGLXDispatchAddress(const GLubyte *procName);

/*!
 * Looks up the vendor by name or screen number. This has the side effect of
 * loading the vendor library if it has not been previously loaded.
 */
__GLXvendorInfo *__glXLookupVendorByName(const char *vendorName);
__GLXvendorInfo *__glXLookupVendorByScreen(Display *dpy, const int screen);

/*!
 * Looks up the __GLXdisplayInfo structure for a display, creating it if
 * necessary.
 */
__GLXdisplayInfo *__glXLookupDisplay(Display *dpy);

/*!
 * This is called to perform any context-related cleanup when a display is
 * closed.
 */
void __glXDisplayClosed(__GLXdisplayInfo *dpyInfo);

void __glXMappingInit(void);

/*
 * Close the vendor library and perform any relevant teardown. This should
 * be called when the API library is unloaded.
 */
void __glXMappingTeardown(Bool doReset);

#endif /* __LIB_GLX_MAPPING_H */
