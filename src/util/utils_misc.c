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

#define TEMP_FILENAME_ARRAY_SIZE 4

/**
 * Populates \p dirs with a NULL-terminated list of directories to use to open
 * a temp file.
 *
 * \param[out] dirs An array with at least \p TEMP_FILENAME_ARRAY_SIZE elements.
 */
static void GetTempDirs(const char **dirs);

/**
 * Creates a temp file.
 *
 * The file will be created with mkstemp(3), then immediately unlinked so that
 * it doesn't risk leaving any clutter behind.
 *
 * \param tempdir The directory to create the file in.
 * \return A file descriptor, or -1 on error.
 */
static int OpenTempFile(const char *tempdir);

/**
 * Allocates executable memory by mapping a file.
 */
static int AllocExecPagesFile(int fd, size_t size, void **writePtr, void **execPtr);
static int AllocExecPagesAnonymous(size_t size, void **writePtr, void **execPtr);

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

int AllocExecPages(size_t size, void **writePtr, void **execPtr)
{
    const char *dirs[TEMP_FILENAME_ARRAY_SIZE];
    int i;

    *writePtr = NULL;
    *execPtr = NULL;

    // Try to allocate the memory by creating a file and mapping it twice.
    // This follows Ulrich Drepper's recommendation for allocating executable
    // memory:
    // http://www.akkadia.org/drepper/selinux-mem.html
    GetTempDirs(dirs);
    for (i=0; dirs[i] != NULL; i++) {
        int fd = OpenTempFile(dirs[i]);
        if (fd >= 0) {
            int rv = AllocExecPagesFile(fd, size, writePtr, execPtr);
            close(fd);

            if (rv == 0) {
                return 0;
            }
        }
    }

    // Using a file failed, so fall back to trying to create a single anonymous
    // mapping.
    return AllocExecPagesAnonymous(size, writePtr, execPtr);
}

void FreeExecPages(size_t size, void *writePtr, void *execPtr)
{
    if (writePtr != NULL) {
        munmap(writePtr, size);
    }
    if (execPtr != NULL && execPtr != writePtr) {
        munmap(execPtr, size);
    }
}

int OpenTempFile(const char *tempdir)
{
    int fd = -1;

#if defined(O_TMPFILE)
    // If it's available, then try creating a file with O_TMPFILE first.
    fd = open(tempdir, O_RDWR | O_TMPFILE | O_EXCL, S_IRUSR | S_IWUSR);
#endif // defined(HAVE_O_TMPFILE)

    if (fd < 0)
    {
        // If O_TMPFILE wasn't available or wasn't supported, then try mkstemp
        // instead.
        char *templateName = NULL;
        if (glvnd_asprintf(&templateName, "%s/.glvndXXXXXX", tempdir) < 0) {
            return -1;
        }

        fd = mkstemp(templateName);
        if (fd >= 0) {
            // Unlink the file so that we don't leave any clutter behind.
            unlink(templateName);
        }
        free(templateName);
        templateName = NULL;
    }

    // Make sure we can still use the file after it's unlinked.
    if (fd >= 0) {
        struct stat sb;
        if (fstat(fd, &sb) != 0) {
            close(fd);
            fd = -1;
        }
    }

    return fd;
}

int AllocExecPagesFile(int fd, size_t size, void **writePtr, void **execPtr)
{
    if (ftruncate(fd, size) != 0) {
        return -1;
    }

    *execPtr = mmap(NULL, size, PROT_READ | PROT_EXEC, MAP_SHARED, fd, 0);
    if (*execPtr == MAP_FAILED) {
        *execPtr = NULL;
        return -1;
    }

    *writePtr = mmap(NULL, size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    if (*writePtr == MAP_FAILED) {
        munmap(*execPtr, size);
        *execPtr = *writePtr = NULL;
        return -1;
    }

    return 0;
}

int AllocExecPagesAnonymous(size_t size, void **writePtr, void **execPtr)
{
    void *ptr = mmap(NULL, size, PROT_READ | PROT_WRITE | PROT_EXEC,
            MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    if (ptr == MAP_FAILED) {
        return -1;
    }

    *writePtr = *execPtr = ptr;
    return 0;
}

void GetTempDirs(const char **dirs)
{
    int count = 0;

    // Don't use the environment variables if we're running as setuid.
    if (getuid() == geteuid()) {
        dirs[count] = getenv("TMPDIR");
        if (dirs[count] != NULL) {
            count++;
        }

        dirs[count] = getenv("HOME");
        if (dirs[count] != NULL) {
            count++;
        }
    }
    dirs[count++] = "/tmp";
    dirs[count] = NULL;
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
