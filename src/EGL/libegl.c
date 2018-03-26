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

/* For RTLD_DEFAULT on x86 systems */
#define _GNU_SOURCE 1

#include <dlfcn.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>
#include <sys/mman.h>

#include <X11/Xlib.h>

#include "glvnd_pthread.h"
#include "libeglabipriv.h"
#include "libeglmapping.h"
#include "libeglcurrent.h"
#include "libeglerror.h"
#include "utils_misc.h"
#include "trace.h"
#include "egldispatchstubs.h"
#include "compiler.h"
#include "utils_misc.h"

#include "lkdhash.h"

#if !defined(HAVE_RTLD_NOLOAD)
#define RTLD_NOLOAD 0
#endif

/*!
 * This is the set of client extensions that libglvnd will support, if at least
 * one vendor library supports them.
 */
static const char *SUPPORTED_CLIENT_EXTENSIONS =
    "EGL_EXT_platform_base"
    " EGL_EXT_device_base"
    " EGL_EXT_device_enumeration"
    " EGL_EXT_device_query"
    " EGL_EXT_platform_device"

    // FIXME: Include the other platform extensions here to make testing
    // easier. These should be split off into a separate string in the vendor
    // library.
    " EGL_KHR_platform_android"
    " EGL_KHR_platform_gbm"
    " EGL_KHR_platform_wayland"
    " EGL_KHR_platform_x11"
    " EGL_EXT_platform_x11"
    " EGL_EXT_platform_wayland"
    " EGL_MESA_platform_gbm"
    ;

/*!
 * This is the set of client extensions that libglvnd will support, regardless
 * of whether any vendor library claims to support them.
 */
static const char *ALWAYS_SUPPORTED_CLIENT_EXTENSIONS =
    "EGL_KHR_client_get_all_proc_addresses"
    " EGL_EXT_client_extensions"
    " EGL_KHR_debug"
    ;

static const char *GLVND_EGL_VERSION_STRING = "1.5 libglvnd";

/*!
 * This is a list of platforms that the user can specify by name to override
 * the platform that eglGetDisplay selects.
 */
static const struct {
   EGLenum platform;
   const char *name;
} EGL_PLATFORMS_NAMES[] = {
   { EGL_PLATFORM_X11_KHR, "x11" },
   { EGL_PLATFORM_WAYLAND_KHR, "wayland" },
   { EGL_PLATFORM_ANDROID_KHR, "android" },
   { EGL_PLATFORM_GBM_KHR, "gbm" },
   { EGL_PLATFORM_GBM_KHR, "drm" },
   { EGL_PLATFORM_DEVICE_EXT, "device" },
   { EGL_NONE, NULL }
};

static char *clientExtensionString = NULL;
glvnd_mutex_t clientExtensionStringMutex = GLVND_MUTEX_INITIALIZER;

void __eglEntrypointCommon(void)
{
    __eglThreadInitialize();
    __eglSetError(EGL_SUCCESS);
}

static EGLBoolean _eglPointerIsDereferencable(void *p)
{
#if defined(HAVE_MINCORE)
    uintptr_t addr = (uintptr_t) p;
    unsigned char unused;
    const long page_size = getpagesize();

    if (p == NULL) {
        return EGL_FALSE;
    }

    /* align addr to page_size */
    addr &= ~(page_size - 1);

    /*
     * mincore() returns 0 on success, and -1 on failure.  The last parameter
     * is a vector of bytes with one entry for each page queried.  mincore
     * returns page residency information in the first bit of each byte in the
     * vector.
     *
     * Residency doesn't actually matter when determining whether a pointer is
     * dereferenceable, so the output vector can be ignored.  What matters is
     * whether mincore succeeds.  It will fail with ENOMEM if the range
     * [addr, addr + length) is not mapped into the process, so all that needs
     * to be checked there is whether the mincore call succeeds or not, as it
     * can only succeed on dereferenceable memory ranges.
     *
     * Also note that the third parameter might be char or unsigned char
     * depending on what system we're building on. Since we don't actually need
     * that result, just cast it to a void* so that it works either way.
     */
    return (mincore((void *) addr, page_size, (void *) &unused) >= 0);
#else
    return EGL_FALSE;
#endif
}

static void *SafeDereference(void *ptr)
{
    if (_eglPointerIsDereferencable(ptr))
        return *((void **)ptr);
    return NULL;
}

static EGLBoolean IsGbmDisplay(void *native_display)
{
    void *first_pointer = SafeDereference(native_display);
    Dl_info info;

    if (dladdr(first_pointer, &info) == 0) {
        return EGL_FALSE;
    }

    if (!info.dli_sname) {
        return EGL_FALSE;
    }

    return !strcmp(info.dli_sname, "gbm_create_device");
}

static EGLBoolean IsX11Display(void *dpy)
{
    void *alloc;
    void *handle;
    void *XAllocID = NULL;

    alloc = SafeDereference(&((_XPrivDisplay)dpy)->resource_alloc);
    if (alloc == NULL) {
        return EGL_FALSE;
    }

    handle = dlopen("libX11.so.6", RTLD_LOCAL | RTLD_LAZY | RTLD_NOLOAD);
    if (handle != NULL) {
        XAllocID = dlsym(handle, "_XAllocID");
        dlclose(handle);
    }

    return (XAllocID != NULL && XAllocID == alloc);
}

