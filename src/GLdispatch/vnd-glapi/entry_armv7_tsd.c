/*
 * Copyright (c) 2015, NVIDIA CORPORATION.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and/or associated documentation files (the
 * "Materials"), to deal in the Materials without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sublicense, and/or sell copies of the Materials, and to
 * permit persons to whom the Materials are furnished to do so, subject to
 * the following conditions:
 *
 * The above copyright notice and this permission notice shall be included
 * unaltered in all copies or substantial portions of the Materials.
 * Any additions, deletions, or changes to the original source files
 * must be clearly indicated in accompanying documentation.
 *
 * If only executable code is distributed, then the accompanying
 * documentation must state that "this software is based in part on the
 * work of the Khronos Group."
 *
 * THE MATERIALS ARE PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 * IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
 * CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 * MATERIALS OR THE USE OR OTHER DEALINGS IN THE MATERIALS.
 */

#include "entry.h"
#include "entry_common.h"

#include <string.h>
#include <stdint.h>
#include <unistd.h>
#include <assert.h>

#include "glapi.h"
#include "u_macros.h"
#include "u_current.h"
#include "utils_misc.h"
#include "glvnd/GLdispatchABI.h"

/*
 * See: https://sourceware.org/binutils/docs/as/ARM-Directives.html
 */
__asm__(".syntax unified\n\t");

/*
 * u_execmem_alloc() allocates 64 bytes per stub.
 */
#define ARMV7_ENTRY_SIZE 64

/*
 * This runs in Thumb mode.
 *
 * libglvnd on armv7 is built with -march=armv7-a, which uses the AAPCS ABI
 * that has ARM/Thumb interworking enabled by default.
 *
 * See: https://gcc.gnu.org/onlinedocs/gcc/ARM-Options.html
 */
#define STUB_ASM_ENTRY(func)                        \
    ".balign " U_STRINGIFY(ARMV7_ENTRY_SIZE) "\n\t" \
    ".thumb_func\n\t"                               \
    ".global " func "\n\t"                          \
    ".type " func ", %function\n\t"                 \
    func ":\n\t"

/*
 * Looks up the current dispatch table, finds the stub address at the given slot
 * then jumps to it.
 *
 * First tries to find a dispatch table in _glapi_Current[GLAPI_CURRENT_DISPATCH],
 * if not found then it jumps to the 'lookup_dispatch' and calls
 * _glapi_get_current() then jumps back to the 'found_dispatch' label.
 *
 * The 'found_dispatch' section computes the correct offset in the dispatch
 * table then does a branch without link to the function address.
 *
 * This routine preserves the r0-r3 volatile registers as they store the
 * parameters of the entry point that is being looked up.
 */
#define STUB_ASM_CODE(slot)                 \
    "push {r0-r3}\n\t"                      \
    "ldr r0, 1f\n\t"                        \
    "ldr r0, [r0]\n\t"                      \
    "cmp r0, #0\n\t"                        \
    "it eq\n\t"                             \
    "beq 10f\n\t"                           \
    "11:\n\t"        /* found_dispatch */   \
    "ldr r1, 3f\n\t"                        \
    "mov r2, #4\n\t" /* sizeof(void *) */   \
    "mul r1, r1, r2\n\t"                    \
    "ldr ip, [r0, +r1]\n\t"                 \
    "pop {r0-r3}\n\t"                       \
    "bx ip\n\t"                             \
    "10:\n\t"        /* lookup_dispatch */  \
    "push {lr}\n\t"                         \
    "ldr r0, 2f\n\t"                        \
    "blx r0\n\t"                            \
    "pop {lr}\n\t"                          \
    "b 11b\n\t"                             \
    "1:\n\t"                                \
    ".word _glapi_Current\n\t"              \
    "2:\n\t"                                \
    ".word _glapi_get_current\n\t"         \
    "3:\n\t"                                \
    ".word " slot "\n\t"

/*
 * Bytecode for STUB_ASM_CODE()
 */
