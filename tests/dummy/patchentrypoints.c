/*
 * Copyright (c) 2016, NVIDIA CORPORATION.
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

#include "patchentrypoints.h"

#include <string.h>
#include <assert.h>

#include "compiler.h"
#include "utils_misc.h"

static void patch_x86_64(char *writeEntry, const char *execEntry,
        int stubSize, void *incrementPtr)
{
#if defined(__x86_64__)
    // On an x32 build, pointers are 32 bits, but the code that we generate
    // here uses a 64-bit address. Cast incrementPtr to a 64-bit integer so
    // that it's the right size for either build.
    uint64_t incrementAddr = (uint64_t) ((uintptr_t) incrementPtr);
    const char tmpl[] = {
        0xa1, 0xf0, 0xde, 0xbc, 0x9a, 0x78, 0x56, 0x34, 0x12, // movabs 0x123456789abcdef0, %eax
        0x83, 0xc0, 0x01,                                     // add    $0x1,%eax
        0xa3, 0xf0, 0xde, 0xbc, 0x9a, 0x78, 0x56, 0x34, 0x12, // movabs %eax,0x123456789abcdef0
        0xc3,                                                 // ret
    };

    if (stubSize < sizeof(tmpl)) {
        return;
    }

    memcpy(writeEntry, tmpl, sizeof(tmpl));
    memcpy(writeEntry + 1, &incrementAddr, sizeof(incrementAddr));
    memcpy(writeEntry + 13, &incrementAddr, sizeof(incrementAddr));

#else
    assert(0); // Should not be calling this
#endif
}


static void patch_x86(char *writeEntry, const char *execEntry,
        int stubSize, void *incrementPtr)
{
#if defined(__i386__)
    uintptr_t *p;
    char tmpl[] = {
        0xa1, 0x0, 0x0, 0x0, 0x0,   // mov 0x0, %eax
        0x83, 0xc0, 0x01,           // add $0x1, %eax
        0xa3, 0x0, 0x0, 0x0, 0x0,   // mov %eax, 0x0
        0xc3                        // ret
    };

    STATIC_ASSERT(sizeof(int) == 0x4);

    if (stubSize < sizeof(tmpl)) {
        return;
    }

    // Patch the address of the incrementPtr variable.
    p = (uintptr_t *)&tmpl[1];
    *p = (uintptr_t) incrementPtr;

    p = (uintptr_t *)&tmpl[9];
    *p = (uintptr_t) incrementPtr;

    memcpy(writeEntry, tmpl, sizeof(tmpl));

    // Jump to an intermediate location
    __asm__(
        "\tjmp 0f\n"
        "\t0:\n"
    );
#else
    assert(0); // Should not be calling this
#endif
}

static void patch_armv7_thumb(char *writeEntry, const char *execEntry,
        int stubSize, void *incrementPtr)
{
#if defined(__arm__)
    // Thumb bytecode
    const uint16_t tmpl[] = {
        0x4802,         // ldr r0, 1f
        0x6801,         // ldr r1, [r0]
        0xf101, 0x0101, // add r1, r1, #1
        0x6001,         // str r1, [r0]
        0x4770,         // bx lr
        // 1:
        0x0000, 0x0000,
    };

    static int offsetAddr = sizeof(tmpl) - 4;
    if (stubSize < sizeof(tmpl)) {
        return;
    }

    memcpy(writeEntry, tmpl, sizeof(tmpl));
    *((uint32_t *)(writeEntry + offsetAddr)) = (uint32_t)incrementPtr;

    __builtin___clear_cache((char *) execEntry, (char *) (execEntry + sizeof(tmpl)));
#else
    assert(0); // Should not be calling this
#endif
}

static void patch_aarch64(char *writeEntry, const char *execEntry,
        int stubSize, void *incrementPtr)
{
#if defined(__aarch64__)
    const uint32_t tmpl[] = {
        // ldr x0, 1f
        0x580000a0,
        // ldr x1, [x0]
        0xf9400001,
        // add x1, x1, #1
        0x91000421,
        // str x1, [x0]
        0xf9000001,
        // br x30
        0xd61f03c0,
        // 1:
        0x00000000, 0x00000000,
    };

    static const int offsetAddr = sizeof(tmpl) - 8;

    if (stubSize < sizeof(tmpl)) {
        return;
    }

    memcpy(writeEntry, tmpl, sizeof(tmpl));
    *((uint64_t *)(writeEntry + offsetAddr)) = (uint64_t) incrementPtr;

    __builtin___clear_cache((char *) execEntry, (char *) (execEntry + sizeof(tmpl)));
#else
    assert(0); // Should not be calling this
#endif
}

static void patch_ppc64le(char *writeEntry, const char *execEntry,
        int stubSize, void *incrementPtr)
{
#if defined(__PPC64__)
    const unsigned int tmpl[] = {
        // NOTE!!!  NOTE!!!  NOTE!!!
        // This representation is correct for both little- and big-endian systems.
        // However, more work needs to be done for big-endian Linux because it
        // adheres to an older, AIX-compatible ABI that uses function descriptors.
        // 1000:
        0x7D2903A6,     //  mtctr 9
        0xE96C0020,     //  ld    11, 9000f-1000b(12)
        0xE92B0000,     //  ld    9, 0(11)
        0x39290001,     //  addi  9, 9, 1
        0xF92B0000,     //  std   9, 0(11)
        0x7D2902A6,     //  mfctr 9
        0x4E800020,     //  blr
        0x60000000,     //  nop
        // 9000:
        0, 0
    };

    static const int offsetAddr = sizeof(tmpl) - 8;

    if (stubSize < sizeof(tmpl)) {
        return;
    }

    memcpy(writeEntry, tmpl, sizeof(tmpl));
    memcpy(writeEntry + offsetAddr, &incrementPtr, sizeof(incrementPtr));

    // This sequence is from the PowerISA Version 2.07B book.
    // It may be a bigger hammer than we need, but it works;
    // note that the __builtin___clear_cache intrinsic for
    // PPC does not seem to generate any code:
    __asm__ __volatile__(
                         "  dcbst 0, %0\n\t"
                         "  sync\n\t"
                         "  icbi 0, %0\n\t"
                         "  isync\n"
                         : : "r" (writeEntry)
                     );
#else
    assert(0); // Should not be calling this
#endif
}


GLboolean dummyCheckPatchSupported(int type, int stubSize)
{
    switch (type) {
        case __GLDISPATCH_STUB_X86_64:
        case __GLDISPATCH_STUB_X86:
        case __GLDISPATCH_STUB_ARMV7_THUMB:
        case __GLDISPATCH_STUB_AARCH64:
        case __GLDISPATCH_STUB_X32:
        case __GLDISPATCH_STUB_PPC64LE:
            return GL_TRUE;
        default:
            return GL_FALSE;
    }
}

GLboolean dummyPatchFunction(int type, int stubSize,
        DispatchPatchLookupStubOffset lookupStubOffset,
        const char *name, int *incrementPtr)
{
    void *writeAddr;
    const void *execAddr;

    if (!dummyCheckPatchSupported(type, stubSize)) {
        return GL_FALSE;
    }

    if (lookupStubOffset(name, &writeAddr, &execAddr)) {
        switch (type) {
            case __GLDISPATCH_STUB_X86_64:
            case __GLDISPATCH_STUB_X32:
                patch_x86_64(writeAddr, execAddr, stubSize, incrementPtr);
                break;
            case __GLDISPATCH_STUB_X86:
                patch_x86(writeAddr, execAddr, stubSize, incrementPtr);
                break;
            case __GLDISPATCH_STUB_ARMV7_THUMB:
                patch_armv7_thumb(writeAddr, execAddr, stubSize, incrementPtr);
                break;
            case __GLDISPATCH_STUB_AARCH64:
                patch_aarch64(writeAddr, execAddr, stubSize, incrementPtr);
                break;
            case __GLDISPATCH_STUB_PPC64LE:
                patch_ppc64le(writeAddr, execAddr, stubSize, incrementPtr);
                break;
            default:
                assert(0);
        }
    }

    return GL_TRUE;
}

