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
#include "libglxstring.h"
#include "glvnd_genentry.h"
#include "trace.h"

#include "lkdhash.h"
#include "x11glvnd.h"

#define _GNU_SOURCE 1

#if !defined(FALLBACK_VENDOR_NAME)
/*!
 * This is the vendor name that we'll use as a fallback if we can't otherwise
 * find one. This will be used, for example, if the server doesn't support
 * the x11glvnd extension.
 */
#define FALLBACK_VENDOR_NAME "default"
#endif

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
 * __glXVendorNameHash is a hash table mapping a vendor name to vendor info.
 */
typedef struct __GLXvendorNameHashRec {
    const char *name;
    __GLXvendorInfo *vendor;
    UT_hash_handle hh;
} __GLXvendorNameHash;

static DEFINE_INITIALIZED_LKDHASH(__GLXvendorNameHash, __glXVendorNameHash);

typedef struct __GLXdisplayInfoHashRec {
    Display *dpy;
    __GLXdisplayInfo info;
    UT_hash_handle hh;
} __GLXdisplayInfoHash;

static DEFINE_INITIALIZED_LKDHASH(__GLXdisplayInfoHash, __glXDisplayInfoHash);

struct __GLXscreenXIDMappingHashRec {
    XID xid;
    int screen;
    UT_hash_handle hh;
};

static glvnd_mutex_t glxGenEntrypointMutex = GLVND_MUTEX_INITIALIZER;

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

/**
 * Looks up a dispatch function from a vendor library.
 *
 * If the vendor library provides a dispatch function, then it will allocate a
 * dispatch index for it.
 *
 * If the vendor library exports it as a normal OpenGL function, then it will
 * return a dispatch function from libGLdispatch.
 *
 * This function is used from __glXGetGLXDispatchAddress and as the callback to
 * glvndUpdateEntrypoints.
 */
static GLVNDentrypointStub __glXFindVendorDispatchAddress(const char *procName, __GLXvendorInfo *vendor)
{
    __GLXextFuncPtr addr = NULL;

    addr = vendor->staticDispatch->
        glxvc.getDispatchAddress((const GLubyte *) procName);
    if (addr != NULL) {
        // Allocate the new dispatch index.
        if (!AllocDispatchIndex(vendor, (const GLubyte *) procName)) {
            addr = NULL;
        }
        return addr;
    }

    // If we didn't find a GLX dispatch function, then check for a normal
    // OpenGL function. This should handle any case where a GL extension
    // function starts with "glX".
    addr = vendor->staticDispatch->glxvc.getProcAddress((const GLubyte *) procName, 1);
    if (addr != NULL) {
        addr = __glDispatchGetProcAddress(procName);
    }
    return addr;
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
     * Note that if a GLX extension function doesn't depend on calling any
     * other GLX functions first, then the app could call it before loading any
     * vendor libraries. If that happens, then the entrypoint would go to a
     * no-op stub instead of the correct dispatch stub.
     *
     * Running into that case would be an application bug, since it means that
     * the application is calling an extension function without checking the
     * extension string -- calling glXGetClientString would have loaded the
     * vendor libraries for every screen.
     *
     * In order to work with a buggy app like that, we might have to find and
     * load all available vendor libraries until we find one that supports the
     * function. Lacking that, a user could work around the issue by setting
     * __GLX_VENDOR_LIBRARY_NAME.
     */

    // Look through the vendors that we've already loaded, and see if any of
    // them support the function.
    LKDHASH_RDLOCK(__glXPthreadFuncs, __glXVendorNameHash);
    HASH_ITER(hh, _LH(__glXVendorNameHash), pEntry, tmp) {
        addr = __glXFindVendorDispatchAddress((const char *)procName, pEntry->vendor);
        if (addr) {
            break;
        }
    }
    LKDHASH_UNLOCK(__glXPthreadFuncs, __glXVendorNameHash);

    return addr;
}

