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

#include <pthread.h>
#include <string.h>

#if defined(HASH_DEBUG)
# include <stdio.h>
#endif

#include "libeglcurrent.h"
#include "libeglmapping.h"
#include "glvnd_pthread.h"
#include "egldispatchstubs.h"
#include "utils_misc.h"
#include "trace.h"

#include "lkdhash.h"

static glvnd_mutex_t dispatchIndexMutex = GLVND_MUTEX_INITIALIZER;

__EGLdeviceInfo *__eglDeviceList = NULL;
__EGLdeviceInfo *__eglDeviceHash = NULL;
int __eglDeviceCount = 0;
static glvnd_once_t deviceListInitOnce = GLVND_ONCE_INIT;

/****************************************************************************/

typedef struct __EGLdisplayInfoHashRec {
    __EGLdisplayInfo info;
    UT_hash_handle hh;
} __EGLdisplayInfoHash;

static DEFINE_INITIALIZED_LKDHASH(__EGLdisplayInfoHash, __eglDisplayInfoHash);

__eglMustCastToProperFunctionPointerType __eglGetEGLDispatchAddress(const char *procName)
{
    struct glvnd_list *vendorList = __eglLoadVendors();
    __EGLvendorInfo *vendor;
    __eglMustCastToProperFunctionPointerType addr = NULL;
    int index;

    __glvndPthreadFuncs.mutex_lock(&dispatchIndexMutex);

    index = __glvndWinsysDispatchFindIndex(procName);
    if (index >= 0) {
        addr = (__eglMustCastToProperFunctionPointerType) __glvndWinsysDispatchGetDispatch(index);
        __glvndPthreadFuncs.mutex_unlock(&dispatchIndexMutex);
        return addr;
    }

    // Check each vendor library for a dispatch stub.
    glvnd_list_for_each_entry(vendor, vendorList, entry) {
        addr = vendor->eglvc.getDispatchAddress(procName);
        if (addr != NULL) {
            break;
        }
    }
    if (addr != NULL) {
        index = __glvndWinsysDispatchAllocIndex(procName, addr);
        if (index >= 0) {
            glvnd_list_for_each_entry(vendor, vendorList, entry) {
                vendor->eglvc.setDispatchIndex(procName, index);
            }
        } else {
            addr = NULL;
        }
    }

    __glvndPthreadFuncs.mutex_unlock(&dispatchIndexMutex);
    return addr;
}

__eglMustCastToProperFunctionPointerType __eglFetchDispatchEntry(
        __EGLvendorInfo *vendor, int index)
{
    __eglMustCastToProperFunctionPointerType addr = NULL;
    const char *procName = NULL;

    addr = (__eglMustCastToProperFunctionPointerType)
        __glvndWinsysVendorDispatchLookupFunc(vendor->dynDispatch, index);
    if (addr != NULL) {
        return addr;
    }

    // Not seen before by this vendor: query the vendor for the right
    // address to use.

    __glvndPthreadFuncs.mutex_lock(&dispatchIndexMutex);
    procName = __glvndWinsysDispatchGetName(index);
    __glvndPthreadFuncs.mutex_unlock(&dispatchIndexMutex);

    if (procName == NULL) {
        // Not a valid function index.
        return NULL;
    }

    // Get the real address.
    addr = vendor->eglvc.getProcAddress(procName);
    if (addr != NULL) {
        // Record the address in the vendor's hashtable. Note that if this
        // fails, it's not fatal. It just means we'll have to call
        // getProcAddress again the next time we need this function.
        __glvndWinsysVendorDispatchAddFunc(vendor->dynDispatch, index, addr);
    }
    return addr;
}

__EGLvendorInfo *__eglGetVendorFromDisplay(EGLDisplay dpy)
{
    __EGLdisplayInfo *dpyInfo = __eglLookupDisplay(dpy);
    if (dpyInfo != NULL) {
        return dpyInfo->vendor;
    } else {
        return NULL;
    }
}

/**
 * Allocates and initializes a __EGLdisplayInfoHash structure.
 *
 * The caller is responsible for adding the structure to the hashtable.
 *
 * \param dpy The display connection.
 * \return A newly-allocated __EGLdisplayInfoHash structure, or NULL on error.
 */
static __EGLdisplayInfoHash *InitDisplayInfoEntry(EGLDisplay dpy, __EGLvendorInfo *vendor)
{
    __EGLdisplayInfoHash *pEntry;

    pEntry = (__EGLdisplayInfoHash *) calloc(1, sizeof(*pEntry));
    if (pEntry == NULL) {
        return NULL;
    }

    pEntry->info.dpy = dpy;
    pEntry->info.vendor = vendor;

    return pEntry;
}

__EGLdisplayInfo *__eglLookupDisplay(EGLDisplay dpy)
{
    __EGLdisplayInfoHash *pEntry = NULL;

    if (dpy == EGL_NO_DISPLAY) {
        return NULL;
    }

    LKDHASH_RDLOCK(__eglDisplayInfoHash);
    HASH_FIND_PTR(_LH(__eglDisplayInfoHash), &dpy, pEntry);
    LKDHASH_UNLOCK(__eglDisplayInfoHash);

    if (pEntry != NULL) {
        return &pEntry->info;
    } else {
        return NULL;
    }
}

