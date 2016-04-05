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

#if !defined(__TRACE_H)
#define __TRACE_H

/*!
 * \defgroup trace Tracing module
 *
 * Code in this module implements routines useful for tracing. To enable
 * tracing, set the __GL_DEBUG environment variable to a non-negative value
 * on a -DDEBUG build. Higher values will enable more verbose tracing output.
 *
 * Optionally, setting the __GL_DEBUG_FILE_LINE_INFO variable will enable
 * printing additional context such as file/line number, and thread id.
 *
 * @{
 */

#if defined(DEBUG)

/*!
 * Macro to define debug-only code used for tracing.
 */
# define DBG_CODE(x) x

// Define DBG_PRINTF_THREAD_ID before including this file to enable per-thread
// logs
#ifndef DBG_PRINTF_THREAD_ID
# define DBG_PRINTF_THREAD_ID 0
#endif

extern void __glvnd_dbg_printf(
    int level,
    const char *file,
    int line,
    const char *function,
    int thread_id,
    const char *fmt,
    ...
) __attribute__((format(printf,6,7)));



/*!
 * Macro to print a tracing message with urgency level given by the "level"
 * parameter.
 */
# define DBG_PRINTF(level, ...)             \
    __glvnd_dbg_printf(level,               \
                      __FILE__,             \
                      __LINE__,             \
                      __FUNCTION__,         \
                      DBG_PRINTF_THREAD_ID, \
                      __VA_ARGS__)

#else
# define DBG_PRINTF(level, ...)
# define DBG_CODE(x)
#endif

/*! @} */

#endif // !defined(__TRACE_H)
