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

#ifndef __LIBGLX_GL_DISPATCH_H__
#define __LIBGLX_GL_DISPATCH_H__

#include "libglxabi.h"

/*
 * These functions define the interface by which a vendor library can install
 * and manage its own collection of dispatch tables. See libglxabi.h for
 * a more detailed explanation of these functions.
 */

__GLXcoreDispatchTable *__glXGetCurrentGLDispatch(void);
__GLXcoreDispatchTable *__glXGetTopLevelDispatch(void);
__GLXcoreDispatchTable *__glXCreateGLDispatch(const __GLXvendorCallbacks *cb,
                                              void *data);
GLint __glXGetGLDispatchOffset(const GLubyte *procName);
void __glXSetGLDispatchEntry(__GLXcoreDispatchTable *table,
                             GLint offset,
                             __GLXextFuncPtr addr);
void __glXMakeGLDispatchCurrent(__GLXcoreDispatchTable *table);
GLboolean __glXDestroyGLDispatch(__GLXcoreDispatchTable *table);

#endif // __LIBGLX_GL_DISPATCH_H__
