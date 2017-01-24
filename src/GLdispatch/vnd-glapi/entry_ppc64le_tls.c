/*
 * Copyright (C) 2010 LunarG Inc.
 * Copyright (c) 2017, NVIDIA CORPORATION.
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


// TODO: Change this macro to be the size of the dispatch stubs.
#define PPC64LE_ENTRY_SIZE 32

__asm__(".section wtext,\"ax\",@progbits\n");
__asm__(".balign 4096\n"
       ".globl public_entry_start\n"
       ".hidden public_entry_start\n"
        "public_entry_start:");

#define STUB_ASM_ENTRY(func)        \
   ".globl " func "\n"              \
   ".type " func ", @function\n"    \
   ".balign " U_STRINGIFY(PPC64LE_ENTRY_SIZE) "\n"                   \
   func ":"

#define STUB_ASM_CODE(slot) \
    "nop"
    // TODO: Fill in this assembly code
    // Conceptually, this is:
    // {
    //     void **dispatchTable = _glapi_tls_Current;
    //     jump_to_address(dispatchTable[slot];
    // }
    //
    // Note that _glapi_tls_Current is a global variable declared with
    // __thread.
    // See the x86-64 TLS code for an example.

#define MAPI_TMP_STUB_ASM_GCC
#include "mapi_tmp.h"


__asm__(".balign 4096\n"
       ".globl public_entry_end\n"
       ".hidden public_entry_end\n"
        "public_entry_end:");
__asm__(".text\n");

const int entry_type = __GLDISPATCH_STUB_PPC64LE;
const int entry_stub_size = PPC64LE_ENTRY_SIZE;

static const unsigned char ENTRY_TEMPLATE[] =
{
    // TODO: Fill in the assembly code here as well. This should be
    // functionally the same code as would be generated from the STUB_ASM_CODE
    // macro, but defined as a buffer.
    // This is used to generate new dispatch stubs. libglvnd will copy this
    // data to the dispatch stub, and then it will patch the slot number and
    // any addresses that it needs to.
};

// These are the offsets in ENTRY_TEMPLATE of the values that we have to patch.
static const int TEMPLATE_OFFSET_SLOT = 0;

/*
 * TODO: Fill in these offsets. These are used in entry_generate_default_code
 * to patch the dispatch table index and any memory addresses in the generated
 * function.
 *
 * TEMPLATE_OFFSET_SLOT is the dispatch table index.
 */
void entry_generate_default_code(char *entry, int slot)
{
    char *writeEntry = u_execmem_get_writable(entry);
    memcpy(writeEntry, ENTRY_TEMPLATE, sizeof(ENTRY_TEMPLATE));

    // TODO: Patch the dispatch table slot.
    *((uint32_t *) (writeEntry + TEMPLATE_OFFSET_SLOT)) = slot * sizeof(mapi_func);

    // TODO: Patch any addresses necessary to look up the _glapi_tls_Current
    // variable from TLS.

    // TODO: Do any cache clears or anything else that is necessary on PPC64LE
    // to make self-modifying code work.
}

