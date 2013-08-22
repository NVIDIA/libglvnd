/*
 * Copyright (c) 2013, NVIDIA CORPORATION.
 */

#include "libglxmapping.h"
#include "libglxnoop.h"

#include "uthash/src/uthash.h"

/****************************************************************************/
/*
 * Locking primitives to protect:
 *
 * __glXDispatchHash
 * __glXScreenPointerMappingHash
 * __glXScreenXIDMappingHash
 */

static inline void TakeLock(int *pLock)
{
    // XXX implement me
}


static inline void ReleaseLock(int *pLock)
{
    // XXX implement me
}


/****************************************************************************/
/*
 * __glXDispatchHash is a hash table which maps a Display+screen to a
 * vendor's __GLXdispatchTable.  Look up this mapping from the X
 * server once, the first time a unique Display+screen pair is seen.
 */
typedef struct {
    Display *dpy;
    int screen;
} __GLXdispatchHashKey;


typedef struct {
    __GLXdispatchHashKey key;
    const __GLXdispatchTable* table;
    UT_hash_handle hh;
} __GLXdispatchHash;


static __GLXdispatchHash *__glXDispatchHash = NULL;
static int __glXDispatchHashLock = 0;


const __GLXdispatchTable* __glXGetDispatch(Display *dpy, const int screen)
{
    __GLXdispatchHash *pEntry = NULL;
    __GLXdispatchHashKey key;

    if (screen < 0) {
        return __glXDispatchNoopPtr;
    }

    memset(&key, 0, sizeof(key));

    key.dpy = dpy;
    key.screen = screen;

    TakeLock(&__glXDispatchHashLock);

    HASH_FIND(hh, __glXDispatchHash, &key, sizeof(key), pEntry);

    if (pEntry == NULL) {

        // XXX send request to server to identify vendor for this X screen
        // XXX load libGLX_VENDOR.so for the identified vendor

        pEntry = malloc(sizeof(*pEntry));
        pEntry->key.dpy = dpy;
        pEntry->key.screen = screen;
        pEntry->table = __glXDispatchNoopPtr;

        HASH_ADD(hh, __glXDispatchHash, key,
                 sizeof(__GLXdispatchHashKey), pEntry);
    }

    ReleaseLock(&__glXDispatchHashLock);

    return pEntry->table;
}


const __GLXdispatchTable *__glXGetDispatchFromTLS(void)
{
    // XXX implement me
    return __glXDispatchNoopPtr;
}


/****************************************************************************/
/*
 * __glXScreenPointerMappingHash is a hash table that maps a void*
 * (either GLXContext or GLXFBConfig) to a screen index.  Note this
 * stores both GLXContext and GLXFBConfig in this table.
 */

typedef struct {
    void *ptr;
    int screen;
    UT_hash_handle hh;
} __GLXscreenPointerMappingHash;


static __GLXscreenPointerMappingHash *__glXScreenPointerMappingHash = NULL;
static int __glXScreenPointerMappingHashLock = 0;


static void AddScreenPointerMapping(void *ptr, int screen)
{
    __GLXscreenPointerMappingHash *pEntry;

    if (ptr == NULL) {
        return;
    }

    if (screen < 0) {
        return;
    }

    TakeLock(&__glXScreenPointerMappingHashLock);

    HASH_FIND(hh, __glXScreenPointerMappingHash, ptr, sizeof(ptr), pEntry);

    if (pEntry == NULL) {
        pEntry = malloc(sizeof(*pEntry));
        pEntry->ptr = ptr;
        pEntry->screen = screen;
        HASH_ADD(hh, __glXScreenPointerMappingHash, ptr, sizeof(ptr), pEntry);
    } else {
        pEntry->screen = screen;
    }

    ReleaseLock(&__glXScreenPointerMappingHashLock);
}


static void RemoveScreenPointerMapping(void *ptr, int screen)
{
    __GLXscreenPointerMappingHash *pEntry;

    if (ptr == NULL) {
        return;
    }

    if (screen < 0) {
        return;
    }

    TakeLock(&__glXScreenPointerMappingHashLock);

    HASH_FIND(hh, __glXScreenPointerMappingHash, ptr, sizeof(ptr), pEntry);

    if (pEntry != NULL) {
        HASH_DELETE(hh, __glXScreenPointerMappingHash, pEntry);
        free(pEntry);
    }

    ReleaseLock(&__glXScreenPointerMappingHashLock);
}


