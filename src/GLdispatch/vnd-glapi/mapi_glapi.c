/*
 * Mesa 3-D graphics library
 *
 * Copyright (C) 1999-2008  Brian Paul   All Rights Reserved.
 * Copyright (C) 2010 LunarG Inc.
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
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 *
 * Authors:
 *    Chia-I Wu <olv@lunarg.com>
 */

#include <string.h>
#include "glapi.h"
#include "u_current.h"
#include "table.h" /* for MAPI_TABLE_NUM_SLOTS */
#include "stub.h"

/*
 * Global variables and _glapi_get_current are defined in
 * u_current.c.
 */

void
_glapi_init(void)
{
    u_current_init();
}

void
_glapi_destroy(void)
{
   u_current_destroy();
   stub_cleanup();
}

void
_glapi_set_multithread(void)
{
    u_current_set_multithreaded();
}

void
_glapi_set_current(const struct _glapi_table *dispatch)
{
    if (dispatch == NULL)
    {
        dispatch = (const struct _glapi_table *) table_noop_array;
    }
    u_current_set(dispatch);
}

const struct _glapi_table *
_glapi_get_current(void)
{
    return u_current_get();
}

/**
 * Return size of dispatch table struct as number of functions (or
 * slots).
 */
unsigned int
_glapi_get_dispatch_table_size(void)
{
   return MAPI_TABLE_NUM_SLOTS;
}

static int
_glapi_get_stub(const char *name, int generate)
{
    int index;

    if (!name) {
        return -1;
    }

    index = stub_find_public(name);
    if (index < 0) {
        index = stub_find_dynamic(name, generate);
    }

    return index;
}

/**
 * Return offset of entrypoint for named function within dispatch table.
 */
int
_glapi_get_proc_offset(const char *funcName)
{
    return _glapi_get_stub(funcName, 0);
}

/**
 * Return pointer to the named function.  If the function name isn't found
 * in the name of static functions, try generating a new API entrypoint on
 * the fly with assembly language.
 */
_glapi_proc
_glapi_get_proc_address(const char *funcName)
{
    int index = _glapi_get_stub(funcName, 1);
    if (index >= 0) {
        return stub_get_addr(index);
    } else {
        return NULL;
    }
}

/**
 * Return the name of the function at the given dispatch offset.
 */
const char *
_glapi_get_proc_name(unsigned int offset)
{
   return stub_get_name(offset);
}


int _glapi_get_stub_count(void)
{
    return stub_get_count();
}

