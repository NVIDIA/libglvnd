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

static int entry_patch_mprotect(int prot)
{
    size_t size;
    size_t pageSize = (size_t) sysconf(_SC_PAGESIZE);

    if (((uintptr_t) public_entry_start) % pageSize != 0
            || ((uintptr_t) public_entry_end) % pageSize != 0) {
        assert(((uintptr_t) public_entry_start) % pageSize == 0);
        assert(((uintptr_t) public_entry_end) % pageSize == 0);
        return 0;
    }

    size = ((uintptr_t) public_entry_end) - ((uintptr_t) public_entry_start);

    if (mprotect(public_entry_start, size, prot) != 0) {
        return 0;
    }
    return 1;
}

int entry_patch_start(void)
{
    // Set the memory protections to read/write/exec.
    // Since this only gets called when no thread has a current context, this
    // could also just be read/write, without exec, and then set it back to
    // read/exec afterward. But then, if the first mprotect succeeds and the
    // second fails, we'll be left with un-executable entrypoints.
    return entry_patch_mprotect(PROT_READ | PROT_WRITE | PROT_EXEC);
}

int entry_patch_finish(void)
{
    return entry_patch_mprotect(PROT_READ | PROT_EXEC);
}

