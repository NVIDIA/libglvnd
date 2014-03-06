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

#include <dlfcn.h>
#include <assert.h>
#include <string.h>
#include <stdlib.h>

#include "trace.h"
#include "glvnd_pthread.h"

/* The real function pointers */
typedef struct GLVNDPthreadRealFuncsRec {
    /* Should never be used by libglvnd. May be used by some unit tests */
    int (*create)(pthread_t *thread, const pthread_attr_t *attr,
                  void *(*start_routine) (void *), void *arg);
    int (*join)(pthread_t thread, void **retval);

    /* Only used in debug/tracing code */
    pthread_t (*self)(void);
    int (*equal)(pthread_t t1, pthread_t t2);

    /* Locking primitives */
    int (*mutex_lock)(pthread_mutex_t *mutex);
    int (*mutex_unlock)(pthread_mutex_t *mutex);
    int (*rwlock_init)(pthread_rwlock_t *rwlock, const pthread_rwlockattr_t *attr);
    int (*rwlock_rdlock)(pthread_rwlock_t *rwlock);
    int (*rwlock_wrlock)(pthread_rwlock_t *rwlock);
    int (*rwlock_unlock)(pthread_rwlock_t *rwlock);

    /* Other used functions */
    int (*once)(pthread_once_t *once_control, void (*init_routine)(void));

    /*
     * TSD key management.  Used to handle the corner case when a thread
     * is destroyed with a context current.
     */
    int (*key_create)(pthread_key_t *key, void (*destr_function)(void *));
    int (*key_delete)(pthread_key_t key);
    int (*setspecific)(pthread_key_t key, const void *p);
    void *(*getspecific)(pthread_key_t key);
} GLVNDPthreadRealFuncs;

static GLVNDPthreadRealFuncs pthreadRealFuncs;

#define GET_MT_FUNC(obj, handle, func) do {        \
    pthreadRealFuncs.func =                        \
        (typeof(pthreadRealFuncs.func))            \
        dlsym(handle, "pthread_" #func);           \
    if (!pthreadRealFuncs.func) {                  \
        DBG_PRINTF(0, "Failed to load pthreads "   \
                   "function pthread_" #func "!"); \
        goto fail;                                 \
    }                                              \
    (obj)->func = mt_ ## func;                     \
} while (0)

#define GET_ST_FUNC(obj, func) do { \
    (obj)->func = st_ ## func;      \
} while (0)

/* Single-threaded functions */

static int st_create(glvnd_thread_t *thread, const glvnd_thread_attr_t *attr,
                     void *(*start_routine) (void *), void *arg)
{
    assert(!"Called st_create()");
    /* insufficient resources to create another thread */
    return EAGAIN;
}

static int st_join(glvnd_thread_t thread, void **retval)
{
    assert(!"Called st_join()");
    /* not a joinable thread */
    return EINVAL;
}

/*
 * There isn't a defined PTHREAD_NULL value. Since we don't really know the
 * underlying type for pthread_t, and don't actually care about the value in
 * single-threaded mode, just return something consistent.  This should be fine
 * so long as this thread ID is only used in the wrapper functions.
 */
static glvnd_thread_t st_self(void)
{
    glvnd_thread_t foo;

    memset(&foo.tid, 0, sizeof(foo.tid));
    foo.singlethreaded = 1;

    return foo;
}

static int st_equal(glvnd_thread_t t1, glvnd_thread_t t2)
{
    assert(t1.singlethreaded && t2.singlethreaded);
    return 0;
}

static int st_mutex_lock(glvnd_mutex_t *mutex)
{
    return 0;
}
static int st_mutex_unlock(glvnd_mutex_t *mutex)
{
    return 0;
}

int st_rwlock_init(glvnd_rwlock_t *rwlock, const glvnd_rwlockattr_t *attr)
{
    return 0;
}

static int st_rwlock_rdlock(glvnd_rwlock_t *rwlock)
{
    return 0;
}

static int st_rwlock_wrlock(glvnd_rwlock_t *rwlock)
{
    return 0;
}

static int st_rwlock_unlock(glvnd_rwlock_t *rwlock)
{
    return 0;
}

static int st_once(glvnd_once_t *once_control, void (*init_routine)(void))
{
    if (!once_control->done) {
        init_routine();
        once_control->done = 1;
    }
    return 0;
}

static int st_key_create(glvnd_key_t *key, void (*destr_function)(void *))
{
    return 0;
}

static int st_key_delete(glvnd_key_t key)
{
    return 0;
}

static int st_setspecific(glvnd_key_t key, const void *p)
{
    return 0;
}

static void *st_getspecific(glvnd_key_t key)
{
    return NULL;
}

/* Multi-threaded functions */

static int mt_create(glvnd_thread_t *thread, const glvnd_thread_attr_t *attr,
                     void *(*start_routine) (void *), void *arg)
{
    thread->singlethreaded = 0;
    return pthreadRealFuncs.create(&thread->tid, attr, start_routine, arg);
}

