/*
 * Copyright (c) 2015, NVIDIA CORPORATION.
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
__asm__(".syntax unified\n\t");

/*
 * The size of each dispatch stub.
 */
#define ENTRY_STUB_ALIGN 128
#if !defined(GLDISPATCH_PAGE_SIZE)
#define GLDISPATCH_PAGE_SIZE 4096
#endif

/*
 * This runs in Thumb mode.
 *
 * libglvnd on armv7 is built with -march=armv7-a, which uses the AAPCS ABI
 * that has ARM/Thumb interworking enabled by default.
 *
 * See: https://gcc.gnu.org/onlinedocs/gcc/ARM-Options.html
 */
#define STUB_ASM_ENTRY(func)                        \
    ".balign " U_STRINGIFY(ENTRY_STUB_ALIGN) "\n\t" \
    ".thumb_func\n\t"                               \
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
 *
 * This routine preserves the r0-r3 volatile registers as they store the
 * parameters of the entry point that is being looked up.
 */
#define STUB_ASM_CODE(slot)                  \
    "push {r0-r3}\n\t"                       \
    "ldr r2, 1f\n\t"                         \
    "12:\n\t"                                \
    "add r2, pc\n\t"                         \
    "ldr r3, 1f+4\n\t"                       \
    "ldr r0, [r2, r3]\n\t"                   \
    "ldr r0, [r0]\n\t"                       \
    "cmp r0, #0\n\t"                         \
    "it eq\n\t"                              \
    "beq 10f\n\t"                            \
    "11:\n\t"        /* found_dispatch */    \
    "ldr r1, 3f\n\t"                         \
    "mov r2, #4\n\t" /* sizeof(void *) */    \
    "mul r1, r1, r2\n\t"                     \
    "ldr ip, [r0, +r1]\n\t"                  \
    "pop {r0-r3}\n\t"                        \
    "bx ip\n\t"                              \
    "10:\n\t"        /* lookup_dispatch */   \
    "push {lr}\n\t"                          \
    "ldr r2, 2f\n\t"                         \
    "13:\n\t"                                \
    "add r2, pc\n\t"                         \
    "ldr r3, 2f+4\n\t"                       \
    "ldr r0, [r2, r3]\n\t"                   \
    "blx r0\n\t"                             \
    "pop {lr}\n\t"                           \
    "b 11b\n\t"                              \
    "1:\n\t"                                 \
    ".word _GLOBAL_OFFSET_TABLE_-(12b+4)\n\t"\
    ".word _glapi_Current(GOT)\n\t"          \
    "2:\n\t"                                 \
    ".word _GLOBAL_OFFSET_TABLE_-(13b+4)\n\t"\
    ".word _glapi_get_current(GOT)\n\t"      \
    "3:\n\t"                                 \
    ".word " slot "\n\t"

__asm__(".section wtext,\"ax\"\n"
        ".balign " U_STRINGIFY(GLDISPATCH_PAGE_SIZE) "\n"
        ".syntax unified\n"
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

/*
 * If built with -marm, let the assembler know that we are done with Thumb
 */
#if !defined(__thumb__)
__asm__(".arm\n\t");
#endif

const int entry_type = __GLDISPATCH_STUB_ARMV7_THUMB;
const int entry_stub_size = ENTRY_STUB_ALIGN;

// Note: The rest of these functions could also be used for ARMv7 TLS stubs,
// once those are implemented.

mapi_func
entry_get_public(int index)
{
    // Add 1 to the base address to force Thumb mode when jumping to the stub
    return (mapi_func)(public_entry_start + (index * entry_stub_size) + 1);
}

