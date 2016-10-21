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

/**
 * \file
 *
 * Tests eglGetDisplay and eglGetPlatformDisplay.
 *
 * This test uses the dummy platform to create an EGLDisplay for each of the
 * dummy vendor libraries, then calls eglQueryString to make sure that each
 * display goes to the correct vendor.
 */

#include <EGL/egl.h>
#include <EGL/eglext.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "dummy/EGL_dummy.h"
#include "egl_test_utils.h"

int main(int argc, char **argv)
{
    EGLDisplay displays[DUMMY_VENDOR_COUNT];
    EGLDisplay dpy;
    EGLint error;
    int i;

    for (i=0; i<DUMMY_VENDOR_COUNT; i++) {
        const char *name = DUMMY_VENDOR_NAMES[i];
        const char *str;
        EGLint major, minor;

        printf("Testing vendor %d, with name \"%s\"\n", i, name);

        displays[i] = eglGetPlatformDisplay(EGL_DUMMY_PLATFORM, (void *) name, NULL);
        if (displays[i] == EGL_NO_DISPLAY) {
            printf("eglGetPlatformDisplay failed with vendor \"%s\", error 0x%04x\n", name, eglGetError());
            exit(1);
        }

        if (!eglInitialize(displays[i], &major, &minor)) {
            printf("eglInitialize failed\n");
            return 1;
        }

        str = eglQueryString(displays[i], EGL_VENDOR);
        if (str == NULL) {
            printf("eglQueryString failed with vendor \"%s\", error 0x%04x\n", name, eglGetError());
            exit(1);
        }

        if (strcmp(str, name) != 0) {
            printf("Got wrong vendor string: Expected \"%s\", but got \"%s\"\n", name, str);
            exit(1);
        }
    }

    // Test getting a default display from eglGetDisplay. This should iterate
    // over each vendor, and the first vendor library should return the same
    // display as it did for EGL_DUMMY_PLATFORM.
    dpy = eglGetDisplay(EGL_DEFAULT_DISPLAY);
    if (dpy == EGL_NO_DISPLAY) {
        printf("eglGetDisplay failed with error 0x%04x\n", eglGetError());
        return 1;
    }
    if (dpy != displays[0]) {
        printf("eglGetDisplay returned incorrect display: Expected %p, got %p\n",
                displays[0], dpy);
        return 1;
    }

    // Try getting a display using an invalid platform enum.
    dpy = eglGetPlatformDisplay(EGL_ALPHA_SIZE, NULL, NULL);
    if (dpy != EGL_NO_DISPLAY) {
        printf("Got an EGLDisplay for an invalid platform.\n");
        return 1;
    }
    error = eglGetError();
    if (error != EGL_BAD_PARAMETER) {
        printf("Got the wrong error 0x%04x for eglGetPlatformDisplay with invalid platform\n", error);
        return 1;
    }

    // Pass a valid platform, but with a name that the vendor's won't
    // recognize. Each vendor will return EGL_NO_DISPLAY, but won't raise an
    // error.
    dpy = eglGetPlatformDisplay(EGL_DUMMY_PLATFORM, (void *) "invalid", NULL);
    if (dpy != EGL_NO_DISPLAY) {
        printf("Got an EGLDisplay for an invalid vendor name.\n");
        return 1;
    }
    error = eglGetError();
    if (error != EGL_SUCCESS) {
        printf("Got the wrong error 0x%04x for eglGetPlatformDisplay with invalid vendor name\n", error);
        return 1;
    }

    return 0;
}

