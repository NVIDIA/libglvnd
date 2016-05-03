#include "winsys_dispatch.h"

#include "glvnd_pthread.h"
#include "lkdhash.h"
#include <assert.h>

// The initial size to use when we allocate the function list. This is large
// enough to hold all of the functions defined in libGLX.
#define INITIAL_LIST_SIZE 64

typedef struct __GLVNDwinsysDispatchIndexHashRec {
    char *name;
    void *dispatchFunc;
} __GLVNDwinsysDispatchIndexHash;

static __GLVNDwinsysDispatchIndexHash *dispatchIndexList = NULL;
static int dispatchIndexCount = 0;
static int dispatchIndexAllocCount = 0;

void __glvndWinsysDispatchInit(void)
{
    // Nothing to do.
}

void __glvndWinsysDispatchCleanup(void)
{
    int i;

    for (i=0; i<dispatchIndexCount; i++) {
        free(dispatchIndexList[i].name);
    }
    free(dispatchIndexList);
    dispatchIndexList = NULL;
    dispatchIndexCount = dispatchIndexAllocCount = 0;
}


int __glvndWinsysDispatchFindIndex(const char *name)
{
    int i;

    for (i=0; i<dispatchIndexCount; i++) {
        if (strcmp(dispatchIndexList[i].name, name) == 0) {
            return i;
        }
    }

    return -1;
}

int __glvndWinsysDispatchAllocIndex(const char *name, void *dispatch)
{
    assert(__glvndWinsysDispatchFindIndex(name) < 0);

    if (dispatchIndexCount == dispatchIndexAllocCount) {
        __GLVNDwinsysDispatchIndexHash *newList;
        int newSize = dispatchIndexAllocCount * 2;
        if (newSize <= 0) {
            newSize = INITIAL_LIST_SIZE;
        }

        newList = realloc(dispatchIndexList, newSize * sizeof(__GLVNDwinsysDispatchIndexHash));
        if (newList == NULL) {
            return -1;
        }

        dispatchIndexList = newList;
        dispatchIndexAllocCount = newSize;
    }

    dispatchIndexList[dispatchIndexCount].name = strdup(name);
    if (dispatchIndexList[dispatchIndexCount].name == NULL) {
        return -1;
    }

    dispatchIndexList[dispatchIndexCount].dispatchFunc = dispatch;
    return dispatchIndexCount++;
}

const char *__glvndWinsysDispatchGetName(int index)
{
    if (index >= 0 && index < dispatchIndexCount) {
        return dispatchIndexList[index].name;
    } else {
        return NULL;
    }
}

void *__glvndWinsysDispatchGetDispatch(int index)
{
    if (index >= 0 && index < dispatchIndexCount) {
        return dispatchIndexList[index].dispatchFunc;
    } else {
        return NULL;
    }
}

int __glvndWinsysDispatchGetCount(void)
{
    return dispatchIndexCount;
}


typedef struct __GLVNDwinsysDispatchFuncHashRec {
    int index;
    void *implFunc;
    UT_hash_handle hh;
} __GLVNDwinsysDispatchFuncHash;

struct __GLVNDwinsysVendorDispatchRec {
    DEFINE_LKDHASH(__GLVNDwinsysDispatchFuncHash, table);
};

__GLVNDwinsysVendorDispatch *__glvndWinsysVendorDispatchCreate(void)
{
    __GLVNDwinsysVendorDispatch *table = (__GLVNDwinsysVendorDispatch *)
        malloc(sizeof(__GLVNDwinsysVendorDispatch));
    if (table == NULL) {
        return NULL;
    }

    LKDHASH_INIT(table->table);
    return table;
}

void __glvndWinsysVendorDispatchDestroy(__GLVNDwinsysVendorDispatch *table)
{
    if (table != NULL) {
        LKDHASH_TEARDOWN(__GLVNDwinsysDispatchFuncHash,
                         table->table, NULL, NULL, 0);
        free(table);
    }
}

int __glvndWinsysVendorDispatchAddFunc(__GLVNDwinsysVendorDispatch *table, int index, void *func)
{
    __GLVNDwinsysDispatchFuncHash *entry;

    LKDHASH_WRLOCK(table->table);
    HASH_FIND_INT(_LH(table->table), &index, entry);
    if (entry == NULL) {
        entry = (__GLVNDwinsysDispatchFuncHash *) malloc(sizeof(__GLVNDwinsysDispatchFuncHash));
        if (entry == NULL) {
            LKDHASH_UNLOCK(table->table);
            return -1;
        }

        entry->index = index;
        HASH_ADD_INT(_LH(table->table), index, entry);
    }
    entry->implFunc = func;
    LKDHASH_UNLOCK(table->table);
    return 0;
}

void *__glvndWinsysVendorDispatchLookupFunc(__GLVNDwinsysVendorDispatch *table, int index)
{
    __GLVNDwinsysDispatchFuncHash *entry;
    void *func;

    LKDHASH_RDLOCK(table->table);
    HASH_FIND_INT(_LH(table->table), &index, entry);
    if (entry != NULL) {
        func = entry->implFunc;
    } else {
        func = NULL;
    }
    LKDHASH_UNLOCK(table->table);

    return func;
}

