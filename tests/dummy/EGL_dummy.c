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

#include "EGL_dummy.h"

#include <stdlib.h>
#include <assert.h>

#include "glvnd/libeglabi.h"
#include "glvnd_list.h"
#include "glvnd_pthread.h"
#include "compiler.h"

enum
{
    DI_eglTestDispatchDisplay,
    DI_eglTestDispatchDevice,
    DI_eglTestDispatchCurrent,
    DI_eglTestReturnDevice,
    DI_eglQueryDeviceAttribEXT,
    DI_eglQueryDeviceStringEXT,    
    DI_COUNT,
};

static const char *CLIENT_EXTENSIONS =
    "EGL_KHR_client_get_all_proc_addresses"
    " EGL_EXT_client_extensions"
    " EGL_EXT_device_base"
    " EGL_EXT_device_enumeration"
    " EGL_EXT_device_query"
    ;

static const char *PLATFORM_EXTENSIONS =
    "EGL_EXT_platform_device"
    ;

static const char *DISPLAY_EXTENSIONS = "";

typedef struct DummyEGLDisplayRec {
    EGLenum platform;
    void *native_display;
    EGLLabelKHR label;
    EGLDeviceEXT device;

    struct glvnd_list entry;
} DummyEGLDisplay;

typedef struct DummyThreadStateRec {
    EGLint lastError;
    EGLContext currentContext;
    EGLLabelKHR label;

    struct glvnd_list entry;
} DummyThreadState;

static const __EGLapiExports *apiExports = NULL;

static glvnd_mutex_t threadStateLock = GLVND_MUTEX_INITIALIZER;
static glvnd_key_t threadStateKey;
static struct glvnd_list threadStateList = { &threadStateList, &threadStateList };

static struct glvnd_list displayList = { &displayList, &displayList };
static glvnd_mutex_t displayListLock = GLVND_MUTEX_INITIALIZER;
static EGLint failNextMakeCurrentError = EGL_NONE;

static glvnd_mutex_t contextListLock = GLVND_MUTEX_INITIALIZER;
static struct glvnd_list contextList = { &contextList, &contextList };

static EGLDEBUGPROCKHR debugCallbackFunc = NULL;
static EGLBoolean debugCallbackEnabled = EGL_TRUE;

// The EGLDeviceEXT handle values have to be pointers, so just use the
// address of an array element for each EGLDeviceEXT handle.
static const char EGL_DEVICE_HANDLES[DUMMY_EGL_MAX_DEVICE_COUNT];
static EGLint deviceCount = DUMMY_EGL_DEVICE_COUNT;

static DummyThreadState *GetThreadState(void)
{
    DummyThreadState *thr = (DummyThreadState *)
        __glvndPthreadFuncs.getspecific(threadStateKey);
    if (thr == NULL) {
        thr = (DummyThreadState *) calloc(1, sizeof(DummyThreadState));
        if (thr == NULL) {
            printf("Can't allocate thread state\n");
            abort();
        }
        thr->lastError = EGL_SUCCESS;
        __glvndPthreadFuncs.mutex_lock(&threadStateLock);
        glvnd_list_append(&thr->entry, &threadStateList);
        __glvndPthreadFuncs.mutex_unlock(&threadStateLock);
        __glvndPthreadFuncs.setspecific(threadStateKey, thr);
    }
    return thr;
}

void DestroyThreadState(DummyThreadState *thr)
{
    if (thr != NULL)
    {
        __glvndPthreadFuncs.mutex_lock(&threadStateLock);
        glvnd_list_del(&thr->entry);
        __glvndPthreadFuncs.mutex_unlock(&threadStateLock);
        free(thr);
    }
}

static void OnThreadTerminate(void *ptr)
{
    DestroyThreadState((DummyThreadState *) ptr);
}

static void CommonEntrypoint(void)
{
    DummyThreadState *thr = GetThreadState();
    thr->lastError = EGL_SUCCESS;
}

static void SetLastError(const char *command, EGLLabelKHR label, EGLint error)
{
    DummyThreadState *thr = GetThreadState();

    thr->lastError = error;

    if (error != EGL_SUCCESS && debugCallbackFunc != NULL && debugCallbackEnabled) {
        debugCallbackFunc(error, command, EGL_DEBUG_MSG_ERROR_KHR, thr->label,
                label, DUMMY_VENDOR_NAME);
    }
}

