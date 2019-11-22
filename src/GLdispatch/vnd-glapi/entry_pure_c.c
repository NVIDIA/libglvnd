/*
 * Mesa 3-D graphics library
 *
 * Copyright (C) 2010 LunarG Inc.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included
 * in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 *
 * Authors:
 *    Chia-I Wu <olv@lunarg.com>
 */

#include "entry.h"
#include <assert.h>
#include <stdlib.h>

#include "glapi.h"
#include "utils_misc.h"
#include "glvnd/GLdispatchABI.h"

static INLINE const struct _glapi_table *
entry_current_get(void)
{
#if defined (GLDISPATCH_USE_TLS)
   return _glapi_tls_Current[GLAPI_CURRENT_DISPATCH];
#else
   return (likely(_glapi_Current[GLAPI_CURRENT_DISPATCH]) ?
         _glapi_Current[GLAPI_CURRENT_DISPATCH] : _glapi_get_current());
#endif
}

/* C version of the public entries */
#define MAPI_TMP_DEFINES
#define MAPI_TMP_PUBLIC_DECLARES
#define MAPI_TMP_PUBLIC_ENTRIES
#include "mapi_tmp.h"

const int entry_type = __GLDISPATCH_STUB_UNKNOWN;
const int entry_stub_size = 0;

mapi_func
entry_get_public(int index)
{
    /* pubic_entries are defined by MAPI_TMP_PUBLIC_ENTRIES */
    if (index >= 0 && index < ARRAY_LEN(public_entries)) {
        return public_entries[index];
    } else {
        return NULL;
    }
}

int entry_patch_start(void)
{
    assert(!"This should never be called");
    return 0;
}

int entry_patch_finish(void)
{
    assert(!"This should never be called");
    return 0;
}

void *entry_get_patch_address(int index)
{
    assert(!"This should never be called");
    return NULL;
}

void *entry_save_entrypoints(void)
{
    assert(!"This should never be called");
    return NULL;
}

void entry_restore_entrypoints(void *saved)
{
    assert(!"This should never be called");
}
