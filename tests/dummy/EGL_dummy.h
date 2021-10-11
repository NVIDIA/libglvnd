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
 * Some declarations for the dummy vendor library used to test libEGL.
 *
 * For the EGL tests, we create two vendor libraries. They're both identical,
 * except that each one has its own vendor name string.
 *
 * The vendor name is returned for things like the EGL_VENDOR and GL_VENDOR
 * strings, so that we can check that an EGL or GL call gets dispatched to the
 * right vendor.
 *
 * In addition, you can pass the vendor name as the native display handle to
 * eglGetPlatformDisplay, with EGL_DUMMY_PLATFORM for the platform type. That
 * mainly provides an easy way to create an EGLDisplay without having to mess
 * around with EGLDeviceEXT handles.
 *
 * Using EGL_DUMMY_PLATFORM also tests whether libEGL.so can deal with a call
 * to eglGetPlatformDisplay with an unknown platform, since it does have
 * special handling for EGL_PLATFORM_DEVICE_EXT.
 */


#ifndef EGL_DUMMY_H
#define EGL_DUMMY_H

#include <EGL/egl.h>
#include <EGL/eglext.h>

#include "glvnd_list.h"

#define DUMMY_VENDOR_NAME_0 "dummy0"
#define DUMMY_VENDOR_NAME_1 "dummy1"

/**
 * The number of devices that each dummy vendor library exposes by default.
 * This is used to figure out which vendor library should be behind each
 * device.
 */
#define DUMMY_EGL_DEVICE_COUNT 2

/**
 * The maximum number of devices that each dummy vendor library can expose.
 *
 * This is used to test adding a device after the initial eglQueryDevicesEXT
 * call.
 */
#define DUMMY_EGL_MAX_DEVICE_COUNT 3

/**
 * A platform enum to select a vendor library by name.
 * The native display should be a pointer to a string with the vendor name.
 */
#define EGL_DUMMY_PLATFORM 0x010000

/**
 * This attrbute tells eglCreateContext to set an error and fail. This is used
 * for testing eglGetError and the EGL_KHR_debug functions.
 *
 * For EGL_KHR_debug, the vendor will call the debug callback with its vendor
 * name for the message string.
 */
#define EGL_CREATE_CONTEXT_FAIL 0x010001

/**
 * This attribute is for eglCreatePlatformDisplay. The attribute is the index
 * of the EGLDeviceEXT handle to associate with the display.
 *
 * This is used to test eglQueryDisplayEXT.
 */
#define EGL_DEVICE_INDEX 0x010002

enum
{
    DUMMY_COMMAND_GET_VENDOR_NAME,
    DUMMY_COMMAND_GET_CURRENT_CONTEXT,
    DUMMY_COMMAND_FAIL_NEXT_MAKE_CURRENT,
};

/**
 * The struct that an EGLContext points to. This is used to test
 * eglCreateContext and eglMakeCurrent.
 */
typedef struct DummyEGLContextRec {
    const char *vendorName;

    // Everything after this is used internally by EGL_dummy.c.
    struct glvnd_list entry;
} DummyEGLContext;

/**
 * A simple EGL extension function with a vendor-provided dispatch stub.
 *
 * The function will return the vendor name, so the caller can check whether it
 * gets dispatched to the correct vendor.
 */
typedef void * (* pfn_eglTestDispatchDisplay) (EGLDisplay dpy, EGLint command, EGLAttrib param);

/**
 * Does the same thing as \c eglTestDispatchDisplay, but dispatches based on an
 * EGLDeviceEXT instead of an EGLDisplay.
 */
typedef void * (* pfn_eglTestDispatchDevice) (EGLDeviceEXT dev, EGLint command, EGLAttrib param);

/**
 * Does the same thing as \c eglTestDispatchDisplay, but dispatches based on
 * the current context.
 */
typedef void * (* pfn_eglTestDispatchCurrent) (EGLint command, EGLAttrib param);

/**
 * Returns an EGLDeviceEXT handle from the vendor library.
 *
 * This is used to test returning a device that wasn't listed in a call to
 * eglQueryDevicesEXT.
 */
typedef EGLDeviceEXT (* pfn_eglTestReturnDevice) (EGLDisplay dpy, EGLint index);

/**
 * Changes the number of EGLDeviceEXT handles that the dummy library exposes.
 *
 * This function has to be looked up using dlsym, not eglGetProcAddress.
 */
typedef void (* pfn_DummySetDeviceCount) (EGLint count);

#endif // EGL_DUMMY_H
