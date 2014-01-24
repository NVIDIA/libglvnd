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

#include <assert.h>
#include "utils_misc.h"
#include "u_macros.h"

#define ENTRY_STUB_ALIGN 32
#define ENTRY_STUB_SIZE ENTRY_STUB_ALIGN
#define ENTRY_STUB_ALIGN_DIRECTIVE ".balign " U_STRINGIFY(ENTRY_STUB_ALIGN) "\n"

__asm__(".section wtext,\"awx\",@progbits\n");

__asm__(ENTRY_STUB_ALIGN_DIRECTIVE
        "x86_64_entry_start:");

#define STUB_ASM_ENTRY(func)                             \
   ".globl " func "\n"                                   \
   ".type " func ", @function\n"                         \
   ENTRY_STUB_ALIGN_DIRECTIVE                            \
   func ":"

#define STUB_ASM_CODE(slot)                                 \
   "movq " ENTRY_CURRENT_TABLE "@GOTTPOFF(%rip), %rax\n\t"  \
   "movq %fs:(%rax), %r11\n\t"                              \
   "jmp *(8 * " slot ")(%r11)"

#define MAPI_TMP_STUB_ASM_GCC
#include "mapi_tmp.h"

__asm__(".text\n");

__asm__("x86_64_current_tls:\n\t"
    ENTRY_STUB_ALIGN_DIRECTIVE
	"movq " ENTRY_CURRENT_TABLE "@GOTTPOFF(%rip), %rax\n\t"
	"ret");

extern unsigned long
x86_64_current_tls();

#include <string.h>
#include "u_execmem.h"

const int entry_type = ENTRY_X86_64_TLS;
const int entry_stub_size = ENTRY_STUB_SIZE;

void entry_generate_default_code(char *entry, int slot)
{
    unsigned int *p;
    unsigned long tls_addr;
    char tmpl[] = {
        0x48, 0xc7, 0xc0, 0x0, 0x0, 0x0, 0x0, // mov 0x0,%rax
        0x64, 0x4c, 0x8b, 0x18,               // mov %fs:(%rax),%r11
        0x41, 0xff, 0xa3,                     // jmpq *0x0(%r11)
        0x00, 0x00, 0x00, 0x00,
        0x90                                  // nop
    };

    STATIC_ASSERT(sizeof(mapi_func) == 8);
    STATIC_ASSERT(ENTRY_STUB_SIZE >= sizeof(tmpl));

    assert(slot >= 0);

    tls_addr = x86_64_current_tls();

    p = (unsigned int *)&tmpl[3];
    *p = (unsigned int)tls_addr;

    p = (unsigned int *)&tmpl[14];
    *p = (unsigned int)(8 * slot);

    memcpy(entry, tmpl, sizeof(tmpl));
}

void
entry_patch_public(void)
{
}

static char
x86_64_entry_start[];

mapi_func
entry_get_public(int slot)
{
   return (mapi_func) (x86_64_entry_start + slot * ENTRY_STUB_SIZE);
}

#if !defined(STATIC_DISPATCH_ONLY)
void
entry_patch(mapi_func entry, int slot)
{
   entry_generate_default_code((char *)entry, slot);
}

mapi_func
entry_generate(int slot)
{
   void *code;

   code = u_execmem_alloc(ENTRY_STUB_SIZE);
   if (!code)
      return NULL;

   entry_generate_default_code(code, slot);

   return (mapi_func)code;
}
#endif // !defined(STATIC_DISPATCH_ONLY)
