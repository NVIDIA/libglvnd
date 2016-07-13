/*
 * Copyright (c) 2013, NVIDIA CORPORATION.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and/or associated documentation files (the
 * "Materials"), to deal in the Materials without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sublicense, and/or sell copies of the Materials, and to
 * permit persons to whom the Materials are furnished to do so, subject to
 * the following conditions:
 *
 * The above copyright notice and this permission notice shall be included
 * unaltered in all copies or substantial portions of the Materials.
 * Any additions, deletions, or changes to the original source files
 * must be clearly indicated in accompanying documentation.
 *
 * If only executable code is distributed, then the accompanying
 * documentation must state that "this software is based in part on the
 * work of the Khronos Group."
 *
 * THE MATERIALS ARE PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 * IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
 * CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 * MATERIALS OR THE USE OR OTHER DEALINGS IN THE MATERIALS.
 */

#ifndef __GLVND_PTHREAD_H__
#define __GLVND_PTHREAD_H__

#include <pthread.h>
#include <errno.h>

/*
 * pthread wrapper functions used to prevent the vendor-neutral library from
 * needing to link against pthreads. The locking functions are no-ops unless
 * the library is linked against pthreads.
 * This wrapper code is also utilized by some unit tests which dynamically load
 * pthreads.
 */

/*
 * Since the underlying pthreads types are opaque, to correctly handle the
 * single-threaded case we need to wrap some of these types with metadata.  For
 * consistency, we typedef all pthreads types, including those which don't need
 * to be wrapped.
 */
typedef pthread_mutex_t glvnd_mutex_t;
typedef pthread_mutexattr_t glvnd_mutexattr_t;

#if defined(HAVE_PTHREAD_RWLOCK_T)
typedef pthread_rwlock_t glvnd_rwlock_t;
typedef pthread_rwlockattr_t glvnd_rwlockattr_t;
#define GLVND_RWLOCK_INITIALIZER PTHREAD_RWLOCK_INITIALIZER
#else
typedef pthread_mutex_t glvnd_rwlock_t;
typedef pthread_mutexattr_t glvnd_rwlockattr_t;
#define GLVND_RWLOCK_INITIALIZER PTHREAD_MUTEX_INITIALIZER
#endif

#define GLVND_MUTEX_INITIALIZER PTHREAD_MUTEX_INITIALIZER

typedef struct _glvnd_once_t {
    pthread_once_t once;
    int done;
} glvnd_once_t;

#define GLVND_ONCE_INIT { PTHREAD_ONCE_INIT, 0 }

typedef struct _glvnd_thread_t {
    pthread_t tid;
    int valid;
} glvnd_thread_t;

#define GLVND_THREAD_NULL_INIT {}

typedef pthread_attr_t glvnd_thread_attr_t;

typedef union {
    pthread_key_t key;
    void **data;
} glvnd_key_t;
#define GLVND_KEYS_MAX PTHREAD_KEYS_MAX

/*!
 * Struct defining the wrapper functions implemented by this library.
 * The implementations will differ depending on whether we're in the
 * singlethreaded case.
 */
typedef struct GLVNDPthreadFuncsRec {
    /* Should never be used by libglvnd. May be used by some unit tests */
    int (*create)(glvnd_thread_t *thread, const glvnd_thread_attr_t *attr,
                  void *(*start_routine) (void *), void *arg);
    int (*join)(glvnd_thread_t thread, void **retval);

    /* Only used in debug/tracing code */
    glvnd_thread_t (*self)(void);
    int (*equal)(glvnd_thread_t t1, glvnd_thread_t t2);

    /* Locking primitives */
    int (*mutex_init)(glvnd_mutex_t *mutex, const glvnd_mutexattr_t *attr);
    int (*mutex_destroy)(glvnd_mutex_t *mutex);
    int (*mutex_lock)(glvnd_mutex_t *mutex);
    int (*mutex_trylock)(glvnd_mutex_t *mutex);
    int (*mutex_unlock)(glvnd_mutex_t *mutex);

    int (* mutexattr_init) (glvnd_mutexattr_t *attr);
    int (* mutexattr_destroy) (glvnd_mutexattr_t *attr);
    int (* mutexattr_settype) (glvnd_mutexattr_t *attr, int kind);

    int (*rwlock_init)(glvnd_rwlock_t *rwlock, const glvnd_rwlockattr_t *attr);
    int (*rwlock_destroy)(glvnd_rwlock_t *rwlock);
    int (*rwlock_rdlock)(glvnd_rwlock_t *rwlock);
    int (*rwlock_wrlock)(glvnd_rwlock_t *rwlock);
    int (*rwlock_tryrdlock)(glvnd_rwlock_t *rwlock);
    int (*rwlock_trywrlock)(glvnd_rwlock_t *rwlock);
    int (*rwlock_unlock)(glvnd_rwlock_t *rwlock);

    /* Other used functions */
    int (*once)(glvnd_once_t *once_control, void (*init_routine)(void));

    /*
     * TSD key management.  Used to handle the corner case when a thread
     * is destroyed with a context current.
     */
    int (*key_create)(glvnd_key_t *key, void (*destr_function)(void *));
    int (*key_delete)(glvnd_key_t key);
    int (*setspecific)(glvnd_key_t key, const void *p);
    void *(*getspecific)(glvnd_key_t key);

    /*
     * Are we single-threaded?
     */
    int is_singlethreaded;
} GLVNDPthreadFuncs;

/**
 * A NULL glvnd_thread_t value. This is mainly useful as something to pass to
 * \c GLVNDPthreadFuncs.equal. To initialize a glvnd_thread_t variable, use
 * \c GLVND_THREAD_NULL_INIT.
 */
extern const glvnd_thread_t GLVND_THREAD_NULL;

/**
 * The function table with all of the pthreads function pointers. This table
 * is initialized by \c glvndSetupPthreads.
 */
extern GLVNDPthreadFuncs __glvndPthreadFuncs;

/*!
 * \brief Sets up pthreads wrappers.
 *
 * This fills the given function pointer table with the appropriate wrapper
 * functions, using the passed-in handle to look for pthreads functions. This
 * should only be called once on initialization.
 */
void glvndSetupPthreads(void);



#endif // __GLVND_PTHREAD_H__
