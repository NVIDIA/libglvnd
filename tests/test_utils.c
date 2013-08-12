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

#include "test_utils.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>

GLboolean testUtilsCreateWindow(Display *dpy,
                                struct window_info *wi,
                                int screen)
{
    Window root;
    XSetWindowAttributes wattr;
    int wattr_mask;

    int visattr[] = {
        GLX_CONFIG_CAVEAT, GLX_NONE,
        GLX_RENDER_TYPE, GLX_RGBA_BIT,
        GLX_RED_SIZE, 1,
        GLX_GREEN_SIZE, 1,
        GLX_BLUE_SIZE, 1,
        GLX_DOUBLEBUFFER, 1,
        None,
    };

    memset(wi, 0, sizeof(*wi));

    wi->dpy = dpy;
    root = RootWindow(dpy, screen);

    wi->visinfo = glXChooseVisual(dpy, screen, visattr);
    if (!wi->visinfo) {
        printError("Failed to find a suitable visual!\n");
        return GL_FALSE;
    }

    wi->cmap = XCreateColormap(dpy, root, wi->visinfo->visual, AllocNone);

    if (!wi->cmap) {
        printError("Failed to create colormap!\n");
        return GL_FALSE;
    }

    wattr_mask = CWBackPixmap | CWBorderPixel | CWColormap;
    wattr.background_pixmap = None;
    wattr.border_pixel = 0;
    wattr.bit_gravity = StaticGravity;
    wattr.colormap = wi->cmap;

    wi->win = XCreateWindow(dpy, root, 0, 0, 512, 512, 0,
                            wi->visinfo->depth, InputOutput,
                            wi->visinfo->visual, wattr_mask, &wattr);

    if (!wi->win) {
        printError("Failed to create window!\n");
        return GL_FALSE;
    }


    return GL_TRUE;
}

void testUtilsDestroyWindow(Display *dpy,
                            struct window_info *wi)
{
    assert(dpy == wi->dpy);
    if (wi->win) {
        XDestroyWindow(dpy, wi->win);
    }
    if (wi->cmap) {
        XFreeColormap(dpy, wi->cmap);
    }
    XFree(wi->visinfo);
}

