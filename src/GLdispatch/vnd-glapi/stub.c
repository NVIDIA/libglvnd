/*
 * Mesa 3-D graphics library
 *
 * Copyright (C) 2010 LunarG Inc.
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
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 *
 * Authors:
 *    Chia-I Wu <olv@lunarg.com>
 */

#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include "u_current.h"
#include "entry.h"
#include "stub.h"
#include "table.h"
#include "utils_misc.h"

#if !defined(STATIC_DISPATCH_ONLY)
static void stub_cleanup_dynamic(void);
#endif

struct mapi_stub {
    /*!
     * The name of the stub function.
     */
    const char *name;

    int slot;
};

static void *savedEntrypoints = NULL;

/* define public_stubs */
#define MAPI_TMP_PUBLIC_STUBS
#include "mapi_tmp.h"

static int
stub_compare(const void *key, const void *elem)
{
   const char *name = (const char *) key;
   const struct mapi_stub *stub = (const struct mapi_stub *) elem;
   const char *stub_name;

   // Skip the "gl" prefix.
   stub_name = stub->name + 2;

   return strcmp(name, stub_name);
}

/**
 * Return the public stub with the given name.
 */
int
stub_find_public(const char *name)
{
    const struct mapi_stub *stub;

    // All of the function names start with "gl", so skip that prefix when
    // comparing names.
    if (name[0] == 'g' && name[1] == 'l') {
        name += 2;
    }

    stub = (const struct mapi_stub *) bsearch(name, public_stubs,
            ARRAY_LEN(public_stubs), sizeof(public_stubs[0]), stub_compare);
    if (stub != NULL) {
        return (stub - public_stubs);
    } else {
        return -1;
    }
}

void stub_cleanup(void)
{
    free(savedEntrypoints);
    savedEntrypoints = NULL;

#if !defined(STATIC_DISPATCH_ONLY)
    stub_cleanup_dynamic();
#endif
}

#if !defined(STATIC_DISPATCH_ONLY)
static char *dynamic_stub_names[MAPI_TABLE_NUM_DYNAMIC];
static int num_dynamic_stubs;

void stub_cleanup_dynamic(void)
{
    int i;

    // Free the copies of the stub names.
    for (i=0; i<num_dynamic_stubs; i++) {
        free(dynamic_stub_names[i]);
        dynamic_stub_names[i] = NULL;
    }

    num_dynamic_stubs = 0;
}

/**
 * Add a dynamic stub.
 */
int
stub_add_dynamic(const char *name)
{
   int idx;

   idx = num_dynamic_stubs;
   if (idx >= MAPI_TABLE_NUM_DYNAMIC)
      return -1;

   // Make sure that we have a dispatch stub for this index. If the stubs are
   // in C instead of assembly, then we can't use dynamic dispatch stubs, and
   // entry_get_public will return NULL.
   if (entry_get_public(MAPI_TABLE_NUM_STATIC + idx) == NULL) {
       return -1;
   }

   assert(dynamic_stub_names[idx] == NULL);

   /*
    * name is the pointer passed to glXGetProcAddress, so the caller may free
    * or modify it later. Allocate a copy of the name to store.
    */
   dynamic_stub_names[idx] = strdup(name);
   if (dynamic_stub_names[idx] == NULL) {
       return -1;
   }

   num_dynamic_stubs = idx + 1;

   return (MAPI_TABLE_NUM_STATIC + idx);
}

/**
 * Return the dynamic stub with the given name.  If no such stub exists and
 * generate is true, a new stub is generated.
 */
int
stub_find_dynamic(const char *name, int generate)
{
    int found = -1;
    int i;
   
    if (generate) {
        assert(stub_find_public(name) < 0);
    }

    for (i = 0; i < num_dynamic_stubs; i++) {
        if (strcmp(name, dynamic_stub_names[i]) == 0) {
            found = MAPI_TABLE_NUM_STATIC + i;
            break;
        }
    }

    /* generate a dynamic stub */
    if (generate && found < 0) {
        found = stub_add_dynamic(name);
    }

    return found;
}

/**
 * Return the name of a stub.
 */
const char *
stub_get_name(int index)
{
    if (index < MAPI_TABLE_NUM_STATIC) {
        return public_stubs[index].name;
    } else {
        return dynamic_stub_names[index - MAPI_TABLE_NUM_STATIC];
    }
}

int stub_get_count(void)
{
    return ARRAY_LEN(public_stubs) + num_dynamic_stubs;
}

/**
 * Return the address of a stub.
 */
mapi_func
stub_get_addr(int index)
{
    return entry_get_public(index);
}
#endif // !defined(STATIC_DISPATCH_ONLY)

static int stub_allow_override(void)
{
    return !!entry_stub_size;
}

static GLboolean stubStartPatch(void)
{
    assert(savedEntrypoints == NULL);

    if (!stub_allow_override()) {
        return GL_FALSE;
    }

    savedEntrypoints = entry_save_entrypoints();
    if (savedEntrypoints == NULL) {
        return GL_FALSE;
    }

    if (!entry_patch_start()) {
        free(savedEntrypoints);
        savedEntrypoints = NULL;
        return GL_FALSE;
    }

    return GL_TRUE;
}

static void stubFinishPatch(void)
{
    entry_patch_finish();
}

static void stubRestoreFuncsInternal(void)
{
    assert(savedEntrypoints != NULL);

    assert(stub_allow_override());

    entry_restore_entrypoints(savedEntrypoints);
    free(savedEntrypoints);
    savedEntrypoints = NULL;
}

static GLboolean stubRestoreFuncs(void)
{
    if (entry_patch_start()) {
        stubRestoreFuncsInternal();
        entry_patch_finish();
        return GL_TRUE;
    } else {
        return GL_FALSE;
    }
}

static void stubAbortPatch(void)
{
    stubRestoreFuncsInternal();
    entry_patch_finish();
}

static GLboolean stubGetPatchOffset(const char *name, void **writePtr, const void **execPtr)
{
    int index;
    void *addr = NULL;

    index = stub_find_public(name);

#if !defined(STATIC_DISPATCH_ONLY)
    if (index < 0) {
        index = stub_find_dynamic(name, 0);
    }
#endif // !defined(STATIC_DISPATCH_ONLY)

    if (index >= 0) {
        addr = entry_get_patch_address(index);
    }

    if (writePtr != NULL) {
        *writePtr = addr;
    }
    if (execPtr != NULL) {
        *execPtr = addr;
    }

    return (addr != NULL ? GL_TRUE : GL_FALSE);
}

static int stubGetStubType(void)
{
    return entry_type;
}

static int stubGetStubSize(void)
{
    return entry_stub_size;
}

static const __GLdispatchStubPatchCallbacks stubPatchCallbacks =
{
    stubStartPatch,     // startPatch
    stubFinishPatch,    // finishPatch
    stubAbortPatch,     // abortPatch
    stubRestoreFuncs,   // restoreFuncs
    stubGetPatchOffset, // getPatchOffset
    stubGetStubType,    // getStubType
    stubGetStubSize,    // getStubSize
};

const __GLdispatchStubPatchCallbacks *stub_get_patch_callbacks(void)
{
    if (stub_allow_override())
    {
        return &stubPatchCallbacks;
    }
    else
    {
        return NULL;
    }
}

