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

#if !defined(STATIC_DISPATCH_ONLY)
#include "u_execmem.h"
#endif

#define ARRAY_SIZE(x) (sizeof(x)/sizeof((x)[0]))
#define MAPI_LAST_SLOT (MAPI_TABLE_NUM_STATIC + MAPI_TABLE_NUM_DYNAMIC - 1)

struct mapi_stub {
    /*!
     * The name of the stub function.
     */
    const char *name;

    int slot;
    mapi_func addr;

    /**
     * A buffer to store the name of the function. This is only used for
     * dynamic stubs. For static stubs, mapi_stub::name is a static
     * string and mapi_stub::nameBuffer is NULL.
     */
    char *nameBuffer;
};

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
const struct mapi_stub *
stub_find_public(const char *name)
{
    // All of the function names start with "gl", so skip that prefix when
    // comparing names.
    if (name[0] == 'g' && name[1] == 'l') {
        name += 2;
    }

   return (const struct mapi_stub *) bsearch(name, public_stubs,
         ARRAY_SIZE(public_stubs), sizeof(public_stubs[0]), stub_compare);
}

#if !defined(STATIC_DISPATCH_ONLY)
static struct mapi_stub dynamic_stubs[MAPI_TABLE_NUM_DYNAMIC];
static int num_dynamic_stubs;

void stub_cleanup_dynamic(void)
{
    int i;

    // Free the copies of the stub names.
    for (i=0; i<num_dynamic_stubs; i++) {
        struct mapi_stub *stub = &dynamic_stubs[i];
        free(stub->nameBuffer);
        stub->nameBuffer = NULL;
    }

    num_dynamic_stubs = 0;
    u_execmem_free();
}

/**
 * Add a dynamic stub.
 */
static struct mapi_stub *
stub_add_dynamic(const char *name)
{
   struct mapi_stub *stub;
   int idx;

   idx = num_dynamic_stubs;
   /* minus 1 to make sure we can never reach the last slot */
   if (idx >= MAPI_TABLE_NUM_DYNAMIC - 1)
      return NULL;

   stub = &dynamic_stubs[idx];

   /*
    * name is the pointer passed to glXGetProcAddress, so the caller may free
    * or modify it later. Allocate a copy of the name to store.
    */
   stub->nameBuffer = strdup(name);
   if (stub->nameBuffer == NULL) {
       return NULL;
   }

   /* Assign the next unused slot. */
   stub->slot = MAPI_TABLE_NUM_STATIC + idx;
   stub->addr = entry_generate(stub->slot);
   if (!stub->addr) {
      free(stub->nameBuffer);
      stub->nameBuffer = NULL;
      return NULL;
   }
   stub->name = stub->nameBuffer;

   num_dynamic_stubs = idx + 1;

   return stub;
}

/**
 * Return the dynamic stub with the given name.  If no such stub exists and
 * generate is true, a new stub is generated.
 */
struct mapi_stub *
stub_find_dynamic(const char *name, int generate)
{
   struct mapi_stub *stub = NULL;
   int count, i;
   
   if (generate)
      assert(!stub_find_public(name));

   count = num_dynamic_stubs;
   for (i = 0; i < count; i++) {
      if (strcmp(name, dynamic_stubs[i].name) == 0) {
         stub = &dynamic_stubs[i];
         break;
      }
   }

   /* generate a dynamic stub */
   if (generate && !stub)
         stub = stub_add_dynamic(name);

   return stub;
}

const struct mapi_stub *
stub_find_by_slot(int slot)
{
    assert(slot >= 0);

    if (slot < ARRAY_SIZE(public_stubs)) {
        return &public_stubs[slot];
    } else if (slot - ARRAY_SIZE(public_stubs) < num_dynamic_stubs) {
        return &dynamic_stubs[slot - ARRAY_SIZE(public_stubs)];
    } else {
        return NULL;
    }
}

/**
 * Return the name of a stub.
 */
const char *
stub_get_name(const struct mapi_stub *stub)
{
   return stub->name;
}

int stub_get_count(void)
{
    return ARRAY_SIZE(public_stubs) + num_dynamic_stubs;
}

#endif // !defined(STATIC_DISPATCH_ONLY)

/**
 * Return the slot of a stub.
 */
int
stub_get_slot(const struct mapi_stub *stub)
{
   return stub->slot;
}

/**
 * Return the address of a stub.
 */
mapi_func
stub_get_addr(const struct mapi_stub *stub)
{
   assert(stub->addr || (unsigned int) stub->slot < MAPI_TABLE_NUM_STATIC);
   if (stub->addr != NULL)
   {
      return stub->addr;
   }
   else
   {
      int index = stub - public_stubs;
      return entry_get_public(index);
   }
}

static int stub_allow_override(void)
{
    return !!entry_stub_size;
}

static GLboolean stubStartPatch(void)
{
    if (!stub_allow_override()) {
        return GL_FALSE;
    }

    if (!entry_patch_start()) {
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
    int i, slot;
    const struct mapi_stub *stub;

    assert(stub_allow_override());

    for (stub = public_stubs, i = 0;
         i < ARRAY_SIZE(public_stubs);
         stub++, i++) {
        slot = (stub->slot == -1) ? MAPI_LAST_SLOT : stub->slot;
        entry_generate_default_code((char *)stub_get_addr(stub), slot);
    }

#if !defined(STATIC_DISPATCH_ONLY)
    for (stub = dynamic_stubs, i = 0;
         i < num_dynamic_stubs;
         stub++, i++) {
        slot = (stub->slot == -1) ? MAPI_LAST_SLOT : stub->slot;
        entry_generate_default_code((char *)stub_get_addr(stub), slot);
    }
#endif // !defined(STATIC_DISPATCH_ONLY)
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
    const struct mapi_stub *stub;
    void *writeAddr = NULL;
    const void *execAddr = NULL;

    stub = stub_find_public(name);

#if !defined(STATIC_DISPATCH_ONLY)
    if (!stub) {
        stub = stub_find_dynamic(name, 0);
    }
#endif // !defined(STATIC_DISPATCH_ONLY)

    if (stub) {
        mapi_func addr = stub_get_addr(stub);
        if (addr != NULL) {
            entry_get_patch_addresses(addr, &writeAddr, &execAddr);
        }
    }

    if (writePtr != NULL) {
        *writePtr = writeAddr;
    }
    if (execPtr != NULL) {
        *execPtr = execAddr;
    }

    return ((writeAddr != NULL && execAddr != NULL) ? GL_TRUE : GL_FALSE);
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