static EGLBoolean IsWaylandDisplay(void *native_display)
{
    void *first_pointer = SafeDereference(native_display);
    Dl_info info;

    if (dladdr(first_pointer, &info) == 0) {
        return EGL_FALSE;
    }

    if (!info.dli_sname) {
        return EGL_FALSE;
    }

    return !strcmp(info.dli_sname, "wl_display_interface");
}

/*!
 * This is a helper function for eglGetDisplay to try to guess the platform
 * type to use.
 */
static EGLenum GuessPlatformType(EGLNativeDisplayType display_id)
{
    EGLBoolean gbmSupported = EGL_FALSE;
    EGLBoolean waylandSupported = EGL_FALSE;
    EGLBoolean x11Supported = EGL_FALSE;
    struct glvnd_list *vendorList = __eglLoadVendors();
    __EGLvendorInfo *vendor;

    // First, see if any of the vendor libraries can identify the display.
    glvnd_list_for_each_entry(vendor, vendorList, entry) {
        if (vendor->eglvc.findNativeDisplayPlatform != NULL) {
            EGLenum platform = vendor->eglvc.findNativeDisplayPlatform((void *) display_id);
            if (platform != EGL_NONE) {
                return platform;
            }
        }
    }

    // Next, see if this is a valid EGLDisplayEXT handle.
    if (__eglGetVendorFromDevice((EGLDeviceEXT) display_id)) {
        return EGL_PLATFORM_DEVICE_EXT;
    }

    // Check if any vendor supports EGL_KHR_platform_wayland.
    glvnd_list_for_each_entry(vendor, vendorList, entry) {
        if (vendor->supportsPlatformGbm) {
            gbmSupported = EGL_TRUE;
        }
        if (vendor->supportsPlatformWayland) {
            waylandSupported = EGL_TRUE;
        }
        if (vendor->supportsPlatformX11) {
            x11Supported = EGL_TRUE;
        }
    }

    if (gbmSupported && IsGbmDisplay(display_id)) {
        return EGL_PLATFORM_GBM_KHR;
    }
    if (waylandSupported && IsWaylandDisplay(display_id)) {
        return EGL_PLATFORM_WAYLAND_KHR;
    }
    if (x11Supported && IsX11Display(display_id)) {
        return EGL_PLATFORM_X11_KHR;
    }

    return EGL_NONE;
};

static EGLDisplay GetPlatformDisplayCommon(EGLenum platform,
        void *native_display, const EGLAttrib *attrib_list,
        const char *funcName)
{
    __EGLdisplayInfo *dpyInfo = NULL;
    EGLint errorCode = EGL_SUCCESS;
    EGLBoolean anyVendorSuccess = EGL_FALSE;
    struct glvnd_list *vendorList;

    vendorList = __eglLoadVendors();
    if (glvnd_list_is_empty(vendorList)) {
        // If there are no vendor libraries, then no platforms are supported.
        __eglReportError(EGL_BAD_PARAMETER, funcName, __eglGetThreadLabel(),
                "No EGL drivers found.");
        return EGL_NO_DISPLAY;
    }

    if (platform == EGL_PLATFORM_DEVICE_EXT
            && native_display != (void *) EGL_DEFAULT_DISPLAY) {
        EGLDeviceEXT dev = (EGLDeviceEXT) native_display;
        EGLDisplay dpy;

        __EGLvendorInfo *vendor = __eglGetVendorFromDevice(dev);
        if (vendor == NULL) {
            __eglReportError(EGL_BAD_PARAMETER, funcName, __eglGetThreadLabel(),
                    "Invalid EGLDevice handle %p", dev);
            return EGL_NO_DISPLAY;
        }

        dpy = vendor->eglvc.getPlatformDisplay(platform, native_display, attrib_list);
        if (dpy == EGL_NO_DISPLAY) {
            return EGL_NO_DISPLAY;
        }

        dpyInfo = __eglAddDisplay(dpy, vendor);
        if (dpyInfo == NULL) {
            __eglReportCritical(EGL_BAD_ALLOC, funcName, __eglGetThreadLabel(),
                    "Can't allocate display");
            return EGL_NO_DISPLAY;
        }
    }

    // Note that if multiple threads try to call eglGetPlatformDisplay with the
    // same arguments, then the same vendor library should return the same
    // EGLDisplay handle. In that case, __eglAddDisplay will return the same
    // __EGLdisplayInfo structure for both threads.

    // TODO: How should this deal with EGL_KHR_debug messages? We don't want
    // one vendor library to report an error only for another vendor to
    // succeed. Maybe just require vendors to only use WARN or INFO level
    // messages, and then report an error later on based on the error code?
    if (dpyInfo == NULL) {
        __EGLvendorInfo *vendor;
        glvnd_list_for_each_entry(vendor, vendorList, entry) {
            EGLDisplay dpy = vendor->eglvc.getPlatformDisplay(platform, native_display, attrib_list);
            if (dpy != EGL_NO_DISPLAY) {
                dpyInfo = __eglAddDisplay(dpy, vendor);
                break;
            } else {
                EGLint vendorError = vendor->staticDispatch.getError();
                if (vendorError == EGL_SUCCESS) {
                    anyVendorSuccess = EGL_TRUE;
                } else if (errorCode == EGL_SUCCESS) {
                    errorCode = vendorError;
               }
            }
        }
    }
    if (dpyInfo != NULL) {
        // We got a valid EGLDisplay, so the function succeeded.
        __eglSetError(EGL_SUCCESS);
        return dpyInfo->dpy;
    } else {
        if (anyVendorSuccess) {
            // We didn't get an EGLDisplay, but at least one vendor library
            // returned an error code of EGL_SUCCESS. Assume that the
            // parameters are valid, and that the display was unavailable for
            // some other reason.
            __eglSetError(EGL_SUCCESS);
        } else {
            // Every vendor library returned an error code, so return one of
            // them to the application.
            __eglReportError(errorCode, funcName, __eglGetThreadLabel(),
                    "Could not create EGLDisplay");
        }
        return EGL_NO_DISPLAY;
    }
}

