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
    char *pSawVertex3fv = (char *)incrementPtr;
    int *p;
    char tmpl[] = {
        0x8b, 0x05, 0x0, 0x0, 0x0, 0x0,  // mov 0x0(%rip), %eax
        0x83, 0xc0, 0x01,                // add $0x1, %eax
        0x89, 0x05, 0x0, 0x0, 0x0, 0x0,  // mov %eax, 0x0(%rip)
        0xc3,                            // ret
    };

    STATIC_ASSERT(sizeof(int) == 0x4);

    if (stubSize < sizeof(tmpl)) {
        return;
    }

    p = (int *)&tmpl[2];
    *p = (int)(pSawVertex3fv - (execEntry + 6));

    p = (int *)&tmpl[11];
    *p = (int)(pSawVertex3fv - (execEntry + 15));

    memcpy(writeEntry, tmpl, sizeof(tmpl));
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

    // Patch the address of the incrementPtr variable. Note that we patch
    // in an absolute address in this case. Unlike x86-64, x86 does not allow
    // PC-relative addressing for MOV instructions.
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
    char tmpl[] = {
        // ldr r0, 1f
        0x48, 0x02,
        // ldr r1, [r0]
        0x68, 0x01,
        // add r1, r1, #1
        0xf1, 0x01, 0x01, 0x01,
        // str r1, [r0]
        0x60, 0x01,
        // bx lr
        0x47, 0x70,
        // 1:
        0x00, 0x00, 0x00, 0x00,
    };

    int offsetAddr = sizeof(tmpl) - 4;

    if (stubSize < sizeof(tmpl)) {
        return;
    }

#if __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
    glvnd_byte_swap16((uint16_t *)tmpl, offsetAddr);
#endif

    *((uint32_t *)(tmpl + offsetAddr)) = (uint32_t)incrementPtr;

    memcpy(writeEntry, tmpl, sizeof(tmpl));

    __builtin___clear_cache((char *) execEntry, (char *) (execEntry + sizeof(tmpl)));
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
            return GL_TRUE;
        default:
            return GL_FALSE;
    }
}

GLboolean commonInitiatePatch(int type, int stubSize,
        DispatchPatchLookupStubOffset lookupStubOffset,
        int *incrementPtr)
{
    void *writeAddr;
    const void *execAddr;

    if (!dummyCheckPatchSupported(type, stubSize)) {
        return GL_FALSE;
    }

    if (lookupStubOffset("Vertex3fv", &writeAddr, &execAddr)) {
        switch (type) {
            case __GLDISPATCH_STUB_X86_64:
                patch_x86_64(writeAddr, execAddr, stubSize, incrementPtr);
                break;
            case __GLDISPATCH_STUB_X86:
                patch_x86(writeAddr, execAddr, stubSize, incrementPtr);
                break;
            case __GLDISPATCH_STUB_ARMV7_THUMB:
                patch_armv7_thumb(writeAddr, execAddr, stubSize, incrementPtr);
                break;
            default:
                assert(0);
        }
    }

    return GL_TRUE;
}

