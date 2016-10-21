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

int main(int argc, char **argv)
{
    EGLDeviceEXT devices[DUMMY_TOTAL_DEVICE_COUNT];
    EGLDisplay displays[DUMMY_TOTAL_DEVICE_COUNT];
    EGLint deviceCount = -1;
    EGLint i, j;

    loadEGLExtensions();

    // Make sure the total number of devices is what we expect.
    printf("Checking device count\n");
    if (!ptr_eglQueryDevicesEXT(0, NULL, &deviceCount)) {
        printf("eglQueryDevicesEXT failed\n");
        return 1;
    }
    if (deviceCount != DUMMY_TOTAL_DEVICE_COUNT) {
        printf("eglQueryDevicesEXT returned the wrong count\n");
        printf("Expected %d, but got %d\n", DUMMY_TOTAL_DEVICE_COUNT, deviceCount);
        return 1;
    }

    printf("Getting device handles.\n");
    if (!ptr_eglQueryDevicesEXT(DUMMY_TOTAL_DEVICE_COUNT, devices, &deviceCount)) {
        printf("eglQueryDevicesEXT failed\n");
        return 1;
    }
    if (deviceCount != DUMMY_TOTAL_DEVICE_COUNT) {
        printf("eglQueryDevicesEXT returned the wrong count\n");
        printf("Expected %d, but got %d\n", DUMMY_TOTAL_DEVICE_COUNT, deviceCount);
        return 1;
    }

    // Check to make sure that there are no duplicates in the device list.
    for (i=0; i<deviceCount; i++) {
        for (j=0; j<i; j++) {
            if (devices[i] == EGL_NO_DEVICE_EXT) {
                printf("Got EGL_NO_DEVICE_EXT at index %d\n", i);
                return 1;
            }
            if (devices[i] == devices[j]) {
                printf("Got duplicate device handles at index %d, %d\n", i, j);
                return 1;
            }
        }
    }

    for (i=0; i<deviceCount; i++) {
        EGLint major, minor;
        const char *name = DUMMY_VENDOR_NAMES[i / DUMMY_EGL_DEVICE_COUNT];
        const char *str;

        // First, test whether an EGL function gets dispatched to the correct
        // vendor based on this device.
        str = ptr_eglTestDispatchDevice(devices[i], DUMMY_COMMAND_GET_VENDOR_NAME, 0);
        if (str == NULL) {
            printf("eglTestDispatchDevice failed at index %d, error 0x%04x\n", i, eglGetError());
            return 1;
        }
        if (strcmp(str, name) != 0) {
            printf("Got the wrong vendor string from device at index %d\n", i);
            printf("Expected \"%s\", but got \"%s\"\n", name, str);
            return 1;
        }

        // Create an EGLDisplay from the device.
        displays[i] = eglGetPlatformDisplay(EGL_PLATFORM_DEVICE_EXT,
                devices[i], NULL);
        if (displays[i] == EGL_NO_DISPLAY) {
            printf("eglGetPlatformDisplay failed at index %d, error 0x%04x\n", i, eglGetError());
            return 1;
        }

        // Each EGLDeviceEXT handle should give us a different EGLDisplay.
        for (j=0; j<i; j++) {
            if (displays[i] == displays[j]) {
                printf("Got duplicate EGLDisplay at index %d, %d\n", i, j);
                return 1;
            }
        }

        if (!eglInitialize(displays[i], &major, &minor)) {
            printf("eglInitialize failed at index %d, error 0x%04x\n", i, eglGetError());
            return 1;
        }

        // Call eglQueryString to make sure that each display belongs to the
        // correct vendor.
        str = eglQueryString(displays[i], EGL_VENDOR);
        if (str == NULL) {
            printf("eglQueryString failed at index %d, error 0x%04x\n", i, eglGetError());
            return 1;
        }
        if (strcmp(str, name) != 0) {
            printf("Got the wrong vendor string from display at index %d\n", i);
            printf("Expected \"%s\", but got \"%s\"\n", name, str);
            return 1;
        }
    }

    return 0;
}