PUBLIC EGLDisplay EGLAPIENTRY eglGetDisplay(EGLNativeDisplayType display_id)
{
    EGLenum platform = EGL_NONE;
    const char *name;

    __eglEntrypointCommon();

    // First, see if the user specified a platform to use.
    name = getenv("EGL_PLATFORM");
    if (name != NULL && name[0] != '\x00') {
        int i;

        for (i=0; EGL_PLATFORMS_NAMES[i].name != NULL; i++) {
            if (strcmp(name, EGL_PLATFORMS_NAMES[i].name) == 0) {
                platform = EGL_PLATFORMS_NAMES[i].platform;
                break;
            }
        }

        // Since libglvnd might not know about every possible platform name,
        // allow the user to specify a platform by the enum value.
        if (platform == EGL_NONE) {
            char *end;
            long value = strtol(name, &end, 0);
            if (*end == '\x00') {
                platform = (EGLenum) value;
            }
        }

        if (platform != EGL_NONE) {
            return GetPlatformDisplayCommon(platform, display_id, NULL, "eglGetDisplay");
        }
    }

    // For EGL_DEFAULT_DISPLAY, we can let the vendor libraries figure out a
    // default.
    if (display_id == EGL_DEFAULT_DISPLAY) {
        return GetPlatformDisplayCommon(EGL_NONE, display_id, NULL, "eglGetDisplay");
    }

    // Otherwise, try to guess a platform type.
    platform = GuessPlatformType(display_id);
    if (platform == EGL_NONE) {
        return EGL_NO_DISPLAY;
    }

    return GetPlatformDisplayCommon(platform, display_id, NULL, "eglGetDisplay");
}

PUBLIC EGLDisplay EGLAPIENTRY eglGetPlatformDisplay(EGLenum platform, void *native_display, const EGLAttrib *attrib_list)
{
    __eglEntrypointCommon();

    if (platform == EGL_NONE) {
        __eglReportError(EGL_BAD_PARAMETER, "eglGetPlatformDisplay", __eglGetThreadLabel(),
                "platform may not be EGL_NONE.");
        return EGL_NO_DISPLAY;
    }

    return GetPlatformDisplayCommon(platform, native_display, attrib_list, "eglGetPlatformDisplay");
}

EGLDisplay EGLAPIENTRY eglGetPlatformDisplayEXT(EGLenum platform, void *native_display, const EGLint *attrib_list)
{
    __eglEntrypointCommon();

    if (platform == EGL_NONE) {
        __eglReportError(EGL_BAD_PARAMETER, "eglGetPlatformDisplayEXT", __eglGetThreadLabel(),
                "platform may not be EGL_NONE.");
        return EGL_NO_DISPLAY;
    }

    if (sizeof(EGLAttrib) == sizeof(EGLint) || attrib_list == NULL) {
        return GetPlatformDisplayCommon(platform, native_display,
                (const EGLAttrib *) attrib_list, "eglGetPlatformDisplayEXT");
    } else {
        EGLAttrib *attribs = NULL;
        EGLDisplay dpy;
        int count = 0;
        int i;

        while (attrib_list[count] != EGL_NONE) {
            count += 2;
        }
        count++;
        attribs = malloc(count * sizeof(EGLAttrib));
        if (attribs == NULL) {
            __eglReportCritical(EGL_BAD_ALLOC, "eglGetPlatformDisplayEXT",
                    __eglGetThreadLabel(), NULL);
            return EGL_NO_DISPLAY;
        }

        for (i=0; i<count; i++) {
            attribs[i] = (EGLAttrib) attrib_list[i];
        }

        dpy = GetPlatformDisplayCommon(platform, native_display, attribs,
                "eglGetPlatformDisplayEXT");
        free(attribs);
        return dpy;
    }
}

