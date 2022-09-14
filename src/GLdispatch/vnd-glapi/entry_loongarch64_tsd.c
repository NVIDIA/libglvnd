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
 * The size of each dispatch stub.
 */
#define ENTRY_STUB_ALIGN 256
#if !defined(GLDISPATCH_PAGE_SIZE)
// Note that on loongarch64, the page size is 16K.
#define GLDISPATCH_PAGE_SIZE 16384
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
#define STUB_ASM_CODE(slot)                      \
    "addi.d $sp, $sp, -16\n\t"                   \
    "st.d $a1, $sp, 0\n\t"                       \
    "st.d $a0, $sp, 8\n\t"                       \
    "la.global $a0,_glapi_Current\n\t"           \
    "ld.d $a0, $a0,0\n\t"                        \
    "beqz $a0, 10f\n\t"                          \
    "11:\n\t"        /* found dispatch */        \
    "la.local $a1, 3f\n\t"                       \
    "ld.d $a1, $a1,0\n\t"                        \
    "ldx.d $t1, $a0, $a1\n\t"                    \
    "ld.d $a1, $sp, 0\n\t"                       \
    "ld.d $a0, $sp, 8\n\t"                       \
    "addi.d $sp, $sp, 16\n\t"                    \
    "jirl $r0,$t1,0\n\t"                         \
    "10:\n\t"        /* lookup dispatch */       \
    "addi.d $sp, $sp, -8*8\n\t"                  \
    "st.d $ra, $sp, 0\n\t"                       \
    "st.d $a7, $sp, 8\n\t"                       \
    "st.d $a6, $sp, 16\n\t"                      \
    "st.d $a5, $sp, 24\n\t"                      \
    "st.d $a4, $sp, 32\n\t"                      \
    "st.d $a3, $sp, 40\n\t"                      \
    "st.d $a2, $sp, 48\n\t"                      \
    "la.global $a0,_glapi_get_current\n\t"       \
    "jirl $ra, $a0,0\n\t"                        \
    "ld.d $ra, $sp, 0\n\t"                       \
    "ld.d $a7, $sp, 8\n\t"                       \
    "ld.d $a6, $sp, 16\n\t"                      \
    "ld.d $a5, $sp, 24\n\t"                      \
    "ld.d $a4, $sp, 32\n\t"                      \
    "ld.d $a3, $sp, 40\n\t"                      \
    "ld.d $a2, $sp, 48\n\t"                      \
    "addi.d $sp, $sp, 8*8\n\t"                   \
    "b 11b\n\t"                                  \
    "3:\n\t"                                     \
    ".dword " slot " * 8\n\t" /* size of (void *) */

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

const int entry_type = __GLDISPATCH_STUB_LOONGARCH64;
const int entry_stub_size = ENTRY_STUB_ALIGN;

