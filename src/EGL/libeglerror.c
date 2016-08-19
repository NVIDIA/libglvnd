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

#include "libeglerror.h"

#include <string.h>
#include <assert.h>

#include <EGL/egl.h>
#include <EGL/eglext.h>

#include "glvnd_pthread.h"
#include "libeglmapping.h"
#include "libeglcurrent.h"
#include "utils_misc.h"

enum
{
    __EGL_DEBUG_BIT_CRITICAL = 0x1,
    __EGL_DEBUG_BIT_ERROR = 0x2,
    __EGL_DEBUG_BIT_WARN = 0x4,
    __EGL_DEBUG_BIT_INFO = 0x8,
};

static inline unsigned int DebugBitFromType(GLenum type);

static EGLDEBUGPROCKHR debugCallback = NULL;
static unsigned int debugTypeEnabled = __EGL_DEBUG_BIT_CRITICAL | __EGL_DEBUG_BIT_ERROR;
static glvnd_rwlock_t debugLock = GLVND_RWLOCK_INITIALIZER;

static inline unsigned int DebugBitFromType(EGLenum type)
{
    assert(type >= EGL_DEBUG_MSG_CRITICAL_KHR &&
            type <= EGL_DEBUG_MSG_INFO_KHR);
    return (1 << (type - EGL_DEBUG_MSG_CRITICAL_KHR));
}

EGLint eglDebugMessageControlKHR(EGLDEBUGPROCKHR callback,
        const EGLAttrib *attrib_list)
{
    unsigned int newEnabled = debugTypeEnabled;
    struct glvnd_list *vendorList;
    __EGLvendorInfo *vendor;
    int i;

    __eglEntrypointCommon();

    // Parse the attribute list. Note that if (callback != NULL), then we'll
    // check for errors even though we otherwise ignore it.
    if (attrib_list != NULL) {
        for (i = 0; attrib_list[i] != EGL_NONE; i += 2) {
            if (attrib_list[i] >= EGL_DEBUG_MSG_CRITICAL_KHR &&
                    attrib_list[i] <= EGL_DEBUG_MSG_INFO_KHR) {
                if (attrib_list[i + 1]) {
                    newEnabled |= DebugBitFromType(attrib_list[i]);
                } else {
                    newEnabled &= ~DebugBitFromType(attrib_list[i]);
                }
            } else {
                // On error, set the last error code, call the current
                // debug callback, and return the error code.
                __eglReportError(EGL_BAD_ATTRIBUTE, "eglDebugMessageControlKHR", NULL,
                        "Invalid attribute 0x%04lx", (unsigned long) attrib_list[i]);
                return EGL_BAD_ATTRIBUTE;
            }
        }
    }

    __glvndPthreadFuncs.rwlock_wrlock(&debugLock);

    if (callback != NULL) {
        debugCallback = callback;
        debugTypeEnabled = newEnabled;
    } else {
        debugCallback = NULL;
        debugTypeEnabled = __EGL_DEBUG_BIT_CRITICAL | __EGL_DEBUG_BIT_ERROR;
    }

    // Call into each vendor library.
    vendorList = __eglLoadVendors();
    glvnd_list_for_each_entry(vendor, vendorList, entry) {
        if (vendor->staticDispatch.debugMessageControlKHR != NULL) {
            EGLint result = vendor->staticDispatch.debugMessageControlKHR(callback, attrib_list);
            if (result != EGL_SUCCESS && (debugTypeEnabled & __EGL_DEBUG_BIT_WARN) && callback != NULL) {
                char buf[200];
                snprintf(buf, sizeof(buf), "eglDebugMessageControlKHR failed in vendor library with error 0x%04x. Error reporting may not work correctly.", result);
                callback(EGL_SUCCESS, "eglDebugMessageControlKHR", EGL_DEBUG_MSG_WARN_KHR,
                        __eglGetThreadLabel(), NULL, buf);
            }
        } else if ((debugTypeEnabled & __EGL_DEBUG_BIT_WARN) && callback != NULL) {
            callback(EGL_SUCCESS, "eglDebugMessageControlKHR", EGL_DEBUG_MSG_WARN_KHR,
                    __eglGetThreadLabel(), NULL,
                    "eglDebugMessageControlKHR is not supported by vendor library. Error reporting may not work correctly.");
        }
    }

    __glvndPthreadFuncs.rwlock_unlock(&debugLock);
    return EGL_SUCCESS;
}

