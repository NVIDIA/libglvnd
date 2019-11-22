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

/*!
 * Swaps the bytes of an array.
 *
 * @param array The array.
 * @param size  The size in bytes of the array, must be a multiple of 2.
 */
void glvnd_byte_swap16(uint16_t* array, const size_t size);

/*!
 * Helper function for tokenizing a string.
 *
 * The function is similar to strtok, except that it doesn't modify the string.
 * Instead, it returns a pointer to each token and the length of each token.
 *
 * On the first call, \p tok should point to the start of the string to be
 * scanned, and \p *len should be zero.
 *
 * On subsequent calls, it will use the values in \p tok and \p len to find
 * the next token, so the caller must leave those values unmodified between
 * calls.
 *
 * \param[in,out] tok Returns a pointer to the next token.
 * \param[in,out] len Returns the length of the token.
 * \param[in] sep A set of characters to separate tokens.
 * \return 1 if another token was found, 0 if we hit the end of the string.
 */
int FindNextStringToken(const char **tok, size_t *len, const char *sep);

/*!
 * Splits a string into tokens.
 *
 * This function will split a string into an array of tokens.
 *
 * It will return an array of null-terminated strings. Empty tokens are
 * skipped, and the whole array is terminated with NULL.
 *
 * The array is allocated in a single block, which the caller should free.
 *
 * \param[in] str The string to split.
 * \param[out] count Optional. Returns the number of tokens in the array.
 * \param[in] sep A set of characters to separate tokens.
 * \return An array of tokens. Returns NULL on error or if \p str had no tokens
 * (that is, if it was empty or only contained separator characters).
 */
char **SplitString(const char *str, size_t *count, const char *sep);

/*!
 * Searches for a token in a string.
 *
 * This function will use \c FindNextStringToken to split up the string, and
 * then will look for a token that matches \p token.
 *
 * \param str The string to search.
 * \param token The token to search for. It does not need to be null-terminated.
 * \param tokenLen The length of \p token.
 * \param sep A set of characters to separate tokens.
 * \return 1 if the token was found, or 0 if it was not.
 */
int IsTokenInString(const char *str, const char *token, size_t tokenLen, const char *sep);

/**
 * Merges two extension strings (that is, finds the union of two sets of
 * extensions).
 *
 * If \p newString is a subset of \c currentString, then \c currentString will
 * be returned unmodified. Otherwise, \c currentString will be re-allocated
 * with enough space to hold the union of both string.
 *
 * If an error occurrs, then \c currentString will be freed before returning.
 *
 * \param currentString The current string, which must have been allocated with malloc.
 * \param newString The extension string to merge.
 * \return A new extension string.
 */
char *UnionExtensionStrings(char *currentString, const char *newString);

/**
 * Finds the intersection between two extension strings.
 *
 * This function will modify \p currentString so that it only includes the
 * extensions that are listed in both \p currentString and \p newString.
 *
 * Note that unlike \c UnionExtensionString, the result cannot be longer than
 * \p currentString, so it won't need to reallocate the string.
 *
 * \param currentString The extension string that will be modified.
 * \param newString The other extension string.
 */
void IntersectionExtensionStrings(char *currentString, const char *newString);

#endif // !defined(__UTILS_MISC_H)