static DummyEGLDisplay *LookupEGLDisplay(EGLDisplay dpy)
{
    DummyEGLDisplay *disp = NULL;

    __glvndPthreadFuncs.mutex_lock(&displayListLock);
    glvnd_list_for_each_entry(disp, &displayList, entry) {
        if (dpy == (EGLDisplay) disp) {
            __glvndPthreadFuncs.mutex_unlock(&displayListLock);
            return disp;
        }
    }
    __glvndPthreadFuncs.mutex_unlock(&displayListLock);
    // Libglvnd should never pass an invalid EGLDisplay handle to a vendor
    // library.
    printf("Invalid EGLDisplay %p\n", dpy);
    abort();
}

static EGLDeviceEXT GetEGLDevice(EGLint index)
{
    assert(index >= 0 && index < DUMMY_EGL_MAX_DEVICE_COUNT);
    return (EGLDeviceEXT) (EGL_DEVICE_HANDLES + index);
}

static EGLBoolean IsEGLDeviceValid(EGLDeviceEXT dev)
{
    int i;
    for (i=0; i<deviceCount; i++) {
        if (dev == GetEGLDevice(i)) {
            return EGL_TRUE;
        }
    }
    return EGL_FALSE;
}

static const char *dummyGetVendorString(int name)
{
    if (name == __EGL_VENDOR_STRING_PLATFORM_EXTENSIONS) {
        return PLATFORM_EXTENSIONS;
    }

    return NULL;
}

static EGLDisplay dummyGetPlatformDisplay(EGLenum platform, void *native_display,
      const EGLAttrib *attrib_list)
{
    CommonEntrypoint();
    DummyEGLDisplay *disp = NULL;
    EGLDeviceEXT device = EGL_NO_DEVICE_EXT;

    if (platform == EGL_NONE) {
        if (native_display != EGL_DEFAULT_DISPLAY) {
            // If the native display is not EGL_DEFAULT_DISPLAY, then libEGL
            // is supposed to guess a platform enum.
            printf("getPlatformDisplay called without a platform enum.");
            abort();
        }

        platform = EGL_DUMMY_PLATFORM;
        native_display = NULL;
    } else if (platform == EGL_DUMMY_PLATFORM) {
        if (native_display != NULL) {
            const char *name = (const char *) native_display;
            if (strcmp(name, DUMMY_VENDOR_NAME) != 0) {
                return EGL_NO_DISPLAY;
            }

            // Set the native_display pointer to NULL. This makes it simpler to
            // find the same dispaly below.
            native_display = NULL;

            if (attrib_list != NULL) {
                int i;
                for (i=0; attrib_list[i] != EGL_NONE; i += 2) {
                    if (attrib_list[i] == EGL_DEVICE_INDEX) {
                        EGLint index = (EGLint) attrib_list[i + 1];
                        assert(index >= 0 && index < deviceCount);
                        device = GetEGLDevice(index);
                    } else {
                        printf("Invalid attribute 0x%04llx\n", (unsigned long long) attrib_list[i]);
                        abort();
                    }
                }
            }
        }
    } else if (platform == EGL_PLATFORM_DEVICE_EXT) {
        if (native_display == EGL_DEFAULT_DISPLAY) {
            native_display = (void *) GetEGLDevice(0);
        } else {
            if (!IsEGLDeviceValid((EGLDeviceEXT) native_display)) {
                return EGL_NO_DISPLAY;
            }
        }
        device = (EGLDeviceEXT) native_display;
    } else {
        // We don't support this platform.
        SetLastError("eglGetPlatformDisplay", NULL, EGL_BAD_PARAMETER);
        return EGL_NO_DISPLAY;
    }

    __glvndPthreadFuncs.mutex_lock(&displayListLock);
    glvnd_list_for_each_entry(disp, &displayList, entry) {
        if (disp->platform == platform && disp->native_display == native_display && disp->device == device) {
            __glvndPthreadFuncs.mutex_unlock(&displayListLock);
            return disp;
        }
    }

    // Create a new DummyEGLDisplay structure.
    disp = (DummyEGLDisplay *) calloc(1, sizeof(DummyEGLDisplay));
    disp->platform = platform;
    disp->native_display = native_display;
    disp->device = device;
    glvnd_list_append(&disp->entry, &displayList);
    __glvndPthreadFuncs.mutex_unlock(&displayListLock);
    return disp;
}

/**
 * A common function for a bunch of EGL functions that the dummy vendor doesn't
 * implement. This just checks that the display is valid, and returns EGL_FALSE.
 */
static EGLBoolean CommonDisplayStub(EGLDisplay dpy)
{
    CommonEntrypoint();
    LookupEGLDisplay(dpy);
    return EGL_FALSE;
}

