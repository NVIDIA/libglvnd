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
#include <string.h>
#include <stdarg.h>
#include <sys/mman.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <fcntl.h>
#include <assert.h>

int glvnd_asprintf(char **strp, const char *fmt, ...)
{
    va_list args;
    int ret;

    va_start(args, fmt);
    ret = glvnd_vasprintf(strp, fmt, args);
    va_end(args);
    return ret;
}

int glvnd_vasprintf(char **strp, const char *fmt, va_list args)
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

            va_copy(ap, args);
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

void glvnd_byte_swap16(uint16_t* array, const size_t size)
{
    int i;

    assert((size % 2) == 0);

    for (i = 0; i < size / 2; i++) {
        array[i] = (array[i] << 8) | (array[i] >> 8);
    }
}

int FindNextStringToken(const char **tok, size_t *len, const char *sep)
{
    // Skip to the end of the current name.
    const char *ptr = *tok + *len;

    // Skip any leading separators.
    while (*ptr != '\x00' && strchr(sep, *ptr) != NULL) {
        ptr++;
    }

    // Find the length of the current token.
    *len = 0;
    while (ptr[*len] != '\x00' && strchr(sep, ptr[*len]) == NULL) {
        (*len)++;
    }
    *tok = ptr;
    return (*len > 0 ? 1 : 0);
}

char **SplitString(const char *str, size_t *count, const char *sep)
{
    char **tokens = NULL;
    char *tokenBuf;
    size_t tokenCount = 0;
    size_t totalLen = 0;
    const char *tok;
    size_t len;

    if (count != NULL) {
        *count = 0;
    }

    tok = str;
    len = 0;
    while (FindNextStringToken(&tok, &len, sep)) {
        tokenCount++;
        totalLen += len + 1;
    }

    if (tokenCount == 0) {
        return NULL;
    }

    tokens = (char **) malloc((tokenCount + 1) * sizeof(char *)
            + totalLen);
    if (tokens == NULL) {
        return NULL;
    }

    tokenBuf = (char *) (tokens + tokenCount + 1);

    tok = str;
    len = 0;
    tokenCount = 0;
    while (FindNextStringToken(&tok, &len, sep)) {
        memcpy(tokenBuf, tok, len);
        tokenBuf[len] = '\x00';
        tokens[tokenCount++] = tokenBuf;
        tokenBuf += len + 1;
    }
    tokens[tokenCount] = NULL;
    if (count != NULL) {
        *count = tokenCount;
    }
    return tokens;
}

int IsTokenInString(const char *str, const char *token, size_t tokenLen, const char *sep)
{
    const char *ptr = str;
    size_t len = 0;

    while (FindNextStringToken(&ptr, &len, sep)) {
        if (tokenLen == len && strncmp(token, ptr, len) == 0) {
            return 1;
        }
    }
    return 0;
}

char *UnionExtensionStrings(char *currentString, const char *newString)
{
    size_t origLen;
    size_t newLen;
    const char *token;
    size_t tokenLen;
    char *buf, *ptr;

    // Calculate the length of the new string.
    origLen = newLen = strlen(currentString);

    // The code below assumes that currentString is not empty, so if it is
    // empty, then just copy the new string.
    if (origLen == 0) {
        buf = currentString;
        if (newString[0] != '\0') {
            buf = strdup(newString);
            free(currentString);
        }
        return buf;
    }

    token = newString;
    tokenLen = 0;
    while (FindNextStringToken(&token, &tokenLen, " ")) {
        if (!IsTokenInString(currentString, token, tokenLen, " ")) {
            newLen += tokenLen + 1;
        }
    }
    if (origLen == newLen) {
        // No new extensions to add.
        return currentString;
    }

    buf = (char *) realloc(currentString, newLen + 1);
    if (buf == NULL) {
        free(currentString);
        return NULL;
    }
    currentString = NULL;

    ptr = buf + origLen;
    token = newString;
    tokenLen = 0;
    while (FindNextStringToken(&token, &tokenLen, " ")) {
        if (!IsTokenInString(buf, token, tokenLen, " ")) {
            *ptr++ = ' ';
            memcpy(ptr, token, tokenLen);
            ptr += tokenLen;
            *ptr = '\0';
        }
    }
    assert((size_t) (ptr - buf) == newLen);
    return buf;
}

void IntersectionExtensionStrings(char *currentString, const char *newString)
{
    const char *token;
    size_t tokenLen;
    char *ptr;

    token = currentString;
    tokenLen = 0;
    ptr = currentString;
    while(FindNextStringToken(&token, &tokenLen, " ")) {
        if (IsTokenInString(newString, token, tokenLen, " ")) {
            if (ptr != currentString) {
                *ptr++ = ' ';
            }
            memmove(ptr, token, tokenLen);
            ptr += tokenLen;
        }
    }
    *ptr = '\0';
}
