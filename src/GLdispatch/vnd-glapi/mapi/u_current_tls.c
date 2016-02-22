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
#include <assert.h>

#include "table.h"
#include "stub.h"

__thread const void *_glapi_tls_Current[GLAPI_NUM_CURRENT_ENTRIES]
    __attribute__((tls_model("initial-exec")))
    = {
        (void *) table_noop_array,
      };

const void *_glapi_Current[GLAPI_NUM_CURRENT_ENTRIES] = {};

void
u_current_init(void)
{
}

void
u_current_destroy(void)
{
}

void
u_current_set_multithreaded(void)
{
}

void
u_current_set(const struct _glapi_table *tbl)
{
   _glapi_tls_Current[GLAPI_CURRENT_DISPATCH] = (const void *) tbl;
}

const struct _glapi_table *u_current_get(void)
{
   return (const struct _glapi_table *) _glapi_tls_Current[GLAPI_CURRENT_DISPATCH];
}

