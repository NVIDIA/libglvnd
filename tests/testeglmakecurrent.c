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
#include <assert.h>

#include "dummy/EGL_dummy.h"
#include "egl_test_utils.h"
#include "utils_misc.h"

typedef struct {
    const char *vendorName;
    EGLDisplay dpy;
    EGLContext ctx;
} TestContextInfo;

void checkIsCurrent(const TestContextInfo *ci);
void testSwitchContext(const TestContextInfo *oldCi, const TestContextInfo *ci);
void testSwitchContextFail(const TestContextInfo *oldCi,
        const TestContextInfo *newCi, const TestContextInfo *failCi);

int main(int argc, char **argv)
{
    TestContextInfo contexts[3];
    int i;

    loadEGLExtensions();

    contexts[0].vendorName = DUMMY_VENDOR_NAMES[0];
    contexts[1].vendorName = DUMMY_VENDOR_NAMES[0];
    contexts[2].vendorName = DUMMY_VENDOR_NAMES[1];

    for (i=0; i<ARRAY_LEN(contexts); i++) {
        DummyEGLContext *dctx;

        contexts[i].dpy = eglGetPlatformDisplay(EGL_DUMMY_PLATFORM,
                (void *) contexts[i].vendorName, NULL);
        if (contexts[i].dpy == EGL_NO_DISPLAY) {
            printf("eglGetPlatformDisplay failed\n");
            return 1;
        }

        contexts[i].ctx = eglCreateContext(contexts[i].dpy, NULL, EGL_NO_CONTEXT, NULL);
        if (contexts[i].ctx == EGL_NO_CONTEXT) {
            printf("Failed to create context for vendor %s\n", contexts[i].vendorName);
            return 1;
        }

        // Make sure the context came from the correct vendor library.
        dctx = (DummyEGLContext *) contexts[i].ctx;
        if (strcmp(dctx->vendorName, contexts[i].vendorName) != 0) {
            printf("EGLContext is from the wrong vendor: Expected \"%s\", but got \"%s\"\n",
                    contexts[i].vendorName, dctx->vendorName);
            return 1;
        }
        printf("Created context %d = %p\n", i, contexts[i].ctx);
    }

    // Test successful calls to eglMakeCurrent.

    printf("Test NULL -> ctx1\n");
    testSwitchContext(NULL, &contexts[0]);

    printf("Test ctx1 -> ctx1\n");
    testSwitchContext(&contexts[0], &contexts[0]);

    printf("Test ctx1 -> ctx2 (same vendor)\n");
    testSwitchContext(&contexts[0], &contexts[1]);

    printf("Test ctx2 -> ctx3 (different vendor)\n");
    testSwitchContext(&contexts[1], &contexts[2]);

    printf("Test ctx3 -> NULL\n");
    testSwitchContext(&contexts[2], NULL);

    // Next, make sure libEGL can deal with cases where the vendor's
    // eglMakeCurrent call fails.

    printf("Test failed NULL -> ctx1\n");
    testSwitchContextFail(NULL, &contexts[0], &contexts[0]);

    printf("Test failed ctx1 -> ctx2 (same vendor)\n");
    if (!eglMakeCurrent(contexts[0].dpy, EGL_NO_SURFACE, EGL_NO_SURFACE, contexts[0].ctx)) {
        printf("eglMakeCurrent failed\n");
        return 1;
    }
    testSwitchContextFail(&contexts[0], &contexts[1], &contexts[1]);

    printf("Test failed ctx1 -> NULL\n");
    testSwitchContextFail(&contexts[0], NULL, &contexts[0]);

    // If the current vendor library fails to release the current context, then
    // libEGL should return immediately, so the old context will still be
    // current.
    printf("Test failed ctx1 -> ctx3 (different vendor, old vendor fails)\n");
    testSwitchContextFail(&contexts[0], &contexts[2], &contexts[0]);

    // In this case, the old vendor library succeeds, but the new vendor
    // library fails. libEGL doesn't keep track of whether the previous context
    // is still valid, so it should be left with no current context.
    printf("Test failed ctx1 -> ctx3 (different vendor, new vendor fails)\n");
    testSwitchContextFail(NULL, &contexts[2], &contexts[2]);

    // Cleanup.

    eglMakeCurrent(EGL_NO_DISPLAY, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);
    for (i=0; i<ARRAY_LEN(contexts); i++) {
        eglDestroyContext(contexts[i].dpy, contexts[i].ctx);
    }

    return 0;
}

