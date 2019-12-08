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

#include "glvnd_genentry.h"
#include "utils_misc.h"

#include <string.h>
#include <stdint.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/mman.h>
#include <assert.h>

#if defined(USE_DISPATCH_ASM)

#define _U_STRINGIFY(x) #x
#define U_STRINGIFY(x) _U_STRINGIFY(x)

#define GLX_STUBS_COUNT
#include "g_glx_dispatch_stub_list.h"

static GLVNDentrypointStub entrypointFunctions[GENERATED_ENTRYPOINT_MAX];
static char *entrypointNames[GENERATED_ENTRYPOINT_MAX] = {};
static int entrypointCount = 0;

extern char glx_entrypoint_start[];
extern char glx_entrypoint_end[];

#if defined(USE_X86_ASM)

#define STUB_SIZE 32
#define STUB_ASM_ARCH(slot) \
    "push %ebx\n" \
    "call 1f\n" \
    "1:\n" \
    "popl %ebx\n" \
    "addl $_GLOBAL_OFFSET_TABLE_+[.-1b], %ebx\n" \
    "movl entrypointFunctions@GOT(%ebx), %eax\n" \
    "pop %ebx\n" \
    "jmp *(4 * " slot ")(%eax)\n"

#elif defined(USE_X86_64_ASM)

#define STUB_SIZE 16
#define STUB_ASM_ARCH(slot) \
    "movq entrypointFunctions@GOTPCREL(%rip), %rax\n\t" \
    "jmp *(8 * " slot ")(%rax)\n"

#elif defined(USE_ARMV7_ASM)

#define STUB_SIZE 64
#define STUB_ASM_ARCH(slot) \
    "ldr ip, 1f\n" \
    "12:\n" \
    "add ip, pc\n" \
    "push { r0 }\n" \
    "ldr r0, 1f+4\n" \
    "add ip, r0\n" \
    "pop { r0 }\n" \
    "ldr ip, [ip]\n" \
    "bx ip\n" \
    "1:\n" \
    ".word entrypointFunctions - (12b + 8)\n" \
    ".word " slot " * 4\n"

#elif defined(USE_AARCH64_ASM)

#define STUB_SIZE 16
#define STUB_ASM_ARCH(slot) \
    "adrp x16, entrypointFunctions + " slot "*8\n" \
    "ldr x16, [x16, #:lo12:(entrypointFunctions + " slot "*8)]\n" \
    "br x16\n"

#elif defined(USE_PPC64_ASM) && defined(_CALL_ELF) && (_CALL_ELF == 2)

#define STUB_SIZE 32
#define STUB_ASM_ARCH(slot) \
    "0:\n" \
    "addis 2,12,.TOC.-0b@ha\n" \
    "addi 2,2,.TOC.-0b@l\n" \
    "addis  11, 2, entrypointFunctions@got@ha\n" \
    "ld     11, entrypointFunctions@got@l(11)\n" \
    "ld 12, (" slot " * 8)(11)\n" \
    "mtctr 12\n" \
    "bctr\n"

#else
#error "Can't happen -- not implemented"
#endif

#define STUB_ASM(slot) \
    ".globl glx_entrypoint_stub_" slot "\n" \
    ".hidden glx_entrypoint_stub_" slot "\n" \
    ".balign " U_STRINGIFY(STUB_SIZE) "\n" \
    "glx_entrypoint_stub_" slot ":\n" \
    STUB_ASM_ARCH(slot)

__asm__(".globl glx_entrypoint_start\n"
        ".hidden glx_entrypoint_start\n"
        ".balign " U_STRINGIFY(STUB_SIZE) "\n" \
        "glx_entrypoint_start:\n"

#define GLX_STUBS_ASM
#include "g_glx_dispatch_stub_list.h"

        ".globl glx_entrypoint_end\n"
        ".hidden glx_entrypoint_end\n"
        ".balign " U_STRINGIFY(STUB_SIZE) "\n" \
        "glx_entrypoint_end:\n"
);

static void *DefaultDispatchFunc(void)
{
    // Print a warning message?
    return NULL;
}

static GLVNDentrypointStub GetEntrypointStub(int index)
{
    return (GLVNDentrypointStub) (glx_entrypoint_start + (index * STUB_SIZE));
}

GLVNDentrypointStub glvndGenerateEntrypoint(const char *procName)
{
    int i;

    for (i=0; i<entrypointCount; i++) {
        if (strcmp(procName, entrypointNames[i]) == 0) {
            // We already generated this function, so return it.
            return GetEntrypointStub(i);
        }
    }

    if (entrypointCount >= GENERATED_ENTRYPOINT_MAX) {
        return NULL;
    }

    entrypointNames[entrypointCount] = strdup(procName);
    if (entrypointNames[entrypointCount] == NULL) {
        return NULL;
    }

    entrypointFunctions[entrypointCount] = (GLVNDentrypointStub) DefaultDispatchFunc;
    entrypointCount++;
    return GetEntrypointStub(entrypointCount - 1);
}

void glvndFreeEntrypoints(void)
{
    int i;
    for (i=0; i<entrypointCount; i++) {
        free(entrypointNames[i]);
        entrypointNames[i] = NULL;
        entrypointFunctions[i] = NULL;
    }
    entrypointCount = 0;
}

void glvndUpdateEntrypoints(GLVNDentrypointUpdateCallback callback, void *param)
{
    int i;
    for (i=0; i<entrypointCount; i++) {
        if (entrypointFunctions[i] == (GLVNDentrypointStub) DefaultDispatchFunc) {
            GLVNDentrypointStub addr = callback(entrypointNames[i], param);
            if (addr != NULL) {
                entrypointFunctions[i] = addr;
            }
        }
    }
}

#else // defined(USE_DISPATCH_ASM)

GLVNDentrypointStub glvndGenerateEntrypoint(const char *procName)
{
    return NULL;
}

void glvndFreeEntrypoints(void)
{
}

void glvndUpdateEntrypoints(GLVNDentrypointUpdateCallback callback, void *param)
{
}

#endif // defined(USE_DISPATCH_ASM)