PUBLIC EGLBoolean EGLAPIENTRY eglBindAPI(EGLenum api)
{
    EGLBoolean supported = EGL_FALSE;
    __EGLThreadAPIState *state;
    struct glvnd_list *vendorList;
    __EGLvendorInfo *vendor;

    // We only support GL and GLES right now.
    if (api != EGL_OPENGL_API && api != EGL_OPENGL_ES_API) {
        __eglReportError(EGL_BAD_PARAMETER, "eglBindAPI", __eglGetThreadLabel(),
                "Unsupported rendering API 0x%04x", api);
        return EGL_FALSE;
    }

    if (api == eglQueryAPI()) {
        // Nothing to do.
        return EGL_TRUE;
    }

    // First, check if any vendor library supports the requested API.
    vendorList = __eglLoadVendors();
    glvnd_list_for_each_entry(vendor, vendorList, entry) {
        if ((api == EGL_OPENGL_API && vendor->supportsGL)
                || (api == EGL_OPENGL_ES_API && vendor->supportsGLES)) {
            supported = EGL_TRUE;
            break;
        }
    }
    if (!supported) {
        __eglReportError(EGL_BAD_PARAMETER, "eglBindAPI", __eglGetThreadLabel(),
                "Unsupported rendering API 0x%04x", api);
        return EGL_FALSE;
    }

    // Note: We do not call into the vendor library here. The vendor is
    // responsible for looking up the current API instead.

    state = __eglGetCurrentThreadAPIState(EGL_TRUE);
    if (state == NULL) {
        // Probably out of memory. Not much else we can do here.
        return EGL_FALSE;
    }
    state->currentClientApi = api;
    glvnd_list_for_each_entry(vendor, vendorList, entry) {
        if (vendor->staticDispatch.bindAPI != NULL) {
            vendor->staticDispatch.bindAPI(api);
        }
    }
    return EGL_TRUE;
}

PUBLIC EGLenum EGLAPIENTRY eglQueryAPI(void)
{
    __eglEntrypointCommon();
    return __eglQueryAPI();
}

PUBLIC EGLDisplay EGLAPIENTRY eglGetCurrentDisplay(void)
{
    __eglEntrypointCommon();
    return __eglGetCurrentDisplay();
}

PUBLIC EGLContext EGLAPIENTRY eglGetCurrentContext(void)
{
    __eglEntrypointCommon();
    return __eglGetCurrentContext();
}

PUBLIC EGLSurface EGLAPIENTRY eglGetCurrentSurface(EGLint readdraw)
{
    __eglEntrypointCommon();
    if (readdraw != EGL_DRAW && readdraw != EGL_READ) {
        __eglReportError(EGL_BAD_PARAMETER, "eglGetCurrentSurface", __eglGetThreadLabel(),
                "Invalid enum 0x%04x\n", readdraw);
    }
    return __eglGetCurrentSurface(readdraw);
}

static EGLBoolean InternalLoseCurrent(void)
{
    __EGLdispatchThreadState *apiState = __eglGetCurrentAPIState();
    EGLBoolean ret;

    if (apiState == NULL) {
        return EGL_TRUE;
    }

    __eglSetLastVendor(apiState->currentVendor);
    ret = apiState->currentVendor->staticDispatch.makeCurrent(
            apiState->currentDisplay->dpy,
            EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);
    if (!ret) {
        return EGL_FALSE;
    }

    __glDispatchLoseCurrent();
    __eglDestroyAPIState(apiState);
    return EGL_TRUE;
}

/**
 * Calls into the vendor library to set the current context, and then updates
 * the API state fields to match.
 *
 * This function does *not* call into libGLdispatch, so it can only switch
 * to another context with the same vendor.
 *
 * If this function succeeds, then it will update the current display, context,
 * and drawables in \p apiState.
 *
 * If it fails, then it will leave \p apiState unmodified. It's up to the
 * vendor library to ensure that the old context is still current in that case.
 */
static EGLBoolean InternalMakeCurrentVendor(
        __EGLdisplayInfo *dpy, EGLSurface draw, EGLSurface read,
        EGLContext context,
        __EGLdispatchThreadState *apiState,
        __EGLvendorInfo *vendor)
{
    EGLBoolean ret;

    assert(apiState->currentVendor == vendor);

    __eglSetLastVendor(dpy->vendor);
    ret = dpy->vendor->staticDispatch.makeCurrent(dpy->dpy, draw, read, context);
    if (ret) {
        apiState->currentDisplay = dpy;
        apiState->currentDraw = draw;
        apiState->currentRead = read;
        apiState->currentContext = context;
    }

    return ret;
}

/**
 * Makes a context current. This function handles both the vendor library and
 * libGLdispatch.
 *
 * There must not be a current API state in libGLdispatch when this function is
 * called.
 *
 * If this function fails, then it will release the context and dispatch state
 * before returning.
 */
static EGLBoolean InternalMakeCurrentDispatch(
        __EGLdisplayInfo *dpy, EGLSurface draw, EGLSurface read,
        EGLContext context,
        __EGLvendorInfo *vendor)
{
    __EGLdispatchThreadState *apiState;
    EGLBoolean ret;

    assert(__eglGetCurrentAPIState() == NULL);

    apiState = __eglCreateAPIState();
    if (apiState == NULL) {
        return False;
    }

    ret = __glDispatchMakeCurrent(
        &apiState->glas,
        vendor->glDispatch,
        vendor->vendorID,
        (vendor->patchSupported ? &vendor->patchCallbacks : NULL)
    );

    if (ret) {
        apiState->currentVendor = vendor;
        ret = InternalMakeCurrentVendor(dpy, draw, read, context,
                apiState, vendor);
        if (!ret) {
            __glDispatchLoseCurrent();
        }
    }

    if (!ret) {
        __eglDestroyAPIState(apiState);
    }

    return ret;
}

