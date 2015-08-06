/*
 * Mesa 3-D graphics library
 *
 * Copyright (C) 1999-2008  Brian Paul   All Rights Reserved.
 * Copyright (c) 2015, NVIDIA CORPORATION.
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

#include "u_current.h"
#include "u_thread.h"
#include <assert.h>

#include "table.h"
#include "stub.h"

const void *_glapi_Current[GLAPI_NUM_CURRENT_ENTRIES]
    = {
        (void *) table_noop_array
      };

static glvnd_key_t u_current_tsd[GLAPI_NUM_CURRENT_ENTRIES];
static int ThreadSafe;

static void
u_current_init_tsd(void)
{
    int i;
    for (i = 0; i < GLAPI_NUM_CURRENT_ENTRIES; i++) {
        if (pthreadFuncs.key_create(&u_current_tsd[i], NULL) != 0) {
            perror("_glthread_: failed to allocate key for thread specific data");
            abort();
        }
    }
}

void
u_current_init(void)
{
    static int firstCall = 1;
    assert(firstCall); // This should only ever be called once.
    if (firstCall) {
        u_current_init_tsd();
        firstCall = 0;
    }
}

void
u_current_destroy(void)
{
}

void
u_current_set_multithreaded(void)
{
    int i;

    ThreadSafe = 1;
    for (i = 0; i < GLAPI_NUM_CURRENT_ENTRIES; i++) {
        _glapi_Current[i] = NULL;
    }
}

void u_current_set(const struct _glapi_table *tbl)
{
    if (pthreadFuncs.setspecific(u_current_tsd[GLAPI_CURRENT_DISPATCH], (void *) tbl) != 0) {
        perror("_glthread_: thread failed to set thread specific data");
        abort();
    }
    _glapi_Current[GLAPI_CURRENT_DISPATCH] = (ThreadSafe) ? NULL : (const void *) tbl;
}

const struct _glapi_table *u_current_get(void)
{
   return (const struct _glapi_table *) ((ThreadSafe) ?
         pthreadFuncs.getspecific(u_current_tsd[GLAPI_CURRENT_DISPATCH]) : _glapi_Current[GLAPI_CURRENT_DISPATCH]);
}

