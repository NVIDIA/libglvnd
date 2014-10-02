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

#include <X11/Xlibint.h>

#include <pthread.h>
#include <dlfcn.h>

#if defined(HASH_DEBUG)
# include <stdio.h>
#endif

#include "libglxcurrent.h"
#include "libglxmapping.h"
#include "libglxnoop.h"
#include "libglxthread.h"
#include "trace.h"

#include "lkdhash.h"
#include "x11glvnd.h"

#define _GNU_SOURCE 1

/*
 * Hash table containing a mapping from dispatch table index entries to
 * entry point names. This is used in  __glXFetchDispatchEntry() to query
 * the appropriate vendor in the case where the entry hasn't been seen before
 * by this vendor.
 */
typedef struct __GLXdispatchIndexHashRec {
    int index;
    GLubyte *procName;
    UT_hash_handle hh;
} __GLXdispatchIndexHash;

static DEFINE_INITIALIZED_LKDHASH(__GLXdispatchIndexHash, __glXDispatchIndexHash);

/*
 * Monotonically-increasing number describing both the virtual "size" of the
 * dynamic dispatch table and the next unused index. Must be accessed holding
 * the __glXDispatchIndexHash lock.
 */
static int __glXNextUnusedHashIndex;

typedef struct __GLXdispatchFuncHashRec {
    int index;
    __GLXextFuncPtr addr;
    UT_hash_handle hh;
} __GLXdispatchFuncHash;

struct __GLXdispatchTableDynamicRec {
    /*
     * Hash table containing the dynamic dispatch funcs. This is used instead of
     * a flat array to avoid sparse array usage. XXX might be more performant to
     * use an array, though?
     */
    DEFINE_LKDHASH(__GLXdispatchFuncHash, hash);

    /*
     * Pointer to the vendor library info, used by __glXFetchDispatchEntry()
     */
     __GLXvendorInfo *vendor;
};

/****************************************************************************/
/*
 * __glXVendorScreenHash is a hash table which maps a Display+screen to a vendor.
 * Look up this mapping from the X server once, the first time a unique
 * Display+screen pair is seen.
 */
typedef struct {
    Display *dpy;
    int screen;
} __GLXvendorScreenHashKey;


typedef struct __GLXvendorScreenHashRec {
    __GLXvendorScreenHashKey key;
    __GLXvendorInfo *vendor;
    // XXX for performance reasons we may want to stash the dispatch tables here
    // as well
    UT_hash_handle hh;
} __GLXvendorScreenHash;

static DEFINE_INITIALIZED_LKDHASH(__GLXvendorScreenHash, __glXVendorScreenHash);

/*
 * __glXVendorNameHash is a hash table mapping a vendor name to vendor info.
 */
typedef struct __GLXvendorNameHashRec {
    const char *name;
    __GLXvendorInfo *vendor;
    UT_hash_handle hh;
} __GLXvendorNameHash;

static DEFINE_INITIALIZED_LKDHASH(__GLXvendorNameHash, __glXVendorNameHash);

static GLboolean AllocDispatchIndex(__GLXvendorInfo *vendor,
                                    const GLubyte *procName)
{
    __GLXdispatchIndexHash *pEntry = malloc(sizeof(*pEntry));
    if (!pEntry) {
        return GL_FALSE;
    }

    pEntry->procName = (GLubyte *)strdup((const char *)procName);
    if (!pEntry->procName) {
        free(pEntry);
        return GL_FALSE;
    }

    LKDHASH_WRLOCK(__glXPthreadFuncs, __glXDispatchIndexHash);
    pEntry->index = __glXNextUnusedHashIndex++;

    // Notify the vendor this is the index which should be used
    vendor->staticDispatch->
        glxvc.setDispatchIndex(procName, pEntry->index);

    HASH_ADD_INT(_LH(__glXDispatchIndexHash),
                 index, pEntry);
    LKDHASH_UNLOCK(__glXPthreadFuncs, __glXDispatchIndexHash);

    return GL_TRUE;
}

/*!
 * Callback function used when freeing the dispatch index hash table.
 */
static void CleanupDispatchIndexEntry(void *unused, __GLXdispatchIndexHash *pEntry)
{
    assert(pEntry);
    free(pEntry->procName);
}