static EGLBoolean EGLAPIENTRY dummy_eglInitialize(EGLDisplay dpy,
        EGLint *major, EGLint *minor)
{
    CommonEntrypoint();
    LookupEGLDisplay(dpy);

    *major = 1;
    *minor = 5;
    return EGL_TRUE;
}

static EGLBoolean EGLAPIENTRY dummy_eglTerminate(EGLDisplay dpy)
{
    CommonEntrypoint();
    LookupEGLDisplay(dpy);
    return EGL_TRUE;
}

static EGLBoolean EGLAPIENTRY dummy_eglChooseConfig(EGLDisplay dpy,
        const EGLint *attrib_list, EGLConfig *configs, EGLint config_size,
        EGLint *num_config)
{
    return CommonDisplayStub(dpy);
}

static EGLBoolean EGLAPIENTRY dummy_eglGetConfigs(EGLDisplay dpy, EGLConfig *configs,
        EGLint config_size, EGLint *num_config)
{
    return CommonDisplayStub(dpy);
}

static EGLBoolean EGLAPIENTRY dummy_eglCopyBuffers(EGLDisplay dpy, EGLSurface surface, EGLNativePixmapType target)
{
    return CommonDisplayStub(dpy);
}

static EGLContext EGLAPIENTRY dummy_eglCreateContext(EGLDisplay dpy,
        EGLConfig config, EGLContext share_context, const EGLint *attrib_list)
{
    DummyEGLContext *dctx;
    DummyEGLDisplay *disp;

    CommonEntrypoint();
    disp = LookupEGLDisplay(dpy);

    if (attrib_list != NULL) {
        int i;
        for (i=0; attrib_list[i] != EGL_NONE; i += 2) {
            if (attrib_list[i] == EGL_CREATE_CONTEXT_FAIL) {
                SetLastError("eglCreateContext", disp->label, attrib_list[i + 1]);
                return EGL_NO_CONTEXT;
            } else {
                printf("Invalid attribute 0x%04x in eglCreateContext\n", attrib_list[i]);
                abort();
            }
        }
    }

    dctx = (DummyEGLContext *) calloc(1, sizeof(DummyEGLContext));
    dctx->vendorName = DUMMY_VENDOR_NAME;

    __glvndPthreadFuncs.mutex_lock(&contextListLock);
    glvnd_list_append(&dctx->entry, &contextList);
    __glvndPthreadFuncs.mutex_unlock(&contextListLock);

    return (EGLContext) dctx;
}

static EGLBoolean EGLAPIENTRY dummy_eglDestroyContext(EGLDisplay dpy, EGLContext ctx)
{
    CommonEntrypoint();
    LookupEGLDisplay(dpy);

    if (ctx != EGL_NO_CONTEXT) {
        DummyEGLContext *dctx = (DummyEGLContext *) ctx;
        __glvndPthreadFuncs.mutex_lock(&contextListLock);
        glvnd_list_del(&dctx->entry);
        __glvndPthreadFuncs.mutex_unlock(&contextListLock);
        free(dctx);
    }
    return EGL_TRUE;
}

static EGLSurface CommonCreateSurface(EGLDisplay dpy)
{
    CommonEntrypoint();
    LookupEGLDisplay(dpy);
    return EGL_NO_SURFACE;
}

static EGLSurface dummy_eglCreatePlatformWindowSurface(EGLDisplay dpy,
        EGLConfig config, void *native_window, const EGLAttrib *attrib_list)
{
    return CommonCreateSurface(dpy);
}

static EGLSurface dummy_eglCreatePlatformPixmapSurface(EGLDisplay dpy,
        EGLConfig config, void *native_pixmap, const EGLAttrib *attrib_list)
{
    return CommonCreateSurface(dpy);
}

static EGLSurface EGLAPIENTRY dummy_eglCreatePbufferSurface(EGLDisplay dpy,
        EGLConfig config, const EGLint *attrib_list)
{
    return CommonCreateSurface(dpy);
}

static EGLSurface EGLAPIENTRY dummy_eglCreatePixmapSurface(EGLDisplay dpy,
        EGLConfig config, EGLNativePixmapType pixmap, const EGLint *attrib_list)
{
    return CommonCreateSurface(dpy);
}

static EGLSurface EGLAPIENTRY dummy_eglCreateWindowSurface(EGLDisplay dpy,
        EGLConfig config, EGLNativeWindowType win, const EGLint *attrib_list)
{
    return CommonCreateSurface(dpy);
}

static EGLSurface EGLAPIENTRY dummy_eglCreatePbufferFromClientBuffer(EGLDisplay dpy,
        EGLenum buftype, EGLClientBuffer buffer, EGLConfig config,
        const EGLint *attrib_list)
{
    return CommonCreateSurface(dpy);
}

