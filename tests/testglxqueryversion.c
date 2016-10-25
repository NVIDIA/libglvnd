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

#include <GL/gl.h>
#include <GL/glx.h>
#include <stdio.h>

#define printError(...) fprintf(stderr, __VA_ARGS__)

int main(int argc, char **argv)
{
    Display *dpy = XOpenDisplay(NULL);
    Bool ret;
    int major, minor;
    int event, error;

    if (!dpy) {
        printError("No display!\n");
        return 1;
    }

    ret = XQueryExtension(dpy, "GLX", &major, &event, &error);
    if (!ret) {
        printError("Skipping test: The server does not support the GLX extension.\n");
        // For automake tests, returning 77 indicates that this test was
        // skipped instead of failing.
        return 77;
    }

    ret = glXQueryVersion(dpy, &major, &minor);

    if (!ret) {
        printError("glXQueryVersion error!\n");
        XCloseDisplay(dpy);
        return 1;
    }

    printf("GLX version %d.%d\n", major, minor);

    XCloseDisplay(dpy);
    return 0;
}
