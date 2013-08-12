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

#include "compiler.h"
#include "libglxthread.h"
#include "libglxabipriv.h"
#include "libglxmapping.h"
#include "libglxnoop.h"
#include "uthash.h"

/*
 * Define current API library state here. An API state is per-thread, per-winsys
 * library. Due to this definition libGLX's state could always be "current",
 * though in practice (to conserve TLS space) only up to one API library's state
 * is current at a time. Hence, the __GLXAPIState may or may not be in TLS
 * (depending on whether GLX has a context current at the time).
 */
typedef struct __GLXAPIStateRec {
    /* TODO: insert a winsys-neutral API state handle here */

    Display *currentDisplay;
    GLXDrawable currentDraw;
    GLXDrawable currentRead;
    const __GLXdispatchTableStatic *currentStaticDispatch;
    __GLXdispatchTableDynamic *currentDynDispatch;
    __GLXvendorInfo *currentVendor;
    UT_hash_handle hh;
} __GLXAPIState;

/*!
 * This is a fallback function in the case where the API library is not in
 * TLS, to look up the API state given the current thread id.
 */
__GLXAPIState *__glXGetAPIState(glvnd_thread_t tid);

/*!
 * This attempts to pull the current API state from TLS, and falls back to
 * __glXGetAPIState() if that fails.
 */
static inline __GLXAPIState *__glXGetCurrentAPIState(void)
{
    /* TODO: check TLS */
    return __glXGetAPIState(__glXPthreadFuncs.self());
}

/*!
 * This gets the current GLX static dispatch table, which is stored in the API
 * state.
 */
static inline const __GLXdispatchTableStatic *__glXGetCurrentDispatch(void)
{
    __GLXAPIState *apiState = __glXGetCurrentAPIState();
    if (likely(apiState)) {
        return apiState->currentStaticDispatch ?
               apiState->currentStaticDispatch : __glXDispatchNoopPtr;
    } else {
        return __glXDispatchNoopPtr;
    }
}

/*!
 * This gets the current GLX dynamic dispatch table, which is stored in the API
 * state.
 */
__GLXdispatchTableDynamic *__glXGetCurrentDynDispatch(void);

/*!
 * This gets the current (vendor-specific) context, which is stored directly
 * in TLS.
 */
static inline GLXContext __glXGetCurrentContext(void)
{
    /* TODO: check TLS */
    return NULL;
}

#endif // !defined(__LIB_GLX_TLS)
