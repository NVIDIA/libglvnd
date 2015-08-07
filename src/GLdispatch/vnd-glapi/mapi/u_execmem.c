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
#include "u_thread.h"
#include "u_execmem.h"
#include "utils_misc.h"


/*
 * Allocate 64 bytes for each stub so that they're large enough to hold the
 * x86-64 TSD stubs. The x86 TSD and x86-64 TLS stubs both take 32 bytes each.
 *
 * The x86-64 TSD stubs are larger than the others because it has to deal with
 * 64-bit addresses and preserving the function arguments.
 *
 * The generated stubs may not be within 2GB of u_current or
 * u_current_get_internal, so we can't use RIP-relative addressing for them.
 * Instead we have to use movabs instructions to load the 64-bit absolute
 * addresses, which take 10 bytes each.
 *
 * In addition, x86-64 passes the first 6 parameters in registers, which the
 * callee does not have to preserve. Since the stub has to pass those same
 * parameters to the real function, we have to preserve them across the call to
 * u_current_get_internal. Pushing and popping those registers takes another 24
 * bytes.
 */
#define EXEC_MAP_SIZE (64*4096) // DISPATCH_FUNCTION_SIZE * MAPI_TABLE_NUM_DYNAMIC

u_mutex_declare_static(exec_mutex);

static unsigned int head = 0;

static unsigned char *exec_mem = NULL;
static unsigned char *write_mem = NULL;


#if defined(__linux__) || defined(__OpenBSD__) || defined(_NetBSD__) || defined(__sun) || defined(__HAIKU__)

#include <unistd.h>
#include <sys/mman.h>

#ifndef MAP_ANONYMOUS
#define MAP_ANONYMOUS MAP_ANON
#endif


/*
 * Dispatch stubs are of fixed size and never freed. Thus, we do not need to
 * overlay a heap, we just mmap a page and manage through an index.
 */

static int
init_map(void)
{
    if (exec_mem == NULL) {
        void *writePtr, *execPtr;
        if (AllocExecPages(EXEC_MAP_SIZE, &writePtr, &execPtr) == 0) {
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
        FreeExecPages(EXEC_MAP_SIZE, write_mem, exec_mem);
        write_mem = NULL;
        exec_mem = NULL;
    }
}

#elif defined(_WIN32)

#include <windows.h>


/*
 * Avoid Data Execution Prevention.
 */

static int
init_map(void)
{
   exec_mem = VirtualAlloc(NULL, EXEC_MAP_SIZE, MEM_COMMIT, PAGE_EXECUTE_READWRITE);
   write_mem = exec_mem;

   return (exec_mem != NULL);
}


#else

#include <stdlib.h>

static int
init_map(void)
{
   exec_mem = malloc(EXEC_MAP_SIZE);
   write_mem = exec_mem;

   return (exec_mem != NULL);
}


#endif

void *
u_execmem_alloc(unsigned int size)
{
   void *addr = NULL;

   u_mutex_lock(exec_mutex);

   if (!init_map())
      goto bail;

   /* free space check, assumes no integer overflow */
   if (head + size > EXEC_MAP_SIZE)
      goto bail;

   /* allocation, assumes proper addr and size alignement */
   addr = exec_mem + head;
   head += size;

bail:
   u_mutex_unlock(exec_mutex);

   return addr;
}

void *u_execmem_get_writable(void *execPtr)
{
    // If execPtr is within the executable mapping, then return the same offset
    // in the writable mapping.
    if (((uintptr_t) execPtr) >= ((uintptr_t) exec_mem))
    {
        uintptr_t offset = ((uintptr_t) execPtr) - ((uintptr_t) exec_mem);
        if (offset < EXEC_MAP_SIZE)
        {
            return (void *) (((uintptr_t) write_mem) + offset);
        }
    }
    return execPtr;
}

