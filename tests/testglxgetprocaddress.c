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

#include <stdio.h>
#include <stdlib.h>

#include <GL/glx.h>
#include <GL/gl.h>

#include <GLdispatch.h>
#include "dummy/GLX_dummy.h"

typedef const char * (* pfn_glXQueryServerString) (Display *dpy, int screen, int name);

static void *LoadFunction(const char *name);

int main(int argc, char **argv)
{
    pfn_glXQueryServerString ptr_glXQueryServerString;
    PFNGLXEXAMPLEEXTENSIONFUNCTION ptr_glXExampleExtensionFunction;

    Display *dpy = NULL;
    const char *str;
    __GLdispatchProc dispatchPtr, glxPtr;
    int retval = 0;

    dpy = XOpenDisplay(NULL);
    if (dpy == NULL) {
        printf("Can't open display\n");
        return 1;
    }

    // Call glXGetClientString to force libGLX to load the vendor library.
    glXGetClientString(dpy, GLX_EXTENSIONS);

    // Test a core GLX function first.
    ptr_glXQueryServerString = (pfn_glXQueryServerString) LoadFunction("glXQueryServerString");
    str = ptr_glXQueryServerString(dpy, 0, GLX_VENDOR);
    if (str == NULL) {
        printf("glXQueryServerString returned NULL\n");
        return 1;
    }
    if (strcmp(str, "testlib") != 0) {
        printf("glXQueryServerString returned unexpected value: %s\n", str);
        return 1;
    }

    /*
     * Test a "GLX extension" function with a vendor-neutral dispatcher
     * implemented by the vendor library (in this case, libGLX_dummy). If we
     * successfully used libGLX_dummy's dispatcher, retval should be 1.
     */
    ptr_glXExampleExtensionFunction = (PFNGLXEXAMPLEEXTENSIONFUNCTION)
        LoadFunction("glXExampleExtensionFunction");
    ptr_glXExampleExtensionFunction(dpy, 0, &retval);
    if (retval != 1) {
        printf("Unexpected glXExampleExtensionFunction() return value: %d\n", retval);
        return 1;
    }

    // Test loading a normal GL function. Load the function through
    // glXGetProcAddress, and then again directly through libGLdispatch. We
    // should get the same poiner for both.
    glxPtr = LoadFunction("glVertex3fv");
    dispatchPtr = __glDispatchGetProcAddress("glVertex3fv");
    if (dispatchPtr != glxPtr) {
        printf("Mismatch for function glVertex3fv: GLX returned %p, GLdispatch returned %p\n",
                glxPtr, dispatchPtr);
        return 1;
    }

    // Success!
    return 0;
}

static void *LoadFunction(const char *name)
{
    __GLXextFuncPtr func, func2;
    func = glXGetProcAddress((const GLubyte *) name);
    if (func == NULL) {
        printf("failed to get %s!\n", name);
        exit(1);
    }

    // Call glXGetProcAddress again to make sure that we get the same address.
    func2 = glXGetProcAddress((const GLubyte *) name);
    if (func != func2) {
        printf("glXGetProcAddress returned different address for %s: %p, %p\n",
                name, func, func2);
        exit(1);
    }

    return (void *) func;
}

