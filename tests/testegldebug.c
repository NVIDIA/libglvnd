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

/**
 * \file
 *
 * Tests for EGL_KHR_debug.
 *
 * This test works by recording the parameters that we expect the debug
 * callback function to get, and then calling an EGL function that generates an
 * error.
 *
 * The debug callback checks its parameters against the expected values, and
 * exits if any of them don't match.
 */

static const EGLLabelKHR THREAD_LABEL = (EGLLabelKHR) "THREAD_LABEL";
static const EGLLabelKHR DISPLAY_LABEL = (EGLLabelKHR) "DISPLAY_LABEL";

static void testCallback(EGLDisplay dpy, EGLBoolean callbackEnabled);

static void EGLAPIENTRY debugCallback(EGLenum error, const char *command,
        EGLint messageType, EGLLabelKHR threadLabel, EGLLabelKHR objectLabel,
        const char *message);

/**
 * Records the expected parameters for the next call to the debug callback.
 */
static void setCallbackExpected(const char *command, EGLenum error,
    EGLLabelKHR objectLabel, const char *message);
static void setCallbackNotExpected(void);
static void checkError(EGLint expectedError);

/**
 * True if the debug callback has been called since the last call to
 * \c setCallbackExpected. This is used to make sure that the debug callback
 * is called exactly once when a function generates an error.
 */
static EGLBoolean callbackWasCalled = EGL_FALSE;

// These are the expected values for the next call to the debug callback, set
// from setCallbackExpected and setCallbackNotExpected.
static EGLBoolean shouldExpectCallback = EGL_FALSE;
static const char *nextExpectedCommand = NULL;
static EGLint nextExpectedError = EGL_NONE;
static EGLLabelKHR nextExpectedObject = NULL;
static const char *nextExpectedMessage = NULL;

int main(int argc, char **argv)
{
    EGLDisplay dpy;
    EGLAttrib callbackAttribs[] = {
        EGL_DEBUG_MSG_ERROR_KHR, EGL_TRUE,
        EGL_NONE
    };

    // We shouldn't get a callback for anything yet.
    setCallbackNotExpected();

    loadEGLExtensions();
    dpy = eglGetDisplay(EGL_DEFAULT_DISPLAY);
    checkError(EGL_SUCCESS);

    ptr_eglLabelObjectKHR(EGL_NO_DISPLAY, EGL_OBJECT_THREAD_KHR, NULL, THREAD_LABEL);
    ptr_eglLabelObjectKHR(dpy, EGL_OBJECT_DISPLAY_KHR, (EGLObjectKHR) dpy, DISPLAY_LABEL);

    // Start by enabling the callback and generating some EGL errors. Make sure
    // that the callback gets called with the correct parameters.
    printf("Testing with callback\n");
    ptr_eglDebugMessageControlKHR(debugCallback, NULL);
    testCallback(dpy, EGL_TRUE);

    // Disable the callback and try again. This time, the callback should not
    // be called, but we should still get the same errors from eglGetError.
    printf("Testing with no callback\n");
    ptr_eglDebugMessageControlKHR(NULL, NULL);
    testCallback(dpy, EGL_FALSE);

    // Set a callback, but disable error messages. Again, the callback should
    // not be called.
    printf("Testing with callback and error messages disabled\n");
    callbackAttribs[1] = EGL_FALSE;
    ptr_eglDebugMessageControlKHR(debugCallback, callbackAttribs);
    testCallback(dpy, EGL_FALSE);

    return 0;
}

