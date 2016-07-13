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

/* For RTLD_DEFAULT on x86 systems */
#define _GNU_SOURCE 1

#include <dlfcn.h>
#include <assert.h>
#include <string.h>
#include <stdlib.h>

#include "trace.h"
#include "glvnd_pthread.h"

GLVNDPthreadFuncs __glvndPthreadFuncs = {};

const glvnd_thread_t GLVND_THREAD_NULL = GLVND_THREAD_NULL_INIT;

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
    int (*mutex_init)(pthread_mutex_t *mutex, const pthread_mutexattr_t *attr);
    int (*mutex_destroy)(pthread_mutex_t *mutex);
    int (*mutex_lock)(pthread_mutex_t *mutex);
    int (*mutex_trylock)(pthread_mutex_t *mutex);
    int (*mutex_unlock)(pthread_mutex_t *mutex);

    int (* mutexattr_init) (pthread_mutexattr_t *attr);
    int (* mutexattr_destroy) (pthread_mutexattr_t *attr);
    int (* mutexattr_settype) (pthread_mutexattr_t *attr, int kind);

#if defined(HAVE_PTHREAD_RWLOCK_T)
    int (*rwlock_init)(pthread_rwlock_t *rwlock, const pthread_rwlockattr_t *attr);
    int (*rwlock_destroy)(pthread_rwlock_t *rwlock);
    int (*rwlock_rdlock)(pthread_rwlock_t *rwlock);
    int (*rwlock_wrlock)(pthread_rwlock_t *rwlock);
    int (*rwlock_tryrdlock)(pthread_rwlock_t *rwlock);
    int (*rwlock_trywrlock)(pthread_rwlock_t *rwlock);
    int (*rwlock_unlock)(pthread_rwlock_t *rwlock);
#endif

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

#if defined(HAVE_PTHREAD_RWLOCK_T)
# define GET_MT_RWLOCK_FUNC(obj, handle, func) \
    GET_MT_FUNC(obj, handle, func)
#else
# define GET_MT_RWLOCK_FUNC(obj, handle, func) \
    (obj)->func = mt_ ## func;
#endif

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
    foo.valid = 1;

    return foo;
}

static int st_equal(glvnd_thread_t t1, glvnd_thread_t t2)
{
    return (t1.valid == t2.valid);
}

static int st_mutex_init(glvnd_mutex_t *mutex, const glvnd_mutexattr_t *attr)
{
    return 0;
}

static int st_mutex_destroy(glvnd_mutex_t *mutex)
{
    return 0;
}

static int st_mutex_lock(glvnd_mutex_t *mutex)
{
    return 0;
}

static int st_mutex_trylock(glvnd_mutex_t *mutex)
{
    return 0;
}

static int st_mutex_unlock(glvnd_mutex_t *mutex)
{
    return 0;
}

int st_mutexattr_init(glvnd_mutexattr_t *attr)
{
    return 0;
}

int st_mutexattr_destroy(glvnd_mutexattr_t *attr)
{
    return 0;
}

int st_mutexattr_settype(glvnd_mutexattr_t *attr, int kind)
{
    return 0;
}

int st_rwlock_init(glvnd_rwlock_t *rwlock, const glvnd_rwlockattr_t *attr)
{
    return 0;
}

int st_rwlock_destroy(glvnd_rwlock_t *rwlock)
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

static int st_rwlock_tryrdlock(glvnd_rwlock_t *rwlock)
{
    return 0;
}

static int st_rwlock_trywrlock(glvnd_rwlock_t *rwlock)
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
    key->data = malloc(sizeof(void *));
    if (key->data == NULL)
    {
        return ENOMEM;
    }
    *(key->data) = NULL;
    return 0;
}

static int st_key_delete(glvnd_key_t key)
{
    free(key.data);
    return 0;
}

static int st_setspecific(glvnd_key_t key, const void *p)
{
    (*key.data) = (void *) p;
    return 0;
}

static void *st_getspecific(glvnd_key_t key)
{
    return *(key.data);
}

/* Multi-threaded functions */