/*!
 * This function queries each loaded vendor to determine if there is
 * a vendor-implemented dispatch function. The dispatch function
 * uses the vendor <-> API library ABI to determine the screen given
 * the parameters of the function and dispatch to the correct vendor's
 * implementation.
 */
__GLXextFuncPtr __glXGetGLXDispatchAddress(const GLubyte *procName)
{
    __GLXextFuncPtr addr = NULL;
    __GLXvendorNameHash *pEntry, *tmp;

    /*
     * XXX for full correctness, we should probably load vendors
     * on all screens up-front before doing this. However, that
     * might be bad for performance?
     *
     * A better way to solve this issue might be to tweak the ABI slightly
     * to allow vendors to provide a standalone DSO which only
     * exports vendor-neutral dispatch functions, and nothing else.
     */
    LKDHASH_RDLOCK(__glXPthreadFuncs, __glXVendorNameHash);
    HASH_ITER(hh, _LH(__glXVendorNameHash), pEntry, tmp) {
        // See if the current vendor supports this GLX entry point
        addr = pEntry->vendor->staticDispatch->
            glxvc.getDispatchAddress(procName);
        if (addr) {
            // Allocate the new dispatch index.
            if (!AllocDispatchIndex(pEntry->vendor, procName)) {
                addr = NULL;
            }
            break;
        }
    }
    LKDHASH_UNLOCK(__glXPthreadFuncs, __glXVendorNameHash);

    return addr;
}

__GLXextFuncPtr __glXFetchDispatchEntry(__GLXdispatchTableDynamic *dynDispatch,
                                        int index)
{
    __GLXextFuncPtr addr = NULL;
    __GLXdispatchFuncHash *pEntry;
    GLubyte *procName = NULL;

    LKDHASH_RDLOCK(__glXPthreadFuncs, dynDispatch->hash);

    HASH_FIND_INT(_LH(dynDispatch->hash), &index, pEntry);

    if (pEntry) {
        // This can be NULL, which indicates the vendor does not implement this
        // entry. Vendor library provided dispatch functions are expected to
        // default to a no-op in case dispatching fails.
        addr = pEntry->addr;
    }

    LKDHASH_UNLOCK(__glXPthreadFuncs, dynDispatch->hash);

    if (!pEntry) {
        // Not seen before by this vendor: query the vendor for the right
        // address to use.

        __GLXdispatchIndexHash *pdiEntry;

        // First retrieve the procname of this index
        LKDHASH_RDLOCK(__glXPthreadFuncs, __glXDispatchIndexHash);
        HASH_FIND_INT(_LH(__glXDispatchIndexHash), &index, pdiEntry);
        procName = pdiEntry->procName;
        LKDHASH_UNLOCK(__glXPthreadFuncs, __glXDispatchIndexHash);

        // This should have a valid entry point associated with it.
        assert(procName);

        if (procName) {
            // Get the real address
            addr = dynDispatch->vendor->staticDispatch->
                glxvc.getProcAddress(procName, GL_FALSE);
        }

        LKDHASH_WRLOCK(__glXPthreadFuncs, dynDispatch->hash);
        HASH_FIND_INT(_LH(dynDispatch->hash), &index, pEntry);
        if (!pEntry) {
            pEntry = malloc(sizeof(*pEntry));
            if (!pEntry) {
                // Uh-oh!
                assert(pEntry);
                LKDHASH_UNLOCK(__glXPthreadFuncs, dynDispatch->hash);
                return NULL;
            }
            pEntry->index = index;
            pEntry->addr = addr;

            HASH_ADD_INT(_LH(dynDispatch->hash), index, pEntry);
        } else {
            addr = pEntry->addr;
        }
        LKDHASH_UNLOCK(__glXPthreadFuncs, dynDispatch->hash);
    }

    return addr;
}

static __GLXapiExports glxExportsTable;
static glvnd_once_t glxExportsTableOnceControl = GLVND_ONCE_INIT;

