/*
 * Copyright (c) 2016, NVIDIA CORPORATION.
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

#include "entry.h"
#include "entry_common.h"

#include <string.h>
#include <stdint.h>
#include <unistd.h>
#include <assert.h>

#include "glapi.h"
#include "u_macros.h"
#include "u_current.h"
#include "utils_misc.h"
#include "glvnd/GLdispatchABI.h"

/*
 * See: https://sourceware.org/binutils/docs/as/ARM-Directives.html
 */

/*
 * The size of each dispatch stub.
 */
#define ENTRY_STUB_ALIGN 128
#if !defined(GLDISPATCH_PAGE_SIZE)
// Note that on aarch64, the page size could be 4K or 64K. Pick 64K, since that
// will work in either case.
#define GLDISPATCH_PAGE_SIZE 65536
#endif

#define STUB_ASM_ENTRY(func)                        \
    ".balign " U_STRINGIFY(ENTRY_STUB_ALIGN) "\n\t" \
    ".global " func "\n\t"                          \
    ".type " func ", %function\n\t"                 \
    func ":\n\t"

/*
 * Looks up the current dispatch table, finds the stub address at the given slot
 * then jumps to it.
 *
 * First tries to find a dispatch table in _glapi_Current[GLAPI_CURRENT_DISPATCH],
 * if not found then it jumps to the 'lookup_dispatch' and calls
 * _glapi_get_current() then jumps back to the 'found_dispatch' label.
 *
 * The 'found_dispatch' section computes the correct offset in the dispatch
 * table then does a branch without link to the function address.
 */
#define STUB_ASM_CODE(slot)                           \
    "stp x1, x0, [sp, #-16]!\n\t"                     \
    "adrp x0, :got:_glapi_Current\n\t"                \
    "ldr x0, [x0, #:got_lo12:_glapi_Current]\n\t"     \
    "ldr x0, [x0]\n\t"                                \
    "cbz x0, 10f\n\t"                                 \
    "11:\n\t"        /* found dispatch */             \
    "ldr x1, 3f\n\t"                                  \
    "ldr x16, [x0, x1]\n\t"                           \
    "ldp x1, x0, [sp], #16\n\t"                       \
    "br x16\n\t"                                      \
    "10:\n\t"        /* lookup dispatch */            \
    "str x30, [sp, #-16]!\n\t"                        \
    "stp x7, x6, [sp, #-16]!\n\t"                     \
    "stp x5, x4, [sp, #-16]!\n\t"                     \
    "stp x3, x2, [sp, #-16]!\n\t"                     \
    "adrp x0, :got:_glapi_get_current\n\t"            \
    "ldr x0, [x0, #:got_lo12:_glapi_get_current]\n\t" \
    "blr x0\n\t"                                      \
    "ldp x3, x2, [sp], #16\n\t"                       \
    "ldp x5, x4, [sp], #16\n\t"                       \
    "ldp x7, x6, [sp], #16\n\t"                       \
    "ldr x30, [sp], #16\n\t"                          \
    "b 11b\n\t"                                       \
    "3:\n\t"                                          \
    ".xword " slot " * 8\n\t" /* size of (void *) */

__asm__(".section wtext,\"ax\"\n"
        ".balign " U_STRINGIFY(GLDISPATCH_PAGE_SIZE) "\n"
       ".globl public_entry_start\n"
       ".hidden public_entry_start\n"
        "public_entry_start:\n");

#define MAPI_TMP_STUB_ASM_GCC
#include "mapi_tmp.h"

__asm__(".balign " U_STRINGIFY(GLDISPATCH_PAGE_SIZE) "\n"
       ".globl public_entry_end\n"
       ".hidden public_entry_end\n"
        "public_entry_end:\n"
        ".text\n\t");

const int entry_type = __GLDISPATCH_STUB_AARCH64;
const int entry_stub_size = ENTRY_STUB_ALIGN;

