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
 * THE MATERIALS ARE PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 * IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
 * CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 * MATERIALS OR THE USE OR OTHER DEALINGS IN THE MATERIALS.
 */

#ifndef __GLX_MAKECURRENT_H__
#define __GLX_MAKECURRENT_H__

#include <X11/Xlib.h>
#include <GL/gl.h>
#include <GL/glx.h>

/*
 * Contains definition of the fake GL extension functions exported by the
 * GLX_makecurrent vendor library used in the testglxmakecurrent test.
 */

typedef struct
{
    GLint beginCount;
    GLint vertex3fvCount;
    GLint endCount;
} GLContextCounts;

/**
 * This is an attribute to query using glXQueryContext to test dispatching by
 * GLXContext.
 *
 * The dummy vendor library will just return 1 for this attribute.
 */
#define GLX_CONTEX_ATTRIB_DUMMY 0x10000

/**
 * glXExampleExtensionFunction(): Dummy GLX extension function.
 *
 * This function just assigns 1 to *retval. It's used to test dispatching
 * through a venodr-supplied dispatch function.
 */
typedef void (* PFNGLXEXAMPLEEXTENSIONFUNCTION) (Display *dpy, int screen, int *retval);

/**
 * glXMakeCurrentTestResults(): perform queries on vendor library state.
 *
 * This explicitly is designed to not return anything, in case a bug causes the
 * API library to dispatch this to a no-op stub. If this function returned a
 * value and dispatched to a no-op, the return value would be bogus and hard to
 * debug.  To detect this issue, clients should initialize *saw to GL_FALSE
 * before passing it to this function.
 *
 * \param [out] saw Expected to point to a GLboolean initialied to GL_FALSE.
 * *saw is set to GL_TRUE if we dispatched to the vendor function.
 * \param [out] counts Should point to a GLContextCounts struct. This returns the
 *      number of GL calls that the vendor library has seen with the current
 *      context.
 */
typedef void (*PFNGLXMAKECURRENTTESTRESULTSPROC) (GLboolean *saw, GLContextCounts *counts);

/**
 * glXCreateContextVendorDUMMY(): Dummy extension function to create a context.
 *
 * This tests using a vendor-provided dispach stub to create a context and add
 * it to GLVND's tracking.
 */
typedef GLXContext (* PFNGLXCREATECONTEXTVENDORDUMMYPROC) (Display *dpy,
        GLXFBConfig config, GLXContext share_list, Bool direct,
        const int *attrib_list);

#endif
