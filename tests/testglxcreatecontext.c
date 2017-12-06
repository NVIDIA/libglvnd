/*
 * Copyright (c) 2017, NVIDIA CORPORATION.
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
 * This program tests the various GLX functions to create a context.
 */

#include <X11/Xlib.h>
#include <GL/glx.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "test_utils.h"
#include "dummy/GLX_dummy.h"

static int runTestGLX12(struct window_info *wi);
static int runTestGLX13(struct window_info *wi);
static int runTestGLXAttribsConfig(struct window_info *wi);
static int runTestGLXAttribsScreen(struct window_info *wi);
static int runTestGLXAttribsVendor(struct window_info *wi);
static int runTestCommon(struct window_info *wi, GLXContext ctx, const char *testName);

static PFNGLXCREATECONTEXTATTRIBSARBPROC ptr_glXCreateContextAttribsARB;
static PFNGLXCREATECONTEXTVENDORDUMMYPROC ptr_glXCreateContextVendorDUMMY;

int main(int argc, char **argv)
{
    Display *dpy = NULL;
    struct window_info wi = {};
    int result = 1;

    dpy = XOpenDisplay(NULL);
    if (dpy == NULL) {
        printError("No display! Please re-test with a running X server\n"
                   "and the DISPLAY environment variable set appropriately.\n");
        goto done;
    }

    // Call glXQueryServerString to make libGLX.so load the vendor library for
    // the screen before we try to load any extension functions.
    glXQueryServerString(dpy, DefaultScreen(dpy), GLX_EXTENSIONS);

    ptr_glXCreateContextAttribsARB = (PFNGLXCREATECONTEXTATTRIBSARBPROC)
        glXGetProcAddress((const GLubyte *) "glXCreateContextAttribsARB");
    if (ptr_glXCreateContextAttribsARB == NULL) {
        printError("Could not load glXCreateContextAttribsARB\n");
        goto done;
    }

    ptr_glXCreateContextVendorDUMMY = (PFNGLXCREATECONTEXTVENDORDUMMYPROC)
        glXGetProcAddress((const GLubyte *) "glXCreateContextVendorDUMMY");
    if (ptr_glXCreateContextVendorDUMMY == NULL) {
        printError("Could not load glXCreateContextVendorDUMMY\n");
        goto done;
    }

    if (!testUtilsCreateWindowConfig(dpy, &wi, DefaultScreen(dpy))) {
        printError("Failed to create window\n");
        goto done;
    }

    // Start by testing the core GLX functions, glXCreateContext and
    // glXCreateNewContext.
    result = runTestGLX12(&wi);
    if (result != 0) {
        goto done;
    }
    result = runTestGLX13(&wi);
    if (result != 0) {
        goto done;
    }

    // Next, test using glXCreateContextAttribsARB. This can dispatch one of
    // two ways. First, test dispatching using a GLXFBConfig handle.
    result = runTestGLXAttribsConfig(&wi);
    if (result != 0) {
        goto done;
    }

    // Next, test using the GLX_EXT_no_config_context extension. In this case,
    // we'll pass NULL for the GLXFBConfig parameter, but then specify a screen
    // number using an attribute.
    result = runTestGLXAttribsScreen(&wi);
    if (result != 0) {
        goto done;
    }

    // All of the above functions have dispatch stubs in libGLX.so itself, so
    // test to make sure that a vendor can provide an extension function and
    // dispatch stub to create a context.
    result = runTestGLXAttribsVendor(&wi);
    if (result != 0) {
        goto done;
    }

done:
    if (dpy != NULL) {
        if (wi.dpy != NULL) {
            testUtilsDestroyWindow(dpy, &wi);
        }
        XCloseDisplay(dpy);
    }
    return result;
}

int runTestGLX12(struct window_info *wi)
{
    GLXContext ctx = glXCreateContext(wi->dpy, wi->visinfo, NULL, True);
    return runTestCommon(wi, ctx, "glXCreateContext");
}

int runTestGLX13(struct window_info *wi)
{
    GLXContext ctx = glXCreateNewContext(wi->dpy, wi->config, GLX_RGBA_TYPE, NULL, True);
    return runTestCommon(wi, ctx, "glXCreateNewContext");
}

int runTestGLXAttribsConfig(struct window_info *wi)
{
    GLXContext ctx = ptr_glXCreateContextAttribsARB(wi->dpy, wi->config, NULL, True, NULL);
    return runTestCommon(wi, ctx, "glXCreateContextAttribsARB(config)");
}

int runTestGLXAttribsScreen(struct window_info *wi)
{
    // Create a context using the GLX_EXT_no_config_context extension. In this
    // case, we pass NULL for the GLXFBConfig parameter, but then specify a
    // screen number using an attribute.
    const int attribs[] = {
        GLX_SCREEN, wi->visinfo->screen,
        None
    };
    GLXContext ctx = ptr_glXCreateContextAttribsARB(wi->dpy, NULL, NULL, True, attribs);
    return runTestCommon(wi, ctx, "glXCreateContextAttribsARB(screen)");
}

int runTestGLXAttribsVendor(struct window_info *wi)
{
    GLXContext ctx = ptr_glXCreateContextAttribsARB(wi->dpy, wi->config, NULL, True, NULL);
    return runTestCommon(wi, ctx, "glXCreateContextVendorDUMMY");
}

int runTestCommon(struct window_info *wi, GLXContext ctx, const char *testName)
{
    int value = -1;
    int ret = 1;
    if (ctx == NULL) {
        printError("%s: failed to create context\n", testName);
        goto done;
    }

    // Call glXQueryContext to make sure that we can dispatch to this context
    if (glXQueryContext(wi->dpy, ctx, GLX_CONTEX_ATTRIB_DUMMY, &value) != Success) {
        printError("%s: glXQueryContext failed\n", testName);
        goto done;
    }
    if (value != 1) {
        printError("%s: glXQueryContext returned wrong value %d\n", testName, value);
        goto done;
    }

    printf("Test succeeded: %s\n", testName);
    ret = 0;

done:
    if (ctx != NULL) {
        glXDestroyContext(wi->dpy, ctx);
    }
    return ret;
}
