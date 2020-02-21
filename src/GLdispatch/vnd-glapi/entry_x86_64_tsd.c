/*
 * Mesa 3-D graphics library
 *
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

#include <assert.h>
#include <stdint.h>
#include <sys/mman.h>
#include <unistd.h>
#include <string.h>

#include "u_macros.h"
#include "glapi.h"
#include "glvnd/GLdispatchABI.h"

#define ENTRY_STUB_ALIGN 64
#if !defined(GLDISPATCH_PAGE_SIZE)
#define GLDISPATCH_PAGE_SIZE 4096
#endif

__asm__(".section wtext,\"ax\",@progbits\n");
__asm__(".balign " U_STRINGIFY(GLDISPATCH_PAGE_SIZE) "\n"
       ".globl public_entry_start\n"
       ".hidden public_entry_start\n"
        "public_entry_start:");

#define STUB_ASM_ENTRY(func)        \
    ".globl " func "\n"              \
    ".type " func ", @function\n"    \
    ".balign " U_STRINGIFY(ENTRY_STUB_ALIGN) "\n"                   \
    func ":"

/*
 * Note that this stub does not exactly match the machine code in
 * ENTRY_TEMPLATE[] below.  In particular, we take advantage of the GOT and PLT
 * to produce RIP-relative relocations for the stubs stamped out by mapi_tmp.h.
 * We can't do that in general for the generated stubs since they're emitted
 * into malloc()ed memory which may not be within 2GB of %rip, as explained in
 * the comment in u_execmem.c.
 *
 * TODO: The dynamic stubs are no longer allocated, so we should be able to
 * assume that they're within 2GB of %rip.
 */
#define STUB_ASM_CODE(slot) \
    ENDBR \
    "movq _glapi_Current@GOTPCREL(%rip), %rax\n\t" \
    "movq (%rax), %rax\n" \
    "test %rax, %rax\n\t"           \
    "jne 1f\n\t"                      \
    "push %rdi\n" \
    "push %rsi\n" \
    "push %rdx\n" \
    "push %rcx\n" \
    "push %r8\n" \
    "push %r9\n" \
    "call _glapi_get_current@PLT\n" \
    "pop %r9\n" \
    "pop %r8\n" \
    "pop %rcx\n" \
    "pop %rdx\n" \
    "pop %rsi\n" \
    "pop %rdi\n" \
    "1:\n\t"                         \
    "jmp *(8 * " slot ")(%rax)"

#define MAPI_TMP_STUB_ASM_GCC
#include "mapi_tmp.h"


__asm__(".balign " U_STRINGIFY(GLDISPATCH_PAGE_SIZE) "\n"
       ".globl public_entry_end\n"
       ".hidden public_entry_end\n"
        "public_entry_end:");
__asm__(".text\n");

const int entry_type = __GLDISPATCH_STUB_X86_64;
const int entry_stub_size = ENTRY_STUB_ALIGN;

