/*
 * Mesa 3-D graphics library
 *
 * Copyright (C) 1999-2006  Brian Paul   All Rights Reserved.
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
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR
 * OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
 * ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
 * OTHER DEALINGS IN THE SOFTWARE.
 */


/*
 * Thread support for gl dispatch.
 *
 * Initial version by John Stone (j.stone@acm.org) (johns@cs.umr.edu)
 *                and Christoph Poliwoda (poliwoda@volumegraphics.com)
 * Revised by Keith Whitwell
 * Adapted for new gl dispatcher by Brian Paul
 * Modified for use in mapi by Chia-I Wu
 */

/*
 * If this file is accidentally included by a non-threaded build,
 * it should not cause the build to fail, or otherwise cause problems.
 * In general, it should only be included when needed however.
 */

#ifndef _U_THREAD_H_
#define _U_THREAD_H_

#include <stdio.h>
#include <stdlib.h>
#include "u_compiler.h"

#include <pthread.h> /* POSIX threads headers */
#include "glvnd_pthread/glvnd_pthread.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef glvnd_mutex_t u_mutex;

#define u_mutex_declare_static(name) \
   static u_mutex name = GLVND_MUTEX_INITIALIZER

#define u_mutex_init(name)    __glvndPthreadFuncs.mutex_init(&(name), NULL)
#define u_mutex_destroy(name) __glvndPthreadFuncs.mutex_destroy(&(name))
#define u_mutex_lock(name)    (void) __glvndPthreadFuncs.mutex_lock(&(name))
#define u_mutex_unlock(name)  (void) __glvndPthreadFuncs.mutex_unlock(&(name))

#ifdef __cplusplus
}
#endif

#endif /* _U_THREAD_H_ */
