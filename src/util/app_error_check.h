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

#ifndef __APP_ERROR_CHECK_H
#define __APP_ERROR_CHECK_H

#include "compiler.h"

/**
 * \file
 *
 * Functions for reporting application errors.
 *
 * These functions are used for reporting application errors that might
 * otherwise go unnoticed, not for debugging libglvnd itself. For example,
 * they're used for reporting when an application tries to call an OpenGL
 * function without a current context.
 *
 * There are two environment variables to control this:
 *
 * __GLVND_APP_ERROR_CHECKING: This flag will enable whatever application error
 * checks are available in each component. In the future, there may be other
 * flags to enable and disable other error checks. If that happens, then the
 * __GLVND_APP_ERROR_CHECKING flag will still enable all available checks by
 * default.
 *
 * __GLVND_ABORT_ON_APP_ERROR: If set to 1, then libglvnd will call \c abort(3)
 * when it detects an application error. This is enabled by default if
 * __GLVND_APP_ERROR_CHECKING is enabled, but the user can manually disable it.
 */

/**
 * Initializes the debug output state. This will handle things like reading the
 * environment variables.
 */
void glvndAppErrorCheckInit(void);

/**
 * Reports an application error.
 *
 * If __GLVND_ABORT_ON_APP_ERROR is enabled, then this will also cause the
 * process to abort, so it should only be used for clear errors.
 *
 * \param format A printf-style format string.
 */
void glvndAppErrorCheckReportError(const char *format, ...) PRINTFLIKE(1, 2);

/**
 * Returns non-zero if error checking is enabled.
 */
int glvndAppErrorCheckGetEnabled(void);

#endif // __APP_ERROR_CHECK_H
