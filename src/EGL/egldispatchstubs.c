#include "egldispatchstubs.h"
#include "g_egldispatchstubs.h"

#include <string.h>

// This is needed only for __eglDebugReport. Vendor libraries will typically
// want to replace this with whatever error reporting function they use.
#include "libeglerror.h"

static const __EGLapiExports *exports;

const int __EGL_DISPATCH_FUNC_COUNT = __EGL_DISPATCH_COUNT;
int __EGL_DISPATCH_FUNC_INDICES[__EGL_DISPATCH_COUNT + 1];

void __eglInitDispatchStubs(const __EGLapiExports *exportsTable)
{
    int i;
    exports = exportsTable;
    for (i=0; i<__EGL_DISPATCH_FUNC_COUNT; i++) {
        __EGL_DISPATCH_FUNC_INDICES[i] = -1;
    }
}

static __eglMustCastToProperFunctionPointerType FetchVendorFunc(__EGLvendorInfo *vendor,
        int index, EGLint errorCode)
{
    __eglMustCastToProperFunctionPointerType func = NULL;

    if (vendor != NULL) {
        func = exports->fetchDispatchEntry(vendor, __EGL_DISPATCH_FUNC_INDICES[index]);
    }
    if (func == NULL) {
        if (errorCode != EGL_SUCCESS) {
            __eglReportError(errorCode, __EGL_DISPATCH_FUNC_NAMES[index], NULL, NULL);
        }
        return NULL;
    }

    if (!exports->setLastVendor(vendor)) {
        // Don't bother trying to set an error code. If setLastVendor
        // failed, then setEGLError would also fail.
        __eglReportError(EGL_BAD_ALLOC, __EGL_DISPATCH_FUNC_NAMES[index], NULL,
                "Could not initialize thread state");
        return NULL;
    }

    return func;
}

__eglMustCastToProperFunctionPointerType __eglDispatchFetchByCurrent(int index)
{
    __EGLvendorInfo *vendor;

    // Note: This is only used for the eglWait* functions. For those, if
    // there's no current context, then they're supposed to do nothing but
    // return success.
    exports->threadInit();
    vendor = exports->getCurrentVendor();
    return FetchVendorFunc(vendor, index, EGL_SUCCESS);
}

__eglMustCastToProperFunctionPointerType __eglDispatchFetchByDisplay(EGLDisplay dpy, int index)
{
    __EGLvendorInfo *vendor;

    exports->threadInit();
    vendor = exports->getVendorFromDisplay(dpy);
    return FetchVendorFunc(vendor, index, EGL_BAD_DISPLAY);
}

__eglMustCastToProperFunctionPointerType __eglDispatchFetchByDevice(EGLDeviceEXT dev, int index)
{
    __EGLvendorInfo *vendor;

    exports->threadInit();
    vendor = exports->getVendorFromDevice(dev);
    return FetchVendorFunc(vendor, index, EGL_BAD_DEVICE_EXT);
}

