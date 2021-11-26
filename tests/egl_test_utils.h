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

#ifndef EGL_TEST_UTILS_H
#define EGL_TEST_UTILS_H

#include <EGL/egl.h>
#include <EGL/eglext.h>
#include "dummy/EGL_dummy.h"

/**
 * The number of dummy vendor libraries.
 */
#define DUMMY_VENDOR_COUNT 2

/**
 * The expected number of devices that should be returned from
 * eglQueryDevicesEXT().
 */
#define DUMMY_TOTAL_DEVICE_COUNT (DUMMY_VENDOR_COUNT * DUMMY_EGL_DEVICE_COUNT)

/**
 * Functions that are exported directly from a vendor library, rather than
 * being accessed through eglGetProcAddress.
 */
typedef struct
{
    pfn_DummySetDeviceCount SetDeviceCount;
} DummyVendorFunctions;

extern const char *DUMMY_VENDOR_NAMES[DUMMY_VENDOR_COUNT];

extern PFNEGLQUERYDEVICESEXTPROC ptr_eglQueryDevicesEXT;
extern PFNEGLQUERYDEVICEATTRIBEXTPROC ptr_eglQueryDeviceAttribEXT;
extern PFNEGLQUERYDEVICESTRINGEXTPROC ptr_eglQueryDeviceStringEXT;
extern PFNEGLQUERYDISPLAYATTRIBEXTPROC ptr_eglQueryDisplayAttribEXT;
extern PFNEGLDEBUGMESSAGECONTROLKHRPROC ptr_eglDebugMessageControlKHR;
extern PFNEGLQUERYDEBUGKHRPROC ptr_eglQueryDebugKHR;
extern PFNEGLLABELOBJECTKHRPROC ptr_eglLabelObjectKHR;

extern pfn_eglTestDispatchDisplay ptr_eglTestDispatchDisplay;
extern pfn_eglTestDispatchDevice ptr_eglTestDispatchDevice;
extern pfn_eglTestDispatchCurrent ptr_eglTestDispatchCurrent;
extern pfn_eglTestReturnDevice ptr_eglTestReturnDevice;

extern DummyVendorFunctions dummyFuncs[DUMMY_VENDOR_COUNT];

/**
 * Loads an EGL extension function with eglGetProcAddress. If it fails, then it
 * calls abort().
 */
__eglMustCastToProperFunctionPointerType loadEGLFunction(const char *name);

/**
 * Loads all of the EGL extension functions that the dummy vendor library
 * supports.
 */
void loadEGLExtensions(void);

/**
 * Loads the additional functions exported by the dummy vendor libraries.
 */
void loadDummyVendorExtensions(void);

/**
 * Frees up any memory allocated by loadDummyVendorExtensions.
 */
void cleanupDummyVendorExtensions(void);

#endif // EGL_TEST_UTILS_H
