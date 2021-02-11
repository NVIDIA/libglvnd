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
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "dummy/EGL_dummy.h"
#include "egl_test_utils.h"

#define DEVICE_ARRAY_SIZE (DUMMY_EGL_MAX_DEVICE_COUNT * DUMMY_VENDOR_COUNT)

EGLBoolean CheckDeviceHandles(EGLDeviceEXT *devices, EGLint count)
{
    EGLint i, j;

    for (i=0; i<count; i++) {
        for (j=0; j<i; j++) {
            if (devices[i] == EGL_NO_DEVICE_EXT) {
                printf("Got EGL_NO_DEVICE_EXT at index %d\n", i);
                return EGL_FALSE;
            }
            if (devices[i] == devices[j]) {
                printf("Got duplicate device handles at index %d, %d\n", i, j);
                return EGL_FALSE;
            }
        }
    }

    return EGL_TRUE;
}

/**
 * Common function to get a list of devices. This will check to make sure that
 * we get the number of devices that we expect, that none of them are
 * EGL_NO_DEVICE_EXT, and that there are no duplicates.
 */
EGLBoolean CommonGetDevices(EGLDeviceEXT *devices, EGLint expectedCount)
{
    EGLint count = -1;

    if (!ptr_eglQueryDevicesEXT(0, NULL, &count)) {
        printf("eglQueryDevicesEXT(count) failed\n");
        return EGL_FALSE;
    }

    if (count != expectedCount) {
        printf("eglQueryDevicesEXT(count) returned the wrong count\n");
        printf("Expected %d, but got %d\n", expectedCount, count);
        return EGL_FALSE;
    }

    if (!ptr_eglQueryDevicesEXT(expectedCount, devices, &count)) {
        printf("eglQueryDevicesEXT(get) failed\n");
        return EGL_FALSE;
    }

    if (count != expectedCount) {
        printf("eglQueryDevicesEXT(ge) returned the wrong count\n");
        printf("Expected %d, but got %d\n", expectedCount, count);
        return EGL_FALSE;
    }

    if (!CheckDeviceHandles(devices, count)) {
        return EGL_FALSE;
    }

    return EGL_TRUE;
}

EGLBoolean CheckDeviceVendors(EGLDeviceEXT *devices, EGLint count, const char *name)
{
    EGLint i;
    for (i=0; i<count; i++)
    {
        const char *str = ptr_eglTestDispatchDevice(devices[i], DUMMY_COMMAND_GET_VENDOR_NAME, 0);
        if (str == NULL) {
            printf("eglTestDispatchDevice failed at index %s/%d, error 0x%04x\n", name, i, eglGetError());
            return EGL_FALSE;
        }
        if (strcmp(str, name) != 0) {
            printf("Got the wrong vendor string from device at index %d\n", i);
            printf("Expected \"%s\", but got \"%s\"\n", name, str);
            return EGL_FALSE;
        }
    }

    return EGL_TRUE;
}

EGLBoolean TestNewDevice(EGLDeviceEXT newDevice, EGLDeviceEXT *oldDevices, EGLint oldDeviceCount)
{
    EGLint i;
    const char *str;

    if (newDevice == EGL_NO_DEVICE_EXT) {
        printf("New device is EGL_NO_DEVICE_EXT\n");
        return EGL_FALSE;
    }

    // Make sure that the newly added device doesn't show up in the old list
    for (i=0; i<oldDeviceCount; i++) {
        if (oldDevices[i] == newDevice) {
            printf("New device was at index %d\n", i);
            return EGL_FALSE;
        }
    }

    // Make sure we can dispatch using the new device
    str = ptr_eglTestDispatchDevice(newDevice, DUMMY_COMMAND_GET_VENDOR_NAME, 0);
    if (str == NULL) {
        printf("eglTestDispatchDevice failed with new device, error 0x%04x\n", eglGetError());
        return EGL_FALSE;
    }
    if (strcmp(str, DUMMY_VENDOR_NAMES[0]) != 0) {
        printf("Got the wrong vendor string from device at index %d\n", i);
        printf("Expected \"%s\", but got \"%s\"\n", DUMMY_VENDOR_NAMES[0], str);
        return EGL_FALSE;
    }

    return EGL_TRUE;
}

