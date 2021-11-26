#include "egl_test_utils.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dlfcn.h>

const char *DUMMY_VENDOR_NAMES[DUMMY_VENDOR_COUNT] = {
    DUMMY_VENDOR_NAME_0,
    DUMMY_VENDOR_NAME_1,
};

PFNEGLQUERYDEVICESEXTPROC ptr_eglQueryDevicesEXT;
PFNEGLDEBUGMESSAGECONTROLKHRPROC ptr_eglDebugMessageControlKHR;
PFNEGLQUERYDEBUGKHRPROC ptr_eglQueryDebugKHR;
PFNEGLLABELOBJECTKHRPROC ptr_eglLabelObjectKHR;
PFNEGLQUERYDEVICEATTRIBEXTPROC ptr_eglQueryDeviceAttribEXT;
PFNEGLQUERYDEVICESTRINGEXTPROC ptr_eglQueryDeviceStringEXT;
PFNEGLQUERYDISPLAYATTRIBEXTPROC ptr_eglQueryDisplayAttribEXT;

pfn_eglTestDispatchDisplay ptr_eglTestDispatchDisplay;
pfn_eglTestDispatchDevice ptr_eglTestDispatchDevice;
pfn_eglTestDispatchCurrent ptr_eglTestDispatchCurrent;
pfn_eglTestReturnDevice ptr_eglTestReturnDevice;

static void *dummyVendorHandles[DUMMY_VENDOR_COUNT] = {};
DummyVendorFunctions dummyFuncs[DUMMY_VENDOR_COUNT] = {};

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
    ptr_eglQueryDeviceAttribEXT = (PFNEGLQUERYDEVICEATTRIBEXTPROC)
        loadEGLFunction("eglQueryDeviceAttribEXT");
    ptr_eglQueryDeviceStringEXT = (PFNEGLQUERYDEVICESTRINGEXTPROC)
        loadEGLFunction("eglQueryDeviceStringEXT");
    ptr_eglQueryDisplayAttribEXT = (PFNEGLQUERYDISPLAYATTRIBEXTPROC)
        loadEGLFunction("eglQueryDisplayAttribEXT");
    ptr_eglDebugMessageControlKHR = (PFNEGLDEBUGMESSAGECONTROLKHRPROC)
        loadEGLFunction("eglDebugMessageControlKHR");
    ptr_eglQueryDebugKHR = (PFNEGLQUERYDEBUGKHRPROC)
        loadEGLFunction("eglQueryDebugKHR");
    ptr_eglLabelObjectKHR = (PFNEGLLABELOBJECTKHRPROC)
        loadEGLFunction("eglLabelObjectKHR");

    ptr_eglTestDispatchDisplay = (pfn_eglTestDispatchDisplay)
        loadEGLFunction("eglTestDispatchDisplay");
    ptr_eglTestDispatchDevice = (pfn_eglTestDispatchDevice)
        loadEGLFunction("eglTestDispatchDevice");
    ptr_eglTestDispatchCurrent = (pfn_eglTestDispatchCurrent)
        loadEGLFunction("eglTestDispatchCurrent");
    ptr_eglTestReturnDevice = (pfn_eglTestReturnDevice)
        loadEGLFunction("eglTestReturnDevice");
}

void loadDummyVendorExtensions(void)
{
    int i;

    for (i=0; i<DUMMY_VENDOR_COUNT; i++)
    {
        if (dummyVendorHandles[i] == NULL)
        {
            char filename[128];
            snprintf(filename, sizeof(filename), "libEGL_%s.so.0", DUMMY_VENDOR_NAMES[i]);
            dummyVendorHandles[i] = dlopen(filename, RTLD_LAZY | RTLD_LOCAL);
            if (dummyVendorHandles[i] == NULL)
            {
                printf("Failed to load %s: %s\n", filename, dlerror());
                abort();
            }

            dummyFuncs[i].SetDeviceCount = dlsym(dummyVendorHandles[i], "DummySetDeviceCount");
            if (dummyFuncs[i].SetDeviceCount == NULL)
            {
                printf("Can't load DummySetDeviceCount from %s\n", filename);
                abort();
            }
        }
    }
}

void cleanupDummyVendorExtensions(void)
{
    int i;

    for (i=0; i<DUMMY_VENDOR_COUNT; i++)
    {
        if (dummyVendorHandles[i] != NULL)
        {
            dlclose(dummyVendorHandles[i]);
            dummyVendorHandles[i] = NULL;
        }
    }

    memset(&dummyFuncs, 0, sizeof(dummyFuncs));
}

