#ifndef __LKDHASH_H__
#define __LKDHASH_H__

/**
 * \file
 *
 * Macros to implement a hashtable protected by a rwlock.
 *
 * With the exception of \c DEFINE_LKDHASH and \c DEFINE_INITIALIZED_LKDHASH,
 * these macros all use the pthreads function table in glvnd_pthread.h, so you
 * have to call \c glvndSetupPthreads before using them.
 */

#include "glvnd_pthread.h"
#include "uthash.h"

/*
 * Macros for defining a "locked hash": a hashtable protected by a lock.
 */
#define DEFINE_LKDHASH(_hashtype, _hashname)              \
    struct {                                              \
        _hashtype *hash;                                  \
        glvnd_rwlock_t lock;                              \
    } _hashname

#define DEFINE_INITIALIZED_LKDHASH(_hashtype, _hashname)  \
    struct {                                              \
        _hashtype *hash;                                  \
        glvnd_rwlock_t lock;                              \
    } _hashname = { NULL, GLVND_RWLOCK_INITIALIZER }

#define LKDHASH_INIT(_lockedhash) do {               \
    (_lockedhash).hash = NULL;                            \
    __glvndPthreadFuncs.rwlock_init(&(_lockedhash).lock, NULL);         \
} while (0)

/*
 * Macros for locking/unlocking the locked hash.
 */
#define LKDHASH_RDLOCK(_lockedhash) \
    __glvndPthreadFuncs.rwlock_rdlock(&(_lockedhash).lock)
#define LKDHASH_WRLOCK(_lockedhash) \
    __glvndPthreadFuncs.rwlock_wrlock(&(_lockedhash).lock)
#define LKDHASH_UNLOCK(_lockedhash) \
    __glvndPthreadFuncs.rwlock_unlock(&(_lockedhash).lock)

/*
 * Converts a locked hash into a hash suitable for use with uthash.
 */
#define _LH(_lockedhash) ((_lockedhash).hash)

#define LKDHASH_TEARDOWN_2(_lockedhash, _param, _cur, _tmp, _cleanup) do { \
    LKDHASH_WRLOCK(_lockedhash);                                           \
    HASH_ITER(hh, _LH( _lockedhash), _cur, _tmp) {                              \
        HASH_DEL(_LH(_lockedhash), _cur);                                       \
        if (_cleanup) {                                                         \
            _cleanup(_param, _cur);                                             \
        }                                                                       \
        free(_cur);                                                             \
    }                                                                           \
    assert(!_LH(_lockedhash));                                                  \
    LKDHASH_UNLOCK(_lockedhash);                                           \
} while (0)

/*!
 * Macro for deleting all entries in a locked hash, as well as the protecting
 * lock.  Assumes that hash entries have been allocated using malloc(3) or
 * similar and are safe to pass into free(3).
 *
 * _ht indicates the type of the hash table to use, and _lh indicates the
 * hash table variable.
 *
 * _cleanup is a callback function which takes (_void *, _ht *) as arguments,
 * or NULL.
 *
 * _param is an extra parmeter to pass into the callback function of type
 * (void *).
 *
 * _reset indicates whether the lock needs to be re-initialized (for fork
 * handling).
 */
#define LKDHASH_TEARDOWN(_ht, _lh, _cleanup, _param, _reset) do { \
    _ht *cur ## _ht, *tmp ## _ht;                                      \
    typedef void (*pfnCleanup ## _ht)(void *p, _ht *h);                \
    pfnCleanup ## _ht pCleanup ## _ht = _cleanup;                      \
    LKDHASH_TEARDOWN_2(_lh, _param, cur ## _ht,                   \
                       tmp ## _ht, pCleanup ## _ht);                   \
    if (_reset) {                                                      \
        __glvndPthreadFuncs.rwlock_init(&(_lh).lock, NULL);                          \
    } else {                                                           \
        __glvndPthreadFuncs.rwlock_destroy(&(_lh).lock);                             \
    }                                                                  \
} while (0)

#endif
