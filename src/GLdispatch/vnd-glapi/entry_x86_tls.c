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

#define ENTRY_STUB_ALIGN 16
#if !defined(GLDISPATCH_PAGE_SIZE)
#define GLDISPATCH_PAGE_SIZE 4096
#endif

__asm__(".section wtext,\"ax\",@progbits\n");
__asm__(".balign " U_STRINGIFY(GLDISPATCH_PAGE_SIZE) "\n"
       ".globl public_entry_start\n"
       ".hidden public_entry_start\n"
        "public_entry_start:");

#define STUB_ASM_ENTRY(func)     \
    ".globl " func "\n"           \
    ".type " func ", @function\n" \
    ".balign " U_STRINGIFY(ENTRY_STUB_ALIGN) "\n" \
    func ":\n"

#define STUB_ASM_CODE(slot)      \
    "call x86_current_tls\n\t"    \
    "movl %gs:(%eax), %eax\n\t"   \
    "jmp *(4 * " slot ")(%eax)"

#define MAPI_TMP_STUB_ASM_GCC
#include "mapi_tmp.h"

__asm__(".balign " U_STRINGIFY(GLDISPATCH_PAGE_SIZE) "\n"
       ".globl public_entry_end\n"
       ".hidden public_entry_end\n"
        "public_entry_end:");

__asm__(".text\n");

__asm__("x86_current_tls:\n\t"
    ".balign " U_STRINGIFY(ENTRY_STUB_ALIGN) "\n"
    "call 1f\n"
        "1:\n\t"
        "popl %eax\n\t"
    "addl $_GLOBAL_OFFSET_TABLE_+[.-1b], %eax\n\t"
    "movl _glapi_tls_Current@GOTNTPOFF(%eax), %eax\n\t"
    "ret");

extern uint32_t
x86_current_tls();

const int entry_type = __GLDISPATCH_STUB_X86;
const int entry_stub_size = ENTRY_STUB_ALIGN;

static const unsigned char ENTRY_TEMPLATE[] =
{
    0x65, 0xa1, 0x00, 0x00, 0x00, 0x00, /* movl %gs:0x0, %eax */
    0xff, 0xa0, 0x34, 0x12, 0x00, 0x00, /* jmp *0x1234(%eax) */
};
static const int TEMPLATE_OFFSET_TLS_OFFSET = 2;
static const int TEMPLATE_OFFSET_SLOT = 8;

void entry_generate_default_code(char *entry, int slot)
{
    char *writeEntry = u_execmem_get_writable(entry);

    memcpy(writeEntry, ENTRY_TEMPLATE, sizeof(ENTRY_TEMPLATE));
    *((uint32_t *) (writeEntry + TEMPLATE_OFFSET_TLS_OFFSET)) = x86_current_tls();
    *((uint32_t *) (writeEntry + TEMPLATE_OFFSET_SLOT)) = (uint32_t) (slot * sizeof(mapi_func));
}

