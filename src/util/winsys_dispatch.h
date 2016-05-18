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

#ifndef WINSYS_DISPATCH_H
#define WINSYS_DISPATCH_H

/*!
 * \file
 *
 * Utility functions for keeping track of the dispatch stubs for window-system
 * functions.
 *
 * These functions keep track of an array of functions, with a name and pointer
 * for each.
 */

/*!
 * Initializes the window system list.
 */
void __glvndWinsysDispatchInit(void);

/*!
 * Frees the list and all the items in it.
 */
void __glvndWinsysDispatchCleanup(void);

/*!
 * Looks up a function by name.
 *
 * \param name The name of the function to look up.
 * \return The index of the function, or -1 if the function was not found.
 */
int __glvndWinsysDispatchFindIndex(const char *name);

/*!
 * Adds a function to the list. The function must not already be in the list.
 *
 * Note that the function index list is global state, and this function is not
 * thread-safe. The caller is responsible for protecting access to it.
 *
 * \param name The name of the function.
 * \param dispatch A pointer to the dispatch stub for the function.
 * \return The index of the function, or -1 on error.
 */
int __glvndWinsysDispatchAllocIndex(const char *name, void *dispatch);

/*!
 * Returns the name of a function.
 *
 * \param index The index of the function.
 * \return A pointer to the function name, or \c NULL if the index is invalid.
 */
const char *__glvndWinsysDispatchGetName(int index);

/*!
 * Returns the dispatch stub for a function.
 *
 * \param index The index of the function.
 * \return A pointer to the dispatch stub, or \c NULL if the index is invalid.
 */
void *__glvndWinsysDispatchGetDispatch(int index);

/*!
 * Returns the number of functions in the list.
 */
int __glvndWinsysDispatchGetCount(void);


/*!
 * A dispatch table to keep track of the window-system functions from a vendor
 * library.
 */
typedef struct __GLVNDwinsysVendorDispatchRec __GLVNDwinsysVendorDispatch;

/*!
 * Creates an empty dispatch table.
 */
__GLVNDwinsysVendorDispatch *__glvndWinsysVendorDispatchCreate(void);

/*!
 * Frees a dispatch table.
 */
void __glvndWinsysVendorDispatchDestroy(__GLVNDwinsysVendorDispatch *table);

/*!
 * Adds a function to a dispatch table.
 *
 * \param table The dispatch table.
 * \param index The index of the function to add.
 * \param func The pointer to the vendor library's function.
 */
int __glvndWinsysVendorDispatchAddFunc(__GLVNDwinsysVendorDispatch *table, int index, void *func);

/*!
 * Looks up a function from a dispatch table.
 *
 * \param table The dispatch table.
 * \param index The index of the function to look up.
 * \return The function pointer, or \c NULL if the function is not in the
 * table.
 */
void *__glvndWinsysVendorDispatchLookupFunc(__GLVNDwinsysVendorDispatch *table, int index);

#endif // WINSYS_DISPATCH_H
