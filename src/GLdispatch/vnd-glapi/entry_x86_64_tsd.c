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
 */
#define STUB_ASM_CODE(slot) \
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

static const unsigned char ENTRY_TEMPLATE[] =
{
    // <ENTRY+0>: movabs ENTRY_CURRENT_TABLE, %rax
    0x48, 0xa1, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08,
    0x48, 0x85, 0xc0, // <ENTRY+10>: test %rax,%rax
    0x75, 0x1c,       // <ENTRY+13>: jne <ENTRY+43>
    // <ENTRY+15>: movabs $_glapi_get_current, %rax
    0x48, 0xb8, 0x10, 0x20, 0x30, 0x40, 0x50, 0x60, 0x70, 0x80,
    0x57,                              // <ENTRY+25>: push %rdi
    0x56,                              // <ENTRY+26>: push %rsi
    0x52,                              // <ENTRY+27>: push %rdx
    0x51,                              // <ENTRY+28>: push %rcx
    0x41, 0x50,                        // <ENTRY+29>: push %r8
    0x41, 0x51,                        // <ENTRY+31>: push %r9
    0xff, 0xd0,                        // <ENTRY+33>: callq *%rax
    0x41, 0x59,                        // <ENTRY+35>: pop %r9
    0x41, 0x58,                        // <ENTRY+37>: pop %r8
    0x59,                              // <ENTRY+39>: pop %rcx
    0x5a,                              // <ENTRY+40>: pop %rdx
    0x5e,                              // <ENTRY+41>: pop %rsi
    0x5f,                              // <ENTRY+42>: pop %rdi
    0xff, 0xa0, 0x00, 0x00, 0x00, 0x00 // <ENTRY+43:> jmpq *SLOT(%rax)
};

// These are the offsets in ENTRY_TEMPLATE of the values that we have to patch.
static const int TEMPLATE_OFFSET_CURRENT_TABLE = 2;
static const int TEMPLATE_OFFSET_CURRENT_TABLE_GET = 17;
static const int TEMPLATE_OFFSET_SLOT = 45;

void entry_generate_default_code(char *entry, int slot)
{
    char *writeEntry = u_execmem_get_writable(entry);
    memcpy(writeEntry, ENTRY_TEMPLATE, sizeof(ENTRY_TEMPLATE));

    *((uint32_t *) (writeEntry + TEMPLATE_OFFSET_SLOT)) = slot * sizeof(mapi_func);
    *((uintptr_t *) (writeEntry + TEMPLATE_OFFSET_CURRENT_TABLE)) = (uintptr_t) _glapi_Current;
    *((uintptr_t *) (writeEntry + TEMPLATE_OFFSET_CURRENT_TABLE_GET)) = (uintptr_t) _glapi_get_current;
}

