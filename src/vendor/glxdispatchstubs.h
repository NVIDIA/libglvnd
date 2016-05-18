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

#ifndef GLXDISPATCHSTUBS_H
#define GLXDISPATCHSTUBS_H

/*!
 * \file
 *
 * Functions for GLX dispatch stubs.
 *
 * This file declares various helper functions used with the GLX dispatch stubs.
 */

#include "glvnd/libglxabi.h"

#include "compiler.h" // Only needed for the PUBLIC macro

/*!
 * An array containing the names of all known GLX functions.
 */
extern const char * const __GLX_DISPATCH_FUNC_NAMES[];

/*!
 * An array containing the dispatch stubs for GLX functions. Note that some
 * elements may be NULL if no dispatch function is defined. This is used for
 * functions like glXDestroyContext, where we'll need an index so that we can
 * look up the function from other dispatch stubs.
 */
extern const __GLXextFuncPtr __GLX_DISPATCH_FUNCS[];

/*!
 * The dispatch index for each function.
 */
extern int __glXDispatchFuncIndices[];

/*!
 * The number of GLX functions. This is the length of the
 * \c __GLX_DISPATCH_FUNC_NAMES, \c __GLX_DISPATCH_FUNCS, and
 * \c __glXDispatchFuncIndices arrays.
 */
extern const int __GLX_DISPATCH_FUNCTION_COUNT;

/*!
 * A pointer to the exports table from libGLX.
 */
extern const __GLXapiExports *__glXDispatchApiExports;

/*!
 * Initializes the dispatch functions.
 *
 * This will set the __GLXapiExports pointer for the stubs to use and will
 * initialize the index array.
 */
void __glxInitDispatchStubs(const __GLXapiExports *exportsTable);

/*!
 * Sets the dispatch index for a function.
 *
 * This function can be used for the vendor's \c __GLXapiImports::setDispatchIndex
 * callback.
 */
void __glxSetDispatchIndex(const GLubyte *name, int index);

/*!
 * Returns the dispatch function for the given name, or \c NULL if the function
 * isn't supported.
 *
 * This function can be used for the vendor's \c __GLXapiImports::getDispatchAddress
 * callback.
 */
void *__glxDispatchFindDispatchFunction(const GLubyte *name);

/*!
 * Reports an X error. This function must be defined by the vendor library.
 */
void __glXSendError(Display *dpy, unsigned char errorCode,
        XID resourceID, unsigned char minorCode, Bool coreX11error);

// Helper functions used by the generated stubs.

/*!
 * Looks up a vendor from a drawable.
 *
 * If \p opcode and \p error are non-negative, then they are used to report an
 * X error if the lookup fails.
 */
__GLXvendorInfo *__glxDispatchVendorByDrawable(Display *dpy, GLXDrawable draw,
        int opcode, int error);

/*!
 * Looks up a vendor from a context.
 */
__GLXvendorInfo *__glxDispatchVendorByContext(Display *dpy, GLXContext ctx,
        int opcode);

/*!
 * Looks up a vendor from a GLXFBConfig.
 */
__GLXvendorInfo *__glxDispatchVendorByConfig(Display *dpy, GLXFBConfig config,
        int opcode);

/*!
 * Adds a GLXContext to libGLX's mapping.
 *
 * If it fails to add the context to the map, then this function will try to
 * destroy the context before returning.
 *
 * \param dpy The display pointer.
 * \param context The context to add.
 * \param vendor The vendor that owns the context.
 * \return \p context on success, or \c NULL on failure.
 */
GLXContext __glXDispatchAddContextMapping(Display *dpy, GLXContext context, __GLXvendorInfo *vendor);

/*!
 * Adds a drawable to libGLX's mapping.
 *
 * Note that unlike contexts and configs, failing to add a drawable is not a
 * problem. libGLX can query the server later to find out which vendor owns the
 * drawable.
 *
 * \param dpy The display pointer.
 * \param draw The drawable to add.
 * \param vendor The vendor that owns the drawable.
 */
void __glXDispatchAddDrawableMapping(Display *dpy, GLXDrawable draw, __GLXvendorInfo *vendor);

/*!
 * Adds a GLXFBConfig to libGLX's mapping.
 *
 * \param dpy The display pointer.
 * \param config The config to add.
 * \param vendor The vendor that owns the config.
 * \return \p config on success, or \c NULL on failure.
 */
GLXFBConfig __glXDispatchAddFBConfigMapping(Display *dpy, GLXFBConfig config, __GLXvendorInfo *vendor);

/*!
 * Adds an array of GLXFBConfigs to libGLX's mapping.
 *
 * If it fails to add any config, then it will free the \p configs array and
 * set \p nelements to zero before returning.
 *
 * \param dpy The display pointer.
 * \param configs The array of configs to add.
 * \param nelements A pointer to the number of elements in \p configs.
 * \param vendor The vendor that owns the configs.
 * \return \p configs on success, or \c NULL on failure.
 */
GLXFBConfig *__glXDispatchAddFBConfigListMapping(Display *dpy, GLXFBConfig *configs, int *nelements, __GLXvendorInfo *vendor);

#endif // GLXDISPATCHSTUBS_H
