#ifndef __LKDHASH_H__
#define __LKDHASH_H__

// This is intended to be used in conjunction with uthash and libglvnd_pthread.
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

#define LKDHASH_INIT(imp, _lockedhash) do {               \
    (_lockedhash).hash = NULL;                            \
    (imp).rwlock_init(&(_lockedhash).lock, NULL);         \
} while (0)

/*
 * Macros for locking/unlocking the locked hash.
 */
#define LKDHASH_RDLOCK(imp, _lockedhash) \
    (imp).rwlock_rdlock(&(_lockedhash).lock)
#define LKDHASH_WRLOCK(imp, _lockedhash) \
    (imp).rwlock_wrlock(&(_lockedhash).lock)
#define LKDHASH_UNLOCK(imp, _lockedhash) \
    (imp).rwlock_unlock(&(_lockedhash).lock)

/*
 * Converts a locked hash into a hash suitable for use with uthash.
 */
#define _LH(_lockedhash) ((_lockedhash).hash)

#endif
