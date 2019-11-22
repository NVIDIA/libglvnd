/*
 * Mesa 3-D graphics library
 *
 * Copyright (C) 2010 LunarG Inc.
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
 *
 * Authors:
 *    Chia-I Wu <olv@lunarg.com>
 */

#ifndef _STUB_H_
#define _STUB_H_

#include "entry.h"
#include "glapi.h"

/**
 * Frees any memory that was allocated for the stub functions.
 *
 * This should only be called when the library is unloaded.
 */
void stub_cleanup(void);

#if !defined(STATIC_DISPATCH_ONLY)

int
stub_find_public(const char *name);

int
stub_find_dynamic(const char *name, int generate);

const char *
stub_get_name(int index);

mapi_func
stub_get_addr(int index);

int stub_get_count(void);
#endif // !defined(STATIC_DISPATCH_ONLY)

/**
 * Returns the \c __GLdispatchStubPatchCallbacks struct that should be used for
 * patching the entrypoints, or \c NULL if patching is not supported.
 */
const __GLdispatchStubPatchCallbacks *stub_get_patch_callbacks(void);

#endif /* _STUB_H_ */
