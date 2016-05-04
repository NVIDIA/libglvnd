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

#if !defined(__LIB_GLX_TLS)
#define __LIB_GLX_TLS

#include <pthread.h>

#include "libglxthread.h"
#include "libglxabipriv.h"
#include "libglxmapping.h"
#include "GLdispatch.h"
#include "lkdhash.h"
#include "glvnd_list.h"

typedef struct __GLXcontextInfoRec __GLXcontextInfo;

/*!
 * Define current API library state here.
 *
 * A thread will have a __GLXThreadState struct if and only if it has a current
 * GLX context. If we don't have a current context, then there's nothing useful
 * to store in it.
 *
 * The pointer to the current __GLXThreadState is stored in libGLdispatch, since
 * it's also the current __GLdispatchThreadState struct.
 */
typedef struct __GLXThreadStateRec {
    __GLdispatchThreadState glas; /* Must be the first entry! */

    __GLXvendorInfo *currentVendor;

    Display *currentDisplay;
    GLXDrawable currentDraw;
    GLXDrawable currentRead;
    __GLXcontextInfo *currentContext;

    struct glvnd_list entry;
} __GLXThreadState;

/*!
 * Looks up the current thread state.
 *
 * If there isn't a current context, or if the current context was set by
 * another library like EGL, then this will return \c NULL.
 */
static inline __GLXThreadState *__glXGetCurrentThreadState(void)
{
    __GLdispatchThreadState *glas = __glDispatchGetCurrentThreadState();
    if (unlikely(!glas ||
                 (glas->tag != GLDISPATCH_API_GLX))) {
        return NULL;
    } else {
        return (__GLXThreadState *)(glas);
    }
}

/*!
 * This gets the current GLX dynamic dispatch table, which is stored in the
 * thread state.
 */
__GLXvendorInfo *__glXGetCurrentDynDispatch(void);

#endif // !defined(__LIB_GLX_TLS)
