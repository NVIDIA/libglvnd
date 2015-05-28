/*
 * Copyright (c) 2015, NVIDIA CORPORATION.
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

#include "utils_misc.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>

int glvnd_asprintf(char **strp, const char *fmt, ...)
{
    static const int GLVND_ASPRINTF_BUF_LEN = 256;
    char *str = NULL;
    int ret = -1;

    if (fmt) {
        va_list ap;
        int len, current_len = GLVND_ASPRINTF_BUF_LEN;

        while (1) {
            str = malloc(current_len);
            if (str == NULL) {
                break;
            }

            va_start(ap, fmt);
            len = vsnprintf(str, current_len, fmt, ap);
            va_end(ap);

            // If the buffer isn't large enough, then vsnprintf will either
            // return -1 (for glibc < 2.1) or the number of bytes the buffer
            // needs to be (for glibc >= 2.1).
            if ((len > -1) && (len < current_len)) {
                ret = len;
                break;
            } else if (len > -1) {
                current_len = len + 1;
            } else {
                current_len += GLVND_ASPRINTF_BUF_LEN;
            }

            free(str);
        }
    }

    *strp = str;
    return ret;
}
