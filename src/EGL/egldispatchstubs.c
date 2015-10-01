#include "egldispatchstubs.h"
#include "g_egldispatchstubs.h"

#include <string.h>

static const __EGLapiExports *exports;

const int __EGL_DISPATCH_FUNC_COUNT = __EGL_DISPATCH_COUNT;
int __EGL_DISPATCH_FUNC_INDICES[__EGL_DISPATCH_COUNT + 1];

static int FindProcIndex(const char *name)
{
    unsigned first = 0;
    unsigned last = __EGL_DISPATCH_COUNT - 1;

    while (first <= last) {
        unsigned middle = (first + last) / 2;
        int comp = strcmp(name,
                          __EGL_DISPATCH_FUNC_NAMES[middle]);

        if (comp > 0)
            first = middle + 1;
        else if (comp < 0)
            last = middle - 1;
        else
            return middle;
    }

    /* Just point to the dummy entry at the end of the respective table */
    return __EGL_DISPATCH_COUNT;
}

void __eglInitDispatchStubs(const __EGLapiExports *exportsTable)
{
    int i;
    exports = exportsTable;
    for (i=0; i<__EGL_DISPATCH_FUNC_COUNT; i++) {
        __EGL_DISPATCH_FUNC_INDICES[i] = -1;
    }
}

void __eglSetDispatchIndex(const char *name, int dispatchIndex)
{
    int index = FindProcIndex(name);
    __EGL_DISPATCH_FUNC_INDICES[index] = dispatchIndex;
}

__eglMustCastToProperFunctionPointerType __eglDispatchFindDispatchFunction(const char *name)
{
    int index = FindProcIndex(name);
    return __EGL_DISPATCH_FUNCS[index];
}

static __eglMustCastToProperFunctionPointerType FetchVendorFunc(__EGLvendorInfo *vendor, int index, EGLint errorCode)
{
    if (vendor != NULL) {
        __eglMustCastToProperFunctionPointerType func =
            exports->fetchDispatchEntry(vendor, __EGL_DISPATCH_FUNC_INDICES[index]);
        if (func == NULL) {
            // TODO: What error should we set in this case?
            exports->setEGLError(errorCode);
            return NULL;
        }

        if (!exports->setLastVendor(vendor)) {
            // Don't bother trying to set an error code. If setLastVendor
            // failed, then setEGLError would also fail.
            return NULL;
        }
        return func;
    } else {
        // Set whatever error code we're supposed to set.
        exports->setEGLError(errorCode);
        return NULL;
    }
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
    return FetchVendorFunc(vendor, index, EGL_BAD_DISPLAY);
}

