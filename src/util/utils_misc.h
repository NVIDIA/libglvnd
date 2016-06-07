/*
 * Copyright (c) 2013-2015, NVIDIA CORPORATION.
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

#if !defined(__UTILS_MISC_H)
#define __UTILS_MISC_H

#include <stddef.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdarg.h>

/*
 * Various macros which may prove useful in various places
 */

#define ARRAY_LEN(_arr) (sizeof(_arr)/sizeof((_arr)[0]))

#if (201104 <= __STDC_VERSION__ \
     || (4 < __GNUC__) || ((__GNUC__ == 4) && (__GNUC_MINOR__ >= 6)))
# define STATIC_ASSERT(x) _Static_assert((x), #x)
#else
# define STATIC_ASSERT(x) do {             \
    (void) sizeof(char [1 - 2*(!(x))]); \
} while (0)
#endif

#if ((2 < __GNUC__) || ((__GNUC__ == 2) && (__GNUC_MINOR__ >= 93)))
# define UNUSED __attribute__((__unused__))
#else
# define UNUSED 
#endif

#define ASSERT_CODE(x) x

/*!
 * A local implementation of asprintf(3), for systems that don't support it.
 */
int glvnd_asprintf(char **strp, const char *fmt, ...);

/*!
 * A local implementation of vasprintf(3), for systems that don't support it.
 */
int glvnd_vasprintf(char **strp, const char *fmt, va_list args);

/**
 * Allocates executable memory.
 *
 * To avoid having the same page be both writable and executable, this function
 * returns two pointers to the same mapping, one read/write and one
 * read/execute.
 *
 * Depending on the system, \p writePtr and \p execPtr may return the same
 * pointer, mapped as read/write/execute.
 *
 * \param size The number of bytes to allocate.
 * \param[out] writePtr Returns a pointer to the read/write mapping.
 * \param[out] execPtr Returns a pointer to the read/exec mapping.
 * \return Zero on success, non-zero on error.
 */
int AllocExecPages(size_t size, void **writePtr, void **execPtr);

/**
 * Frees the pages allocated from \p allocExecMem.
 */
void FreeExecPages(size_t size, void *writePtr, void *execPtr);

/*!
 * Swaps the bytes of an array.
 *
 * @param array The array.
 * @param size  The size in bytes of the array, must be a multiple of 2.
 */
void glvnd_byte_swap16(uint16_t* array, const size_t size);

#endif // !defined(__UTILS_MISC_H)
