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
 * u_execmem_alloc() allocates 128 bytes per stub.
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

/*
 * Bytecode for STUB_ASM_CODE()
 */
static const uint32_t ENTRY_TEMPLATE[] =
{
    0xa9bf03e1, // <ENTRY>:	stp	x1, x0, [sp,#-16]!
    0x58000240, // <ENTRY+4>:	ldr	x0, <ENTRY+76>
    0xf9400000, // <ENTRY+8>:	ldr	x0, [x0]
    0xb40000a0, // <ENTRY+12>:	cbz	x0, <ENTRY+32>
    0x58000261, // <ENTRY+16>:	ldr	x1, <ENTRY+92>
    0xf8616810, // <ENTRY+20>:	ldr	x16, [x0,x1]
    0xa8c103e1, // <ENTRY+24>:	ldp	x1, x0, [sp],#16
    0xd61f0200, // <ENTRY+28>:	br	x16
    0xf81f0ffe, // <ENTRY+32>:	str	x30, [sp,#-16]!
    0xa9bf1be7, // <ENTRY+36>:	stp	x7, x6, [sp,#-16]!
    0xa9bf13e5, // <ENTRY+40>:	stp	x5, x4, [sp,#-16]!
    0xa9bf0be3, // <ENTRY+44>:	stp	x3, x2, [sp,#-16]!
    0x58000120, // <ENTRY+48>:	ldr	x0, <ENTRY+84>
    0xd63f0000, // <ENTRY+52>:	blr	x0
    0xa8c10be3, // <ENTRY+56>:	ldp	x3, x2, [sp],#16
    0xa8c113e5, // <ENTRY+60>:	ldp	x5, x4, [sp],#16
    0xa8c11be7, // <ENTRY+64>:	ldp	x7, x6, [sp],#16
    0xf84107fe, // <ENTRY+68>:	ldr	x30, [sp],#16
    0x17fffff2, // <ENTRY+72>:	b	<ENTRY+16>

    // Offsets that need to be patched
    0x00000000, 0x00000000, // <ENTRY+76>: _glapi_Current
    0x00000000, 0x00000000, // <ENTRY+84>: _glapi_get_current
    0x00000000, 0x00000000, // <ENTRY+92>: slot * sizeof(void*)
};

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

// The offsets in ENTRY_TEMPLATE that need to be patched.
static const int TEMPLATE_OFFSET_CURRENT_TABLE     = sizeof(ENTRY_TEMPLATE) - 3*8;
static const int TEMPLATE_OFFSET_CURRENT_TABLE_GET = sizeof(ENTRY_TEMPLATE) - 2*8;
static const int TEMPLATE_OFFSET_SLOT              = sizeof(ENTRY_TEMPLATE) - 8;

void entry_generate_default_code(char *entry, int slot)
{
    char *writeEntry;

    // Get the pointer to the writable mapping.
    writeEntry = (char *) u_execmem_get_writable(entry);

    memcpy(writeEntry, ENTRY_TEMPLATE, sizeof(ENTRY_TEMPLATE));

    // Patch the slot number and whatever addresses need to be patched.
    *((uint64_t *)(writeEntry + TEMPLATE_OFFSET_SLOT)) = (uint64_t)(slot * sizeof(mapi_func));
    *((uint64_t *)(writeEntry + TEMPLATE_OFFSET_CURRENT_TABLE)) =
        (uint64_t)_glapi_Current;
    *((uint64_t *)(writeEntry + TEMPLATE_OFFSET_CURRENT_TABLE_GET)) =
        (uint64_t)_glapi_get_current;

    // See http://community.arm.com/groups/processors/blog/2010/02/17/caches-and-self-modifying-code
    __builtin___clear_cache(writeEntry, writeEntry + sizeof(ENTRY_TEMPLATE));
}
