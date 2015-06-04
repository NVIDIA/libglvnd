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


/**
 * \mainpage Mesa GL API Module
 *
 * \section GLAPIIntroduction Introduction
 *
 * The Mesa GL API module is responsible for dispatching all the
 * gl*() functions.  All GL functions are dispatched by jumping through
 * the current dispatch table (basically a struct full of function
 * pointers.)
 *
 * A per-thread current dispatch table and per-thread current context
 * pointer are managed by this module too.
 *
 * This module is intended to be non-Mesa-specific so it can be used
 * with the X/DRI libGL also.
 */


#ifndef _GLAPI_H
#define _GLAPI_H

#include <stddef.h>
#include "u_compiler.h"

#ifdef __cplusplus
extern "C" {
#endif


#ifdef _GLAPI_NO_EXPORTS
#  define _GLAPI_EXPORT
#else /* _GLAPI_NO_EXPORTS */
#  ifdef _WIN32
#    ifdef _GLAPI_DLL_EXPORTS
#      define _GLAPI_EXPORT __declspec(dllexport)
#    else
#      define _GLAPI_EXPORT __declspec(dllimport)
#    endif
#  elif defined(__GNUC__) || (defined(__SUNPRO_C) && (__SUNPRO_C >= 0x590))
#    define _GLAPI_EXPORT __attribute__((visibility("default")))
#  else
#    define _GLAPI_EXPORT
#  endif
#endif /* _GLAPI_NO_EXPORTS */


/* Is this needed?  It is incomplete anyway. */

typedef void (*_glapi_proc)(void);
struct _glapi_table;

enum {
    GLAPI_CURRENT_DISPATCH = 0, /* This MUST be the first entry! */
    GLAPI_CURRENT_CONTEXT,
    GLAPI_CURRENT_USER1,
    GLAPI_CURRENT_USER2,
    GLAPI_CURRENT_USER3,
    GLAPI_NUM_CURRENT_ENTRIES
};


#if defined (GLX_USE_TLS)

_GLAPI_EXPORT extern __thread void *
    _glapi_tls_Current[GLAPI_NUM_CURRENT_ENTRIES]
    __attribute__((tls_model("initial-exec")));

_GLAPI_EXPORT extern const void *_glapi_Current[GLAPI_NUM_CURRENT_ENTRIES];

# define GET_DISPATCH() ((struct _glapi_table *) \
    _glapi_tls_Current[GLAPI_CURRENT_DISPATCH])
# define GET_CURRENT_CONTEXT(C)  struct gl_context *C = (struct gl_context *) \
    _glapi_tls_Current[GLAPI_CURRENT_CONTEXT]

#else

_GLAPI_EXPORT extern void *_glapi_Current[GLAPI_NUM_CURRENT_ENTRIES];

# ifdef THREADS

#  define GET_DISPATCH() \
     (likely(_glapi_Current[GLAPI_CURRENT_DISPATCH]) ? \
     (struct _glapi_table *)_glapi_Current[GLAPI_CURRENT_DISPATCH] : \
      _glapi_get_dispatch())

#  define GET_CURRENT_CONTEXT(C)  struct gl_context *C = (struct gl_context *) \
     (likely(_glapi_Current[GLAPI_CURRENT_CONTEXT]) ? \
      _glapi_Current[GLAPI_CURRENT_CONTEXT] : _glapi_get_context())

# else

#  define GET_DISPATCH() ((struct _glapi_table *) \
    _glapi_Current[GLAPI_CURRENT_DISPATCH])
#  define GET_CURRENT_CONTEXT(C)  struct gl_context *C = \
    (struct gl_context *) _glapi_Current[GLAPI_CURRENT_CONTEXT]

# endif

#endif /* defined (GLX_USE_TLS) */


/**
 * Initializes the GLAPI layer.
 */
void
_glapi_init(void);

void
_glapi_destroy(void);


/**
 * Switches to multi-threaded mode. Some systems may have a more efficient
 * dispatch path for single-threaded applications. This function is called from
 * __glDispatchCheckMultithreaded when a second thread starts calling GLX
 * functions.
 */
void
_glapi_set_multithread(void);

_GLAPI_EXPORT void
_glapi_set_context(void *context);


_GLAPI_EXPORT void *
_glapi_get_context(void);


_GLAPI_EXPORT void
_glapi_set_dispatch(struct _glapi_table *dispatch);

_GLAPI_EXPORT void
_glapi_set_current(void *p, int index);

_GLAPI_EXPORT void *
_glapi_get_current(int index);


_GLAPI_EXPORT struct _glapi_table *
_glapi_get_dispatch(void);


_GLAPI_EXPORT unsigned int
_glapi_get_dispatch_table_size(void);


_GLAPI_EXPORT int
_glapi_add_dispatch(const char * const * function_names);

_GLAPI_EXPORT int
_glapi_get_proc_offset(const char *funcName);


_GLAPI_EXPORT _glapi_proc
_glapi_get_proc_address(const char *funcName);


_GLAPI_EXPORT const char *
_glapi_get_proc_name(unsigned int offset);


_GLAPI_EXPORT struct _glapi_table *
_glapi_create_table_from_handle(void *handle, const char *symbol_prefix);

_GLAPI_EXPORT void
_glapi_init_table_from_callback(struct _glapi_table *table,
                                size_t entries,
                                void *(*get_proc_addr)(const unsigned char *name,
                                                       int isClientAPI));


/*
 * These stubs are kept so that the old DRI drivers still load.
 */
_GLAPI_EXPORT void
_glapi_noop_enable_warnings(unsigned char enable);


_GLAPI_EXPORT void
_glapi_set_warning_func(_glapi_proc func);


#ifdef __cplusplus
}
#endif

#endif /* _GLAPI_H */
