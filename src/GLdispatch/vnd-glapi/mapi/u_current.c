/*
 * Mesa 3-D graphics library
 *
 * Copyright (C) 1999-2008  Brian Paul   All Rights Reserved.
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
 * This file manages the OpenGL API dispatch layer.
 * The dispatch table (struct _glapi_table) is basically just a list
 * of function pointers.
 * There are functions to set/get the current dispatch table for the
 * current thread and to manage registration/dispatch of dynamically
 * added extension functions.
 *
 * It's intended that this file and the other glapi*.[ch] files are
 * flexible enough to be reused in several places:  XFree86, DRI-
 * based libGL.so, and perhaps the SGI SI.
 *
 * NOTE: There are no dependencies on Mesa in this code.
 *
 * Versions (API changes):
 *   2000/02/23  - original version for Mesa 3.3 and XFree86 4.0
 *   2001/01/16  - added dispatch override feature for Mesa 3.5
 *   2002/06/28  - added _glapi_set_warning_func(), Mesa 4.1.
 *   2002/10/01  - _glapi_get_proc_address() will now generate new entrypoints
 *                 itself (using offset ~0).  _glapi_add_entrypoint() can be
 *                 called afterward and it'll fill in the correct dispatch
 *                 offset.  This allows DRI libGL to avoid probing for DRI
 *                 drivers!  No changes to the public glapi interface.
 */

#include "u_current.h"
#include "u_thread.h"
#include <assert.h>


#include "table.h"
#include "stub.h"


/**
 * \name Current dispatch and current context control variables
 *
 * TODO: rewrite this section
 *
 * Depending on whether or not multithreading is support, and the type of
 * support available, several variables are used to store the current context
 * pointer and the current dispatch table pointer.  In the non-threaded case,
 * the variables \c _glapi_Dispatch and \c _glapi_Context are used for this
 * purpose.
 *
 * In the "normal" threaded case, the variables \c _glapi_Dispatch and
 * \c _glapi_Context will be \c NULL if an application is detected as being
 * multithreaded.  Single-threaded applications will use \c _glapi_Dispatch
 * and \c _glapi_Context just like the case without any threading support.
 * When \c _glapi_Dispatch and \c _glapi_Context are \c NULL, the thread state
 * data \c _gl_DispatchTSD and \c ContextTSD are used.  Drivers and the
 * static dispatch functions access these variables via \c _glapi_get_dispatch.
 *
 * There is a race condition in setting \c _glapi_Dispatch to \c NULL.  It is
 * possible for the original thread to be setting it at the same instant a new
 * thread, perhaps running on a different processor, is clearing it.  Because
 * of that, \c ThreadSafe, which can only ever be changed to \c GL_TRUE, is
 * used to determine whether or not the application is multithreaded.
 * 
 * In the TLS case, the variables \c _glapi_Dispatch and \c _glapi_Context are
 * hardcoded to \c NULL.  Instead the TLS variables \c _glapi_tls_Dispatch and
 * \c _glapi_tls_Context are used.  Having \c _glapi_Dispatch and
 * \c _glapi_Context be hardcoded to \c NULL maintains binary compatability
 * between TLS enabled loaders and non-TLS DRI drivers.
 */

void
u_current_destroy(void)
{
}


#if !defined(GLX_USE_TLS)

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

#else

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

#endif

