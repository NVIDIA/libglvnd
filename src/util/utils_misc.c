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