static void InitExportsTable(void)
{
    glxExportsTable.getDynDispatch = __glXGetDynDispatch;
    glxExportsTable.getCurrentDynDispatch = __glXGetCurrentDynDispatch;
    glxExportsTable.fetchDispatchEntry = __glXFetchDispatchEntry;

    /* We use the real function since __glXGetCurrentContext is inline */
    glxExportsTable.getCurrentContext = glXGetCurrentContext;

    glxExportsTable.addScreenContextMapping = __glXAddScreenContextMapping;
    glxExportsTable.removeScreenContextMapping = __glXRemoveScreenContextMapping;
    glxExportsTable.screenFromContext = __glXScreenFromContext;

    glxExportsTable.addScreenFBConfigMapping = __glXAddScreenFBConfigMapping;
    glxExportsTable.removeScreenFBConfigMapping = __glXRemoveScreenFBConfigMapping;
    glxExportsTable.screenFromFBConfig = __glXScreenFromFBConfig;

    glxExportsTable.addScreenDrawableMapping = __glXAddScreenDrawableMapping;
    glxExportsTable.removeScreenDrawableMapping = __glXRemoveScreenDrawableMapping;
    glxExportsTable.screenFromDrawable = __glXScreenFromDrawable;

}

static char *ConstructVendorLibraryFilename(const char *vendorName)
{
    char *filename;
    int ret;

    ret = asprintf(&filename, "libGLX_%s.so.0", vendorName);

    if (ret < 0) {
        return NULL;
    }

    return filename;
}

void TeardownVendor(__GLXvendorInfo *vendor, Bool doLibraryUnload)
{
    free(vendor->name);
    if (vendor->glDispatch) {
        __glDispatchDestroyTable(vendor->glDispatch);
    }

    /* Clean up the dynamic dispatch table */
    LKDHASH_TEARDOWN(__glXPthreadFuncs, __GLXdispatchFuncHash,
                     vendor->dynDispatch->hash, NULL, NULL, True);

    free(vendor->dynDispatch);

    if (doLibraryUnload) {
        dlclose(vendor->dlhandle);
    }

    free(vendor);
}

__GLXvendorInfo *__glXLookupVendorByName(const char *vendorName)
{
    __GLXvendorNameHash *pEntry = NULL;
    void *dlhandle = NULL;
    __PFNGLXMAINPROC glxMainProc;
    const __GLXdispatchTableStatic *dispatch;
    __GLXdispatchTableDynamic *dynDispatch;
    __GLXvendorInfo *vendor = NULL;
    Bool locked = False;
    int vendorID = -1;

    LKDHASH_RDLOCK(__glXPthreadFuncs, __glXVendorNameHash);
    HASH_FIND(hh, _LH(__glXVendorNameHash), vendorName, strlen(vendorName), pEntry);

    if (pEntry) {
        vendor = pEntry->vendor;
    }
    LKDHASH_UNLOCK(__glXPthreadFuncs, __glXVendorNameHash);

    if (!pEntry) {
        LKDHASH_WRLOCK(__glXPthreadFuncs, __glXVendorNameHash);
        locked = True;
        // Do another lookup to check uniqueness
        HASH_FIND(hh, _LH(__glXVendorNameHash), vendorName, strlen(vendorName), pEntry);
        if (!pEntry) {
            char *filename;

            // Previously unseen vendor. dlopen() the new vendor and add it to the
            // hash table.
            pEntry = calloc(1, sizeof(*pEntry));
            if (!pEntry) {
                goto fail;
            }

            filename = ConstructVendorLibraryFilename(vendorName);
            if (filename) {
                dlhandle = dlopen(filename, RTLD_LAZY);
            }
            free(filename);
            if (!dlhandle) {
                goto fail;
            }

            glxMainProc = dlsym(dlhandle, __GLX_MAIN_PROTO_NAME);
            if (!glxMainProc) {
                goto fail;
            }

            /* Initialize the glxExportsTable if we haven't already */
            __glXPthreadFuncs.once(&glxExportsTableOnceControl,
                                   InitExportsTable);

            vendorID = __glDispatchNewVendorID();
            assert(vendorID >= 0);

            dispatch = (*glxMainProc)(GLX_VENDOR_ABI_VERSION,
                                      &glxExportsTable,
                                      vendorName,
                                      vendorID);
            if (!dispatch) {
                goto fail;
            }

            vendor = pEntry->vendor
                = calloc(1, sizeof(__GLXvendorInfo));
            if (!vendor) {
                goto fail;
            }

            pEntry->name = vendor->name = strdup(vendorName);
            vendor->vendorID = vendorID;

            if (!vendor->name) {
                goto fail;
            }
            vendor->dlhandle = dlhandle;
            vendor->staticDispatch = dispatch;

            vendor->glDispatch = (__GLdispatchTable *)
                __glDispatchCreateTable(
                    dispatch->glxvc.getProcAddress
                );
            if (!vendor->glDispatch) {
                goto fail;
            }

            dynDispatch = vendor->dynDispatch
                = malloc(sizeof(__GLXdispatchTableDynamic));
            if (!dynDispatch) {
                goto fail;
            }

            /* Initialize the dynamic dispatch table */
            LKDHASH_INIT(__glXPthreadFuncs, dynDispatch->hash);
            dynDispatch->vendor = vendor;

            HASH_ADD_KEYPTR(hh, _LH(__glXVendorNameHash), vendor->name,
                            strlen(vendor->name), pEntry);
        } else {
            /* Some other thread added a vendor */
            vendor = pEntry->vendor;
        }
        LKDHASH_UNLOCK(__glXPthreadFuncs, __glXVendorNameHash);
    }

    return vendor;

fail:
    if (locked) {
        LKDHASH_UNLOCK(__glXPthreadFuncs, __glXVendorNameHash);
    }
    if (dlhandle) {
        dlclose(dlhandle);
    }
    if (vendor) {
        TeardownVendor(vendor, False/* doLibraryUnload */);
    }
    free(pEntry);
    return NULL;
}

