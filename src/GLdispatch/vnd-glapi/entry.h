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

#ifndef _ENTRY_H_
#define _ENTRY_H_

#include "u_compiler.h"

typedef void (*mapi_func)(void);

extern const int entry_type;
extern const int entry_stub_size;

/**
 * Returns the address of an entrypoint.
 *
 * Note that \p index is the index into the array of public stubs, not the slot
 * in the dispatch table. The public stub array may be different depending on
 * which library is being built. For example, the array in libOpenGL.so is a
 * subset of the array in libGLdispatch.so.
 *
 * \param index The index into the public stub table.
 * \return A pointer to the function, suitable to hand back from
 * glX/eglGetProcAddress.
 */
mapi_func
entry_get_public(int index);

/**
 * Saves and returns a copy of all of the entrypoints.
 */
void *entry_save_entrypoints(void);

/**
 * Restores the entrypoints that were saved with entry_save_entrypoints.
 */
void entry_restore_entrypoints(void *saved);

/**
 * Called before starting entrypoint patching.
 *
 * This function will generally call mprotect(2) to make the static entrypoints
 * writable.
 *
 * \return Non-zero on success, zero on failure.
 */
int entry_patch_start(void);

/**
 * Called after the vendor library finishes patching the entrypoints.
 *
 * \return Non-zero on success, zero on failure.
 */
int entry_patch_finish(void);

/**
 * Returns the address for an entrypoint that a vendor library can patch.
 *
 * Note that this may be different than \c entry_get_public. For example, in
 * ARMv7, \c entry_get_public adds one to the address so that it switches to
 * thumb mode.
 *
 * \param int The index of the entrypoint to patch.
 * \return The address of the function to patch.
 */
void *entry_get_patch_address(int index);

#endif /* _ENTRY_H_ */
