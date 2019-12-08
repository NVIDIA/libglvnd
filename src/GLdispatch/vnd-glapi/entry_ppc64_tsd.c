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

#if !defined(_CALL_ELF) || (_CALL_ELF == 1)
#error "ELFv1 ABI is not supported"
#endif

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
    "  addis  11, 2, _glapi_Current@got@ha\n" \
    "  ld     11, _glapi_Current@got@l(11)\n" \
    "  ld     11, 0(11)\n" \
    "  cmpldi 11, 0\n" \
    "  bne    1000f\n" \
    "  mflr   0\n" \
    "  std    0, 16(1)\n" \
    "  stdu   1, -120(1)\n" \
    "  std    3, 56(1)\n" \
    "  std    4, 64(1)\n" \
    "  std    5, 72(1)\n" \
    "  std    6, 80(1)\n" \
    "  std    7, 88(1)\n" \
    "  std    8, 96(1)\n" \
    "  std    9, 104(1)\n" \
    "  std    10, 112(1)\n" \
    "  bl _glapi_get_current\n" \
    "  nop\n" \
    "  mr     11, 3\n" \
    "  ld     3, 56(1)\n" \
    "  ld     4, 64(1)\n" \
    "  ld     5, 72(1)\n" \
    "  ld     6, 80(1)\n" \
    "  ld     7, 88(1)\n" \
    "  ld     8, 96(1)\n" \
    "  ld     9, 104(1)\n" \
    "  ld     10, 112(1)\n" \
    "  addi   1, 1, 120\n" \
    "  ld     0, 16(1)\n" \
    "  mtlr   0\n" \
    "1000:\n" \
    "  addis  11, 11, (" slot "*8)@ha\n" \
    "  ld     12, (" slot "*8)@l (11)\n" \
    "  mtctr  12\n" \
    "  bctr\n" \

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

const int entry_type = __GLDISPATCH_STUB_PPC64;
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
                //              1000:
    0x7c0802a6, // <ENTRY+000>: mflr   0
    0xf8010010, // <ENTRY+004>: std    0, 16(1)
    0xe96c009c, // <ENTRY+008>: ld     11, 9000f-1000b+0(12)
    0xe96b0000, // <ENTRY+012>: ld     11, 0(11)
    0x282b0000, // <ENTRY+016>: cmpldi 11, 0
    0x41820014, // <ENTRY+020>: beq    2000f
                //              1050:
    0xe80c00ac, // <ENTRY+024>: ld     0, 9000f-1000b+16(12)
    0x7d8b002a, // <ENTRY+028>: ldx    12, 11, 0
    0x7d8903a6, // <ENTRY+032>: mtctr  12
    0x4e800420, // <ENTRY+036>: bctr
                //              2000:
    0xf821ff71, // <ENTRY+040>: stdu   1, -144(1)
    0xf8410018, // <ENTRY+044>: std    2, 24(1)
    0xf8610038, // <ENTRY+048>: std    3, 56(1)
    0xf8810040, // <ENTRY+052>: std    4, 64(1)
    0xf8a10048, // <ENTRY+056>: std    5, 72(1)
    0xf8c10050, // <ENTRY+060>: std    6, 80(1)
    0xf8e10058, // <ENTRY+064>: std    7, 88(1)
    0xf9010060, // <ENTRY+068>: std    8, 96(1)
    0xf9210068, // <ENTRY+072>: std    9, 104(1)
    0xf9410070, // <ENTRY+076>: std    10, 112(1)
    0xf9810080, // <ENTRY+080>: std    12, 128(1)
    0xe98c00a4, // <ENTRY+084>: ld     12, 9000f-1000b+8(12)
    0x7d8903a6, // <ENTRY+088>: mtctr  12
    0x4e800421, // <ENTRY+092>: bctrl
    0xe8410018, // <ENTRY+096>: ld     2, 24(1)
    0xe9410070, // <ENTRY+100>: ld     10, 112(1)
    0x7c6b1b78, // <ENTRY+104>: mr     11, 3
    0xe8610038, // <ENTRY+108>: ld     3, 56(1)
    0xe8810040, // <ENTRY+112>: ld     4, 64(1)
    0xe8a10048, // <ENTRY+116>: ld     5, 72(1)
    0xe8c10050, // <ENTRY+120>: ld     6, 80(1)
    0xe8e10058, // <ENTRY+124>: ld     7, 88(1)
    0xe9010060, // <ENTRY+128>: ld     8, 96(1)
    0xe9210068, // <ENTRY+132>: ld     9, 104(1)
    0xe9810080, // <ENTRY+136>: ld     12, 128(1)
    0x38210090, // <ENTRY+140>: addi   1, 1, 144
    0xe8010010, // <ENTRY+144>: ld     0, 16(1)
    0x7c0803a6, // <ENTRY+148>: mtlr   0
    0x4bffff80, // <ENTRY+152>: b      1050b
                //              9000:
    0, 0,       // <ENTRY+156>: .quad _glapi_Current
    0, 0,       // <ENTRY+164>: .quad _glapi_get_current
    0, 0,       // <ENTRY+172>: .quad <slot>*8
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
void entry_generate_default_code(int index, int slot)
{
    char *entry = (char *) (public_entry_start + (index * entry_stub_size));
    memcpy(entry, ENTRY_TEMPLATE, sizeof(ENTRY_TEMPLATE));

    *((uint32_t *) (entry + TEMPLATE_OFFSET_SLOT)) = slot * sizeof(mapi_func);
    *((uintptr_t *) (entry + TEMPLATE_OFFSET_CURRENT_TABLE)) = (uintptr_t) _glapi_Current;
    *((uintptr_t *) (entry + TEMPLATE_OFFSET_CURRENT_TABLE_GET)) = (uintptr_t) _glapi_get_current;

    // This sequence is from the PowerISA Version 2.07B book.
    // It may be a bigger hammer than we need, but it works;
    // note that the __builtin___clear_cache intrinsic for
    // PPC does not seem to generate any code.
    __asm__ __volatile__(
                         "  dcbst 0, %0\n\t"
                         "  sync\n\t"
                         "  icbi 0, %0\n\t"
                         "  isync\n"
                         : : "r" (entry)
                     );
}

