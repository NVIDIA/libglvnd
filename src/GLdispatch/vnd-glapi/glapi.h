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
#include <GL/gl.h>
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
    GLAPI_NUM_CURRENT_ENTRIES
};


#if defined (GLDISPATCH_USE_TLS)

/**
 * A pointer to each thread's dispatch table.
 */
_GLAPI_EXPORT extern const __thread void *
    _glapi_tls_Current[GLAPI_NUM_CURRENT_ENTRIES]
    __attribute__((tls_model("initial-exec")));

#endif /* defined (GLDISPATCH_USE_TLS) */

/**
 * A pointer to the current dispatch table, used with the TSD versions of the
 * dispatch functions.
 *
 * For applications that only render from a single thread, there's only one
 * dispatch table. In that case, the dispatch functions will look up the
 * dispatch table from this variable, so that they avoid the overhead of
 * calling pthread_getspecific.
 *
 * With a multithreaded app, this variable will contain NULL.
 */
_GLAPI_EXPORT extern const void *_glapi_Current[GLAPI_NUM_CURRENT_ENTRIES];


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


/**
 * Sets the dispatch table for the current thread.
 *
 * If \p dispatch is NULL, then a table of no-op functions will be assigned
 * instead.
 */
void
_glapi_set_current(const struct _glapi_table *dispatch);

/**
 * Returns the dispatch table for the current thread.
 */
_GLAPI_EXPORT const struct _glapi_table *
_glapi_get_current(void);


unsigned int
_glapi_get_dispatch_table_size(void);


int
_glapi_get_proc_offset(const char *funcName);


_glapi_proc
_glapi_get_proc_address(const char *funcName);


const char *
_glapi_get_proc_name(unsigned int offset);

/**
 * Returns the total number of defined stubs. This count only includes dynamic
 * stubs that have been generated, so it will always be less than or equal to
 * the size of the dispatch table.
 */
int _glapi_get_stub_count(void);

/**
 * Functions used for patching entrypoints. These functions are exported from
 * an entrypoint library such as libGL.so or libOpenGL.so, and used in
 * libGLdispatch.
 *
 * \note The \c startPatch, \c finishPatch, and \c abortPatch functions are
 * currently unused, but will be used after some changes to
 * __GLdispatchPatchCallbacks are finished.
 */
typedef struct __GLdispatchStubPatchCallbacksRec {
    /**
     * Called before trying to patch any entrypoints.
     *
     * If startPatch succeeds, then libGLdispatch will call \c getPatchOffsets
     * to fetch the address of each function.
     *
     * After it finishes patching, libGLdispatch will call either
     * \c finishPatch or \c abortPatch.
     *
     * \return GL_TRUE on success, GL_FALSE on failure.
     */
    GLboolean (* startPatch) (void);

    /**
     * Finishes any patching. This is called after \c startPatch if patching
     * is successful.
     */
    void (* finishPatch) (void);

    /**
     * Finishes any patching, and restores the entrypoints to their original
     * state.
     *
     * This is called if an error occurrs and libGLdispatch has to abort
     * patching the entrypoints.
     */
    void (* abortPatch) (void);

    /**
     * Called by libGLdispatch to restore each entrypoint to its normal,
     * unpatched behavior.
     *
     * \return GL_TRUE on success, GL_FALSE on failure.
     */
    GLboolean (* restoreFuncs) (void);

    /**
     * Returns the address of a function to patch. This may or may not create a
     * new stub function if one doesn't already exist.
     *
     * This function is passed to __GLdispatchPatchCallbacks::initiatePatch.
     */
    GLboolean (* getPatchOffset) (const char *name, void **writePtr, const void **execPtr);

    /**
     * Returns the type of the stub functions. This is one of the
     * __GLDISPATCH_STUB_* values.
     */
    int (* getStubType) (void);

    /**
     * Returns the size of each stub.
     */
    int (* getStubSize) (void);

} __GLdispatchStubPatchCallbacks;

/*!
 * This registers stubs with GLdispatch to be overwritten if a vendor library
 * explicitly requests custom entrypoint code.  This is used by the wrapper
 * interface libraries.
 *
 * This function returns an ID number, which is passed to
 * \c __glDispatchUnregisterStubCallbacks to unregister the callbacks.
 *
 * \see stub_get_patch_callbacks for the table used for the entrypoints in
 * libGL, libOpenGL, and libGLdispatch.
 *
 * \param callbacks A table of callback functions.
 * \return A unique ID number, or -1 on failure.
 */
_GLAPI_EXPORT int __glDispatchRegisterStubCallbacks(const __GLdispatchStubPatchCallbacks *callbacks);

/*!
 * This unregisters the GLdispatch stubs, and performs any necessary cleanup.
 *
 * \param stubId The ID number returned from \c __glDispatchRegisterStubCallbacks.
 */
_GLAPI_EXPORT void __glDispatchUnregisterStubCallbacks(int stubId);

#ifdef __cplusplus
}
#endif

#endif /* _GLAPI_H */