static EGLBoolean EGLAPIENTRY dummy_eglDestroySurface(EGLDisplay dpy, EGLSurface surface)
{
    return CommonDisplayStub(dpy);
}

static EGLBoolean EGLAPIENTRY dummy_eglGetConfigAttrib(EGLDisplay dpy, EGLConfig config, EGLint attribute, EGLint *value)
{
    return CommonDisplayStub(dpy);
}

static EGLBoolean EGLAPIENTRY dummy_eglMakeCurrent(EGLDisplay dpy, EGLSurface draw, EGLSurface read, EGLContext ctx)
{
    DummyThreadState *thr;
    CommonEntrypoint();

    LookupEGLDisplay(dpy);

    if (failNextMakeCurrentError != EGL_NONE) {
        SetLastError("eglMakeCurrent", NULL, failNextMakeCurrentError);
        failNextMakeCurrentError = EGL_NONE;
        return EGL_FALSE;
    }

    thr = GetThreadState();
    thr->currentContext = ctx;

    return EGL_TRUE;
}

static EGLBoolean EGLAPIENTRY dummy_eglQueryContext(EGLDisplay dpy, EGLContext ctx, EGLint attribute, EGLint *value)
{
    return CommonDisplayStub(dpy);
}

static const char * EGLAPIENTRY dummy_eglQueryString(EGLDisplay dpy, EGLenum name)
{
    CommonEntrypoint();

    if (dpy == EGL_NO_DISPLAY) {
        if (name == EGL_VERSION) {
            return "1.5 EGL dummy";
        } else if (name == EGL_EXTENSIONS) {
            return CLIENT_EXTENSIONS;
        } else {
            return NULL;
        }
    }

    LookupEGLDisplay(dpy);

    if (name == EGL_VENDOR) {
        return DUMMY_VENDOR_NAME;
    } else if (name == EGL_CLIENT_APIS) {
        return "OpenGL OpenGL_ES";
    } else if (name == EGL_EXTENSIONS) {
        return DISPLAY_EXTENSIONS;
    } else {
        return NULL;
    }
}

static EGLBoolean EGLAPIENTRY dummy_eglQuerySurface(EGLDisplay dpy, EGLSurface surface, EGLint attribute, EGLint *value)
{
    return CommonDisplayStub(dpy);
}

static EGLBoolean EGLAPIENTRY dummy_eglSwapBuffers(EGLDisplay dpy, EGLSurface surface)
{
    return CommonDisplayStub(dpy);
}

static EGLBoolean EGLAPIENTRY dummy_eglWaitGL(void)
{
    CommonEntrypoint();
    return EGL_FALSE;
}

static EGLBoolean EGLAPIENTRY dummy_eglWaitNative(EGLint engine)
{
    CommonEntrypoint();
    return EGL_FALSE;
}

static EGLBoolean EGLAPIENTRY dummy_eglBindTexImage(EGLDisplay dpy, EGLSurface surface, EGLint buffer)
{
    return CommonDisplayStub(dpy);
}

static EGLBoolean EGLAPIENTRY dummy_eglReleaseTexImage(EGLDisplay dpy, EGLSurface surface, EGLint buffer)
{
    return CommonDisplayStub(dpy);
}

static EGLBoolean EGLAPIENTRY dummy_eglSurfaceAttrib(EGLDisplay dpy, EGLSurface surface, EGLint attribute, EGLint value)
{
    return CommonDisplayStub(dpy);
}

static EGLBoolean EGLAPIENTRY dummy_eglSwapInterval(EGLDisplay dpy, EGLint interval)
{
    return CommonDisplayStub(dpy);
}

static EGLBoolean EGLAPIENTRY dummy_eglBindAPI(EGLenum api)
{
    CommonEntrypoint();
    if (api != EGL_OPENGL_API && api != EGL_OPENGL_ES_API) {
        printf("eglBindAPI called with invalid API 0x%04x\n", api);
        abort();
    }
    return EGL_TRUE;
}

static EGLBoolean EGLAPIENTRY dummy_eglReleaseThread(void)
{
    DummyThreadState *thr = (DummyThreadState *)
        __glvndPthreadFuncs.getspecific(threadStateKey);
    if (thr != NULL) {
        __glvndPthreadFuncs.setspecific(threadStateKey, NULL);
        DestroyThreadState(thr);
    }
    return EGL_TRUE;
}

static EGLBoolean EGLAPIENTRY dummy_eglWaitClient(void)
{
    CommonEntrypoint();
    return EGL_FALSE;
}

