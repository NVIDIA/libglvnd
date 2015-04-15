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

#ifndef __X11GLVND_H__
#define __X11GLVND_H__

#include <X11/Xlib.h>

/*
 * Describes the client-side functions implemented by the x11glvnd extension.
 * This is a simple extension to query the X server for XID -> screen and screen
 * -> vendor mappings, used by libGLX. This may eventually be replaced by a
 *  server-side GLX extension which does the same thing.
 */

#define XGLV_EXTENSION_NAME "x11glvnd"

/*!
 * Returns the version of the x11glvnd extension supported by the server.
 *
 * Returns nonzero if the server supports a compatible version of x11glvnd.
 */
Status XGLVQueryVersion(Display *dpy, int *major, int *minor);

/*!
 * Returns the screen associated with this XID, or -1 if there was an error.
 */
int XGLVQueryXIDScreenMapping(
    Display *dpy,
    XID xid
);

/*!
 * Returns the vendor associated with this screen, or NULL if there was an
 * error.
 */
char *XGLVQueryScreenVendorMapping(
    Display *dpy,
    int screen
);

/*
 * Registers a callback with x11glvnd which is fired whenever XCloseDisplay()
 * is called.  This gives x11glvnd clients a lightweight alternative to
 * declaring themselves an X11 extension and using XESetCloseDisplay().
 *
 * This is NOT a thread-safe operation.
 */
void XGLVRegisterCloseDisplayCallback(void (*callback)(Display *));

/*
 * Unregisters all registered callbacks.
 */
void XGLVUnregisterCloseDisplayCallbacks(void);

#endif // __X11GLVND_H__