static uint16_t BYTECODE_TEMPLATE[] =
{
    0xb40f,
    0xf8df, 0x0028,
    0x6800,
    0x2800,
    0xbf08,
    0xe008,
    0x4909,
    0xf04f, 0x0204,
    0xfb01, 0xf102,
    0xf850, 0xc001,
    0xbc0f,
    0x4760,
    0xb500,
    0x4803,
    0x4780,
    0xf85d, 0xeb04,
    0xe7f0,

    // Offsets that need to be patched
    0x0000, 0x0000,
    0x0000, 0x0000,
    0x0000, 0x0000,
};

#define ARMV7_BYTECODE_SIZE sizeof(BYTECODE_TEMPLATE)

__asm__(".section wtext,\"ax\"\n"
        ".balign 4096\n"
       ".globl public_entry_start\n"
       ".hidden public_entry_start\n"
        "public_entry_start:\n");

#define MAPI_TMP_STUB_ASM_GCC
#include "mapi_tmp.h"

__asm__(".balign 4096\n"
       ".globl public_entry_end\n"
       ".hidden public_entry_end\n"
        "public_entry_end:\n"
        ".text\n\t");

/*
 * If built with -marm, let the assembler know that we are done with Thumb
 */
#if !defined(__thumb__)
__asm__(".arm\n\t");
#endif

const int entry_type = __GLDISPATCH_STUB_ARMV7_THUMB;
const int entry_stub_size = ARMV7_ENTRY_SIZE;

static const int TEMPLATE_OFFSET_CURRENT_TABLE     = ARMV7_BYTECODE_SIZE - 3*4;
static const int TEMPLATE_OFFSET_CURRENT_TABLE_GET = ARMV7_BYTECODE_SIZE - 2*4;
static const int TEMPLATE_OFFSET_SLOT              = ARMV7_BYTECODE_SIZE - 4;

void
entry_init_public(void)
{
    STATIC_ASSERT(ARMV7_BYTECODE_SIZE <= ARMV7_ENTRY_SIZE);
}

void entry_generate_default_code(char *entry, int slot)
{
    char *writeEntry;

    // Make sure the base address has the Thumb mode bit
    assert((uintptr_t)entry & (uintptr_t)0x1);

    // Get the pointer to the writable mapping.
    writeEntry = (char *) u_execmem_get_writable(entry - 1);

    memcpy(writeEntry, BYTECODE_TEMPLATE, ARMV7_BYTECODE_SIZE);

    *((uint32_t *)(writeEntry + TEMPLATE_OFFSET_SLOT)) = slot;
    *((uint32_t *)(writeEntry + TEMPLATE_OFFSET_CURRENT_TABLE)) =
        (uint32_t)_glapi_Current;
    *((uint32_t *)(writeEntry + TEMPLATE_OFFSET_CURRENT_TABLE_GET)) =
        (uint32_t)_glapi_get_current;

    // See http://community.arm.com/groups/processors/blog/2010/02/17/caches-and-self-modifying-code
    __builtin___clear_cache(writeEntry, writeEntry + ARMV7_BYTECODE_SIZE);
}

// Note: The rest of these functions could also be used for ARMv7 TLS stubs,
// once those are implemented.

mapi_func
entry_get_public(int index)
{
    // Add 1 to the base address to force Thumb mode when jumping to the stub
    return (mapi_func)(public_entry_start + (index * entry_stub_size) + 1);
}

void entry_get_patch_addresses(mapi_func entry, void **writePtr, const void **execPtr)
{
    // Get the actual beginning of the stub allocation
    void *entryBase = (void *) (((uintptr_t) entry) - 1);
    *execPtr = (const void *) entryBase;
    *writePtr = u_execmem_get_writable(entryBase);
}

#if !defined(STATIC_DISPATCH_ONLY)
mapi_func entry_generate(int slot)
{
    void *code = u_execmem_alloc(entry_stub_size);
    if (!code) {
        return NULL;
    }

    // Add 1 to the base address to force Thumb mode when jumping to the stub
    code = (void *)((char *)code + 1);

    entry_generate_default_code(code, slot);

    return (mapi_func) code;
}
#endif // !defined(STATIC_DISPATCH_ONLY)