static EGLint EGLAPIENTRY dummy_eglGetError(void)
{
    DummyThreadState *thr = GetThreadState();
    EGLint error = thr->lastError;
    thr->lastError = EGL_SUCCESS;
    return error;
}

static EGLBoolean EGLAPIENTRY dummy_eglQueryDevicesEXT(EGLint max_devices, EGLDeviceEXT *devices, EGLint *num_devices)
{
    CommonEntrypoint();
    if (devices != NULL) {
        EGLint i;
        if (max_devices != deviceCount) {
            // libEGL should only every query the full list of devices.
            printf("Wrong max_devices in eglQueryDevicesEXT: %d\n", max_devices);
            abort();
        }
        *num_devices = deviceCount;
        for (i=0; i<*num_devices; i++) {
            devices[i] = GetEGLDevice(i);
        }
    } else {
        *num_devices = deviceCount;
    }
    return EGL_TRUE;
}

static EGLBoolean EGLAPIENTRY dummy_eglQueryDisplayAttribEXT(EGLDisplay dpy, EGLint attribute, EGLAttrib *value)
{
    DummyEGLDisplay *disp = LookupEGLDisplay(dpy);

    if (attribute == EGL_DEVICE_EXT) {
        *value = (EGLAttrib) disp->device;
        return EGL_TRUE;
    } else {
        SetLastError("eglQueryDisplayAttribEXT", disp->label, EGL_BAD_ATTRIBUTE);
        return EGL_FALSE;
    }
}

static EGLBoolean EGLAPIENTRY dummy_eglQueryDeviceAttribEXT(EGLDeviceEXT device, EGLint attribute, EGLAttrib *value)
{
    // No device attributes are defined here.
    SetLastError("eglQueryDeviceAttribEXT", NULL, EGL_BAD_ATTRIBUTE);
    return EGL_FALSE;
}

static const char *EGLAPIENTRY dummy_eglQueryDeviceStringEXT(EGLDeviceEXT device, EGLint name)
{
    if (name == EGL_EXTENSIONS) {
        return "";
    } else {
        SetLastError("eglQueryDeviceStringEXT", NULL, EGL_BAD_ATTRIBUTE);
        return NULL;
    }
}

static EGLint EGLAPIENTRY dummy_eglDebugMessageControlKHR(EGLDEBUGPROCKHR callback, const EGLAttrib *attrib_list)
{
    CommonEntrypoint();

    if (callback != NULL) {
        if (attrib_list != NULL) {
            int i;
            for (i=0; attrib_list[i] != EGL_NONE; i += 2) {
                if (EGL_DEBUG_MSG_ERROR_KHR) {
                    debugCallbackEnabled = (attrib_list[i + 1] != 0);
                }
            }
        }
    } else {
        debugCallbackEnabled = EGL_TRUE;
    }
    debugCallbackFunc = callback;

    return EGL_SUCCESS;
}

static EGLBoolean EGLAPIENTRY dummy_eglQueryDebugKHR(EGLint attribute, EGLAttrib *value)
{
    // eglQueryDebugKHR should never be called, because libEGL keeps track of
    // all of the debug state.
    printf("eglQueryDebugKHR should never be called\n");
    abort();
    return EGL_FALSE;
}

static EGLint EGLAPIENTRY dummy_eglLabelObjectKHR(EGLDisplay dpy,
        EGLenum objectType, EGLObjectKHR object, EGLLabelKHR label)
{
    CommonEntrypoint();

    if (objectType == EGL_OBJECT_THREAD_KHR) {
        DummyThreadState *thr = GetThreadState();
        thr->label = label;
    } else if (objectType == EGL_OBJECT_DISPLAY_KHR) {
        DummyEGLDisplay *disp = LookupEGLDisplay(dpy);
        disp->label = label;
    }
    return EGL_SUCCESS;
}

static const GLubyte *dummy_glGetString(GLenum name)
{
    if (name == GL_VENDOR) {
        return (const GLubyte *) DUMMY_VENDOR_NAME;
    }
    return NULL;
}

