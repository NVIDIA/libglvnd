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
#include <stdint.h>
#include <sys/mman.h>
#include <unistd.h>
#include <string.h>

#include "u_macros.h"
#include "glapi/glapi.h"

#define X86_ENTRY_SIZE 32

__asm__(".section wtext,\"ax\",@progbits\n");
__asm__(".balign 4096\n"
       ".globl public_entry_start\n"
        "public_entry_start:");

#define STUB_ASM_ENTRY(func)        \
   ".globl " func "\n"              \
   ".type " func ", @function\n"    \
   ".balign " U_STRINGIFY(X86_ENTRY_SIZE) "\n"                   \
   func ":"

#define STUB_ASM_CODE(slot)         \
    "movl _glapi_Current, %eax\n\t" \
    "testl %eax, %eax\n\t"           \
    "je 1f\n\t"                      \
    "jmp *(4 * " slot ")(%eax)\n"    \
    "1:\n\t"                         \
    "call _glapi_get_current\n\t" \
    "jmp *(4 * " slot ")(%eax)"

#define MAPI_TMP_STUB_ASM_GCC
#include "mapi_tmp.h"


__asm__(".balign 4096\n"
       ".globl public_entry_end\n"
        "public_entry_end:");
__asm__(".text\n");

const int entry_stub_size = X86_ENTRY_SIZE;

static const unsigned char ENTRY_TEMPLATE[] =
{
    0xa1, 0x40, 0x30, 0x20, 0x10,       // <ENTRY>:    mov    _glapi_Current, %eax
    0x85, 0xc0,                         // <ENTRY+5>:  test   %eax, %eax
    0x74, 0x06,                         // <ENTRY+7>:  je     <ENTRY+15>
    0xff, 0xa0, 0x40, 0x30, 0x20, 0x10, // <ENTRY+9>:  jmp    *slot(%eax)
    0xe8, 0x40, 0x30, 0x20, 0x10,       // <ENTRY+15>: call   _glapi_get_current
    0xff, 0xa0, 0x40, 0x30, 0x20, 0x10, // <ENTRY+20>: jmp    *slot(%eax)
};

// These are the offsets in ENTRY_TEMPLATE of the values that we have to patch.
static const int TEMPLATE_OFFSET_CURRENT_TABLE = 1;
static const int TEMPLATE_OFFSET_CURRENT_TABLE_GET = 16;
static const int TEMPLATE_OFFSET_CURRENT_TABLE_GET_RELATIVE = 20;
static const int TEMPLATE_OFFSET_SLOT1 = 11;
static const int TEMPLATE_OFFSET_SLOT2 = 22;

void entry_generate_default_code(char *entry, int slot)
{
    char *writeEntry = u_execmem_get_writable(entry);
    uintptr_t getTableOffset;

    memcpy(writeEntry, ENTRY_TEMPLATE, sizeof(ENTRY_TEMPLATE));

    *((uint32_t *) (writeEntry + TEMPLATE_OFFSET_SLOT1)) = slot * sizeof(mapi_func);
    *((uint32_t *) (writeEntry + TEMPLATE_OFFSET_SLOT2)) = slot * sizeof(mapi_func);
    *((uintptr_t *) (writeEntry + TEMPLATE_OFFSET_CURRENT_TABLE)) = (uintptr_t) _glapi_Current;

    // Calculate the offset to patch for the CALL instruction to
    // _glapi_get_current.
    getTableOffset = (uintptr_t) _glapi_get_current;
    getTableOffset -= (((uintptr_t) entry) + TEMPLATE_OFFSET_CURRENT_TABLE_GET_RELATIVE);
    *((uintptr_t *) (writeEntry + TEMPLATE_OFFSET_CURRENT_TABLE_GET)) = getTableOffset;
}