static void CleanupVendorNameEntry(void *unused,
                                   __GLXvendorNameHash *pEntry)
{
    TeardownVendor(pEntry->vendor, True/* doLibraryUnload */);
}

__GLXvendorInfo *__glXLookupVendorByScreen(Display *dpy, const int screen)
{
    __GLXvendorInfo *vendor = NULL;
    __GLXvendorScreenHash *pEntry = NULL;
    __GLXvendorScreenHashKey key;

    if (screen < 0) {
        return NULL;
    }

    memset(&key, 0, sizeof(key));

    key.dpy = dpy;
    key.screen = screen;

    LKDHASH_RDLOCK(__glXPthreadFuncs, __glXVendorScreenHash);

    HASH_FIND(hh, _LH(__glXVendorScreenHash), &key,
              sizeof(key), pEntry);
    if (pEntry) {
        vendor = pEntry->vendor;
    }

    LKDHASH_UNLOCK(__glXPthreadFuncs, __glXVendorScreenHash);

    if (!pEntry) {
        /*
         * If we have specified a vendor library, use that. Otherwise,
         * try to lookup the vendor based on the current screen.
         */
        const char *preloadedVendorName = getenv("__GLX_VENDOR_LIBRARY_NAME");
        char *queriedVendorName;

        assert(!vendor);

        if (preloadedVendorName) {
            vendor = __glXLookupVendorByName(preloadedVendorName);
        }

        if (!vendor && (dpy != NULL)) {
            queriedVendorName = XGLVQueryScreenVendorMapping(dpy, screen);
            vendor = __glXLookupVendorByName(queriedVendorName);
            Xfree(queriedVendorName);
        }

        if (!vendor) {
            /* No vendor available */
            return NULL;
        }

        LKDHASH_WRLOCK(__glXPthreadFuncs, __glXVendorScreenHash);

        HASH_FIND(hh, _LH(__glXVendorScreenHash), &key, sizeof(key), pEntry);

        if (!pEntry) {
            pEntry = malloc(sizeof(*pEntry));
            if (!pEntry) {
                return NULL;
            }

            pEntry->key.dpy = dpy;
            pEntry->key.screen = screen;
            pEntry->vendor = vendor;
            HASH_ADD(hh, _LH(__glXVendorScreenHash), key,
                     sizeof(__GLXvendorScreenHashKey), pEntry);
        } else {
            /* Some other thread already added a vendor */
            vendor = pEntry->vendor;
        }

        LKDHASH_UNLOCK(__glXPthreadFuncs, __glXVendorScreenHash);
    }

    DBG_PRINTF(10, "Found vendor \"%s\" for screen %d\n",
               vendor->name, screen);

    return vendor;
}

