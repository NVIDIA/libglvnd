/*
 * Copyright (c) 2016, NVIDIA CORPORATION.
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

#include <EGL/egl.h>
#include <EGL/eglext.h>
#include <GL/gl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "dummy/EGL_dummy.h"
#include "egl_test_utils.h"

static __eglMustCastToProperFunctionPointerType checkEGLFunction(const char *name);
static void checkResult(const char *func, const char *result);

int main(int argc, char **argv)
{
    typedef const char *(EGLAPIENTRY * pfn_eglQueryString) (EGLDisplay dpy, EGLint name);
    typedef const GLubyte * (* pfn_glGetString) (GLenum name);

    EGLDisplay dpy;
    EGLContext ctx;
    const char *result;

    pfn_eglTestDispatchDisplay ptr_eglTestDispatchDisplay;
    pfn_eglTestDispatchCurrent ptr_eglTestDispatchCurrent;
    pfn_eglQueryString ptr_eglQueryString;
    pfn_glGetString ptr_glGetString;

    dpy = eglGetDisplay(EGL_DEFAULT_DISPLAY);

    ptr_eglQueryString = (pfn_eglQueryString)
        checkEGLFunction("eglQueryString");

    ptr_eglTestDispatchDisplay = (pfn_eglTestDispatchDisplay)
        checkEGLFunction("eglTestDispatchDisplay");

    ptr_eglTestDispatchCurrent = (pfn_eglTestDispatchCurrent)
        checkEGLFunction("eglTestDispatchCurrent");

    ptr_glGetString = (pfn_glGetString)
        checkEGLFunction("glGetString");

    // Try to load a non-existant EGL function. This should return NULL.
    if (eglGetProcAddress("eglNonExistantFunction") != NULL) {
        printf("Got a pointer to a non-existant EGL function.\n");
        return 1;
    }

    // Test a built-in EGL function.
    result = ptr_eglQueryString(dpy, EGL_VENDOR);
    checkResult("eglQueryString", result);

    // Test an EGL extension function with a vendor-provided dispatch stub.
    result = ptr_eglTestDispatchDisplay(dpy, DUMMY_COMMAND_GET_VENDOR_NAME, 0);
    checkResult("eglTestDispatchDisplay", result);

    ctx = eglCreateContext(dpy, NULL, EGL_NO_CONTEXT, NULL);
    if (ctx == EGL_NO_CONTEXT) {
        printf("eglCreateContext failed\n");
        return 1;
    }

    if (!eglMakeCurrent(dpy, EGL_NO_SURFACE, EGL_NO_SURFACE, ctx)) {
        printf("eglMakeCurrent failed\n");
        return 1;
    }
    // Test a function that's supposed to dispatch based on the current
    // context.
    result = ptr_eglTestDispatchCurrent(DUMMY_COMMAND_GET_VENDOR_NAME, 0);
    checkResult("eglTestDispatchCurrent", result);

    result = (const char *) ptr_glGetString(GL_VENDOR);
    checkResult("glGetString", result);

    eglMakeCurrent(EGL_NO_DISPLAY, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);
    eglDestroyContext(dpy, ctx);

    return 0;
}

static void checkResult(const char *func, const char *result)
{
    if (result == NULL) {
        printf("%s returned NULL\n", func);
        exit(1);
    }
    if (strcmp(result, DUMMY_VENDOR_NAMES[0]) != 0) {
        printf("%s returned \"%s\", expected \"%s\"\n",
                func, result, DUMMY_VENDOR_NAMES[0]);
        exit(1);
    }
}

static __eglMustCastToProperFunctionPointerType checkEGLFunction(const char *name)
{
    __eglMustCastToProperFunctionPointerType func, func2;

    func = eglGetProcAddress(name);
    if (func == NULL) {
        printf("Can't find function \"%s\"\n", name);
        exit(1);
    }

    // Call eglGetProcAddress again to make sure we get the same pointer.
    func2 = eglGetProcAddress(name);
    if (func2 != func) {
        printf("Got different address for \"%s\"\n", name);
        exit(1);
    }

    return func;
}