static void *CommonTestDispatch(const char *funcName,
        EGLDisplay dpy, EGLDeviceEXT dev,
        EGLint command, EGLAttrib param)
{
    CommonEntrypoint();

    if (dpy != EGL_NO_DISPLAY) {
        LookupEGLDisplay(dpy);
    }

    if (command == DUMMY_COMMAND_GET_VENDOR_NAME) {
        // Just return the vendor name and don't do anything else.
        return DUMMY_VENDOR_NAME;
    } else if (command == DUMMY_COMMAND_GET_CURRENT_CONTEXT) {
        DummyThreadState *thr = GetThreadState();
        return (void *) thr->currentContext;
    } else if (command == DUMMY_COMMAND_FAIL_NEXT_MAKE_CURRENT) {
        failNextMakeCurrentError = (EGLint) param;
        return DUMMY_VENDOR_NAME;
    } else {
        printf("Invalid command: %d\n", command);
        abort();
    }
}

static void *dummy_eglTestDispatchDisplay(EGLDisplay dpy, EGLint command, EGLAttrib param)
{
    return CommonTestDispatch("eglTestDispatchDisplay", dpy, EGL_NO_DEVICE_EXT, command, param);
}

static void *dummy_eglTestDispatchDevice(EGLDeviceEXT dev, EGLint command, EGLAttrib param)
{
    return CommonTestDispatch("eglTestDispatchDevice", EGL_NO_DISPLAY, dev, command, param);
}

static void *dummy_eglTestDispatchCurrent(EGLint command, EGLAttrib param)
{
    return CommonTestDispatch("eglTestDispatchCurrent", EGL_NO_DISPLAY, EGL_NO_DEVICE_EXT, command, param);
}

static EGLDeviceEXT dummy_eglTestReturnDevice(EGLDisplay dpy, EGLint index)
{
    assert(index >= 0 && index < deviceCount);
    return GetEGLDevice(index);
}

static void *dispatch_eglTestDispatchDisplay(EGLDisplay dpy, EGLint command, EGLAttrib param);
static void *dispatch_eglTestDispatchDevice(EGLDeviceEXT dpy, EGLint command, EGLAttrib param);
static void *dispatch_eglTestDispatchCurrent(EGLint command, EGLAttrib param);
static EGLDeviceEXT dispatch_eglTestReturnDevice(EGLDisplay dpy, EGLint index);
static EGLBoolean EGLAPIENTRY dispatch_eglQueryDeviceAttribEXT(EGLDeviceEXT device, EGLint attribute, EGLAttrib *value);
static const char *EGLAPIENTRY dispatch_eglQueryDeviceStringEXT(EGLDeviceEXT device, EGLint name);

static struct {
    const char *name;
    void *addr;
    void *dispatchAddress;
    int index;
} EGL_EXTENSION_PROCS[DI_COUNT] = {
#define PROC_ENTRY(name) { #name, dummy_##name, dispatch_##name, -1 }
    PROC_ENTRY(eglTestDispatchDisplay),
    PROC_ENTRY(eglTestDispatchDevice),
    PROC_ENTRY(eglTestDispatchCurrent),
    PROC_ENTRY(eglTestReturnDevice),
    PROC_ENTRY(eglQueryDeviceAttribEXT),
    PROC_ENTRY(eglQueryDeviceStringEXT),
#undef PROC_ENTRY
};

static __eglMustCastToProperFunctionPointerType FetchVendorFunc(__EGLvendorInfo *vendor,
        int index, EGLint errorCode)
{
    __eglMustCastToProperFunctionPointerType func = NULL;

    if (vendor != NULL) {
        func = apiExports->fetchDispatchEntry(vendor, EGL_EXTENSION_PROCS[index].index);
    }
    if (func == NULL) {
        if (errorCode != EGL_SUCCESS) {
            apiExports->setEGLError(errorCode);
        }
        return NULL;
    }

    if (!apiExports->setLastVendor(vendor)) {
        printf("setLastVendor failed\n");
        abort();
    }

    return func;
}

static __eglMustCastToProperFunctionPointerType FetchByDevice(EGLDeviceEXT dev, int index)
{
    __EGLvendorInfo *vendor;
    __eglMustCastToProperFunctionPointerType func;

    apiExports->threadInit();
    vendor = apiExports->getVendorFromDevice(dev);
    func = FetchVendorFunc(vendor, index, EGL_BAD_DEVICE_EXT);
    return func;
}

static void *dispatch_eglTestDispatchDisplay(EGLDisplay dpy, EGLint command, EGLAttrib param)
{
    __EGLvendorInfo *vendor;
    pfn_eglTestDispatchDisplay func;

    apiExports->threadInit();
    vendor = apiExports->getVendorFromDisplay(dpy);
    func = (pfn_eglTestDispatchDisplay) FetchVendorFunc(vendor, DI_eglTestDispatchDisplay, EGL_BAD_DISPLAY);
    if (func != NULL) {
        return func(dpy, command, param);
    } else {
        return NULL;
    }
}

