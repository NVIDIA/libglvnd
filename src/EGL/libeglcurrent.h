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

#if !defined(__LIB_EGL_TLS)
#define __LIB_EGL_TLS

#include <pthread.h>

#include "glvnd_pthread.h"
#include "libeglabipriv.h"
#include "libeglmapping.h"
#include "GLdispatch.h"
#include "lkdhash.h"
#include "glvnd_list.h"

/*!
 * Defines the state to keep track of a current OpenGL or GLES context.
 *
 * Each thread will have an \c __EGLdispatchThreadState structure if and only if it has a
 * current EGL context. As with GLX, the pointer to the current
 * \c __EGLdispatchThreadState structure is maintained by libGLdispatch.
 */
typedef struct __EGLdispatchThreadStateRec {
    __GLdispatchThreadState glas; /* Must be the first entry! */

    // The current display, context, and drawable for GL or GLES.
    // Note that OpenVG will need a separate current context.
    __EGLdisplayInfo *currentDisplay;
    EGLSurface currentDraw;
    EGLSurface currentRead;
    EGLContext currentContext;
    __EGLvendorInfo *currentVendor;

    struct glvnd_list entry;
} __EGLdispatchThreadState;

/*!
 * Defines per-thread state in libEGL that is not specific to any client API or
 * context.
 */
typedef struct __EGLThreadAPIStateRec {
    /*!
     * The last EGL error code. This is returned from eglGetError().
     */
    EGLint lastError;
    __EGLvendorInfo *lastVendor;

    /*!
     * The current client API, as specified by eglBindAPI.
     */
    EGLenum currentClientApi;

    EGLLabelKHR label;

    struct glvnd_list entry;
} __EGLThreadAPIState;

void __eglCurrentInit(void);
void __eglCurrentTeardown(EGLBoolean doReset);

/*!
 * Returns the \c __EGLThreadAPIState structure for the current thread.
 *
 * \param create If \p create is true, then a new \c __EGLThreadAPIState struct
 * will be allocated if the current thread doesn't already have one.
 *
 * \return A pointer to the current thread's \c __EGLThreadAPIState, or NULL if
 * the current thread doesn't have one yet.
 */
__EGLThreadAPIState *__eglGetCurrentThreadAPIState(EGLBoolean create);

/*!
 * Frees the \c __EGLThreadAPIState struct for the current thread.
 */
void __eglDestroyCurrentThreadAPIState(void);

/*!
 * Returns the current thread's \c __EGLdispatchThreadState structure, if it has one.
 */
static inline __EGLdispatchThreadState *__eglGetCurrentAPIState(void)
{
    __GLdispatchThreadState *glas = __glDispatchGetCurrentThreadState();
    if (unlikely(!glas ||
                 (glas->tag != GLDISPATCH_API_EGL))) {
        return NULL;
    } else {
        return (__EGLdispatchThreadState *)(glas);
    }
}

__EGLdispatchThreadState *__eglCreateAPIState(void);
void __eglDestroyAPIState(__EGLdispatchThreadState *state);

EGLenum __eglQueryAPI(void);
__EGLvendorInfo *__eglGetCurrentVendor(void);
EGLContext __eglGetCurrentContext(void);
EGLDisplay __eglGetCurrentDisplay(void);
EGLSurface __eglGetCurrentSurface(EGLint readDraw);

#endif // !defined(__LIB_EGL_TLS)
