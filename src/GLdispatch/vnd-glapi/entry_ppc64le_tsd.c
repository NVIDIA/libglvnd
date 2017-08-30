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


// NOTE: These must be powers of two:
#define ENTRY_STUB_ALIGN 256
#if !defined(GLDISPATCH_PAGE_SIZE)
#define GLDISPATCH_PAGE_SIZE 65536
#endif

__asm__(".section wtext,\"ax\",@progbits\n");
__asm__(".balign " U_STRINGIFY(GLDISPATCH_PAGE_SIZE) "\n"
       ".globl public_entry_start\n"
       ".hidden public_entry_start\n"
        "public_entry_start:");

#define STUB_ASM_ENTRY(func)                            \
    ".globl " func "\n"                                 \
    ".type " func ", @function\n"                       \
    ".balign " U_STRINGIFY(ENTRY_STUB_ALIGN) "\n"     \
    func ":\n\t"                                        \
    "  addis  2, 12, .TOC.-" func "@ha\n\t"             \
    "  addi   2, 2, .TOC.-" func "@l\n\t"               \
    "  .localentry  " func ", .-" func "\n\t"

#define STUB_ASM_CODE(slot)                             \
    "  addis  11, 2, _glapi_Current@got@ha\n\t"         \
    "  ld     11, _glapi_Current@got@l(11)\n\t"         \
    "  ld     11, 0(11)\n\t"                            \
    "  cmpldi 11, 0\n\t"                                \
    "  beq    2000f\n"                                  \
    "1050:\n\t"                                         \
    "  ld     12, " slot "*8(11)\n\t"                   \
    "  mtctr  12\n\t"                                   \
    "  bctr\n"                                          \
    "2000:\n\t"                                         \
    "  mflr   0\n\t"                                    \
    "  std    0, 16(1)\n\t"                             \
    "  std    2, 40(1)\n\t"                             \
    "  stdu   1, -144(1)\n\t"                           \
    "  std    3, 56(1)\n\t"                             \
    "  std    4, 64(1)\n\t"                             \
    "  std    5, 72(1)\n\t"                             \
    "  std    6, 80(1)\n\t"                             \
    "  std    7, 88(1)\n\t"                             \
    "  std    8, 96(1)\n\t"                             \
    "  std    9, 104(1)\n\t"                            \
    "  std    10, 112(1)\n\t"                           \
    "  std    12, 128(1)\n\t"                           \
    "  addis  12, 2, _glapi_get_current@got@ha\n\t"     \
    "  ld     12, _glapi_get_current@got@l(12)\n\t"     \
    "  mtctr  12\n\t"                                   \
    "  bctrl\n\t"                                       \
    "  ld     2, 144+40(1)\n\t"                         \
    "  mr     11, 3\n\t"                                \
    "  ld     3, 56(1)\n\t"                             \
    "  ld     4, 64(1)\n\t"                             \
    "  ld     5, 72(1)\n\t"                             \
    "  ld     6, 80(1)\n\t"                             \
    "  ld     7, 88(1)\n\t"                             \
    "  ld     8, 96(1)\n\t"                             \
    "  ld     9, 104(1)\n\t"                            \
    "  ld     10, 112(1)\n\t"                           \
    "  ld     12, 128(1)\n\t"                           \
    "  addi   1, 1, 144\n\t"                            \
    "  ld     0, 16(1)\n\t"                             \
    "  mtlr   0\n\t"                                    \
    "  b      1050b\n"
    // Conceptually, this is:
    // {
    //     void **dispatchTable = _glapi_Current[GLAPI_CURRENT_DISPATCH];
    //     if (dispatchTable == NULL) {
    //         dispatchTable = _glapi_get_current();
    //     }
    //     jump_to_address(dispatchTable[slot]);
    // }
    //
    // Note that _glapi_Current is a simple global variable.
    // See the x86 or x86-64 TSD code for examples.

#define MAPI_TMP_STUB_ASM_GCC
#include "mapi_tmp.h"


__asm__(".balign " U_STRINGIFY(GLDISPATCH_PAGE_SIZE) "\n"
       ".globl public_entry_end\n"
       ".hidden public_entry_end\n"
        "public_entry_end:");
__asm__(".text\n");

const int entry_type = __GLDISPATCH_STUB_PPC64LE;
const int entry_stub_size = ENTRY_STUB_ALIGN;