static void *dispatch_eglTestDispatchDevice(EGLDeviceEXT dev, EGLint command, EGLAttrib param)
{
    __EGLvendorInfo *vendor;
    pfn_eglTestDispatchDevice func;

    apiExports->threadInit();
    vendor = apiExports->getVendorFromDevice(dev);
    func = (pfn_eglTestDispatchDevice) FetchVendorFunc(vendor, DI_eglTestDispatchDevice, EGL_BAD_DEVICE_EXT);
    if (func != NULL) {
        return func(dev, command, param);
    } else {
        return NULL;
    }
}

static void *dispatch_eglTestDispatchCurrent(EGLint command, EGLAttrib param)
{
    __EGLvendorInfo *vendor;
    pfn_eglTestDispatchCurrent func;

    apiExports->threadInit();
    vendor = apiExports->getCurrentVendor();
    func = (pfn_eglTestDispatchCurrent) FetchVendorFunc(vendor, DI_eglTestDispatchCurrent, EGL_SUCCESS);
    if (func != NULL) {
        return func(command, param);
    } else {
        return NULL;
    }
}

static EGLDeviceEXT dispatch_eglTestReturnDevice(EGLDisplay dpy, EGLint index)
{
    __EGLvendorInfo *vendor;
    pfn_eglTestReturnDevice func;
    EGLDeviceEXT ret;

    apiExports->threadInit();
    vendor = apiExports->getVendorFromDisplay(dpy);
    func = (pfn_eglTestReturnDevice) FetchVendorFunc(vendor, DI_eglTestReturnDevice, EGL_BAD_DISPLAY);
    if (func != NULL) {
        ret = func(dpy, index);
    } else {
        ret = NULL;
    }
    apiExports->setVendorForDevice(ret, vendor);
    return ret;
}

static EGLBoolean EGLAPIENTRY dispatch_eglQueryDeviceAttribEXT(EGLDeviceEXT device, EGLint attribute, EGLAttrib *value)
{
    PFNEGLQUERYDEVICEATTRIBEXTPROC func = (PFNEGLQUERYDEVICEATTRIBEXTPROC)
        FetchByDevice(device, DI_eglQueryDeviceAttribEXT);
    if (func != NULL) {
        return func(device, attribute, value);
    } else {
        return EGL_FALSE;
    }
}

static const char *EGLAPIENTRY dispatch_eglQueryDeviceStringEXT(EGLDeviceEXT device, EGLint name)
{
    PFNEGLQUERYDEVICESTRINGEXTPROC func = (PFNEGLQUERYDEVICESTRINGEXTPROC)
        FetchByDevice(device, DI_eglQueryDeviceStringEXT);
    if (func != NULL) {
        return func(device, name);
    } else {
        return EGL_FALSE;
    }
}

static const struct {
    const char *name;
    void *addr;
} PROC_ADDRESSES[] = {
#define PROC_ENTRY(name) { #name, (void *)dummy_ ## name }
    PROC_ENTRY(eglInitialize),
    PROC_ENTRY(eglTerminate),
    PROC_ENTRY(eglChooseConfig),
    PROC_ENTRY(eglGetConfigs),
    PROC_ENTRY(eglCopyBuffers),
    PROC_ENTRY(eglCreateContext),
    PROC_ENTRY(eglDestroyContext),
    PROC_ENTRY(eglCreatePlatformWindowSurface),
    PROC_ENTRY(eglCreatePlatformPixmapSurface),
    PROC_ENTRY(eglCreatePbufferSurface),
    PROC_ENTRY(eglCreatePixmapSurface),
    PROC_ENTRY(eglCreateWindowSurface),
    PROC_ENTRY(eglCreatePbufferFromClientBuffer),
    PROC_ENTRY(eglDestroySurface),
    PROC_ENTRY(eglGetConfigAttrib),
    PROC_ENTRY(eglMakeCurrent),
    PROC_ENTRY(eglQueryContext),
    PROC_ENTRY(eglQueryString),
    PROC_ENTRY(eglQuerySurface),
    PROC_ENTRY(eglSwapBuffers),
    PROC_ENTRY(eglWaitGL),
    PROC_ENTRY(eglWaitNative),
    PROC_ENTRY(eglBindTexImage),
    PROC_ENTRY(eglReleaseTexImage),
    PROC_ENTRY(eglSurfaceAttrib),
    PROC_ENTRY(eglSwapInterval),
    PROC_ENTRY(eglBindAPI),
    PROC_ENTRY(eglReleaseThread),
    PROC_ENTRY(eglWaitClient),
    PROC_ENTRY(eglGetError),

    PROC_ENTRY(eglQueryDevicesEXT),
    PROC_ENTRY(eglQueryDisplayAttribEXT),
    PROC_ENTRY(eglQueryDeviceAttribEXT),
    PROC_ENTRY(eglQueryDeviceStringEXT),
    PROC_ENTRY(eglDebugMessageControlKHR),
    PROC_ENTRY(eglQueryDebugKHR),
    PROC_ENTRY(eglLabelObjectKHR),

    PROC_ENTRY(glGetString),
#undef PROC_ENTRY
    { NULL, NULL }
};

