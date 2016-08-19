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

#if !defined(LIB_EGL_ERROR_H)
#define LIB_EGL_ERROR_H

#include <EGL/egl.h>
#include <EGL/eglext.h>

#include "compiler.h"

EGLint eglDebugMessageControlKHR(EGLDEBUGPROCKHR callback,
        const EGLAttrib *attrib_list);

EGLBoolean eglQueryDebugKHR(EGLint attribute, EGLAttrib *value);

EGLint eglLabelObjectKHR(EGLDisplay display, EGLenum objectType,
        EGLObjectKHR object, EGLLabelKHR label);

/*!
 * Reports a debug message.
 *
 * If \p type is EGL_DEBUG_MSG_CRITICAL_KHR or EGL_DEBUG_MSG_ERROR_KHR, then
 * this will also set the thread's last error code to \c error.
 *
 * \param error The error code.
 * \param command The name of the EGL function that generated the error.
 * \param type The error type. One of the EGL_DEBUG_MSG_* enums.
 * \param objectLabel The object label to report to the application.
 * \param message A printf-style format string for the message.
 */
void __eglDebugReport(EGLenum error, const char *command, EGLint type,
        EGLLabelKHR objectLabel, const char *message, ...) PRINTFLIKE(5, 6);

/*!
 * Returns the label set for the current thread.
 */
EGLLabelKHR __eglGetThreadLabel(void);

#define __eglReportCritical(error, command, objLabel, ...) \
    __eglDebugReport(error, command, EGL_DEBUG_MSG_CRITICAL_KHR, objLabel, __VA_ARGS__)

#define __eglReportError(error, command, objLabel, ...) \
    __eglDebugReport(error, command, EGL_DEBUG_MSG_ERROR_KHR, objLabel, __VA_ARGS__)

#define __eglReportWarn(command, objLabel, ...) \
    __eglDebugReport(EGL_SUCCESS, command, EGL_DEBUG_MSG_WARN_KHR, objLabel, __VA_ARGS__)

#define __eglReportInfo(command, objLabel, ...) \
    __eglDebugReport(EGL_SUCCESS, command, EGL_DEBUG_MSG_INFO_KHR, objLabel, __VA_ARGS__)

#endif // LIB_EGL_ERROR_H
