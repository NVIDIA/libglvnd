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

#include <stdio.h>
#include <stdlib.h>
#include <assert.h>

#include "table.h"
#include "stub.h"
#include "glvnd_pthread.h"

const void *_glapi_Current[GLAPI_NUM_CURRENT_ENTRIES]
    = {
        (const void *) table_noop_array
      };

static glvnd_key_t u_current_tsd[GLAPI_NUM_CURRENT_ENTRIES];
static int ThreadSafe;

void
u_current_init(void)
{
    int i;
    for (i = 0; i < GLAPI_NUM_CURRENT_ENTRIES; i++) {
        if (__glvndPthreadFuncs.key_create(&u_current_tsd[i], NULL) != 0) {
            perror("_glthread_: failed to allocate key for thread specific data");
            abort();
        }
        _glapi_Current[i] = (const void *) table_noop_array;
    }
    ThreadSafe = 0;
}

void
u_current_destroy(void)
{
    int i;
    for (i = 0; i < GLAPI_NUM_CURRENT_ENTRIES; i++) {
        __glvndPthreadFuncs.key_delete(u_current_tsd[i]);
    }
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
    if (__glvndPthreadFuncs.setspecific(u_current_tsd[GLAPI_CURRENT_DISPATCH], (void *) tbl) != 0) {
        perror("_glthread_: thread failed to set thread specific data");
        abort();
    }
    _glapi_Current[GLAPI_CURRENT_DISPATCH] = (ThreadSafe) ? NULL : (const void *) tbl;
}

const struct _glapi_table *u_current_get(void)
{
   return (const struct _glapi_table *) ((ThreadSafe) ?
         __glvndPthreadFuncs.getspecific(u_current_tsd[GLAPI_CURRENT_DISPATCH]) : _glapi_Current[GLAPI_CURRENT_DISPATCH]);
}

