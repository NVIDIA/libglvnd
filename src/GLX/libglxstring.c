#include "libglxstring.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int FindNextExtensionName(const char **name, size_t *len)
{
    // Skip to the end of the current name.
    const char *ptr = *name + *len;

    // Skip any leading whitespace.
    while (*ptr == ' ') {
        ptr++;
    }

    // Find the length of the current token.
    *len = 0;
    while (ptr[*len] != '\0' && ptr[*len] != ' ') {
        (*len)++;
    }
    *name = ptr;
    return (*len > 0 ? 1 : 0);
}

int IsExtensionInString(const char *extensions, const char *name, size_t nameLen)
{
    if (extensions != NULL) {
        const char *ptr = extensions;
        size_t tokenLen = 0;
        while (FindNextExtensionName(&ptr, &tokenLen)) {
            if (tokenLen == nameLen && strncmp(ptr, name, nameLen) == 0) {
                return 1;
            }
        }
    }
    return 0;
}

int ParseClientVersionString(const char *version, int *major, int *minor, const char **vendor)
{
    int count;
    const char *ptr;

    count = sscanf(version, "%d.%d", major, minor);
    if (count != 2) {
        return -1;
    }

    // The vendor-specific info should be after the first space character.
    *vendor = NULL;
    ptr = strchr(version, ' ');
    if (ptr != NULL) {
        while (*ptr == ' ') {
            ptr++;
        }
        if (*ptr != '\0') {
            *vendor = ptr;
        }
    }
    return 0;
}