PUBLIC EGLBoolean EGLAPIENTRY eglMakeCurrent(EGLDisplay dpy,
        EGLSurface draw, EGLSurface read, EGLContext context)
{
    __GLdispatchThreadState *glas;
    __EGLdispatchThreadState *apiState;
    __EGLvendorInfo *oldVendor, *newVendor;
    __EGLdisplayInfo *oldDpy, *newDpy;
    EGLSurface oldDraw, oldRead;
    EGLContext oldContext;
    EGLBoolean ret;

    __eglEntrypointCommon();

    // According to the EGL spec, the display handle must be valid, even if
    // the context is NULL.
    newDpy = __eglLookupDisplay(dpy);
    if (newDpy == NULL) {
        __eglReportError(EGL_BAD_DISPLAY, "eglMakeCurrent", NULL,
                "Invalid display %p", dpy);
        return EGL_FALSE;
    }

    if (context == EGL_NO_CONTEXT && (draw != EGL_NO_SURFACE || read != EGL_NO_SURFACE)) {
        __eglReportError(EGL_BAD_MATCH, "eglMakeCurrent", NULL,
                "Got an EGLSurface but no EGLContext");
        return EGL_FALSE;
    }

    glas = __glDispatchGetCurrentThreadState();
    if (glas != NULL) {
        if (glas->tag != GLDISPATCH_API_EGL) {
            // Another API (probably GLX) already has a current context. Just
            // return failure.
            // TODO: What error should this generate?
            __eglReportError(EGL_BAD_ACCESS, "eglMakeCurrent", NULL,
                    "Another window API already has a current context");
            return EGL_FALSE;
        }

        apiState = (__EGLdispatchThreadState *) glas;
        oldVendor = apiState->currentVendor;
        oldDpy = apiState->currentDisplay;
        oldDraw = apiState->currentDraw;
        oldRead = apiState->currentRead;
        oldContext = apiState->currentContext;

        assert(oldContext != EGL_NO_CONTEXT);

        if (dpy == oldDpy->dpy && context == oldContext
                && draw == oldDraw && read == oldRead) {
            // The current display, context, and drawables are the same, so just
            // return.
            return EGL_TRUE;
        }
    } else {
        // We don't have a current context already.

        if (context == EGL_NO_CONTEXT) {
            // If both contexts are NULL, then ignore the other parameters and
            // return early.
            return EGL_TRUE;
        }

        apiState = NULL;
        oldVendor = NULL;
        oldDpy = NULL;
        oldDraw = oldRead = EGL_NO_SURFACE;
        oldContext = EGL_NO_CONTEXT;
    }

    if (context != EGL_NO_CONTEXT) {
        newVendor = newDpy->vendor;
    } else {
        newVendor = NULL;
    }

    if (oldVendor == newVendor) {
        /*
         * We're switching between two contexts that use the same vendor. That
         * means the dispatch table is also the same, which is the only thing
         * that libGLdispatch cares about. Call into the vendor library to
         * switch contexts, but don't call into libGLdispatch.
         */
        ret = InternalMakeCurrentVendor(newDpy, draw, read, context,
                apiState, newVendor);
    } else if (newVendor == NULL) {
        /*
         * We have a current context and we're releasing it.
         */
        assert(context == EGL_NO_CONTEXT);
        ret = InternalLoseCurrent();
    } else if (oldVendor == NULL) {
        /*
         * We don't have a current context, so we only need to make the new one
         * current.
         */
        ret = InternalMakeCurrentDispatch(newDpy, draw, read, context,
                newVendor);
    } else {
        /*
         * We're switching between contexts with different vendors.
         *
         * This gets tricky because we have to call into both vendor libraries
         * and libGLdispatch. Any of those can fail, and if it does, then we
         * have to make sure libEGL, libGLdispatch, and the vendor libraries
         * all agree on what the current context is.
         *
         * To do that, we'll first release the current context, and then make
         * the new context current.
         */
        ret = InternalLoseCurrent();
        if (ret) {
            ret = InternalMakeCurrentDispatch(newDpy, draw, read, context,
                    newVendor);
            /*
             * Ideally, we should try to restore the old context if we fail,
             * but we need to deal with the case where the old context was
             * flagged for deletion, and thus is now deleted. We don't want to
             * pass an invalid context to the vendor library.
             *
             * We could avoid that using a current context hashtable like GLX
             * has. That would allow us to restore the old context when it
             * still exists, but we'd still be left with no context if it was
             * deleted.
             *
             * Note that GLX also needs that hashtable to keep its
             * context-to-screen mapping up to date, but EGL doesn't need to
             * keep track of contexts at all yet.
             *
             * Once we add support for OpenVG, though, then we'll need to keep
             * track of data for every context, not just the current ones. At
             * that point, we'll be able to use that to track context deletion
             * as well.
             */
        }
    }

    return ret;
}

