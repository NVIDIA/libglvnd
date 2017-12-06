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

#ifndef __GLX_MAKECURRENT_H__
#define __GLX_MAKECURRENT_H__

#include <X11/Xlib.h>
#include <GL/gl.h>
#include <GL/glx.h>

/*
 * Contains definition of the fake GL extension functions exported by the
 * GLX_makecurrent vendor library used in the testglxmakecurrent test.
 */

enum {

    /*
     * Returns an array of 3 GLint values containing, respectively,
     * the number of times glBegin(), glVertex3fv(), and glEnd() were called
     * by this thread.
     */
    GL_MC_FUNCTION_COUNTS,

    /*
     * Returns a NULL-terminated string describing the name of this vendor.
     */
    GL_MC_VENDOR_STRING,

    /*
     * Last request. Always returns NULL.
     */
    GL_MC_LAST_REQ
} GLmakeCurrentTestRequest;

/**
 * This is an attribute to query using glXQueryContext to test dispatching by
 * GLXContext.
 *
 * The dummy vendor library will just return 1 for this attribute.
 */
#define GLX_CONTEX_ATTRIB_DUMMY 0x10000

/*
 * glMakeCurrentTestResults(): perform queries on vendor library state.
 *
 * This explicitly is designed to not return anything, in case a bug causes the
 * API library to dispatch this to a no-op stub. If this function returned a
 * value and dispatched to a no-op, the return value would be bogus and hard to
 * debug.  To detect this issue, clients should initialize *saw to GL_FALSE
 * before passing it to this function. Similarly, *ret should be initialized to
 * NULL prior to passing it to this function.
 *
 * \param [in] req The request to perform. Must be a valid
 * GLmakeCurrentTestRequest enum
 * \param [out] saw Expected to point to a GLboolean initialied to GL_FALSE.
 * *saw is set to GL_TRUE if we dispatched to the vendor function.
 * \param [out] ret Expected to point to a (void*) initialized to NULL. *ret is
 * set to NULL if there was an error, or a pointer to request-specific data
 * otherwise. The pointer may be passed into free(3).
 */
typedef void (*PFNGLMAKECURRENTTESTRESULTSPROC)(
    GLint req,
    GLboolean *saw,
    void **ret
);

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
