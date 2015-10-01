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

#if !defined(__LIB_EGL_MAPPING_H)
#define __LIB_EGL_MAPPING_H

#include "glvnd_pthread.h"
#include "libeglabipriv.h"
#include "GLdispatch.h"
#include "lkdhash.h"
#include "libeglvendor.h"

/*!
 * Structure containing per-display information.
 */
typedef struct __EGLdisplayInfoRec {
    EGLDisplay dpy;

    /**
     * The vendor that this display belongs to.
     */
    __EGLvendorInfo *vendor;
} __EGLdisplayInfo;

typedef struct __EGLdeviceInfoRec {
    EGLDeviceEXT handle;
    __EGLvendorInfo *vendor;
    UT_hash_handle hh;
} __EGLdeviceInfo;

extern __EGLdeviceInfo *__eglDeviceList;
extern __EGLdeviceInfo *__eglDeviceHash;
extern int __eglDeviceCount;

void __eglThreadInitialize(void);

/*!
 * Initializes the mapping functions.
 */
void __eglMappingInit(void);

/*!
 * Initializes the EGLDeviceEXT list and hashtable.
 *
 * This function must be called before trying to access the \c __eglDeviceList
 * array.
 */
void __eglInitDeviceList(void);

/*!
 * This handles freeing all mapping state during library teardown
 * or resetting locks on fork recovery.
 */
void __eglMappingTeardown(EGLBoolean doReset);

const __EGLdeviceInfo *__eglGetDeviceList(EGLint *deviceCount);

/*!
 * Looks up the __EGLdisplayInfo structure for a display. If the display does
 * not exist, then this returns NULL.
 */
__EGLdisplayInfo *__eglLookupDisplay(EGLDisplay dpy);

/*!
 * Adds an EGLDisplay to the display hashtable.
 *
 * If \p dpy is not already in the table, then this will create a new
 * __EGLdisplayInfo struct for it and add it to the table.
 *
 * If \p dpy is already in the table, then it will return the existing
 * __EGLdisplayInfo struct for it.
 */
__EGLdisplayInfo *__eglAddDisplay(EGLDisplay dpy, __EGLvendorInfo *vendor);

/*!
 * Frees the __EGLdisplayInfo structure for a display, if one exists.
 */
void __eglFreeDisplay(EGLDisplay dpy);

__EGLvendorInfo *__eglGetVendorFromDisplay(EGLDisplay dpy);

/*!
 * Looks up a dispatch function.
 *
 * procName can be an EGL or OpenGL function.
 */
__eglMustCastToProperFunctionPointerType __eglGetEGLDispatchAddress(const char *procName);

__eglMustCastToProperFunctionPointerType __eglFetchDispatchEntry(__EGLvendorInfo *vendor, int index);

__EGLvendorInfo *__eglGetVendorFromDevice(EGLDeviceEXT dev);

void __eglSetError(EGLint errorCode);

EGLBoolean __eglSetLastVendor(__EGLvendorInfo *vendor);

/*!
 * This is called at the beginning of every EGL function.
 */
void __eglEntrypointCommon(void);

#endif /* __LIB_EGL_MAPPING_H */
