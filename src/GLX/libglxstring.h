#if !defined(__LIB_GLX_STRING_H)
#define __LIB_GLX_STRING_H

#include <stddef.h>

/*!
 * A local implementation of asprintf(3), for systems that don't support it.
 */
int glvnd_asprintf(char **strp, const char *fmt, ...);

/*!
 * Helper function for tokenizing an extension string.
 *
 * The function is similar to strtok, except that it doesn't modify the string.
 * Instead, it returns a pointer to each token and the length of each token.
 *
 * On the first call, \p name should point to the start of the extension
 * string, and \p *len should be zero.
 *
 * On subsequent calls, it will use the values in \p name and \p len to find
 * the next token, so the caller must leave those values unmodified between
 * calls.
 *
 * \param[in,out] name Returns a pointer to the next extension name.
 * \param[in,out] len Returns the length of the extension name.
 * \return 1 if another name was found, 0 if we hit the end of the string.
 */
int FindNextExtensionName(const char **name, size_t *len);

/*!
 * Checks if a name is present in an extension string.
 *
 * \param extensions The extension string to scan.
 * \param name The extension name to look for. It does not need to be null-terminated.
 * \param nameLen The length of the string \p name.
 * \return 1 if the name was found, 0 if it was not.
 */
int IsExtensionInString(const char *extensions, const char *name, size_t nameLen);

/*!
 * Parses the version string that you'd get from calling glXGetClientString
 * with GLX_VERSION.
 *
 * \param version The version string.
 * \param[out] major
 */
int ParseClientVersionString(const char *version, int *major, int *minor, const char **vendor);

#endif // __LIB_GLX_STRING_H