__EGLdisplayInfo *__eglAddDisplay(EGLDisplay dpy, __EGLvendorInfo *vendor)
{
    __EGLdisplayInfoHash *pEntry = NULL;

    if (dpy == EGL_NO_DISPLAY) {
        return NULL;
    }

    LKDHASH_WRLOCK(__eglDisplayInfoHash);
    HASH_FIND_PTR(_LH(__eglDisplayInfoHash), &dpy, pEntry);
    if (pEntry == NULL) {
        pEntry = InitDisplayInfoEntry(dpy, vendor);
        if (pEntry != NULL) {
            HASH_ADD_PTR(_LH(__eglDisplayInfoHash), info.dpy, pEntry);
        }
    }

    LKDHASH_UNLOCK(__eglDisplayInfoHash);
    if (pEntry != NULL && pEntry->info.vendor == vendor) {
        return &pEntry->info;
    } else {
        return NULL;
    }
}

void __eglFreeDisplay(EGLDisplay dpy)
{
    __EGLdisplayInfoHash *pEntry = NULL;

    LKDHASH_WRLOCK(__eglDisplayInfoHash);
    HASH_FIND_PTR(_LH(__eglDisplayInfoHash), &dpy, pEntry);
    if (pEntry != NULL) {
        HASH_DEL(_LH(__eglDisplayInfoHash), pEntry);
    }
    LKDHASH_UNLOCK(__eglDisplayInfoHash);

    if (pEntry != NULL) {
        free(pEntry);
    }
}

void __eglMappingInit(void)
{
    int i;
    __eglInitDispatchStubs(&__eglExportsTable);
    for (i=0; i<__EGL_DISPATCH_FUNC_COUNT; i++) {
        int index = __glvndWinsysDispatchAllocIndex(
                __EGL_DISPATCH_FUNC_NAMES[i],
                __EGL_DISPATCH_FUNCS[i]);
        if (index < 0) {
            fprintf(stderr, "Could not allocate dispatch index array\n");
            abort();
        }
        __EGL_DISPATCH_FUNC_INDICES[i] = index;
    }
}

void __eglMappingTeardown(EGLBoolean doReset)
{
    if (doReset) {
        //__EGLdisplayInfoHash *dpyInfoEntry, *dpyInfoTmp;

        /*
         * If we're just doing fork recovery, we don't actually want to unload
         * any currently loaded vendors _or_ remove any mappings (they should
         * still be valid in the new process, and may be needed if the child
         * tries using pointers/XIDs that were created in the parent).  Just
         * reset the corresponding locks.
         */
        __glvndPthreadFuncs.mutex_init(&dispatchIndexMutex, NULL);
        __glvndPthreadFuncs.rwlock_init(&__eglDisplayInfoHash.lock, NULL);
    } else {
        /* Tear down all hashtables used in this file */
        LKDHASH_TEARDOWN(__EGLdisplayInfoHash,
                         __eglDisplayInfoHash, NULL, NULL, EGL_FALSE);

       __glvndWinsysDispatchCleanup();
    }
}

static EGLBoolean AddVendorDevices(__EGLvendorInfo *vendor)
{
    EGLDeviceEXT *devices = NULL;
    EGLint count = 0;
    __EGLdeviceInfo *newDevList;
    EGLint i, j;

    if (!vendor->supportsDevice) {
        return EGL_TRUE;
    }

    if (!vendor->staticDispatch.queryDevicesEXT(0, NULL, &count)) {
        return EGL_FALSE;
    }
    if (count <= 0) {
        return EGL_TRUE;
    }

    devices = (EGLDeviceEXT *) malloc(count * sizeof(EGLDeviceEXT));
    if (devices == NULL) {
        return EGL_FALSE;
    }

    if (!vendor->staticDispatch.queryDevicesEXT(count, devices, &count)) {
        free(devices);
        return EGL_FALSE;
    }

    newDevList = (__EGLdeviceInfo *) realloc(__eglDeviceList,
            (__eglDeviceCount + count) * sizeof(__EGLdeviceInfo));
    if (newDevList == NULL) {
        free(devices);
        return EGL_FALSE;
    }
    __eglDeviceList = newDevList;

    for (i=0; i<count; i++) {
        // Make sure we haven't already gotten a device with this handle.
        EGLBoolean found = EGL_FALSE;
        for (j=0; j<__eglDeviceCount; j++) {
            if (__eglDeviceList[j].handle == devices[i]) {
                found = EGL_TRUE;
                break;
            }
        }

        if (!found) {
            __eglDeviceList[__eglDeviceCount].handle = devices[i];
            __eglDeviceList[__eglDeviceCount].vendor = vendor;
            __eglDeviceCount++;
        }
    }
    free(devices);

    return EGL_TRUE;
}

void InitDeviceListInternal(void)
{
    struct glvnd_list *vendorList = __eglLoadVendors();
    __EGLvendorInfo *vendor;
    EGLint i;

    __eglDeviceList = NULL;
    __eglDeviceHash = NULL;
    __eglDeviceCount = 0;

    glvnd_list_for_each_entry(vendor, vendorList, entry) {
        if (!AddVendorDevices(vendor)) {
            free(__eglDeviceList);
            __eglDeviceList = NULL;
            __eglDeviceCount = 0;
            return;
        }
    }

    // Build a hashtable for the devices.
    for (i=0; i<__eglDeviceCount; i++) {
        __EGLdeviceInfo *dev = &__eglDeviceList[i];
        HASH_ADD_PTR(__eglDeviceHash, handle, dev);
    }
}

void __eglInitDeviceList(void)
{
    __glvndPthreadFuncs.once(&deviceListInitOnce, InitDeviceListInternal);
}

__EGLvendorInfo *__eglGetVendorFromDevice(EGLDeviceEXT dev)
{
    __EGLdeviceInfo *devInfo;

    __eglInitDeviceList();

    HASH_FIND_PTR(__eglDeviceHash, &dev, devInfo);
    if (devInfo != NULL) {
        return devInfo->vendor;
    } else {
        return NULL;
    }
}