EGLBoolean TestAddQueryDevices(EGLDeviceEXT *oldDevices, EGLint oldDeviceCount)
{
    EGLDeviceEXT devices[DEVICE_ARRAY_SIZE] = {};
    EGLint deviceCount = -1;
    const EGLint expectedDeviceCount = DUMMY_EGL_DEVICE_COUNT * DUMMY_VENDOR_COUNT + 1;
    EGLDeviceEXT newDevice;

    printf("Testing second eglQueryDevicesEXT call.\n");

    if (!ptr_eglQueryDevicesEXT(DEVICE_ARRAY_SIZE, devices, &deviceCount)) {
        printf("eglQueryDevicesEXT (2) failed\n");
        return EGL_FALSE;
    }
    if (deviceCount != expectedDeviceCount) {
        printf("eglQueryDevicesEXT returned the wrong count\n");
        printf("Expected %d, but got %d\n", expectedDeviceCount, deviceCount);
        return EGL_FALSE;
    }

    if (!CheckDeviceHandles(devices, deviceCount)) {
        return EGL_FALSE;
    }

    newDevice = devices[DUMMY_EGL_DEVICE_COUNT];
    return TestNewDevice(newDevice, oldDevices, oldDeviceCount);
}

EGLBoolean TestReturnDevice(EGLDeviceEXT *oldDevices, EGLint oldDeviceCount)
{
    EGLDisplay dpy;
    EGLint major, minor;
    EGLDeviceEXT newDevice;

    printf("Testing vendor-provided dispatch function.\n");

    dpy = eglGetPlatformDisplay(EGL_DUMMY_PLATFORM, (EGLNativeDisplayType) DUMMY_VENDOR_NAMES[0], NULL);
    if (dpy == EGL_NO_DISPLAY) {
        printf("eglGetPlatformDisplay failed with 0x%04x\n", eglGetError());
        return EGL_FALSE;
    }
    if (!eglInitialize(dpy, &major, &minor)) {
        printf("eglInitialize failed with 0x%04x\n", eglGetError());
        return EGL_FALSE;
    }

    newDevice = ptr_eglTestReturnDevice(dpy, DUMMY_EGL_DEVICE_COUNT);
    eglTerminate(dpy);

    return TestNewDevice(newDevice, oldDevices, oldDeviceCount);
}

EGLBoolean TestQueryDisplay(EGLDeviceEXT *oldDevices, EGLint oldDeviceCount)
{
    const EGLAttrib DISPLAY_ATTRIBS[] =
    {
        EGL_DEVICE_INDEX, DUMMY_EGL_DEVICE_COUNT, EGL_NONE
    };
    EGLDisplay dpy;
    EGLint major, minor;
    EGLAttrib newDevice = -1;

    printf("Testing eglQueryDisplayAttribEXT.\n");

    dpy = eglGetPlatformDisplay(EGL_DUMMY_PLATFORM, (EGLNativeDisplayType) DUMMY_VENDOR_NAMES[0], DISPLAY_ATTRIBS);
    if (dpy == EGL_NO_DISPLAY) {
        printf("eglGetPlatformDisplay failed with 0x%04x\n", eglGetError());
        return EGL_FALSE;
    }
    if (!eglInitialize(dpy, &major, &minor)) {
        printf("eglInitialize failed with 0x%04x\n", eglGetError());
        return EGL_FALSE;
    }

    if (!ptr_eglQueryDisplayAttribEXT(dpy, EGL_DEVICE_EXT, &newDevice)) {
        printf("ptr_eglQueryDisplayAttribEXT failed with 0x%04x\n", eglGetError());
        return EGL_FALSE;
    }
    eglTerminate(dpy);

    return TestNewDevice((EGLDeviceEXT) newDevice, oldDevices, oldDeviceCount);
}


int main(int argc, char **argv)
{
    EGLDeviceEXT devices[DEVICE_ARRAY_SIZE] = {};
    const EGLint deviceCount = DUMMY_EGL_DEVICE_COUNT * DUMMY_VENDOR_COUNT;
    EGLint i;

    loadEGLExtensions();
    loadDummyVendorExtensions();

    printf("Getting initial device list.\n");
    if (!CommonGetDevices(devices, deviceCount)) {
        return 1;
    }
    // Make sure that we can dispatch using each device
    for (i=0; i<DUMMY_VENDOR_COUNT; i++) {
        if (!CheckDeviceVendors(devices + (i * DUMMY_EGL_DEVICE_COUNT),
                    DUMMY_EGL_DEVICE_COUNT, DUMMY_VENDOR_NAMES[i])) {
            return 1;
        }
    }

    // Add a device to the first vendor.
    dummyFuncs[0].SetDeviceCount(DUMMY_EGL_DEVICE_COUNT + 1);

    for (i=1; i<argc; i++) {
        EGLBoolean success = EGL_FALSE;
        if (strcmp(argv[i], "querydevices") == 0) {
            success = TestAddQueryDevices(devices, deviceCount);
        } else if (strcmp(argv[i], "returndevice") == 0) {
            success = TestReturnDevice(devices, deviceCount);
        } else if (strcmp(argv[i], "querydisplay") == 0) {
            success = TestQueryDisplay(devices, deviceCount);
        } else {
            printf("Invalid test name: %s\n", argv[i]);
        }
        if (!success) {
            return 1;
        }
    }

    cleanupDummyVendorExtensions();
    return 0;
}

