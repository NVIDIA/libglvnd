/*
 * Copyright (c) 2019, NVIDIA CORPORATION.
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

#if defined(USE_DISPATCH_ASM)

#include <GL/glx.h>
#include <GL/gl.h>
#include <stdio.h>
#include <stdlib.h>

#include "dummy/GLX_dummy.h"

int main(int argc, char **argv)
{
    PFNGLXEXAMPLEEXTENSIONFUNCTION ptr_glXExampleExtensionFunction;
    PFNGLXEXAMPLEEXTENSIONFUNCTION ptr_glXExampleExtensionFunction2;
    __GLXextFuncPtr proc;
    Display *dpy = NULL;
    int result = 0;
    int i;

    // Load the function pointer first, before libGLX can load the dummy vendor
    // library. That'll force libGLX to generate an entrypoint stub.
    ptr_glXExampleExtensionFunction = (PFNGLXEXAMPLEEXTENSIONFUNCTION)
        glXGetProcAddress((const GLubyte *) "glXExampleExtensionFunction");
    if (ptr_glXExampleExtensionFunction == NULL) {
        printf("Can't look up glXExampleExtensionFunction\n");
        return 1;
    }
    printf("Got glXExampleExtensionFunction at address %p\n", ptr_glXExampleExtensionFunction);

    // Call glXGetProcAddress to generate more dummy dispatch stubs, and then a
    // second extension function. This tests that the generated dispatch stubs
    // can correctly handle a large index.
    for (i=0; i<4094; i++) {
        char buf[50];

        snprintf(buf, sizeof(buf), "glXUndefined%dDUMMY", i);
        proc = glXGetProcAddress((const GLubyte *) buf);
        if (proc == NULL) {
            printf("Failed to generate stub for dummy function %d %s\n", i, buf);
            return 1;
        }
    }

    ptr_glXExampleExtensionFunction2 = (PFNGLXEXAMPLEEXTENSIONFUNCTION)
        glXGetProcAddress((const GLubyte *) "glXExampleExtensionFunction2");
    if (ptr_glXExampleExtensionFunction2 == NULL) {
        printf("Can't look up glXExampleExtensionFunction\n");
        return 1;
    }
    printf("Got glXExampleExtensionFunction2 at address %p\n", ptr_glXExampleExtensionFunction2);

    // Make one more call to glXGetProcAddress. This should return NULL.
    proc = glXGetProcAddress((const GLubyte *) "glXLastUndefinedDummy");
    if (proc != NULL) {
        printf("Last glXGetProcAddress returned non-NULL: %p\n", proc);
        return 1;
    }

    dpy = XOpenDisplay(NULL);
    if (dpy == NULL) {
        printf("Can't open display\n");
        return 1;
    }

    // Call glXGetClientString to force libGLX to load the vendor library.
    glXGetClientString(dpy, GLX_EXTENSIONS);

    ptr_glXExampleExtensionFunction(dpy, 0, &result);
    if (result != 1) {
        printf("Unexpected glXExampleExtensionFunction() return value: %d\n", result);
        XCloseDisplay(dpy);
        return 1;
    }

    ptr_glXExampleExtensionFunction2(dpy, 0, &result);
    if (result != 2) {
        printf("Unexpected glXExampleExtensionFunction2() return value: %d\n", result);
        XCloseDisplay(dpy);
        return 1;
    }

    printf("%p - %p = %d\n", ptr_glXExampleExtensionFunction2,
            ptr_glXExampleExtensionFunction,
            (int) (((intptr_t) ptr_glXExampleExtensionFunction2) - ((intptr_t) ptr_glXExampleExtensionFunction)));

    XCloseDisplay(dpy);
    return 0;
}

#else // defined(USE_DISPATCH_ASM)

int main(int argc, char **argv)
{
    // If libGLX can't generate new dispatch stubs, then just skip this test.
    return 77;
}

#endif // defined(USE_DISPATCH_ASM)
