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

#include "glxdispatchstubs.h"

#include <X11/Xlibint.h>
#include <GL/glxproto.h>
#include <string.h>

#include "g_glxdispatchstubs.h"

const __GLXapiExports *__glXDispatchApiExports = NULL;
int __glXDispatchFuncIndices[__GLX_DISPATCH_COUNT + 1];
const int __GLX_DISPATCH_FUNCTION_COUNT = __GLX_DISPATCH_COUNT;

static int FindProcIndex(const GLubyte *name)
{
    unsigned first = 0;
    unsigned last = __GLX_DISPATCH_COUNT - 1;

    while (first <= last) {
        unsigned middle = (first + last) / 2;
        int comp = strcmp((const char *) name,
                          __GLX_DISPATCH_FUNC_NAMES[middle]);

        if (comp > 0)
            first = middle + 1;
        else if (comp < 0)
            last = middle - 1;
        else
            return middle;
    }

    /* Just point to the dummy entry at the end of the respective table */
    return __GLX_DISPATCH_COUNT;
}

void __glxInitDispatchStubs(const __GLXapiExports *exportsTable)
{
    int i;
    __glXDispatchApiExports = exportsTable;
    for (i=0; i<__GLX_DISPATCH_COUNT; i++) {
        __glXDispatchFuncIndices[i] = -1;
    }
}

void __glxSetDispatchIndex(const GLubyte *name, int dispatchIndex)
{
    int index = FindProcIndex(name);
    __glXDispatchFuncIndices[index] = dispatchIndex;
}

void *__glxDispatchFindDispatchFunction(const GLubyte *name)
{
    int index = FindProcIndex(name);
    return __GLX_DISPATCH_FUNCS[index];
}

__GLXvendorInfo *__glxDispatchVendorByDrawable(Display *dpy, GLXDrawable draw,
        int opcode, int error)
{
    __GLXvendorInfo *vendor = NULL;

    if (draw != None) {
        vendor = __glXDispatchApiExports->vendorFromDrawable(dpy, draw);
    }
    if (vendor == NULL && dpy != NULL && opcode >= 0 && error >= 0) {
        __glXSendError(dpy, error, draw, opcode, False);
    }
    return vendor;
}

__GLXvendorInfo *__glxDispatchVendorByContext(Display *dpy, GLXContext context,
        int opcode)
{
    __GLXvendorInfo *vendor = NULL;

    if (context != NULL) {
        vendor = __glXDispatchApiExports->vendorFromContext(context);
    }
    if (vendor == NULL && dpy != NULL && opcode >= 0) {
        __glXSendError(dpy, GLXBadContext, 0, opcode, False);
    }
    return vendor;
}

__GLXvendorInfo *__glxDispatchVendorByConfig(Display *dpy, GLXFBConfig config,
        int opcode)
{
    __GLXvendorInfo *vendor = NULL;

    if (config != NULL) {
        vendor = __glXDispatchApiExports->vendorFromFBConfig(dpy, config);
    }
    if (vendor == NULL && dpy != NULL && opcode >= 0) {
        __glXSendError(dpy, GLXBadFBConfig, 0, opcode, False);
    }
    return vendor;
}


GLXContext __glXDispatchAddContextMapping(Display *dpy, GLXContext context, __GLXvendorInfo *vendor)
{
    if (context != NULL) {
        if (__glXDispatchApiExports->addVendorContextMapping(dpy, context, vendor) != 0) {
            // We couldn't add the new context to libGLX's mapping. Call into
            // the vendor to destroy the context again before returning.
            typedef void (* pfn_glXDestroyContext) (Display *dpy, GLXContext ctx);
            pfn_glXDestroyContext ptr_glXDestroyContext = (pfn_glXDestroyContext)
                __glXDispatchApiExports->fetchDispatchEntry(vendor,
                        __glXDispatchFuncIndices[__GLX_DISPATCH_glXDestroyContext]);
            if (ptr_glXDestroyContext != NULL) {
                ptr_glXDestroyContext(dpy, context);
            }
            context = NULL;
        }
    }
    return context;
}

void __glXDispatchAddDrawableMapping(Display *dpy, GLXDrawable draw, __GLXvendorInfo *vendor)
{
    if (draw != None) {
        // We don't have to worry about failing to add a mapping in this case,
        // because libGLX can look up the vendor for the drawable on its own.
        __glXDispatchApiExports->addVendorDrawableMapping(dpy, draw, vendor);
    }
}

GLXFBConfig __glXDispatchAddFBConfigMapping(Display *dpy, GLXFBConfig config, __GLXvendorInfo *vendor)
{
    if (config != NULL) {
        if (__glXDispatchApiExports->addVendorFBConfigMapping(dpy, config, vendor) != 0) {
            config = NULL;
        }
    }
    return config;
}

GLXFBConfig *__glXDispatchAddFBConfigListMapping(Display *dpy, GLXFBConfig *configs, int *nelements, __GLXvendorInfo *vendor)
{
    if (configs != NULL && nelements != NULL) {
        int i;
        Bool success = True;
        for (i=0; i<*nelements; i++) {
            if (__glXDispatchApiExports->addVendorFBConfigMapping(dpy, configs[i], vendor) != 0) {
                success = False;
                break;
            }
        }
        if (!success) {
            XFree(configs);
            configs = NULL;
            *nelements = 0;
        }
    }
    return configs;
}

