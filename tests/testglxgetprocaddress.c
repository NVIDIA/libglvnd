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
#include <stdlib.h>

typedef __GLXextFuncPtr (* pfn_glXGetProcAddress) (const GLubyte *procName);
static pfn_glXGetProcAddress ptr_glXGetProcAddress;

typedef void (* pfn_glXWaitGL) (void);
static pfn_glXWaitGL ptr_glXWaitGL;

typedef void (* pfn_glVertex3fv) (const GLfloat *v);
static pfn_glVertex3fv ptr_glVertex3fv;

typedef void (* pfn_glXExampleExtensionFunction) (Display *dpy, int screen, int *retval);
static pfn_glXExampleExtensionFunction ptr_glXExampleExtensionFunction;

typedef void (* pfn_glBogusFunc1) (int a, int b, int c);
static pfn_glBogusFunc1 ptr_glBogusFunc1;

typedef void (* pfn_glBogusFunc2) (int a, int b, int c);
static pfn_glBogusFunc2 ptr_glBogusFunc2;

static void *LoadFunction(const char *name);

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
        return 1;
    }

    /*
     * Load the function addresses up front. Note that in order to test GLX
     * entrypoint generation, we have to load the functions early, before
     * libGLX.so loads the vendor library.
     */
    ptr_glXGetProcAddress = (pfn_glXGetProcAddress)
        LoadFunction("glXGetProcAddress");
    ptr_glXWaitGL = (pfn_glXWaitGL) LoadFunction("glXWaitGL");
    ptr_glXExampleExtensionFunction = (pfn_glXExampleExtensionFunction)
        LoadFunction("glXExampleExtensionFunction");
    ptr_glVertex3fv = (pfn_glVertex3fv) LoadFunction("glVertex3fv");
    ptr_glBogusFunc1 = (pfn_glBogusFunc1) LoadFunction("glBogusFunc1");
    ptr_glBogusFunc2 = (pfn_glBogusFunc2) LoadFunction("glBogusFunc2");

    /*
     * Test core GLX dispatch functions implemented by API library. This
     * simply returns the symbol exported by libGLX.
     */
    ptr_glXGetProcAddress((const GLubyte *) "glBogusFunc1");
    ptr_glXWaitGL();

    // Call glXGetClientString to force libGLX to load the vendor library.
    glXGetClientString(dpy, GLX_EXTENSIONS);

    /*
     * Test a "GLX extension" function with a vendor-neutral dispatcher
     * implemented by the vendor library (in this case, libGLX_dummy).  If we
     * successfully used libGLX_dummy's dispatcher, retval should be non-zero
     * (a zero value indicates we might be calling into a no-op stub generated
     * by libGLdispatch).
     */
    ptr_glXExampleExtensionFunction(dpy, 0, &retval);
    if (!retval) {
        printf("Unexpected glXExampleExtensionFunction() return value!\n");
        return 1;
    }

    /*
     * Try getting the address of the core GL function glVertex3fv().
     * This retrieves a static stub from glapi.
     * Note calling this function with a NULL pointer is fine since this is a
     * no-op function while there is no context current.
     */
    ptr_glVertex3fv(NULL);

    /*
     * These are bogus functions, but will get a valid entry point since they
     * are prefixed with "gl". The first GetProcAddress() will early out since
     * there should be a cached copy from the explicit call made above, but
     * the second one will go through glapi's dynamic stub generation path.
     *
     * Again, calling these functions should be a no-op.
     */
    ptr_glBogusFunc1(0, 0, 0);
    ptr_glBogusFunc2(1, 1, 1);

    // Success!
    return 0;
}

static void *LoadFunction(const char *name)
{
    __GLXextFuncPtr func = glXGetProcAddress((const GLubyte *) name);
    if (func == NULL) {
        printf("failed to get %s!\n", name);
        exit(1);
    }

    return (void *) func;
}