/*!
 * Generates an entrypoint for a GLX function. The resulting function will
 * jump to a dispatch function, which we can plug in when we load the vendor
 * library later on.
 *
 * Note that this should still work even if the function turns out to be an
 * OpenGL function, not GLX. In that case, we'll plug in the dispatch function
 * from libGLdispatch instead.
 */
__GLXextFuncPtr __glXGenerateGLXEntrypoint(const GLubyte *procName)
{
    __GLXextFuncPtr addr = NULL;
    if (procName[0] == 'g' && procName[1] == 'l') {
        if (procName[2] == 'X') {
            /*
             * For GLX functions, try to generate an entrypoint. We'll plug in
             * a dispatch function for it if and when we load a vendor library
             * that supports it.
             */
            if (procName[0] == 'g' && procName[1] == 'l' && procName[2] == 'X') {
                __glXPthreadFuncs.mutex_lock(&glxGenEntrypointMutex);
                addr = (__GLXextFuncPtr) glvndGenerateEntrypoint((const char *) procName);
                __glXPthreadFuncs.mutex_unlock(&glxGenEntrypointMutex);
            }
        } else {
            /* For GL functions, request a dispatch stub from libGLdispatch. */
            addr = __glDispatchGetProcAddress((const char *)procName);
        }
    }
    return addr;
}

__GLXextFuncPtr __glXFetchDispatchEntry(__GLXvendorInfo *vendor,
                                        int index)
{
    __GLXextFuncPtr addr = NULL;
    __GLXdispatchFuncHash *pEntry;
    GLubyte *procName = NULL;
    __GLXdispatchTableDynamic *dynDispatch = vendor->dynDispatch;

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
    glxExportsTable.vendorFromContext = __glXVendorFromContext;

    glxExportsTable.addScreenFBConfigMapping = __glXAddScreenFBConfigMapping;
    glxExportsTable.removeScreenFBConfigMapping = __glXRemoveScreenFBConfigMapping;
    glxExportsTable.vendorFromFBConfig = __glXVendorFromFBConfig;

    glxExportsTable.addScreenVisualMapping = __glXAddScreenVisualMapping;
    glxExportsTable.removeScreenVisualMapping = __glXRemoveScreenVisualMapping;
    glxExportsTable.vendorFromVisual = __glXVendorFromVisual;

    glxExportsTable.addScreenDrawableMapping = __glXAddScreenDrawableMapping;
    glxExportsTable.removeScreenDrawableMapping = __glXRemoveScreenDrawableMapping;
    glxExportsTable.vendorFromDrawable = __glXVendorFromDrawable;

}


static char *ConstructVendorLibraryFilename(const char *vendorName)
{
    char *filename;
    int ret;

    ret = glvnd_asprintf(&filename, "libGLX_%s.so.0", vendorName);

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
            LKDHASH_UNLOCK(__glXPthreadFuncs, __glXVendorNameHash);

            // Look up the dispatch functions for any GLX extensions that we
            // generated entrypoints for.
            __glXPthreadFuncs.mutex_lock(&glxGenEntrypointMutex);
            glvndUpdateEntrypoints(
                    (GLVNDentrypointUpdateCallback) __glXFindVendorDispatchAddress,
                    vendor);
            __glXPthreadFuncs.mutex_unlock(&glxGenEntrypointMutex);
        } else {
            /* Some other thread added a vendor */
            vendor = pEntry->vendor;
            LKDHASH_UNLOCK(__glXPthreadFuncs, __glXVendorNameHash);
        }
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
    __GLXdisplayInfo *dpyInfo;

    if (screen < 0 || screen >= ScreenCount(dpy)) {
        return NULL;
    }

    dpyInfo = __glXLookupDisplay(dpy);
    if (dpyInfo == NULL) {
        return NULL;
    }

    __glXPthreadFuncs.rwlock_rdlock(&dpyInfo->vendorLock);
    vendor = dpyInfo->vendors[screen];
    __glXPthreadFuncs.rwlock_unlock(&dpyInfo->vendorLock);

    if (vendor != NULL) {
        return vendor;
    }

    __glXPthreadFuncs.rwlock_wrlock(&dpyInfo->vendorLock);
    vendor = dpyInfo->vendors[screen];

    if (!vendor) {
        /*
         * If we have specified a vendor library, use that. Otherwise,
         * try to lookup the vendor based on the current screen.
         */
        const char *preloadedVendorName = getenv("__GLX_VENDOR_LIBRARY_NAME");

        if (preloadedVendorName) {
            vendor = __glXLookupVendorByName(preloadedVendorName);
        }

        if (!vendor) {
            if (dpyInfo->x11glvndSupported) {
                char *queriedVendorName = XGLVQueryScreenVendorMapping(dpy, screen);
                vendor = __glXLookupVendorByName(queriedVendorName);
                Xfree(queriedVendorName);
            }
        }

        if (!vendor) {
            vendor = __glXLookupVendorByName(FALLBACK_VENDOR_NAME);
        }

        dpyInfo->vendors[screen] = vendor;
    }
    __glXPthreadFuncs.rwlock_unlock(&dpyInfo->vendorLock);

    DBG_PRINTF(10, "Found vendor \"%s\" for screen %d\n",
               (vendor != NULL ? vendor->name : "NULL"), screen);

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