static void *dummyGetProcAddress(const char *procName)
{
    int i;
    for (i=0; PROC_ADDRESSES[i].name != NULL; i++) {
        if (strcmp(procName, PROC_ADDRESSES[i].name) == 0) {
            return PROC_ADDRESSES[i].addr;
        }
    }

    for (i=0; i < DI_COUNT; i++) {
        if (strcmp(procName, EGL_EXTENSION_PROCS[i].name) == 0) {
            return EGL_EXTENSION_PROCS[i].addr;
        }
    }
    return NULL;
}

static void *dummyFindDispatchFunction(const char *name)
{
    int i;
    for (i=0; i < DI_COUNT; i++) {
        if (strcmp(name, EGL_EXTENSION_PROCS[i].name) == 0) {
            return EGL_EXTENSION_PROCS[i].dispatchAddress;
        }
    }
    return NULL;
}

static void dummySetDispatchIndex(const char *name, int index)
{
    int i;
    for (i=0; i < DI_COUNT; i++) {
        if (strcmp(name, EGL_EXTENSION_PROCS[i].name) == 0) {
            EGL_EXTENSION_PROCS[i].index = index;
        }
    }
}

static EGLBoolean dummyGetSupportsAPI(EGLenum api)
{
    if (api == EGL_OPENGL_ES_API || api == EGL_OPENGL_API) {
        return EGL_TRUE;
    } else {
        return EGL_FALSE;
    }
}

PUBLIC void DummySetDeviceCount(EGLint count)
{
    assert(count >= 0 && count <= DUMMY_EGL_MAX_DEVICE_COUNT);
    deviceCount = count;
}

PUBLIC EGLBoolean
__egl_Main(uint32_t version, const __EGLapiExports *exports,
     __EGLvendorInfo *vendor, __EGLapiImports *imports)
{
    if (EGL_VENDOR_ABI_GET_MAJOR_VERSION(version) !=
        EGL_VENDOR_ABI_MAJOR_VERSION) {
        return EGL_FALSE;
    }

    if (apiExports != NULL) {
        // Already initialized.
        return EGL_TRUE;
    }

    glvndSetupPthreads();

    apiExports = exports;
    __glvndPthreadFuncs.key_create(&threadStateKey, OnThreadTerminate);
    glvnd_list_init(&displayList);

    imports->getPlatformDisplay = dummyGetPlatformDisplay;
    imports->getSupportsAPI = dummyGetSupportsAPI;
    imports->getVendorString = dummyGetVendorString;
    imports->getProcAddress = dummyGetProcAddress;
    imports->getDispatchAddress = dummyFindDispatchFunction;
    imports->setDispatchIndex = dummySetDispatchIndex;

    return EGL_TRUE;
}

#if defined(USE_ATTRIBUTE_CONSTRUCTOR)
void __attribute__ ((destructor)) __eglDummyFini(void)
#else
void _fini(void)
#endif
{
    if (apiExports == NULL) {
        // We were never initialized, so there's nothing to clean up.
        return;
    }

    __glvndPthreadFuncs.mutex_lock(&contextListLock);
    while (!glvnd_list_is_empty(&contextList)) {
        DummyEGLContext *ctx = glvnd_list_first_entry(&contextList, DummyEGLContext, entry);
        glvnd_list_del(&ctx->entry);
        free(ctx);
    }
    __glvndPthreadFuncs.mutex_unlock(&contextListLock);

    while (!glvnd_list_is_empty(&displayList)) {
        DummyEGLDisplay *disp = glvnd_list_first_entry(
                &displayList, DummyEGLDisplay, entry);
        glvnd_list_del(&disp->entry);
        free(disp);
    }

    __glvndPthreadFuncs.key_delete(threadStateKey);
    while (!glvnd_list_is_empty(&threadStateList)) {
        DummyThreadState *thr = glvnd_list_first_entry(
                &threadStateList, DummyThreadState, entry);
        glvnd_list_del(&thr->entry);
        free(thr);
    }

    glvndCleanupPthreads();
}