static int mt_create(glvnd_thread_t *thread, const glvnd_thread_attr_t *attr,
                     void *(*start_routine) (void *), void *arg)
{
    int rv;

    rv = pthreadRealFuncs.create(&thread->tid, attr, start_routine, arg);
    thread->valid = (rv == 0 ? 1 : 0);
    return rv;
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
    foo.valid = 1;

    return foo;
}

static int mt_equal(glvnd_thread_t t1, glvnd_thread_t t2)
{
    return (!t1.valid && !t2.valid) ||
            (t1.valid && t2.valid && pthreadRealFuncs.equal(t1.tid, t2.tid));
}

static int mt_mutex_init(glvnd_mutex_t *mutex, const glvnd_mutexattr_t *attr)
{
    return pthreadRealFuncs.mutex_init(mutex, attr);
}

static int mt_mutex_destroy(glvnd_mutex_t *mutex)
{
    return pthreadRealFuncs.mutex_destroy(mutex);
}

static int mt_mutex_lock(glvnd_mutex_t *mutex)
{
    return pthreadRealFuncs.mutex_lock(mutex);
}

static int mt_mutex_trylock(glvnd_mutex_t *mutex)
{
    return pthreadRealFuncs.mutex_trylock(mutex);
}

static int mt_mutex_unlock(glvnd_mutex_t *mutex)
{
    return pthreadRealFuncs.mutex_unlock(mutex);
}

int mt_mutexattr_init(glvnd_mutexattr_t *attr)
{
    return pthreadRealFuncs.mutexattr_init(attr);
}

int mt_mutexattr_destroy(glvnd_mutexattr_t *attr)
{
    return pthreadRealFuncs.mutexattr_destroy(attr);
}

int mt_mutexattr_settype(glvnd_mutexattr_t *attr, int kind)
{
    return pthreadRealFuncs.mutexattr_settype(attr, kind);
}

static int mt_rwlock_init(glvnd_rwlock_t *rwlock, const glvnd_rwlockattr_t *attr)
{
#if defined(HAVE_PTHREAD_RWLOCK_T)
    return pthreadRealFuncs.rwlock_init(rwlock, attr);
#else
    return pthreadRealFuncs.mutex_init(rwlock, attr);
#endif
}

static int mt_rwlock_destroy(glvnd_rwlock_t *rwlock)
{
#if defined(HAVE_PTHREAD_RWLOCK_T)
    return pthreadRealFuncs.rwlock_destroy(rwlock);
#else
    return pthreadRealFuncs.mutex_destroy(rwlock);
#endif
}

static int mt_rwlock_rdlock(glvnd_rwlock_t *rwlock)
{
#if defined(HAVE_PTHREAD_RWLOCK_T)
    return pthreadRealFuncs.rwlock_rdlock(rwlock);
#else
    return pthreadRealFuncs.mutex_lock(rwlock);
#endif
}

static int mt_rwlock_wrlock(glvnd_rwlock_t *rwlock)
{
#if defined(HAVE_PTHREAD_RWLOCK_T)
    return pthreadRealFuncs.rwlock_wrlock(rwlock);
#else
    return pthreadRealFuncs.mutex_lock(rwlock);
#endif
}

static int mt_rwlock_tryrdlock(glvnd_rwlock_t *rwlock)
{
#if defined(HAVE_PTHREAD_RWLOCK_T)
    return pthreadRealFuncs.rwlock_tryrdlock(rwlock);
#else
    return pthreadRealFuncs.mutex_trylock(rwlock);
#endif
}

static int mt_rwlock_trywrlock(glvnd_rwlock_t *rwlock)
{
#if defined(HAVE_PTHREAD_RWLOCK_T)
    return pthreadRealFuncs.rwlock_trywrlock(rwlock);
#else
    return pthreadRealFuncs.mutex_trylock(rwlock);
#endif
}

static int mt_rwlock_unlock(glvnd_rwlock_t *rwlock)
{
#if defined(HAVE_PTHREAD_RWLOCK_T)
    return pthreadRealFuncs.rwlock_unlock(rwlock);
#else
    return pthreadRealFuncs.mutex_unlock(rwlock);
#endif
}