const __GLXdispatchTableStatic * __glXGetDrawableStaticDispatch(Display *dpy,
                                                        GLXDrawable drawable)
{
    __GLXvendorInfo *vendor = NULL;
    __glXVendorFromDrawable(dpy, drawable, NULL, &vendor);

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

__GLXvendorInfo *__glXGetDynDispatch(Display *dpy, const int screen)
{
    __glXThreadInitialize();

    __GLXvendorInfo *vendor = __glXLookupVendorByScreen(dpy, screen);
    return vendor;
}

/**
 * Allocates and initializes a __GLXdisplayInfoHash structure.
 *
 * The caller is responsible for adding the structure to the hashtable.
 *
 * \param dpy The display connection.
 * \return A newly-allocated __GLXdisplayInfoHash structure, or NULL on error.
 */
static __GLXdisplayInfoHash *InitDisplayInfoEntry(Display *dpy)
{
    __GLXdisplayInfoHash *pEntry;
    size_t size;
    int eventBase, errorBase;

    size = sizeof(*pEntry) + ScreenCount(dpy) * sizeof(__GLXvendorInfo *);
    pEntry = (__GLXdisplayInfoHash *) malloc(size);
    if (pEntry == NULL) {
        return NULL;
    }

    memset(pEntry, 0, size);
    pEntry->dpy = dpy;
    pEntry->info.vendors = (__GLXvendorInfo **) (pEntry + 1);

    LKDHASH_INIT(__glXPthreadFuncs, pEntry->info.xidScreenHash);
    __glXPthreadFuncs.rwlock_init(&pEntry->info.vendorLock, NULL);

    if (XGLVQueryExtension(dpy, &eventBase, &errorBase)) {
        pEntry->info.x11glvndSupported = 1;
        XGLVQueryVersion(dpy, &pEntry->info.x11glvndMajor,
                &pEntry->info.x11glvndMinor);
    }

    return pEntry;
}

/**
 * Frees a __GLXdisplayInfoHash structure.
 *
 * The caller is responsible for removing the structure from the hashtable.
 *
 * \param unused Ingored. Needed for the uthash teardown function.
 * \param pEntry The structure to free.
 */
static void CleanupDisplayInfoEntry(void *unused, __GLXdisplayInfoHash *pEntry)
{
    int i;

    if (pEntry == NULL) {
        return;
    }

    for (i=0; i<GLX_CLIENT_STRING_LAST_ATTRIB; i++) {
        free(pEntry->info.clientStrings[i]);
    }

    LKDHASH_TEARDOWN(__glXPthreadFuncs, __GLXscreenXIDMappingHash,
                     pEntry->info.xidScreenHash, NULL, NULL, False);
}

__GLXdisplayInfo *__glXLookupDisplay(Display *dpy)
{
    __GLXdisplayInfoHash *pEntry = NULL;

    if (dpy == NULL) {
        return NULL;
    }

    LKDHASH_RDLOCK(__glXPthreadFuncs, __glXDisplayInfoHash);
    HASH_FIND_PTR(_LH(__glXDisplayInfoHash), &dpy, pEntry);
    LKDHASH_UNLOCK(__glXPthreadFuncs, __glXDisplayInfoHash);

    if (pEntry != NULL) {
        return &pEntry->info;
    }

    LKDHASH_WRLOCK(__glXPthreadFuncs, __glXDisplayInfoHash);
    HASH_FIND_PTR(_LH(__glXDisplayInfoHash), &dpy, pEntry);
    if (pEntry == NULL) {
        pEntry = InitDisplayInfoEntry(dpy);
        if (pEntry != NULL) {
            HASH_ADD_PTR(_LH(__glXDisplayInfoHash), dpy, pEntry);
        }
    }

    LKDHASH_UNLOCK(__glXPthreadFuncs, __glXDisplayInfoHash);
    if (pEntry != NULL) {
        return &pEntry->info;
    } else {
        return NULL;
    }
}

void __glXFreeDisplay(Display *dpy)
{
    __GLXdisplayInfoHash *pEntry = NULL;

    LKDHASH_WRLOCK(__glXPthreadFuncs, __glXDisplayInfoHash);
    HASH_FIND_PTR(_LH(__glXDisplayInfoHash), &dpy, pEntry);
    if (pEntry != NULL) {
        HASH_DEL(_LH(__glXDisplayInfoHash), pEntry);
    }
    LKDHASH_UNLOCK(__glXPthreadFuncs, __glXDisplayInfoHash);

    if (pEntry != NULL) {
        CleanupDisplayInfoEntry(NULL, pEntry);
        free(pEntry);
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

/**
 * Common function for the various __glXVendorFrom* functions.
 */
static int CommonVendorFromScreen(Display *dpy, int screen, int *retScreen, __GLXvendorInfo **retVendor)
{
    if (retScreen != NULL) {
        *retScreen = screen;
    }
    if (retVendor != NULL) {
        if (screen >= 0) {
            *retVendor = __glXLookupVendorByScreen(dpy, screen);
        } else {
            *retVendor = NULL;
        }
    }
    return (screen >= 0 ? 0 : -1);
}

void __glXAddScreenContextMapping(Display *dpy, GLXContext context, int screen, __GLXvendorInfo *vendor)
{
    AddScreenPointerMapping(context, screen);
}


void __glXRemoveScreenContextMapping(Display *dpy, GLXContext context)
{
    RemoveScreenPointerMapping(context);
}


int __glXScreenFromContext(GLXContext context)
{
    return ScreenFromPointer(context);
}

int __glXVendorFromContext(Display *dpy, GLXContext context, int *retScreen, __GLXvendorInfo **retVendor)
{
    int screen = ScreenFromPointer(context);
    return CommonVendorFromScreen(dpy, screen, retScreen, retVendor);
}


void __glXAddScreenFBConfigMapping(Display *dpy, GLXFBConfig config, int screen, __GLXvendorInfo *vendor)
{
    AddScreenPointerMapping(config, screen);
}


void __glXRemoveScreenFBConfigMapping(Display *dpy, GLXFBConfig config)
{
    RemoveScreenPointerMapping(config);
}


int __glXScreenFromFBConfig(GLXFBConfig config)
{
    return ScreenFromPointer(config);
}

int __glXVendorFromFBConfig(Display *dpy, GLXFBConfig config, int *retScreen, __GLXvendorInfo **retVendor)
{
    int screen = ScreenFromPointer(config);
    return CommonVendorFromScreen(dpy, screen, retScreen, retVendor);
}

// Internally, we use the screen number to look up a vendor, so we don't need
// to record anything else for an XVisualInfo.
void __glXAddScreenVisualMapping(Display *dpy, const XVisualInfo *visual, __GLXvendorInfo *vendor)
{
}
void __glXRemoveScreenVisualMapping(Display *dpy, const XVisualInfo *visual)
{
}
int __glXVendorFromVisual(Display *dpy, const XVisualInfo *visual, __GLXvendorInfo **retVendor)
{
    if (retVendor != NULL) {
        *retVendor = __glXLookupVendorByScreen(dpy, visual->screen);
    }
    return 0;
}



/****************************************************************************/
/*
 * __glXScreenXIDMappingHash is a hash table which maps XIDs to screens.
 */


static void AddScreenXIDMapping(Display *dpy, __GLXdisplayInfo *dpyInfo, XID xid, int screen)
{
    __GLXscreenXIDMappingHash *pEntry = NULL;

    if (xid == None) {
        return;
    }

    if (screen < 0) {
        return;
    }

    LKDHASH_WRLOCK(__glXPthreadFuncs, dpyInfo->xidScreenHash);

    HASH_FIND(hh, _LH(dpyInfo->xidScreenHash), &xid, sizeof(xid), pEntry);

    if (pEntry == NULL) {
        pEntry = malloc(sizeof(*pEntry));
        pEntry->xid = xid;
        pEntry->screen = screen;
        HASH_ADD(hh, _LH(dpyInfo->xidScreenHash), xid, sizeof(xid), pEntry);
    } else {
        pEntry->screen = screen;
    }

    LKDHASH_UNLOCK(__glXPthreadFuncs, dpyInfo->xidScreenHash);
}


static void RemoveScreenXIDMapping(Display *dpy, __GLXdisplayInfo *dpyInfo, XID xid)
{
    __GLXscreenXIDMappingHash *pEntry;

    if (xid == None) {
        return;
    }

    LKDHASH_WRLOCK(__glXPthreadFuncs, dpyInfo->xidScreenHash);

    HASH_FIND(hh, _LH(dpyInfo->xidScreenHash), &xid, sizeof(xid), pEntry);

    if (pEntry != NULL) {
        HASH_DELETE(hh, _LH(dpyInfo->xidScreenHash), pEntry);
        free(pEntry);
    }

    LKDHASH_UNLOCK(__glXPthreadFuncs, dpyInfo->xidScreenHash);
}


static int ScreenFromXID(Display *dpy, __GLXdisplayInfo *dpyInfo, XID xid)
{
    __GLXscreenXIDMappingHash *pEntry;
    int screen = -1;

    LKDHASH_RDLOCK(__glXPthreadFuncs, dpyInfo->xidScreenHash);

    HASH_FIND(hh, _LH(dpyInfo->xidScreenHash), &xid, sizeof(xid), pEntry);

    if (pEntry) {
        screen = pEntry->screen;
        LKDHASH_UNLOCK(__glXPthreadFuncs, dpyInfo->xidScreenHash);
    } else {
        LKDHASH_UNLOCK(__glXPthreadFuncs, dpyInfo->xidScreenHash);

        if (dpyInfo->x11glvndSupported) {
            screen = XGLVQueryXIDScreenMapping(dpy, xid);
            AddScreenXIDMapping(dpy, dpyInfo, xid, screen);
        }
    }

    return screen;
}


void __glXAddScreenDrawableMapping(Display *dpy, GLXDrawable drawable, int screen, __GLXvendorInfo *vendor)
{
    __GLXdisplayInfo *dpyInfo = __glXLookupDisplay(dpy);
    if (dpyInfo != NULL) {
        AddScreenXIDMapping(dpy, dpyInfo, drawable, screen);
    }
}


void __glXRemoveScreenDrawableMapping(Display *dpy, GLXDrawable drawable)
{
    __GLXdisplayInfo *dpyInfo = __glXLookupDisplay(dpy);
    if (dpyInfo != NULL) {
        RemoveScreenXIDMapping(dpy, dpyInfo, drawable);
    }
}


int __glXScreenFromDrawable(Display *dpy, GLXDrawable drawable)
{
    __GLXdisplayInfo *dpyInfo = __glXLookupDisplay(dpy);
    if (dpyInfo != NULL) {
        return ScreenFromXID(dpy, dpyInfo, drawable);
    }
    return -1;
}

int __glXVendorFromDrawable(Display *dpy, GLXDrawable drawable, int *retScreen, __GLXvendorInfo **retVendor)
{
    __GLXdisplayInfo *dpyInfo = __glXLookupDisplay(dpy);
    int screen = -1;
    __GLXvendorInfo *vendor = NULL;
    if (dpyInfo != NULL) {
        if (dpyInfo->x11glvndSupported) {
            screen = ScreenFromXID(dpy, dpyInfo, drawable);
            vendor = __glXLookupVendorByScreen(dpy, screen);
        } else {
            // We'll use the same vendor for every screen in this case.
            screen = -1;
            vendor = __glXLookupVendorByScreen(dpy, 0);
        }
    }

    if (retScreen != NULL) {
        *retScreen = screen;
    }
    if (retVendor != NULL) {
        *retVendor = vendor;
    }
    return (vendor != NULL ? 0 : -1);
}

/*!
 * This handles freeing all mapping state during library teardown
 * or resetting locks on fork recovery.
 */
void __glXMappingTeardown(Bool doReset)
{

    if (doReset) {
        __GLXdisplayInfoHash *dpyInfoEntry, *dpyInfoTmp;

        /*
         * If we're just doing fork recovery, we don't actually want to unload
         * any currently loaded vendors _or_ remove any mappings (they should
         * still be valid in the new process, and may be needed if the child
         * tries using pointers/XIDs that were created in the parent).  Just
         * reset the corresponding locks.
         */
        __glXPthreadFuncs.rwlock_init(&__glXDispatchIndexHash.lock, NULL);
        __glXPthreadFuncs.rwlock_init(&__glXScreenPointerMappingHash.lock, NULL);
        __glXPthreadFuncs.rwlock_init(&__glXVendorNameHash.lock, NULL);
        __glXPthreadFuncs.rwlock_init(&__glXDisplayInfoHash.lock, NULL);

        HASH_ITER(hh, _LH(__glXDisplayInfoHash), dpyInfoEntry, dpyInfoTmp) {
            __glXPthreadFuncs.rwlock_init(&dpyInfoEntry->info.xidScreenHash.lock, NULL);
            __glXPthreadFuncs.rwlock_init(&dpyInfoEntry->info.vendorLock, NULL);
        }
    } else {
        /* Tear down all hashtables used in this file */
        LKDHASH_TEARDOWN(__glXPthreadFuncs, __GLXdispatchIndexHash,
                         __glXDispatchIndexHash, CleanupDispatchIndexEntry,
                         NULL, False);

        LKDHASH_WRLOCK(__glXPthreadFuncs, __glXDispatchIndexHash);
        __glXNextUnusedHashIndex = 0;
        LKDHASH_UNLOCK(__glXPthreadFuncs, __glXDispatchIndexHash);

        LKDHASH_TEARDOWN(__glXPthreadFuncs, __GLXdisplayInfoHash,
                         __glXDisplayInfoHash, NULL, NULL, False);

        LKDHASH_TEARDOWN(__glXPthreadFuncs, __GLXscreenPointerMappingHash,
                         __glXScreenPointerMappingHash, NULL, NULL, False);

        LKDHASH_TEARDOWN(__glXPthreadFuncs, __GLXdisplayInfoHash,
                         __glXDisplayInfoHash, CleanupDisplayInfoEntry,
                         NULL, False);
        /*
         * This implicitly unloads vendor libraries that were loaded when
         * they were added to this hashtable.
         */
        LKDHASH_TEARDOWN(__glXPthreadFuncs, __GLXvendorNameHash,
                         __glXVendorNameHash, CleanupVendorNameEntry,
                         NULL, False);

        /* Free any generated entrypoints */
        glvndFreeEntrypoints();
    }

}
