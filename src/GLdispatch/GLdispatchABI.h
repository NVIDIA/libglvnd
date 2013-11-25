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

#include <GL/gl.h>

#if !defined(__GL_DISPATCH_ABI_H)
#define __GL_DISPATCH_ABI_H

#if defined(__cplusplus)
extern "C" {
#endif

/*!
 * \defgroup gldispatchabi GL dispatching ABI
 *
 * This is not a complete ABI, but rather a fragment common to the libEGL and
 * libGLX ABIs.  Changes to this file should be accompanied by a version bump to
 * these client ABIs.
 */

/*!
 * This opaque structure describes the core GL dispatch table.
 */
typedef struct __GLdispatchTableRec __GLdispatchTable;

typedef void (*__GLdispatchProc)(void);

typedef void *(*__GLgetProcAddressCallback)(const GLubyte *procName,
                                            int isClientAPI);

#if defined(__cplusplus)
}
#endif

#endif // __GL_DISPATCH_ABI_H
