/*
 * Copyright (C) 2010 LunarG Inc.
 * Copyright (c) 2015, NVIDIA CORPORATION.
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
 *    Kyle Brenneman <kbrenneman@nvidia.com>
 */

#include "entry.h"
#include "entry_common.h"

#include <string.h>
#include <stdint.h>
#include <sys/mman.h>
#include <unistd.h>
#include <assert.h>

#include "glapi.h"
#include "u_macros.h"
#include "u_current.h"
#include "utils_misc.h"

/**
 * \file
 *
 * Common functions for assembly stubs other than ARMv7.
 *
 * These functions are used for the assembly stubs on every architecture except
 * for ARMv7.
 *
 * ARMv7 is different because the ARM stubs have to add 1 to the address of
 * each entrypoint to force switching to Thumb mode.
 */

void entry_init_public(void)
{
}

mapi_func entry_get_public(int index)
{
    return (mapi_func)(public_entry_start + (index * entry_stub_size));
}

void entry_get_patch_addresses(mapi_func entry, void **writePtr, const void **execPtr)
{
    *execPtr = (const void *) entry;
    *writePtr = u_execmem_get_writable(entry);
}

#if !defined(STATIC_DISPATCH_ONLY)
mapi_func entry_generate(int slot)
{
    void *code = u_execmem_alloc(entry_stub_size);
    if (!code) {
        return NULL;
    }

    entry_generate_default_code(code, slot);

    return (mapi_func) code;
}
#endif // !defined(STATIC_DISPATCH_ONLY)
