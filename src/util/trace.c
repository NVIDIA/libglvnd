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


#define _GNU_SOURCE 1
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>

#include "trace.h"
#include "utils_misc.h"

static int debugPrintfInitialized = 0;
static int debugPrintfLevel = -1;
static int showPrefix = 0;

void __glvnd_dbg_printf(
    int level,
    const char *file,
    int line,
    const char *function,
    int thread_id,
    const char *fmt,
    ...
)
{
    va_list ap;
    char *tmp;
    int ret;

    if (!debugPrintfInitialized) {
        char *debugStr = getenv("__GL_DEBUG");
        char *showPrefixStr = getenv("__GL_DEBUG_FILE_LINE_INFO");
        if (debugStr) {
            debugPrintfLevel = atoi(debugStr);
        }
        if (showPrefixStr) {
            showPrefix = 1;
        }
        debugPrintfInitialized = 1;
    }

    if (level < debugPrintfLevel) {
        va_start(ap, fmt);
        ret = glvnd_vasprintf(&tmp, fmt, ap);
        va_end(ap);
        if (ret == -1 || !tmp) {
            return;
        }
        if (showPrefix) {
            fprintf(stderr, "%s:%d:%s [tid=%x] %s", file, line, function,
                    thread_id, tmp);
        } else {
            fprintf(stderr, "%s", tmp);
        }
        free(tmp);
    }
}
