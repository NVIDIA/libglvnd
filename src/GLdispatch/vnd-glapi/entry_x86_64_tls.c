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
    "movq _glapi_tls_Current@GOTTPOFF(%rip), %rax\n\t"  \
    "movl %fs:(%rax), %r11d\n\t"                          \
    "movl 4*" slot "(%r11d), %r11d\n\t"                   \
    "jmp *%r11"

#else // __ILP32__

#define STUB_ASM_CODE(slot)                                 \
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

__asm__("x86_64_current_tls:\n\t"
	"movq _glapi_tls_Current@GOTTPOFF(%rip), %rax\n\t"
	"ret");

extern uint64_t
x86_64_current_tls();

const int entry_stub_size = ENTRY_STUB_ALIGN;

#ifdef __ILP32__

const int entry_type = __GLDISPATCH_STUB_X32;
static const unsigned char ENTRY_TEMPLATE[] = {
    0x64, 0x44, 0x8b, 0x1c, 0x25, 0x00, 0x00, 0x00, 0x00, // movl %fs:0, %r11d
    0x67, 0x45, 0x8b, 0x9b, 0x34, 0x12, 0x00, 0x00,       // movl 0x1234(%r11d), %r11d
    0x41, 0xff, 0xe3,                                     // jmp *%r11
};

static const unsigned int TLS_ADDR_OFFSET = 5;
static const unsigned int SLOT_OFFSET = 13;

#else // __ILP32__

const int entry_type = __GLDISPATCH_STUB_X86_64;

static const unsigned char ENTRY_TEMPLATE[] = {
    0x64, 0x4c, 0x8b, 0x1c, 0x25, 0x00, 0x00, 0x00, 0x00, // movq %fs:0, %r11
    0x41, 0xff, 0xa3, 0x34, 0x12, 0x00, 0x00,             // jmp *0x1234(%r11)
};
static const unsigned int TLS_ADDR_OFFSET = 5;
static const unsigned int SLOT_OFFSET = 12;

#endif // __ILP32__

void entry_generate_default_code(char *entry, int slot)
{
    char *writeEntry = u_execmem_get_writable(entry);
    uint64_t tls_addr;

    STATIC_ASSERT(ENTRY_STUB_ALIGN >= sizeof(ENTRY_TEMPLATE));

    assert(slot >= 0);

    tls_addr = x86_64_current_tls();

    memcpy(writeEntry, ENTRY_TEMPLATE, sizeof(ENTRY_TEMPLATE));
    *((unsigned int *) &writeEntry[TLS_ADDR_OFFSET]) = (unsigned int) tls_addr;
    *((unsigned int *) &writeEntry[SLOT_OFFSET]) = (unsigned int) (slot * sizeof(mapi_func));
}