void checkIsCurrent(const TestContextInfo *ci)
{
    EGLDisplay dpy = (ci != NULL ? ci->dpy : EGL_NO_DISPLAY);
    EGLContext ctx = (ci != NULL ? ci->ctx : EGL_NO_CONTEXT);
    EGLDisplay currDpy;
    EGLContext currCtx;

    // Make sure the current display and context are correct.
    currDpy = eglGetCurrentDisplay();
    if (currDpy != dpy) {
        printf("eglGetCurrentDisplay returned %p, expected %p\n", currDpy, dpy);
        exit(1);
    }
    currCtx = eglGetCurrentContext();
    if (currCtx != ctx) {
        printf("eglGetCurrentContext returned %p, expected %p\n", currCtx, ctx);
        exit(1);
    }

    if (ci != NULL) {
        const char *str;

        // Make sure the vendor library's view of things matches libEGL's.
        currCtx = ptr_eglTestDispatchDisplay(dpy, DUMMY_COMMAND_GET_CURRENT_CONTEXT, 0);
        if (currCtx != ctx) {
            printf("eglTestDispatchDisplay returned %p, expected %p\n", currCtx, ctx);
            exit(1);
        }

        // Make sure the correct dispatch table is set in libGLdispatch.
        str = (const char *) glGetString(GL_VENDOR);
        if (str != NULL) {
            if (strcmp(str, ci->vendorName) != 0) {
                printf("glGetString returned wrong name: Expected \"%s\", got \"%s\"\n",
                        ci->vendorName, str);
                exit(1);
            }
        } else {
            printf("glGetString returned NULL, expected \"%s\"\n", ci->vendorName);
            exit(1);
        }
    }
}

void testSwitchContext(const TestContextInfo *oldCi, const TestContextInfo *newCi)
{
    EGLDisplay newDpy = (newCi != NULL ? newCi->dpy : oldCi->dpy);
    EGLContext newCtx = (newCi != NULL ? newCi->ctx : EGL_NO_CONTEXT);

    if (!eglMakeCurrent(newDpy, EGL_NO_SURFACE, EGL_NO_SURFACE, newCtx)) {
        printf("eglMakeCurrent failed with error 0x%04x\n", eglGetError());
        exit(1);
    }

    checkIsCurrent(newCi);

    if (oldCi != NULL && newCi != NULL && oldCi->dpy != newCi->dpy) {
        // If we're switching vendors, then make sure the old display got the
        // eglMakeCurrent call to release the old context.
        EGLContext currCtx = ptr_eglTestDispatchDisplay(oldCi->dpy, DUMMY_COMMAND_GET_CURRENT_CONTEXT, 0);
        if (currCtx != EGL_NO_CONTEXT) {
            printf("eglGetCurrentContext returned %p, expected EGL_NO_CONTEXT\n", currCtx);
            exit(1);
        }
    }
}

void testSwitchContextFail(const TestContextInfo *oldCi,
        const TestContextInfo *newCi, const TestContextInfo *failCi)
{
    EGLDisplay newDpy = (newCi != NULL ? newCi->dpy : oldCi->dpy);
    EGLContext newCtx = (newCi != NULL ? newCi->ctx : EGL_NO_CONTEXT);
    EGLint error;

    assert(failCi != NULL);
    assert(oldCi == failCi || newCi == failCi);

    if (!ptr_eglTestDispatchDisplay(failCi->dpy,
                DUMMY_COMMAND_FAIL_NEXT_MAKE_CURRENT, EGL_BAD_ACCESS)) {
        printf("eglFailNextMakeCurrent failed\n");
        exit(1);
    }

    if (eglMakeCurrent(newDpy, EGL_NO_SURFACE, EGL_NO_SURFACE, newCtx)) {
        printf("eglMakeCurrent succeeded, but should have failed.\n");
        exit(1);
    }

    error = eglGetError();
    if (error != EGL_BAD_ACCESS) {
        printf("eglMakeCurrent set the wrong error\n");
        exit(1);
    }

    checkIsCurrent(oldCi);
}