PUBLIC EGLBoolean EGLAPIENTRY eglReleaseThread(void)
{
    __EGLThreadAPIState *threadState = __eglGetCurrentThreadAPIState(EGL_FALSE);
    if (threadState != NULL) {
        __EGLdispatchThreadState *apiState = __eglGetCurrentAPIState();
        __EGLvendorInfo *currentVendor = NULL;
        struct glvnd_list *vendorList = __eglLoadVendors();
        __EGLvendorInfo *vendor;

        if (apiState != NULL) {
            currentVendor = apiState->currentVendor;
            if (!currentVendor->staticDispatch.releaseThread()) {
                threadState->lastVendor = currentVendor;
                return EGL_FALSE;
            }

            __glDispatchLoseCurrent();
            __eglDestroyAPIState(apiState);
        }

        glvnd_list_for_each_entry(vendor, vendorList, entry) {
            // Call into the remaining vendor libraries. Aside from the current
            // vendor, none of these are allowed to fail -- otherwise, we'd end
            // up in an inconsistant state.
            if (vendor != currentVendor) {
                vendor->staticDispatch.releaseThread();
            }
        }

        __eglDestroyCurrentThreadAPIState();
    }
    assert(__eglGetCurrentAPIState() == NULL);

    return EGL_TRUE;
}

PUBLIC EGLint EGLAPIENTRY eglGetError(void)
{
    __EGLThreadAPIState *state;

    // Note: We call __eglThreadInitialize here, not __eglEntrypointCommon,
    // because we have to look up the current error code before clearing it.
    __eglThreadInitialize();

    state = __eglGetCurrentThreadAPIState(EGL_FALSE);
    EGLint ret = EGL_SUCCESS;
    if (state != NULL) {
        if (state->lastVendor != NULL) {
            ret = state->lastVendor->staticDispatch.getError();
        } else {
            ret = state->lastError;
        }
        state->lastVendor = NULL;
        state->lastError = EGL_SUCCESS;
    }
    return ret;
}

void __eglSetError(EGLint error)
{
    __EGLThreadAPIState *state;

    state = __eglGetCurrentThreadAPIState(error != EGL_SUCCESS);
    if (state != NULL) {
        state->lastError = error;
        state->lastVendor = NULL;
    }
}

EGLBoolean __eglSetLastVendor(__EGLvendorInfo *vendor)
{
    __EGLThreadAPIState *state;

    state = __eglGetCurrentThreadAPIState(EGL_TRUE);
    if (state != NULL) {
        state->lastError = EGL_SUCCESS;
        state->lastVendor = vendor;
        return EGL_TRUE;
    } else {
        return EGL_FALSE;
    }
}

static char *GetClientExtensionString(void)
{
    struct glvnd_list *vendorList = __eglLoadVendors();
    __EGLvendorInfo *vendor;
    char *result = NULL;

    // First, find the union of all available vendor libraries. Start with an
    // empty string, then merge the extension string from every vendor library.
    result = malloc(1);
    if (result == NULL) {
        return NULL;
    }
    result[0] = '\0';

    // Merge the extension string from every vendor library.
    glvnd_list_for_each_entry(vendor, vendorList, entry) {
        const char *vendorString = vendor->staticDispatch.queryString(EGL_NO_DISPLAY, EGL_EXTENSIONS);
        if (vendorString != NULL && vendorString[0] != '\0') {
            result = UnionExtensionStrings(result, vendorString);
            if (result == NULL) {
                return NULL;
            }
        }
    }

    // Next, take the intersection of the client extensions from the vendors
    // with the client extensions that libglvnd supports.
    IntersectionExtensionStrings(result, SUPPORTED_CLIENT_EXTENSIONS);

    // Add the extension strings that libEGL itself provides.
    result = UnionExtensionStrings(result, ALWAYS_SUPPORTED_CLIENT_EXTENSIONS);
    if (result == NULL) {
        return NULL;
    }

    glvnd_list_for_each_entry(vendor, vendorList, entry) {
        const char *vendorString = NULL;
        if (vendor->eglvc.getVendorString != NULL) {
            vendorString = vendor->eglvc.getVendorString(__EGL_VENDOR_STRING_PLATFORM_EXTENSIONS);
        }
        if (vendorString != NULL && vendorString[0] != '\0') {
            result = UnionExtensionStrings(result, vendorString);
            if (result == NULL) {
                return NULL;
            }
        }
    }

    return result;
}

PUBLIC const char *EGLAPIENTRY eglQueryString(EGLDisplay dpy, EGLint name)
{
    __eglEntrypointCommon();

    if (dpy == EGL_NO_DISPLAY) {
        if (name == EGL_EXTENSIONS) {
            const char *ret;

            if (glvnd_list_is_empty(__eglLoadVendors())) {
                return "";
            }
            __glvndPthreadFuncs.mutex_lock(&clientExtensionStringMutex);
            if (clientExtensionString == NULL) {
                clientExtensionString = GetClientExtensionString();
            }
            ret = clientExtensionString;
            __glvndPthreadFuncs.mutex_unlock(&clientExtensionStringMutex);

            return ret;
        } else if (name == EGL_VERSION) {
            return GLVND_EGL_VERSION_STRING;
        } else {
            __eglReportError(EGL_BAD_DISPLAY, "eglQueryString", NULL,
                    "Invalid enum 0x%04x without a display", name);
            return NULL;
        }
    } else {
        __EGLdisplayInfo *dpyInfo = __eglLookupDisplay(dpy);
        if (dpyInfo == NULL) {
            __eglReportError(EGL_BAD_DISPLAY, "eglQueryString", NULL,
                    "Invalid display %p", dpy);
            return NULL;
        }
        __eglSetLastVendor(dpyInfo->vendor);
        return dpyInfo->vendor->staticDispatch.queryString(dpy, name);
    }
}

