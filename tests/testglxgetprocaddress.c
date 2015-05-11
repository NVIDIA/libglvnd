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

#include <GL/glx.h>
#include <GL/gl.h>
#include <stdio.h>

#define PROC_DEFINES(ret, proc, params)  \
    typedef ret (*PTR_ ## proc) params ; \
    static PTR_ ## proc p_ ## proc

#define CHECK_PROC(proc, args) do {                             \
    printf("checking " #proc "\n");                             \
    if (!(p_ ## proc =                                          \
         (PTR_ ## proc)glXGetProcAddress((GLubyte *)#proc))) {  \
        printf("failed to get " #proc "!\n");                   \
        goto fail;                                              \
    }                                                           \
    (*p_ ## proc) args;                                         \
} while (0)

#define CHECK_PROC_XFAIL(proc) do {                             \
    printf("checking " #proc "\n");                             \
    if ((p_ ## proc =                                           \
         (PTR_ ## proc)glXGetProcAddress((GLubyte *)#proc))) {  \
        printf("got unexpected " #proc "!\n");                  \
        goto fail;                                              \
    }                                                           \
} while (0)

PROC_DEFINES(void *, glXGetProcAddress, (GLubyte *procName));
PROC_DEFINES(void, glXWaitGL, (void));
PROC_DEFINES(void, glVertex3fv, (GLfloat *v));
PROC_DEFINES(void, glXExampleExtensionFunction,
             (Display *dpy, int screen, int *retval));
PROC_DEFINES(void, glBogusFunc1, (int a, int b, int c));
PROC_DEFINES(void, glBogusFunc2, (int a, int b, int c));
PROC_DEFINES(void, OogaBooga, (int a, int b, int c));

int main(int argc, char **argv)
{
    Display *dpy = NULL;

    int retval = 0;
    /*
     * Try GetProcAddress on different classes of API functions, and bogus
     * functions. The API library should return entry point addresses for
     * any function beginning with gl*(), though bogus functions will resolve
     * to no-ops.
     */

    dpy = XOpenDisplay(NULL);
    if (dpy == NULL) {
        printf("Can't open display\n");
        goto fail;
    }

    /*
     * Test core GLX dispatch functions implemented by API library. This
     * simply returns the symbol exported by libGLX.
     */
    CHECK_PROC(glXGetProcAddress, ((GLubyte *)"glBogusFunc1"));
    CHECK_PROC(glXWaitGL, ());

    /*
     * Test a "GLX extension" function with a vendor-neutral dispatcher
     * implemented by the vendor library (in this case, libGLX_dummy).  If we
     * successfully used libGLX_dummy's dispatcher, retval should be non-zero
     * (a zero value indicates we might be calling into a no-op stub generated
     * by libGLdispatch).
     */
    CHECK_PROC(glXExampleExtensionFunction, (dpy, DefaultScreen(dpy), &retval));

    if (!retval) {
        printf("Unexpected glXExampleExtensionFunction() return value!\n");
        goto fail;
    }

    /*
     * Try getting the address of the core GL function glVertex3fv().
     * This retrieves a static stub from glapi.
     * Note calling this function with a NULL pointer is fine since this is a
     * no-op function while there is no context current.
     */
    CHECK_PROC(glVertex3fv, (NULL));

    /*
     * These are bogus functions, but will get a valid entry point since they
     * are prefixed with "gl". The first GetProcAddress() will early out since
     * there should be a cached copy from the explicit call made above, but
     * the second one will go through glapi's dynamic stub generation path.
     *
     * Again, calling these functions should be a no-op.
     */
    CHECK_PROC(glBogusFunc1, (0, 0, 0));
    CHECK_PROC(glBogusFunc2, (1, 1, 1));

    /*
     * This will return NULL since OogaBooga is not prefixed with "gl".
     */
    CHECK_PROC_XFAIL(OogaBooga);

    // Success!
    return 0;

fail:
    return -1;
}
