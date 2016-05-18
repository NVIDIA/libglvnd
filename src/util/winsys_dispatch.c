/*
 * Copyright (c) 2016, NVIDIA CORPORATION.
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

#include "winsys_dispatch.h"

#include "glvnd_pthread.h"
#include "lkdhash.h"
#include <assert.h>

// The initial size to use when we allocate the function list. This is large
// enough to hold all of the functions defined in libGLX.
#define INITIAL_LIST_SIZE 64

typedef struct __GLVNDwinsysDispatchIndexEntryRec {
    char *name;
    void *dispatchFunc;
} __GLVNDwinsysDispatchIndexEntry;

static __GLVNDwinsysDispatchIndexEntry *dispatchIndexList = NULL;
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
        __GLVNDwinsysDispatchIndexEntry *newList;
        int newSize = dispatchIndexAllocCount * 2;
        if (newSize <= 0) {
            newSize = INITIAL_LIST_SIZE;
        }

        newList = realloc(dispatchIndexList, newSize * sizeof(__GLVNDwinsysDispatchIndexEntry));
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

