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

#include <string.h>
#include <assert.h>
#include "u_macros.h"
#include "utils_misc.h"

#define ENTRY_STUB_ALIGN 32
#define ENTRY_STUB_SIZE ENTRY_STUB_ALIGN
#define ENTRY_STUB_ALIGN_DIRECTIVE ".balign " U_STRINGIFY(ENTRY_STUB_ALIGN) "\n"

__asm__(".section wtext, \"awx\", @progbits");

__asm__(ENTRY_STUB_ALIGN_DIRECTIVE
        "x86_entry_start:");

#define STUB_ASM_ENTRY(func)     \
   ".globl " func "\n"           \
   ".type " func ", @function\n" \
   ENTRY_STUB_ALIGN_DIRECTIVE    \
   func ":"

#define STUB_ASM_CODE(slot)      \
   "call x86_current_tls\n\t"    \
   "movl %gs:(%eax), %eax\n\t"   \
   "jmp *(4 * " slot ")(%eax)"

#define MAPI_TMP_STUB_ASM_GCC
#include "mapi_tmp.h"

__asm__(".text\n");

__asm__("x86_current_tls:\n\t"
    ENTRY_STUB_ALIGN_DIRECTIVE
    "call 1f\n"
        "1:\n\t"
        "popl %eax\n\t"
    "addl $_GLOBAL_OFFSET_TABLE_+[.-1b], %eax\n\t"
    "movl " ENTRY_CURRENT_TABLE "@GOTNTPOFF(%eax), %eax\n\t"
    "ret");


#include "u_execmem.h"

extern unsigned long
x86_current_tls();

static char x86_entry_start[];

const int entry_type = ENTRY_X86_TLS;
const int entry_stub_size = ENTRY_STUB_SIZE;

void entry_generate_default_code(char *entry, int slot)
{
    unsigned int *p;
    unsigned long tls_addr;
    char tmpl[] = {
        0x65, 0xa1, 0x0, 0x0, 0x0, 0x0, // movl %gs:0x0,%eax
        0xff, 0x20,                     // jmp *(%eax)
        0x90, 0x90, 0x90, 0x90,         // nop's
        0x90
    };

    STATIC_ASSERT(sizeof(mapi_func) == 4);
    STATIC_ASSERT(ENTRY_STUB_SIZE >= sizeof(tmpl));

    tls_addr = x86_current_tls();

    p = (unsigned int *)&tmpl[2];
    *p = (unsigned int)tls_addr;

    p = (unsigned int *)&tmpl[8];
    *p = (unsigned int)(4 * slot);

    memcpy(entry, tmpl, sizeof(tmpl));
}

void
entry_patch_public(void)
{
    int slot;

    // Patch the stubs with a more optimal code sequence
    for (slot = 0; slot < MAPI_TABLE_NUM_STATIC; i++)
       entry_generate_default_code(entry, slot);
}

mapi_func
entry_get_public(int slot)
{
   return (mapi_func) (x86_entry_start + slot * 16);
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

   return entry;
}
#endif // !defined(STATIC_DISPATCH_ONLY)

