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
#include <stdint.h>
#include "u_macros.h"

#define X86_64_ENTRY_SIZE 32

__asm__(".pushsection wtext,\"awx\",@progbits\n");
__asm__(".text\n"
        ".balign " U_STRINGIFY(X86_64_ENTRY_SIZE) "\n"
        "x86_64_entry_start:");

#define STUB_ASM_ENTRY(func)        \
   ".globl " func "\n"              \
   ".type " func ", @function\n"    \
   ".balign " U_STRINGIFY(X86_64_ENTRY_SIZE) "\n"                   \
   func ":"

#define STUB_ASM_CODE(slot)         \
    "nop" \

#define MAPI_TMP_STUB_ASM_GCC
#include "mapi_tmp.h"


__asm__(".balign " U_STRINGIFY(X86_64_ENTRY_SIZE) "\n"
        "x86_64_entry_end:");
__asm__(".popsection\n");

#include <string.h>
#include "u_execmem.h"

static const char x86_64_entry_start[];
static const char x86_64_entry_end[];

const int entry_type = ENTRY_X86_64_TSD;
const int entry_stub_size = X86_64_ENTRY_SIZE;

void
entry_init_public(void)
{
    assert(!"Not implemented yet");
}

void
entry_generate_default_code(char *entry, int slot)
{
    assert(!"Not implemented yet");
}

mapi_func
entry_get_public(int slot)
{
   return (mapi_func) (x86_64_entry_start + slot * X86_64_ENTRY_SIZE);
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

   code = u_execmem_alloc(X86_64_ENTRY_SIZE);
   if (!code)
      return NULL;

   entry_generate_default_code(code, slot);

   return (mapi_func)code;
}
#endif // !defined(STATIC_DISPATCH_ONLY)