static int mt_join(glvnd_thread_t thread, void **retval)
{
    return pthreadRealFuncs.join(thread.tid, retval);
}

/*
 * There isn't a defined PTHREAD_NULL value. Since we don't really know the
 * underlying type for pthread_t, and don't actually care about the value in
 * single-threaded mode, just return something consistent.  This should be fine
 * so long as this thread ID is only used in the wrapper functions.
 */
static glvnd_thread_t mt_self(void)
{
    glvnd_thread_t foo;

    foo.tid = pthreadRealFuncs.self();
    foo.singlethreaded = 0;

    return foo;
}

static int mt_equal(glvnd_thread_t t1, glvnd_thread_t t2)
{
    return pthreadRealFuncs.equal(t1.tid, t2.tid);
}

static int mt_mutex_lock(glvnd_mutex_t *mutex)
{
    return pthreadRealFuncs.mutex_lock(mutex);
}
static int mt_mutex_unlock(glvnd_mutex_t *mutex)
{
    return pthreadRealFuncs.mutex_unlock(mutex);
}

int mt_rwlock_init(glvnd_rwlock_t *rwlock, const glvnd_rwlockattr_t *attr)
{
    return pthreadRealFuncs.rwlock_init(rwlock, attr);
}

static int mt_rwlock_rdlock(glvnd_rwlock_t *rwlock)
{
    return pthreadRealFuncs.rwlock_rdlock(rwlock);
}

static int mt_rwlock_wrlock(glvnd_rwlock_t *rwlock)
{
    return pthreadRealFuncs.rwlock_wrlock(rwlock);
}

static int mt_rwlock_unlock(glvnd_rwlock_t *rwlock)
{
    return pthreadRealFuncs.rwlock_unlock(rwlock);
}

static int mt_once(glvnd_once_t *once_control, void (*init_routine)(void))
{
    return pthreadRealFuncs.once(&once_control->once, init_routine);
}

static int mt_key_create(glvnd_key_t *key, void (*destr_function)(void *))
{
    return pthreadRealFuncs.key_create(key, destr_function);
}

static int mt_key_delete(glvnd_key_t key)
{
    return pthreadRealFuncs.key_delete(key);
}

static int mt_setspecific(glvnd_key_t key, const void *p)
{
    return pthreadRealFuncs.setspecific(key, p);
}

static void *mt_getspecific(glvnd_key_t key)
{
    return pthreadRealFuncs.getspecific(key);
}

void glvndSetupPthreads(void *dlhandle, GLVNDPthreadFuncs *funcs)
{
    char *force_st = getenv("__GL_SINGLETHREADED");

    if (force_st && atoi(force_st)) {
        goto fail;
    }

    GET_MT_FUNC(funcs, dlhandle, create);
    GET_MT_FUNC(funcs, dlhandle, join);
    GET_MT_FUNC(funcs, dlhandle, self);
    GET_MT_FUNC(funcs, dlhandle, equal);
    GET_MT_FUNC(funcs, dlhandle, mutex_lock);
    GET_MT_FUNC(funcs, dlhandle, mutex_unlock);

    // TODO: these can fall back on internal implementations
    // if they're not available in pthreads
    GET_MT_FUNC(funcs, dlhandle, rwlock_init);
    GET_MT_FUNC(funcs, dlhandle, rwlock_rdlock);
    GET_MT_FUNC(funcs, dlhandle, rwlock_wrlock);
    GET_MT_FUNC(funcs, dlhandle, rwlock_unlock);
    GET_MT_FUNC(funcs, dlhandle, once);
    GET_MT_FUNC(funcs, dlhandle, key_create);
    GET_MT_FUNC(funcs, dlhandle, key_delete);
    GET_MT_FUNC(funcs, dlhandle, setspecific);
    GET_MT_FUNC(funcs, dlhandle, getspecific);

    // Multi-threaded
    funcs->is_singlethreaded = 0;
    return;
fail:
    if (pthreadRealFuncs.create) {
        // Throw an error if we succeeded in looking up some,
        // but not all required pthreads symbols.
        assert(!"Could not load all pthreads symbols");
    }

    GET_ST_FUNC(funcs, create);
    GET_ST_FUNC(funcs, join);
    GET_ST_FUNC(funcs, self);
    GET_ST_FUNC(funcs, equal);
    GET_ST_FUNC(funcs, mutex_lock);
    GET_ST_FUNC(funcs, mutex_unlock);
    GET_ST_FUNC(funcs, rwlock_init);
    GET_ST_FUNC(funcs, rwlock_rdlock);
    GET_ST_FUNC(funcs, rwlock_wrlock);
    GET_ST_FUNC(funcs, rwlock_unlock);
    GET_ST_FUNC(funcs, once);
    GET_ST_FUNC(funcs, key_create);
    GET_ST_FUNC(funcs, key_delete);
    GET_ST_FUNC(funcs, setspecific);
    GET_ST_FUNC(funcs, getspecific);


    // Single-threaded
    funcs->is_singlethreaded = 1;
}