EGLBoolean EGLAPIENTRY eglQueryDevicesEXT(EGLint max_devices,
        EGLDeviceEXT *devices, EGLint *num_devices)
{
    __eglEntrypointCommon();

    if (num_devices == NULL || (max_devices <= 0 && devices != NULL)) {
        __eglReportError(EGL_BAD_PARAMETER, "eglQueryDevicesEXT", NULL,
                "Missing num_devices pointer");
        return EGL_FALSE;
    }

    __eglInitDeviceList();

    if (devices != NULL) {
        EGLint i;

        *num_devices = (max_devices < __eglDeviceCount ? max_devices : __eglDeviceCount);
        for (i = 0; i < *num_devices; i++) {
            devices[i] = __eglDeviceList[i].handle;
        }
    } else {
        *num_devices = __eglDeviceCount;
    }
    return EGL_TRUE;
}

// TODO: The function hash is the same as in GLX. It should go into a common
// file.
typedef struct {
    char *procName;
    __eglMustCastToProperFunctionPointerType addr;
    UT_hash_handle hh;
} __EGLprocAddressHash;

static DEFINE_INITIALIZED_LKDHASH(__EGLprocAddressHash, __eglProcAddressHash);

static void CacheProcAddress(const char *procName,
        __eglMustCastToProperFunctionPointerType addr)
{
    size_t nameLen = strlen(procName);
    __EGLprocAddressHash *pEntry;

    LKDHASH_WRLOCK(__eglProcAddressHash);

    HASH_FIND(hh, _LH(__eglProcAddressHash), procName,
              nameLen, pEntry);
    if (pEntry == NULL) {
        pEntry = malloc(sizeof(*pEntry) + nameLen + 1);
        if (pEntry != NULL) {
            pEntry->procName = (char *) (pEntry + 1);
            memcpy(pEntry->procName, procName, nameLen + 1);
            pEntry->addr = addr;
            HASH_ADD_KEYPTR(hh, _LH(__eglProcAddressHash), pEntry->procName,
                            nameLen, pEntry);
        }
    } else {
        assert(pEntry->addr == addr);
    }
    LKDHASH_UNLOCK(__eglProcAddressHash);
}

static __eglMustCastToProperFunctionPointerType GetCachedProcAddress(const char *procName)
{
    /*
     * If this is the first time GetProcAddress has been called,
     * initialize the hash table with locally-exported functions.
     */
    __EGLprocAddressHash *pEntry = NULL;

    LKDHASH_RDLOCK(__eglProcAddressHash);
    HASH_FIND(hh, _LH(__eglProcAddressHash), procName,
              strlen(procName), pEntry);
    LKDHASH_UNLOCK(__eglProcAddressHash);

    if (pEntry) {
        return pEntry->addr;
    }

    return NULL;
}

PUBLIC __eglMustCastToProperFunctionPointerType EGLAPIENTRY eglGetProcAddress(const char *procName)
{
    __eglMustCastToProperFunctionPointerType addr = NULL;

    __eglEntrypointCommon();

    /*
     * Easy case: First check if we already know this address from
     * a previous GetProcAddress() call or by virtue of being a function
     * exported by libEGL.
     */
    addr = GetCachedProcAddress(procName);
    if (addr) {
        return addr;
    }

    /*
     * If that doesn't work, try requesting a dispatch function
     * from one of the loaded vendor libraries.
     */
    if (procName[0] == 'e' && procName[1] == 'g' && procName[2] == 'l') {
        addr = __eglGetEGLDispatchAddress(procName);
    } else if (procName[0] == 'g' && procName[1] == 'l') {
        addr = __glDispatchGetProcAddress(procName);
    } else {
        addr = NULL;
    }
    if (addr != NULL) {
        CacheProcAddress(procName, addr);
    }

    return addr;
}


// TODO: Move the Atomic* functions to a common file.
int AtomicIncrement(int volatile *val)
{
#if defined(HAVE_SYNC_INTRINSICS)
    return __sync_add_and_fetch(val, 1);
#elif defined(USE_X86_ASM) || defined(USE_X86_64_ASM)
    int result;
    int delta = 1;

    __asm __volatile__ ("lock; xaddl %0, %1"
                        : "=r" (result), "=m" (*val)
                        : "0" (delta), "m" (*val));

    return result + delta;
#else
#error "Not implemented"
#endif
}

