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

#include "libglxabi.h"
#include "libglxcurrent.h"
#include "libglxmapping.h"
#include "GLdispatch.h"

static __GLdispatchTable *GetCurrentGLDispatch(void)
{
    __GLXAPIState *apiState = __glXGetCurrentAPIState();

    return apiState->glas.dispatch;
}

static __GLdispatchTable *GetTopLevelDispatch(void)
{
    __GLXAPIState *apiState;
    __GLXvendorInfo *vendor;

    apiState = __glXGetCurrentAPIState();
    vendor = apiState ? apiState->currentVendor : NULL;

    return vendor->glDispatch;
}

static void MakeGLDispatchCurrent(__GLdispatchTable *table)
{
    __GLXAPIState *apiState = __glXGetCurrentAPIState();

    if (apiState) {
        apiState->glas.dispatch = (__GLdispatchTable *)table;
        __glDispatchMakeCurrent(&apiState->glas);
    }
}

static GLboolean DestroyGLDispatch(__GLdispatchTable *table)
{
    if (table == GetTopLevelDispatch()) {
        return GL_FALSE;
    }

    __glDispatchDestroyTable((__GLdispatchTable *)table);

    return GL_TRUE;
}

__GLdispatchExports __glXGLdispatchExportsTable = {
    .getCurrentGLDispatch = GetCurrentGLDispatch,
    .getTopLevelDispatch = GetTopLevelDispatch,
    .createGLDispatch = __glDispatchCreateTable,
    .getGLDispatchOffset = __glDispatchGetOffset,
    .setGLDispatchEntry = __glDispatchSetEntry,
    .makeGLDispatchCurrent = MakeGLDispatchCurrent,
    .destroyGLDispatch = DestroyGLDispatch,
};