static int mt_once(glvnd_once_t *once_control, void (*init_routine)(void))
{
    return pthreadRealFuncs.once(&once_control->once, init_routine);
}

static int mt_key_create(glvnd_key_t *key, void (*destr_function)(void *))
{
    return pthreadRealFuncs.key_create(&key->key, destr_function);
}

static int mt_key_delete(glvnd_key_t key)
{
    return pthreadRealFuncs.key_delete(key.key);
}

static int mt_setspecific(glvnd_key_t key, const void *p)
{
    return pthreadRealFuncs.setspecific(key.key, p);
}

static void *mt_getspecific(glvnd_key_t key)
{
    return pthreadRealFuncs.getspecific(key.key);
}

void glvndSetupPthreads(void)
{
    char *force_st = getenv("__GL_SINGLETHREADED");
    void *dlhandle = RTLD_DEFAULT;
    GLVNDPthreadFuncs *funcs = &__glvndPthreadFuncs;

    if (force_st && atoi(force_st)) {
        goto fail;
    }

    GET_MT_FUNC(funcs, dlhandle, create);
    GET_MT_FUNC(funcs, dlhandle, join);
    GET_MT_FUNC(funcs, dlhandle, self);
    GET_MT_FUNC(funcs, dlhandle, equal);
    GET_MT_FUNC(funcs, dlhandle, mutex_init);
    GET_MT_FUNC(funcs, dlhandle, mutex_destroy);
    GET_MT_FUNC(funcs, dlhandle, mutex_lock);
    GET_MT_FUNC(funcs, dlhandle, mutex_trylock);
    GET_MT_FUNC(funcs, dlhandle, mutex_unlock);
    GET_MT_FUNC(funcs, dlhandle, mutexattr_init);
    GET_MT_FUNC(funcs, dlhandle, mutexattr_destroy);
    GET_MT_FUNC(funcs, dlhandle, mutexattr_settype);


    GET_MT_RWLOCK_FUNC(funcs, dlhandle, rwlock_init);
    GET_MT_RWLOCK_FUNC(funcs, dlhandle, rwlock_destroy);
    GET_MT_RWLOCK_FUNC(funcs, dlhandle, rwlock_rdlock);
    GET_MT_RWLOCK_FUNC(funcs, dlhandle, rwlock_wrlock);
    GET_MT_RWLOCK_FUNC(funcs, dlhandle, rwlock_tryrdlock);
    GET_MT_RWLOCK_FUNC(funcs, dlhandle, rwlock_trywrlock);
    GET_MT_RWLOCK_FUNC(funcs, dlhandle, rwlock_unlock);

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
    GET_ST_FUNC(funcs, mutex_init);
    GET_ST_FUNC(funcs, mutex_destroy);
    GET_ST_FUNC(funcs, mutex_lock);
    GET_ST_FUNC(funcs, mutex_trylock);
    GET_ST_FUNC(funcs, mutex_unlock);

    GET_ST_FUNC(funcs, mutexattr_init);
    GET_ST_FUNC(funcs, mutexattr_destroy);
    GET_ST_FUNC(funcs, mutexattr_settype);

    GET_ST_FUNC(funcs, rwlock_init);
    GET_ST_FUNC(funcs, rwlock_destroy);
    GET_ST_FUNC(funcs, rwlock_rdlock);
    GET_ST_FUNC(funcs, rwlock_wrlock);
    GET_ST_FUNC(funcs, rwlock_tryrdlock);
    GET_ST_FUNC(funcs, rwlock_trywrlock);
    GET_ST_FUNC(funcs, rwlock_unlock);

    GET_ST_FUNC(funcs, once);
    GET_ST_FUNC(funcs, key_create);
    GET_ST_FUNC(funcs, key_delete);
    GET_ST_FUNC(funcs, setspecific);
    GET_ST_FUNC(funcs, getspecific);


    // Single-threaded
    funcs->is_singlethreaded = 1;
}