int AtomicSwap(int volatile *val, int newVal)
{
#if defined(HAVE_SYNC_INTRINSICS)
    return __sync_lock_test_and_set(val, newVal);
#elif defined(USE_X86_ASM) || defined(USE_X86_64_ASM)
    int result;

    __asm __volatile__ ("xchgl %0, %1"
                        : "=r" (result), "=m" (*val)
                        : "0" (newVal), "m" (*val));

    return result;
#else
#error "Not implemented"
#endif
}

int AtomicCompareAndSwap(int volatile *val, int oldVal, int newVal)
{
#if defined(HAVE_SYNC_INTRINSICS)
    return __sync_val_compare_and_swap(val, oldVal, newVal);
#elif defined(USE_X86_ASM) || defined(USE_X86_64_ASM)
    int result;

    __asm __volatile__ ("lock; cmpxchgl %2, %1"
                        : "=a" (result), "=m" (*val)
                        : "r" (newVal), "m" (*val), "0" (oldVal));

    return result;
#else
#error "Not implemented"
#endif
}

int AtomicDecrementClampAtZero(int volatile *val)
{
    int oldVal, newVal;

    oldVal = *val;
    newVal = oldVal;

    do {
        if (oldVal <= 0) {
            assert(oldVal == 0);
        } else {
            newVal = oldVal - 1;
            if (newVal < 0) {
                newVal = 0;
            }
            oldVal = AtomicCompareAndSwap(val, oldVal, newVal);
        }
    } while ((oldVal > 0) && (newVal != oldVal - 1));

    return newVal;
}

static void __eglResetOnFork(void);

/*
 * Perform checks that need to occur when entering any EGL entrypoint.
 * Currently, this only detects whether a fork occurred since the last
 * entrypoint was called, and performs recovery as needed.
 */
void CheckFork(void)
{
    volatile static int g_threadsInCheck = 0;
    volatile static int g_lastPid = -1;

    int lastPid;
    int pid = getpid();

    AtomicIncrement(&g_threadsInCheck);

    lastPid = AtomicSwap(&g_lastPid, pid);

    if ((lastPid != -1) &&
        (lastPid != pid)) {

        DBG_PRINTF(0, "Fork detected\n");

        __eglResetOnFork();

        // Force g_threadsInCheck to 0 to unblock other threads waiting here.
        g_threadsInCheck = 0;
    } else {
        AtomicDecrementClampAtZero(&g_threadsInCheck);
        while (g_threadsInCheck > 0) {
            // Wait for other threads to finish checking for a fork.
            //
            // If a fork happens while g_threadsInCheck > 0 the _first_ thread
            // to enter __eglThreadInitialize() will see the fork, handle it, and force
            // g_threadsInCheck to 0, unblocking any other threads stuck here.
            sched_yield();
        }
    }
}

void __eglThreadInitialize(void)
{
    CheckFork();
    __glDispatchCheckMultithreaded();
}

static void __eglAPITeardown(EGLBoolean doReset)
{
    __eglCurrentTeardown(doReset);

    if (doReset) {
        /*
         * XXX: We should be able to get away with just resetting the proc address
         * hash lock, and not throwing away cached addresses.
         */
        __glvndPthreadFuncs.rwlock_init(&__eglProcAddressHash.lock, NULL);
    } else {
        LKDHASH_TEARDOWN(__EGLprocAddressHash,
                         __eglProcAddressHash, NULL,
                         NULL, EGL_FALSE);

        free(clientExtensionString);
        clientExtensionString = NULL;
    }
}

static void __eglResetOnFork(void)
{
    /* Reset all EGL API state */
    __eglAPITeardown(EGL_TRUE);

    /* Reset all mapping state */
    __eglMappingTeardown(EGL_TRUE);

    /* Reset GLdispatch */
    __glDispatchReset();
}

#if defined(USE_ATTRIBUTE_CONSTRUCTOR)
void __attribute__ ((constructor)) __eglInit(void)
#else
void _init(void)
#endif
{
    if (__glDispatchGetABIVersion() != GLDISPATCH_ABI_VERSION) {
        fprintf(stderr, "libGLdispatch ABI version is incompatible with libEGL.\n");
        abort();
    }

    /* Initialize GLdispatch; this will also initialize our pthreads imports */
    __glDispatchInit();
    glvndSetupPthreads();

    // Set up the mapping code, and populate the getprocaddress hashtable.
    __eglMappingInit();

    __eglCurrentInit();
    __eglInitVendors();

    /* TODO install fork handlers using __register_atfork */

    DBG_PRINTF(0, "Loading EGL...\n");

}

#if defined(USE_ATTRIBUTE_CONSTRUCTOR)
void __attribute__ ((destructor)) __eglFini(void)
#else
void _fini(void)
#endif
{
    /* Check for a fork before going further. */
    CheckFork();

    /*
     * If libEGL owns the current API state, lose current
     * in GLdispatch before going further.
     */
    __GLdispatchThreadState *glas =
        __glDispatchGetCurrentThreadState();

    if (glas && glas->tag == GLDISPATCH_API_EGL) {
        __glDispatchLoseCurrent();
    }

    /* Tear down all EGL API state */
    __eglAPITeardown(EGL_FALSE);

    /* Tear down all mapping state */
    __eglMappingTeardown(EGL_FALSE);

    __eglTeardownVendors();

    /* Tear down GLdispatch if necessary */
    __glDispatchFini();
}