static int ScreenFromPointer(void *ptr)
{
    __GLXscreenPointerMappingHash *pEntry;
    int screen = -1;

    TakeLock(&__glXScreenPointerMappingHashLock);

    HASH_FIND(hh, __glXScreenPointerMappingHash, ptr, sizeof(ptr), pEntry);

    if (pEntry != NULL) {
        screen = pEntry->screen;
    }

    ReleaseLock(&__glXScreenPointerMappingHashLock);

    return screen;
}


void __glXAddScreenContextMapping(GLXContext context, int screen)
{
    AddScreenPointerMapping(context, screen);
}


void __glXRemoveScreenContextMapping(GLXContext context, int screen)
{
    RemoveScreenPointerMapping(context, screen);
}


int __glXScreenFromContext(GLXContext context)
{
    return ScreenFromPointer(context);
}


void __glXAddScreenFBConfigMapping(GLXFBConfig config, int screen)
{
    AddScreenPointerMapping(config, screen);
}


void __glXRemoveScreenFBConfigMapping(GLXFBConfig config, int screen)
{
    RemoveScreenPointerMapping(config, screen);
}


int __glXScreenFromFBConfig(GLXFBConfig config)
{
    return ScreenFromPointer(config);
}




/****************************************************************************/
/*
 * __glXScreenXIDMappingHash is a hash table which maps XIDs to screens.
 */


typedef struct {
    XID xid;
    int screen;
    UT_hash_handle hh;
} __GLXscreenXIDMappingHash;


static __GLXscreenXIDMappingHash *__glXScreenXIDMappingHash = NULL;
static int __glXScreenXIDMappingHashLock = 0;


static void AddScreenXIDMapping(XID xid, int screen)
{
    __GLXscreenXIDMappingHash *pEntry = NULL;

    if (xid == None) {
        return;
    }

    if (screen < 0) {
        return;
    }

    TakeLock(&__glXScreenXIDMappingHashLock);

    HASH_FIND(hh, __glXScreenXIDMappingHash, &xid, sizeof(xid), pEntry);

    if (pEntry == NULL) {
        pEntry = malloc(sizeof(*pEntry));
        pEntry->xid = xid;
        pEntry->screen = screen;
        HASH_ADD(hh, __glXScreenXIDMappingHash, xid, sizeof(xid), pEntry);
    } else {
        pEntry->screen = screen;
    }

    ReleaseLock(&__glXScreenXIDMappingHashLock);
}


static void RemoveScreenXIDMapping(XID xid, int screen)
{
    __GLXscreenXIDMappingHash *pEntry;

    if (xid == None) {
        return;
    }

    if (screen < 0) {
        return;
    }

    TakeLock(&__glXScreenXIDMappingHashLock);

    HASH_FIND(hh, __glXScreenXIDMappingHash, &xid, sizeof(xid), pEntry);

    if (pEntry != NULL) {
        HASH_DELETE(hh, __glXScreenXIDMappingHash, pEntry);
        free(pEntry);
    }

    ReleaseLock(&__glXScreenXIDMappingHashLock);
}


static int ScreenFromXID(XID xid)
{
    __GLXscreenXIDMappingHash *pEntry;
    int screen = -1;

    TakeLock(&__glXScreenXIDMappingHashLock);

    HASH_FIND(hh, __glXScreenXIDMappingHash, &xid, sizeof(xid), pEntry);

    if (pEntry != NULL) {
        screen = pEntry->screen;
    }

    ReleaseLock(&__glXScreenXIDMappingHashLock);

    return screen;
}


void __glXAddScreenDrawableMapping(GLXDrawable drawable, int screen)
{
    AddScreenXIDMapping(drawable, screen);
}


void __glXRemoveScreenDrawableMapping(GLXDrawable drawable, int screen)
{
    RemoveScreenXIDMapping(drawable, screen);
}


int __glXScreenFromDrawable(GLXDrawable drawable)
{
    return ScreenFromXID(drawable);
}
