#include "egl_test_utils.h"

#include <stdio.h>
#include <stdlib.h>

const char *DUMMY_VENDOR_NAMES[DUMMY_VENDOR_COUNT] = {
    DUMMY_VENDOR_NAME_0,
    DUMMY_VENDOR_NAME_1,
};

PFNEGLQUERYDEVICESEXTPROC ptr_eglQueryDevicesEXT;

pfn_eglTestDispatchDisplay ptr_eglTestDispatchDisplay;
pfn_eglTestDispatchDevice ptr_eglTestDispatchDevice;
pfn_eglTestDispatchCurrent ptr_eglTestDispatchCurrent;

__eglMustCastToProperFunctionPointerType loadEGLFunction(const char *name)
{
    __eglMustCastToProperFunctionPointerType ret = eglGetProcAddress(name);
    if (ret == NULL) {
        printf("Can't load function: %s\n", name);
        abort();
    }
    return ret;
}

void loadEGLExtensions(void)
{
    ptr_eglQueryDevicesEXT = (PFNEGLQUERYDEVICESEXTPROC)
        loadEGLFunction("eglQueryDevicesEXT");

    ptr_eglTestDispatchDisplay = (pfn_eglTestDispatchDisplay)
        loadEGLFunction("eglTestDispatchDisplay");
    ptr_eglTestDispatchDevice = (pfn_eglTestDispatchDevice)
        loadEGLFunction("eglTestDispatchDevice");
    ptr_eglTestDispatchCurrent = (pfn_eglTestDispatchCurrent)
        loadEGLFunction("eglTestDispatchCurrent");
}

