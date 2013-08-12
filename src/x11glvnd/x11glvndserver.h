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

#ifndef __X11GLVNDSERVER_H__
#define __X11GLVNDSERVER_H__

#include <X11/Xdefs.h>
#include "compiler.h"

/*!
 * Public symbols exported by the x11glvnd X server module. Server-side GLX can
 * hook into these symbols in order to implement tracking of GLX drawables and
 * potentially implement active notification of clients when XID -> screen
 * mappings change (this could be done via shared memory in the direct rendering
 * case). The latter will allow clients to cache XID -> screen values, saving a
 * round trip in the common case.
 *
 * XXX: Currently there is a race between the XID -> screen lookup and potential
 * destruction of a GLX drawable and recycling of its XID. Will we need to
 * somehow lock drawables on the server to prevent them from going away until we
 * have dispatched to the vendor? Or should it be safe to dispatch even if the
 * drawable disappears?
 */

#define XGLV_X_CONFIG_OPTION_NAME "GLVendor"

PUBLIC void _XGLVRegisterGLXDrawableType(RESTYPE rtype);

#endif
