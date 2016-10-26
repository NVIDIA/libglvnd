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

int checkError(EGLint expectedError);

int main(int argc, char **argv)
{
    EGLDisplay dpy;

    static const EGLint ERROR_ATTRIBS[] = {
        EGL_CREATE_CONTEXT_FAIL, EGL_BAD_MATCH,
        EGL_NONE
    };

    loadEGLExtensions();

    // Make sure the last error starts out as EGL_SUCCESS.
    printf("Checking initial state.\n");
    checkError(EGL_SUCCESS);

    dpy = eglGetDisplay(EGL_DEFAULT_DISPLAY);
    checkError(EGL_SUCCESS);

    // Test a function call where libEGL.so will set an error on its own.
    printf("Checking error in libEGL\n");
    eglGetCurrentSurface(EGL_NONE);
    checkError(EGL_BAD_PARAMETER);

    // Test an error set through a dispatch stub in libEGL.so.
    printf("Testing eglCreateContext with invalid display\n");
    eglCreateContext(EGL_NO_DISPLAY, NULL, EGL_NO_CONTEXT, NULL);
    checkError(EGL_BAD_DISPLAY);

    // Test a dispatch stub, with the error set in the vendor library. Note
    // that this case should be identical for a vendor-provided dispatch
    // function or one from libEGL.so.
    printf("Testing eglCreateContext, vendor error\n");
    eglCreateContext(dpy, NULL, EGL_NO_CONTEXT, ERROR_ATTRIBS);
    checkError(EGL_BAD_MATCH);

    // Test an error set through a vendor-provided dispatch stub. This is
    // different from the eglCreateContext error because the vendor-provided
    // stub has to set the error through the setEGLError callback.
    printf("Testing eglTestDispatchDisplay with invalid display\n");
    ptr_eglTestDispatchDisplay(EGL_NO_DISPLAY, DUMMY_COMMAND_GET_VENDOR_NAME, 0);
    checkError(EGL_BAD_DISPLAY);

    // Same, but with a valid display.
    printf("Testing eglTestDispatchDisplay with valid display\n");
    ptr_eglTestDispatchDisplay(dpy, DUMMY_COMMAND_GET_VENDOR_NAME, 0);
    checkError(EGL_SUCCESS);

    return 0;
}

int checkError(EGLint expectedError)
{
    EGLint error = eglGetError();
    if (error != expectedError) {
        printf("Got wrong error: Expected 0x%04x, got 0x%04x\n", expectedError, error);
        exit(1);
    }

    // Calling eglGetError should also clear the last error, so make sure the
    // next call returns EGL_SUCCESS.
    error = eglGetError();
    if (error != EGL_SUCCESS) {
        printf("Got wrong error: Expected 0x%04x, got EGL_SUCCESS\n", expectedError);
        exit(1);
    }

    return 0;
}
