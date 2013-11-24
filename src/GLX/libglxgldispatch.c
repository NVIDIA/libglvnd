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

/*
 * XXX hack: cast (__GLXcoreDispatchTable *) to the real type (__GLdispatchTable
 * *). Maybe cleaner to just expose __GLdispatchTable to the ABI, or have the
 * relevant functions take in (void *)?
 */

__GLXcoreDispatchTable *__glXGetCurrentGLDispatch(void)
{
    __GLXAPIState *apiState = __glXGetCurrentAPIState();

    return (__GLXcoreDispatchTable *)apiState->glas.dispatch;
}

__GLXcoreDispatchTable *__glXGetTopLevelDispatch(void)
{
    __GLXAPIState *apiState;
    __GLXvendorInfo *vendor;

    apiState = __glXGetCurrentAPIState();
    vendor = apiState ? apiState->currentVendor : NULL;

    return (__GLXcoreDispatchTable *)vendor->glDispatch;
}

__GLXcoreDispatchTable *__glXCreateGLDispatch(const __GLXvendorCallbacks *cb,
                                              void *data)
{
    __GLdispatchTable *dispatch = __glDispatchCreateTable(
            cb->getProcAddress,
            cb->getDispatchProto,
            cb->destroyDispatchData,
            data
   );

    return (__GLXcoreDispatchTable *)dispatch;
}

GLint __glXGetGLDispatchOffset(const GLubyte *procName)
{
    return __glDispatchGetOffset((const char *)procName);
}

void __glXSetGLDispatchEntry(__GLXcoreDispatchTable *table,
                             GLint offset,
                             __GLXextFuncPtr addr)
{
    __glDispatchSetEntry((__GLdispatchTable *)table,
                         offset,
                         (__GLdispatchProc)addr);
}

void __glXMakeGLDispatchCurrent(__GLXcoreDispatchTable *table)
{
    __GLXAPIState *apiState = __glXGetCurrentAPIState();

    if (apiState) {
        apiState->glas.dispatch = (__GLdispatchTable *)table;
        __glDispatchMakeCurrent(&apiState->glas,
                                apiState->glas.dispatch,
                                apiState->glas.context);
    }
}

GLboolean __glXDestroyGLDispatch(__GLXcoreDispatchTable *table)
{
    if (table == __glXGetTopLevelDispatch()) {
        return GL_FALSE;
    }

    __glDispatchDestroyTable((__GLdispatchTable *)table);

    return GL_TRUE;
}
