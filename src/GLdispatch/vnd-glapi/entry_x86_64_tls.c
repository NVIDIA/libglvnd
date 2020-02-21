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
#include "entry_common.h"

#include <assert.h>
#include <sys/mman.h>
#include <unistd.h>
#include <string.h>

#include "utils_misc.h"
#include "u_macros.h"
#include "glapi.h"
#include "glvnd/GLdispatchABI.h"

#define ENTRY_STUB_ALIGN 32
#if !defined(GLDISPATCH_PAGE_SIZE)
#define GLDISPATCH_PAGE_SIZE 4096
#endif

__asm__(".section wtext,\"ax\",@progbits\n");
__asm__(".balign " U_STRINGIFY(GLDISPATCH_PAGE_SIZE) "\n"
       ".globl public_entry_start\n"
       ".hidden public_entry_start\n"
        "public_entry_start:");

#define STUB_ASM_ENTRY(func)                             \
    ".globl " func "\n"                                   \
    ".type " func ", @function\n"                         \
    ".balign " U_STRINGIFY(ENTRY_STUB_ALIGN) "\n" \
    func ":"

#ifdef __ILP32__

#define STUB_ASM_CODE(slot)                              \
    ENDBR                                               \
    "movq _glapi_tls_Current@GOTTPOFF(%rip), %rax\n\t"  \
    "movl %fs:(%rax), %r11d\n\t"                          \
    "movl 4*" slot "(%r11d), %r11d\n\t"                   \
    "jmp *%r11"

#else // __ILP32__

#define STUB_ASM_CODE(slot)                                 \
    ENDBR                                               \
    "movq _glapi_tls_Current@GOTTPOFF(%rip), %rax\n\t"  \
    "movq %fs:(%rax), %r11\n\t"                              \
    "jmp *(8 * " slot ")(%r11)"

#endif // __ILP32__

#define MAPI_TMP_STUB_ASM_GCC
#include "mapi_tmp.h"

__asm__(".balign " U_STRINGIFY(GLDISPATCH_PAGE_SIZE) "\n"
       ".globl public_entry_end\n"
       ".hidden public_entry_end\n"
        "public_entry_end:");

__asm__(".text\n");

const int entry_stub_size = ENTRY_STUB_ALIGN;

#ifdef __ILP32__

const int entry_type = __GLDISPATCH_STUB_X32;

#else // __ILP32__

const int entry_type = __GLDISPATCH_STUB_X86_64;

#endif // __ILP32__

