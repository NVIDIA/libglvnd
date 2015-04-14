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

#define GLX_CLIENT_STRING_LAST_ATTRIB GLX_EXTENSIONS

/*!
 * Structure containing relevant per-vendor information.
 */
typedef struct __GLXvendorInfoRec {
    int vendorID; //< unique GLdispatch ID
    char *name; //< name of the vendor
    void *dlhandle; //< shared library handle
    const __GLXdispatchTableStatic *staticDispatch; //< static GLX dispatch table
    __GLXdispatchTableDynamic *dynDispatch; //< dynamic GLX dispatch table
    __GLdispatchTable *glDispatch; //< GL dispatch table
} __GLXvendorInfo;

/*!
 * Structure containing per-display information.
 */
typedef struct __GLXdisplayInfoRec {
    char *clientStrings[GLX_CLIENT_STRING_LAST_ATTRIB];
} __GLXdisplayInfo;

/*!
 * Accessor functions used to retrieve the "current" dispatch table for each of
 * the three types of dispatch tables (see libglxabi.h for an explanation of
 * these types).
 */
const __GLXdispatchTableStatic * __glXGetStaticDispatch(Display *dpy,
                                                        const int screen);
__GLXdispatchTableDynamic *__glXGetDynDispatch(Display *dpy,
                                               const int screen);
__GLdispatchTable *__glXGetGLDispatch(Display *dpy, const int screen);

/*!
 * Various functions to manage mappings used to determine the screen
 * of a particular GLX call.
 */
void __glXAddScreenContextMapping(GLXContext context, int screen);
void __glXRemoveScreenContextMapping(GLXContext context);
int __glXScreenFromContext(GLXContext context);

void __glXAddScreenFBConfigMapping(GLXFBConfig config, int screen);
void __glXRemoveScreenFBConfigMapping(GLXFBConfig config);
int __glXScreenFromFBConfig(GLXFBConfig config);

void __glXAddScreenDrawableMapping(GLXDrawable drawable, int screen);
void __glXRemoveScreenDrawableMapping(GLXDrawable drawable);
int __glXScreenFromDrawable(Display *dpy, GLXDrawable drawable);

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
 * Frees the __GLXdisplayInfo structure for a display, if one exists.
 */
void __glXFreeDisplay(Display *dpy);

/*!
 * Notifies libglvnd that a context has been marked for destruction.
 */
void __glXNotifyContextDestroyed(GLXContext ctx);

/*
 * Close the vendor library and perform any relevant teardown. This should
 * be called when the API library is unloaded.
 */
void __glXMappingTeardown(Bool doReset);

#endif /* __LIB_GLX_MAPPING_H */