EGLBoolean eglQueryDebugKHR(EGLint attribute, EGLAttrib *value)
{
    __eglEntrypointCommon();

    __glvndPthreadFuncs.rwlock_rdlock(&debugLock);
    if (attribute >= EGL_DEBUG_MSG_CRITICAL_KHR &&
            attribute <= EGL_DEBUG_MSG_INFO_KHR) {
        if (debugTypeEnabled & DebugBitFromType(attribute)) {
            *value = EGL_TRUE;
        } else {
            *value = EGL_FALSE;
        }
    } else if (attribute == EGL_DEBUG_CALLBACK_KHR) {
        *value = (EGLAttrib) debugCallback;
    } else {
        __glvndPthreadFuncs.rwlock_unlock(&debugLock);
        __eglReportError(EGL_BAD_ATTRIBUTE, "eglQueryDebugKHR", NULL,
                "Invalid attribute 0x%04lx", (unsigned long) attribute);
        return EGL_FALSE;
    }
    __glvndPthreadFuncs.rwlock_unlock(&debugLock);
    return EGL_TRUE;
}

EGLint eglLabelObjectKHR(
        EGLDisplay display,
        EGLenum objectType,
        EGLObjectKHR object,
        EGLLabelKHR label)
{
    __eglEntrypointCommon();

    if (objectType == EGL_OBJECT_THREAD_KHR) {
        struct glvnd_list *vendorList;
        __EGLThreadAPIState *state = __eglGetCurrentThreadAPIState(label != NULL);
        __EGLvendorInfo *vendor;

        if (state != NULL) {
            if (state->label == label) {
                return EGL_SUCCESS;
            }
            state->label = label;
        } else {
            if (label == NULL) {
                return EGL_SUCCESS;
            }
        }

        vendorList = __eglLoadVendors();
        glvnd_list_for_each_entry(vendor, vendorList, entry) {
            if (vendor->staticDispatch.labelObjectKHR != NULL) {
                EGLint result = vendor->staticDispatch.labelObjectKHR(NULL, objectType, NULL, label);
                if (result != EGL_SUCCESS) {
                    __eglReportWarn("eglLabelObjectKHR", NULL,
                            "eglLabelObjectKHR failed in vendor library with error 0x%04x. Thread label may not be reported correctly.",
                            result);
                }
            } else {
                __eglReportWarn("eglLabelObjectKHR", NULL,
                        "eglLabelObjectKHR is not supported by vendor library. Thread label may not be reported correctly.");
            }
        }
        return EGL_SUCCESS;
    } else {
        __EGLdisplayInfo *dpyInfo = __eglLookupDisplay(display);
        if (dpyInfo == NULL) {
            __eglReportError(EGL_BAD_DISPLAY, "eglLabelObjectKHR", NULL,
                    "Invalid display %p", display);
            return EGL_BAD_DISPLAY;
        }

        if (objectType == EGL_OBJECT_DISPLAY_KHR) {
            if (display != (EGLDisplay) object) {
                __eglReportError(EGL_BAD_PARAMETER, "eglLabelObjectKHR", NULL,
                        "Display %p and object %p do not match", display, object);
                return EGL_BAD_PARAMETER;
            }
        }

        if (dpyInfo->vendor->staticDispatch.labelObjectKHR != NULL) {
            __eglSetLastVendor(dpyInfo->vendor);
            return dpyInfo->vendor->staticDispatch.labelObjectKHR(display, objectType, object, label);
        } else {
            __eglReportError(EGL_BAD_PARAMETER, "eglLabelObjectKHR", NULL,
                    "eglLabelObjectKHR is not supported by vendor library. Object label may not be reported correctly.");
            return EGL_BAD_PARAMETER;
        }
    }
}

EGLLabelKHR __eglGetThreadLabel(void)
{
    __EGLThreadAPIState *state = __eglGetCurrentThreadAPIState(EGL_FALSE);
    if (state != NULL) {
        return state->label;
    } else {
        return NULL;
    }
}

void __eglDebugReport(EGLenum error, const char *command, EGLint type, EGLLabelKHR objectLabel, const char *message, ...)
{
    EGLDEBUGPROCKHR callback = NULL;

    __glvndPthreadFuncs.rwlock_rdlock(&debugLock);
    if (debugTypeEnabled & DebugBitFromType(type)) {
        callback = debugCallback;
    }
    __glvndPthreadFuncs.rwlock_unlock(&debugLock);

    if (callback != NULL) {
        char *buf = NULL;

        if (message != NULL) {
            va_list args;
            va_start(args, message);
            if (glvnd_vasprintf(&buf, message, args) < 0) {
                buf = NULL;
            }
            va_end(args);
        }
        callback(error, command, type, __eglGetThreadLabel(),
                objectLabel, buf);
        free(buf);
    }

    if (type == EGL_DEBUG_MSG_CRITICAL_KHR || type == EGL_DEBUG_MSG_ERROR_KHR) {
        __eglSetError(error);
    }
}

