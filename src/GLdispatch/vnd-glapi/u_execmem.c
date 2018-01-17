/*
 * Mesa 3-D graphics library
 *
 * Copyright (C) 1999-2005  Brian Paul   All Rights Reserved.
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


/**
 * \file glapi_execmem.c
 *
 * Function for allocating executable memory for dispatch stubs.
 *
 * Copied from main/execmem.c and simplified for dispatch stubs.
 */

#include <stdlib.h>
#include <stdint.h>

#include "u_compiler.h"
#include "u_execmem.h"
#include "utils_misc.h"
#include "glvnd_pthread.h"
#include "entry.h"
#include "table.h"

static glvnd_mutex_t exec_mutex = GLVND_MUTEX_INITIALIZER;

static unsigned int head = 0;

static unsigned char *exec_mem = NULL;
static unsigned char *write_mem = NULL;

/*
 * Dispatch stubs are of fixed size and never freed. Thus, we do not need to
 * overlay a heap, we just mmap a page and manage through an index.
 */

static int
init_map(void)
{
    if (entry_stub_size == 0) {
        return 0;
    }

    if (exec_mem == NULL) {
        void *writePtr, *execPtr;
        if (AllocExecPages(entry_stub_size * MAPI_TABLE_NUM_DYNAMIC, &writePtr, &execPtr) == 0) {
            exec_mem = (unsigned char *) execPtr;
            write_mem = (unsigned char *) writePtr;
            head = 0;
        }
    }

    return (exec_mem != NULL);
}

void u_execmem_free(void)
{
    if (exec_mem != NULL) {
        FreeExecPages(entry_stub_size * MAPI_TABLE_NUM_DYNAMIC, write_mem, exec_mem);
        write_mem = NULL;
        exec_mem = NULL;
    }
}

void *
u_execmem_alloc(unsigned int size)
{
   void *addr = NULL;

   __glvndPthreadFuncs.mutex_lock(&exec_mutex);

   if (!init_map())
      goto bail;

   /* free space check, assumes no integer overflow */
   if (head + size > entry_stub_size * MAPI_TABLE_NUM_DYNAMIC)
      goto bail;

   /* allocation, assumes proper addr and size alignement */
   addr = exec_mem + head;
   head += size;

bail:
   __glvndPthreadFuncs.mutex_unlock(&exec_mutex);

   return addr;
}

void *u_execmem_get_writable(void *execPtr)
{
    // If execPtr is within the executable mapping, then return the same offset
    // in the writable mapping.
    if (((uintptr_t) execPtr) >= ((uintptr_t) exec_mem))
    {
        uintptr_t offset = ((uintptr_t) execPtr) - ((uintptr_t) exec_mem);
        if (offset < entry_stub_size * MAPI_TABLE_NUM_DYNAMIC)
        {
            return (void *) (((uintptr_t) write_mem) + offset);
        }
    }
    return execPtr;
}