const __GLXdispatchTableStatic *__glXGetStaticDispatch(Display *dpy, const int screen)
{
    __GLXvendorInfo *vendor = __glXLookupVendorByScreen(dpy, screen);

    if (vendor) {
        assert(vendor->staticDispatch);
        return vendor->staticDispatch;
    } else {
        return __glXDispatchNoopPtr;
    }
}

__GLdispatchTable *__glXGetGLDispatch(Display *dpy, const int screen)
{
    __GLXvendorInfo *vendor = __glXLookupVendorByScreen(dpy, screen);

    if (vendor) {
        assert(vendor->glDispatch);
        return vendor->glDispatch;
    } else {
        return NULL;
    }
}

__GLXdispatchTableDynamic *__glXGetDynDispatch(Display *dpy, const int screen)
{
    __glXThreadInitialize();

    __GLXvendorInfo *vendor = __glXLookupVendorByScreen(dpy, screen);

    if (vendor) {
        assert(vendor->dynDispatch);
        return vendor->dynDispatch;
    } else {
        return NULL;
    }
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


static DEFINE_INITIALIZED_LKDHASH(__GLXscreenPointerMappingHash, __glXScreenPointerMappingHash);

static void AddScreenPointerMapping(void *ptr, int screen)
{
    __GLXscreenPointerMappingHash *pEntry;

    if (ptr == NULL) {
        return;
    }

    if (screen < 0) {
        return;
    }

    LKDHASH_WRLOCK(__glXPthreadFuncs, __glXScreenPointerMappingHash);

    HASH_FIND_PTR(_LH(__glXScreenPointerMappingHash), &ptr, pEntry);

    if (pEntry == NULL) {
        pEntry = malloc(sizeof(*pEntry));
        pEntry->ptr = ptr;
        pEntry->screen = screen;
        HASH_ADD_PTR(_LH(__glXScreenPointerMappingHash), ptr, pEntry);
    } else {
        pEntry->screen = screen;
    }

    LKDHASH_UNLOCK(__glXPthreadFuncs, __glXScreenPointerMappingHash);
}


static void RemoveScreenPointerMapping(void *ptr)
{
    __GLXscreenPointerMappingHash *pEntry;

    if (ptr == NULL) {
        return;
    }

    LKDHASH_WRLOCK(__glXPthreadFuncs, __glXScreenPointerMappingHash);

    HASH_FIND_PTR(_LH(__glXScreenPointerMappingHash), &ptr, pEntry);

    if (pEntry != NULL) {
        HASH_DELETE(hh, _LH(__glXScreenPointerMappingHash), pEntry);
        free(pEntry);
    }

    LKDHASH_UNLOCK(__glXPthreadFuncs, __glXScreenPointerMappingHash);
}


static int ScreenFromPointer(void *ptr)
{
    __GLXscreenPointerMappingHash *pEntry;
    int screen = -1;

    LKDHASH_RDLOCK(__glXPthreadFuncs, __glXScreenPointerMappingHash);

    HASH_FIND_PTR(_LH(__glXScreenPointerMappingHash), &ptr, pEntry);

    if (pEntry != NULL) {
        screen = pEntry->screen;
    }

    LKDHASH_UNLOCK(__glXPthreadFuncs, __glXScreenPointerMappingHash);

    return screen;
}


void __glXAddScreenContextMapping(GLXContext context, int screen)
{
    AddScreenPointerMapping(context, screen);
}


void __glXRemoveScreenContextMapping(GLXContext context)
{
    RemoveScreenPointerMapping(context);
}


int __glXScreenFromContext(GLXContext context)
{
    return ScreenFromPointer(context);
}


void __glXAddScreenFBConfigMapping(GLXFBConfig config, int screen)
{
    AddScreenPointerMapping(config, screen);
}


void __glXRemoveScreenFBConfigMapping(GLXFBConfig config)
{
    RemoveScreenPointerMapping(config);
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


static DEFINE_INITIALIZED_LKDHASH(__GLXscreenXIDMappingHash, __glXScreenXIDMappingHash);

static void AddScreenXIDMapping(XID xid, int screen)
{
    __GLXscreenXIDMappingHash *pEntry = NULL;

    if (xid == None) {
        return;
    }

    if (screen < 0) {
        return;
    }

    LKDHASH_WRLOCK(__glXPthreadFuncs, __glXScreenXIDMappingHash);

    HASH_FIND(hh, _LH(__glXScreenXIDMappingHash), &xid, sizeof(xid), pEntry);

    if (pEntry == NULL) {
        pEntry = malloc(sizeof(*pEntry));
        pEntry->xid = xid;
        pEntry->screen = screen;
        HASH_ADD(hh, _LH(__glXScreenXIDMappingHash), xid, sizeof(xid), pEntry);
    } else {
        pEntry->screen = screen;
    }

    LKDHASH_UNLOCK(__glXPthreadFuncs, __glXScreenXIDMappingHash);
}


static void RemoveScreenXIDMapping(XID xid)
{
    __GLXscreenXIDMappingHash *pEntry;

    if (xid == None) {
        return;
    }

    LKDHASH_WRLOCK(__glXPthreadFuncs, __glXScreenXIDMappingHash);

    HASH_FIND(hh, _LH(__glXScreenXIDMappingHash), &xid, sizeof(xid), pEntry);

    if (pEntry != NULL) {
        HASH_DELETE(hh, _LH(__glXScreenXIDMappingHash), pEntry);
        free(pEntry);
    }

    LKDHASH_UNLOCK(__glXPthreadFuncs, __glXScreenXIDMappingHash);
}


static int ScreenFromXID(Display *dpy, XID xid)
{
    __GLXscreenXIDMappingHash *pEntry;
    int screen = -1;

    LKDHASH_RDLOCK(__glXPthreadFuncs, __glXScreenXIDMappingHash);

    HASH_FIND(hh, _LH(__glXScreenXIDMappingHash), &xid, sizeof(xid), pEntry);

    if (pEntry) {
        screen = pEntry->screen;
        LKDHASH_UNLOCK(__glXPthreadFuncs, __glXScreenXIDMappingHash);
    } else {
        screen = XGLVQueryXIDScreenMapping(dpy, xid);
        LKDHASH_UNLOCK(__glXPthreadFuncs, __glXScreenXIDMappingHash);
        AddScreenXIDMapping(xid, screen);
    }

    return screen;
}


void __glXAddScreenDrawableMapping(GLXDrawable drawable, int screen)
{
    AddScreenXIDMapping(drawable, screen);
}


void __glXRemoveScreenDrawableMapping(GLXDrawable drawable)
{
    RemoveScreenXIDMapping(drawable);
}


int __glXScreenFromDrawable(Display *dpy, GLXDrawable drawable)
{
    return ScreenFromXID(dpy, drawable);
}

/*!
 * This handles freeing all mapping state during library teardown
 * and fork recovery.
 */
void __glXMappingTeardown(Bool doReset)
{
    /* Tear down all hashtables used in this file */
    LKDHASH_TEARDOWN(__glXPthreadFuncs, __GLXdispatchIndexHash,
                     __glXDispatchIndexHash, CleanupDispatchIndexEntry,
                     NULL, doReset);

    LKDHASH_WRLOCK(__glXPthreadFuncs, __glXDispatchIndexHash);
    __glXNextUnusedHashIndex = 0;
    LKDHASH_UNLOCK(__glXPthreadFuncs, __glXDispatchIndexHash);

    LKDHASH_TEARDOWN(__glXPthreadFuncs, __GLXvendorScreenHash,
                     __glXVendorScreenHash, NULL, NULL, doReset);

    LKDHASH_TEARDOWN(__glXPthreadFuncs, __GLXscreenPointerMappingHash,
                     __glXScreenPointerMappingHash, NULL, NULL, doReset);

    LKDHASH_TEARDOWN(__glXPthreadFuncs, __GLXscreenXIDMappingHash,
                     __glXScreenXIDMappingHash, NULL, NULL, doReset);

    if (doReset) {
        /*
         * If we're just doing fork recovery, we don't actually want to unload
         * any currently loaded vendors.  Just reset the corresponding lock.
         */
        __glXPthreadFuncs.rwlock_init(&__glXVendorNameHash.lock, NULL);
    } else {
        /*
         * This implicitly unloads vendor libraries that were loaded when
         * they were added to this hashtable.
         */
        LKDHASH_TEARDOWN(__glXPthreadFuncs, __GLXvendorNameHash,
                         __glXVendorNameHash, CleanupVendorNameEntry,
                         NULL, False);
    }

}