static const uint32_t ENTRY_TEMPLATE[] =
{
    // This should be functionally the same code as would be generated from
    // the STUB_ASM_CODE macro, but defined as a buffer.
    // This is used to generate new dispatch stubs. libglvnd will copy this
    // data to the dispatch stub, and then it will patch the slot number and
    // any addresses that it needs to.
    // NOTE!!!  NOTE!!!  NOTE!!!
    // This representation is correct for both little- and big-endian systems.
    // However, more work needs to be done for big-endian Linux because it
    // adheres to an older, AIX-compatible ABI that uses function descriptors.
    // 1000:
    // 1000:
    0x7C0802A6,    // <ENTRY+000>:    mflr   0
    0xF8010010,    // <ENTRY+004>:    std    0, 16(1)
    0xE96C0098,    // <ENTRY+008>:    ld     11, 9000f-1000b+0(12)
    0xE96B0000,    // <ENTRY+012>:    ld     11, 0(11)
    0x282B0000,    // <ENTRY+016>:    cmpldi 11, 0
    0x41820014,    // <ENTRY+020>:    beq    2000f
    // 1050:
    0xE80C00A8,    // <ENTRY+024>:    ld     0, 9000f-1000b+16(12)
    0x7D8B002A,    // <ENTRY+028>:    ldx    12, 11, 0
    0x7D8903A6,    // <ENTRY+032>:    mtctr  12
    0x4E800420,    // <ENTRY+036>:    bctr
    // 2000:
    0xF8410028,    // <ENTRY+040>:    std    2, 40(1)
    0xF821FF71,    // <ENTRY+044>:    stdu   1, -144(1)
    0xF8610038,    // <ENTRY+048>:    std    3, 56(1)
    0xF8810040,    // <ENTRY+052>:    std    4, 64(1)
    0xF8A10048,    // <ENTRY+056>:    std    5, 72(1)
    0xF8C10050,    // <ENTRY+060>:    std    6, 80(1)
    0xF8E10058,    // <ENTRY+064>:    std    7, 88(1)
    0xF9010060,    // <ENTRY+068>:    std    8, 96(1)
    0xF9210068,    // <ENTRY+072>:    std    9, 104(1)
    0xF9410070,    // <ENTRY+076>:    std    10, 112(1)
    0xF9810080,    // <ENTRY+080>:    std    12, 128(1)
    0xE98C00A0,    // <ENTRY+084>:    ld     12, 9000f-1000b+8(12)
    0x7D8903A6,    // <ENTRY+088>:    mtctr  12
    0x4E800421,    // <ENTRY+092>:    bctrl
    0xE9410070,    // <ENTRY+096>:    ld     10, 112(1)
    0x7C6B1B78,    // <ENTRY+100>:    mr     11, 3
    0xE8610038,    // <ENTRY+104>:    ld     3, 56(1)
    0xE8810040,    // <ENTRY+108>:    ld     4, 64(1)
    0xE8A10048,    // <ENTRY+112>:    ld     5, 72(1)
    0xE8C10050,    // <ENTRY+116>:    ld     6, 80(1)
    0xE8E10058,    // <ENTRY+120>:    ld     7, 88(1)
    0xE9010060,    // <ENTRY+124>:    ld     8, 96(1)
    0xE9210068,    // <ENTRY+128>:    ld     9, 104(1)
    0xE9810080,    // <ENTRY+132>:    ld     12, 128(1)
    0x38210090,    // <ENTRY+136>:    addi   1, 1, 144
    0xE8010010,    // <ENTRY+140>:    ld     0, 16(1)
    0x7C0803A6,    // <ENTRY+144>:    mtlr   0
    0x4BFFFF84,    // <ENTRY+148>:    b      1050b
    // 9000:
    0, 0,          // <ENTRY+152>:    .quad _glapi_Current
    0, 0,          // <ENTRY+160>:    .quad _glapi_get_current
    0, 0           // <ENTRY+168>:    .quad <slot>*8
};

// These are the offsets in ENTRY_TEMPLATE of the values that we have to patch.
static const int TEMPLATE_OFFSET_CURRENT_TABLE = (sizeof(ENTRY_TEMPLATE) - 24);
static const int TEMPLATE_OFFSET_CURRENT_TABLE_GET = (sizeof(ENTRY_TEMPLATE) - 16);
static const int TEMPLATE_OFFSET_SLOT = (sizeof(ENTRY_TEMPLATE) - 8);

/*
 * These offsets are used in entry_generate_default_code
 * to patch the dispatch table index and any memory addresses in the generated
 * function.
 *
 * TEMPLATE_OFFSET_SLOT is the dispatch table index.
 *
 * TEMPLATE_OFFSET_CURRENT_TABLE is the address of the global _glapi_Current
 * variable.
 *
 * TEMPLATE_OFFSET_CURRENT_TABLE_GET is the address of the function
 * _glapi_get_current.
 */
void entry_generate_default_code(char *entry, int slot)
{
    char *writeEntry = u_execmem_get_writable(entry);
    memcpy(writeEntry, ENTRY_TEMPLATE, sizeof(ENTRY_TEMPLATE));

    *((uint32_t *) (writeEntry + TEMPLATE_OFFSET_SLOT)) = slot * sizeof(mapi_func);
    *((uintptr_t *) (writeEntry + TEMPLATE_OFFSET_CURRENT_TABLE)) = (uintptr_t) _glapi_Current;
    *((uintptr_t *) (writeEntry + TEMPLATE_OFFSET_CURRENT_TABLE_GET)) = (uintptr_t) _glapi_get_current;

    // This sequence is from the PowerISA Version 2.07B book.
    // It may be a bigger hammer than we need, but it works;
    // note that the __builtin___clear_cache intrinsic for
    // PPC does not seem to generate any code.
    __asm__ __volatile__(
                         "  dcbst 0, %0\n\t"
                         "  sync\n\t"
                         "  icbi 0, %0\n\t"
                         "  isync\n"
                         : : "r" (writeEntry)
                     );
}

