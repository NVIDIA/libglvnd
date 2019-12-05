/*
 * Copyright (c) 2013, NVIDIA CORPORATION.
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

#include <dlfcn.h>
#include <GL/gl.h>
#include "compiler.h"
#include "entry.h"
#include "stub.h"
#include "GLdispatch.h"

static int patchStubId = -1;

// Initialize OpenGL imports
#if defined(USE_ATTRIBUTE_CONSTRUCTOR)
void __attribute__((constructor)) __libGLInit(void)
#else
void _init(void)
#endif
{
    __glDispatchInit();

    // Register these entrypoints with GLdispatch so they can be
    // overwritten at runtime
    patchStubId = __glDispatchRegisterStubCallbacks(stub_get_patch_callbacks());
}

#if defined(USE_ATTRIBUTE_CONSTRUCTOR)
void __attribute__((destructor)) __libOpenGLFini(void)
#else
void _fini(void)
#endif
{
    // Unregister the GLdispatch entrypoints
    stub_cleanup();
    __glDispatchUnregisterStubCallbacks(patchStubId);
    __glDispatchFini();
}