void testCallback(EGLDisplay dpy, EGLBoolean callbackEnabled)
{
    static const EGLint ERROR_ATTRIBS[] = {
        EGL_CREATE_CONTEXT_FAIL, EGL_BAD_MATCH,
        EGL_NONE
    };

    if (!callbackEnabled) {
        setCallbackNotExpected();
    }

    // Generate an error from libEGL.so.
    printf("Checking eglGetCurrentSurface\n");
    if (callbackEnabled) {
        setCallbackExpected("eglGetCurrentSurface", EGL_BAD_PARAMETER,
                THREAD_LABEL, NULL);
    }
    eglGetCurrentSurface(EGL_NONE);
    checkError(EGL_BAD_PARAMETER);

    // Generate an error from a dispatch stub that expects a display. This
    // should go through the same error reporting as eglGetCurrentSurface did.
    printf("Checking eglCreateContext with invalid display\n");
    if (callbackEnabled) {
        setCallbackExpected("eglCreateContext", EGL_BAD_DISPLAY,
                NULL, NULL);
    }
    eglCreateContext(EGL_NO_DISPLAY, NULL, EGL_NO_CONTEXT, NULL);
    checkError(EGL_BAD_DISPLAY);

    // Generate an error from the vendor library, to make sure that all of the
    // EGL_KHR_debug calls got passed through correctly. The vendor library
    // should pass the display label to the callback, and it uses the vendor
    // name as the message.
    printf("Checking eglCreateContext with valid display\n");
    if (callbackEnabled) {
        setCallbackExpected("eglCreateContext", EGL_BAD_MATCH,
                DISPLAY_LABEL, DUMMY_VENDOR_NAMES[0]);
    }
    eglCreateContext(dpy, NULL, EGL_NO_CONTEXT, ERROR_ATTRIBS);
    checkError(EGL_BAD_MATCH);
}

void setCallbackExpected(const char *command, EGLenum error,
    EGLLabelKHR objectLabel, const char *message)
{
    shouldExpectCallback = EGL_TRUE;
    nextExpectedCommand = command;
    nextExpectedError = error;
    nextExpectedObject = objectLabel;
    nextExpectedMessage = message;
    callbackWasCalled = EGL_FALSE;
}

void setCallbackNotExpected(void)
{
    shouldExpectCallback = EGL_FALSE;
    callbackWasCalled = EGL_FALSE;
}

void EGLAPIENTRY debugCallback(EGLenum error, const char *command,
        EGLint messageType, EGLLabelKHR threadLabel, EGLLabelKHR objectLabel,
        const char *message)
{
    // First, make sure the debug callback was supposed to be called at all.
    if (!shouldExpectCallback) {
        printf("Unexpected callback from \"%s\"\n", command);
        exit(1);
    }

    // Make sure the callback only gets called once.
    if (callbackWasCalled) {
        printf("Callback called multiple times from \"%s\"\n", command);
        exit(1);
    }
    callbackWasCalled = EGL_TRUE;

    if (messageType != EGL_DEBUG_MSG_ERROR_KHR) {
        printf("Unexpected callback type: Expected 0x%04x, got 0x%04x\n",
                EGL_DEBUG_MSG_ERROR_KHR, messageType);
        exit(1);
    }

    if (error != nextExpectedError) {
        printf("Unexpected callback error: Expected 0x%04x, got 0x%04x\n",
                nextExpectedError, error);
        exit(1);
    }

    if (command == NULL) {
        printf("Command is NULL\n");
        exit(1);
    }

    if (nextExpectedCommand != NULL) {
        if (strcmp(nextExpectedCommand, command) != 0) {
            printf("Unexpected command: Expected \"%s\", got \"%s\"\n",
                    nextExpectedCommand, command);
            exit(1);
        }
    }

    if (nextExpectedMessage != NULL) {
        if (message != NULL) {
            if (strcmp(nextExpectedMessage, message) != 0) {
                printf("Unexpected message: Expected \"%s\", got \"%s\"\n",
                        nextExpectedMessage, message);
                exit(1);
            }
        } else {
            printf("Message is NULL, but should be \"%s\"\n", nextExpectedMessage);
            exit(1);
        }
    }

    if (threadLabel != THREAD_LABEL) {
        printf("Unexpected thread label: Expected %p, got %p\n",
                THREAD_LABEL, threadLabel);
        exit(1);
    }

    if (objectLabel != nextExpectedObject) {
        printf("Unexpected object label: Expected %p, got %p\n",
                nextExpectedObject, objectLabel);
        exit(1);
    }
}

void checkError(EGLint expectedError)
{
    EGLint error;

    // If we expected a callback, then make sure we got one.
    if (shouldExpectCallback) {
        if (!callbackWasCalled) {
            printf("Callback was not called\n");
            exit(1);
        }
    }

    // Nothing else should call the callback now.
    setCallbackNotExpected();

    // Regardless of whether we expected a callback, make sure we get the
    // correct error code.
    error = eglGetError();
    if (error != expectedError) {
        printf("Got wrong error: Expected 0x%04x, got 0x%04x\n", expectedError, error);
        exit(1);
    }
}
