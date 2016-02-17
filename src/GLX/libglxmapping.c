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
#include <string.h>

#if defined(HASH_DEBUG)
# include <stdio.h>
#endif

#include "libglxcurrent.h"
#include "libglxmapping.h"
#include "libglxnoop.h"
#include "libglxthread.h"
#include "libglxstring.h"
#include "utils_misc.h"
#include "glvnd_genentry.h"
#include "trace.h"

#include "lkdhash.h"
#include "x11glvnd.h"

#define _GNU_SOURCE 1

#if !defined(FALLBACK_VENDOR_NAME)
/*!
 * This is the vendor name that we'll use as a fallback if we can't otherwise
 * find one.
 *
 * The only place where this should happen is if the display connection is to a
 * remote X server, which might not support the x11glvnd extension, or might
 * specify a vendor library that's not available to the client. In that case,
 * only indirect rendering will be possible.
 *
 * Eventually, libglvnd should have a dedicated vendor library for indirect
 * rendering, independent of any hardware vendor. Until then, this will
 * typically be a symlink to an existing vendor library.
 */
#define FALLBACK_VENDOR_NAME "indirect"
#endif

#define GLX_EXTENSION_NAME "GLX"

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

/****************************************************************************/

/*
 * __glXVendorNameHash is a hash table mapping a vendor name to vendor info.
 */
typedef struct __GLXvendorNameHashRec {
    __GLXvendorInfo vendor;
    UT_hash_handle hh;
} __GLXvendorNameHash;

static DEFINE_INITIALIZED_LKDHASH(__GLXvendorNameHash, __glXVendorNameHash);

typedef struct __GLXdisplayInfoHashRec {
    Display *dpy;
    __GLXdisplayInfo info;
    UT_hash_handle hh;
} __GLXdisplayInfoHash;

static DEFINE_INITIALIZED_LKDHASH(__GLXdisplayInfoHash, __glXDisplayInfoHash);

struct __GLXvendorXIDMappingHashRec {
    XID xid;
    __GLXvendorInfo *vendor;
    UT_hash_handle hh;
};

static glvnd_mutex_t glxGenEntrypointMutex = GLVND_MUTEX_INITIALIZER;

static __GLXextFuncPtr __glXFetchDispatchEntry(__GLXvendorInfo *vendor, int index);

static const __GLXapiExports glxExportsTable = {
    .getDynDispatch = __glXGetDynDispatch,
    .getCurrentDynDispatch = __glXGetCurrentDynDispatch,
    .fetchDispatchEntry = __glXFetchDispatchEntry,

    /* We use the real function since __glXGetCurrentContext is inline */
    .getCurrentContext = glXGetCurrentContext,

    .addVendorContextMapping = __glXAddVendorContextMapping,
    .removeVendorContextMapping = __glXRemoveVendorContextMapping,
    .vendorFromContext = __glXVendorFromContext,

    .addVendorFBConfigMapping = __glXAddVendorFBConfigMapping,
    .removeVendorFBConfigMapping = __glXRemoveVendorFBConfigMapping,
    .vendorFromFBConfig = __glXVendorFromFBConfig,

    .addScreenVisualMapping = __glXAddScreenVisualMapping,
    .removeScreenVisualMapping = __glXRemoveScreenVisualMapping,
    .vendorFromVisual = __glXVendorFromVisual,

    .addVendorDrawableMapping = __glXAddVendorDrawableMapping,
    .removeVendorDrawableMapping = __glXRemoveVendorDrawableMapping,
    .vendorFromDrawable = __glXVendorFromDrawable,
};

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

    LKDHASH_WRLOCK(__glXDispatchIndexHash);
    pEntry->index = __glXNextUnusedHashIndex++;

    // Notify the vendor this is the index which should be used
    vendor->glxvc->setDispatchIndex(procName, pEntry->index);

    HASH_ADD_INT(_LH(__glXDispatchIndexHash),
                 index, pEntry);
    LKDHASH_UNLOCK(__glXDispatchIndexHash);

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

    addr = vendor->glxvc->getDispatchAddress((const GLubyte *) procName);
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
    addr = vendor->glxvc->getProcAddress((const GLubyte *) procName);
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
    LKDHASH_RDLOCK(__glXVendorNameHash);
    HASH_ITER(hh, _LH(__glXVendorNameHash), pEntry, tmp) {
        addr = __glXFindVendorDispatchAddress((const char *)procName, &pEntry->vendor);
        if (addr) {
            break;
        }
    }
    LKDHASH_UNLOCK(__glXVendorNameHash);

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
    /*
     * For GLX functions, try to generate an entrypoint. We'll plug in
     * a dispatch function for it if and when we load a vendor library
     * that supports it.
     */
    if (procName[0] == 'g' && procName[1] == 'l' && procName[2] == 'X') {
        __glvndPthreadFuncs.mutex_lock(&glxGenEntrypointMutex);
        addr = (__GLXextFuncPtr) glvndGenerateEntrypoint((const char *) procName);
        __glvndPthreadFuncs.mutex_unlock(&glxGenEntrypointMutex);
    } else {
        /* For GL functions, request a dispatch stub from libGLdispatch. */
        addr = __glDispatchGetProcAddress((const char *)procName);
    }
    return addr;
}

__GLXextFuncPtr __glXFetchDispatchEntry(__GLXvendorInfo *vendor,
                                        int index)
{
    __GLXextFuncPtr addr = NULL;
    __GLXdispatchFuncHash *pEntry;
    GLubyte *procName = NULL;

    LKDHASH_RDLOCK(vendor->dynDispatchHash);

    HASH_FIND_INT(_LH(vendor->dynDispatchHash), &index, pEntry);

    if (pEntry) {
        // This can be NULL, which indicates the vendor does not implement this
        // entry. Vendor library provided dispatch functions are expected to
        // default to a no-op in case dispatching fails.
        addr = pEntry->addr;
    }

    LKDHASH_UNLOCK(vendor->dynDispatchHash);

    if (!pEntry) {
        // Not seen before by this vendor: query the vendor for the right
        // address to use.

        __GLXdispatchIndexHash *pdiEntry;

        // First retrieve the procname of this index
        LKDHASH_RDLOCK(__glXDispatchIndexHash);
        HASH_FIND_INT(_LH(__glXDispatchIndexHash), &index, pdiEntry);
        procName = pdiEntry->procName;
        LKDHASH_UNLOCK(__glXDispatchIndexHash);

        // This should have a valid entry point associated with it.
        assert(procName);

        if (procName) {
            // Get the real address
            addr = vendor->glxvc->getProcAddress(procName);
        }

        LKDHASH_WRLOCK(vendor->dynDispatchHash);
        HASH_FIND_INT(_LH(vendor->dynDispatchHash), &index, pEntry);
        if (!pEntry) {
            pEntry = malloc(sizeof(*pEntry));
            if (!pEntry) {
                // Uh-oh!
                assert(pEntry);
                LKDHASH_UNLOCK(vendor->dynDispatchHash);
                return NULL;
            }
            pEntry->index = index;
            pEntry->addr = addr;

            HASH_ADD_INT(_LH(vendor->dynDispatchHash), index, pEntry);
        } else {
            addr = pEntry->addr;
        }
        LKDHASH_UNLOCK(vendor->dynDispatchHash);
    }

    return addr;
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

static void CleanupVendorNameEntry(void *unused,
                                   __GLXvendorNameHash *pEntry)
{
    __GLXvendorInfo *vendor = &pEntry->vendor;
    if (vendor->glDispatch != NULL) {
        __glDispatchDestroyTable(vendor->glDispatch);
        vendor->glDispatch = NULL;
    }

    /* Clean up the dynamic dispatch table */
    LKDHASH_TEARDOWN(__GLXdispatchFuncHash,
                     vendor->dynDispatchHash, NULL, NULL, True);

    if (vendor->dlhandle != NULL) {
        dlclose(vendor->dlhandle);
        vendor->dlhandle = NULL;
    }
}

static GLboolean LookupVendorEntrypoints(__GLXvendorInfo *vendor)
{
#define LOADENTRYPOINT(ptr, name) do { \
    vendor->staticDispatch.ptr = vendor->glxvc->getProcAddress((const GLubyte *) name); \
    if (vendor->staticDispatch.ptr == NULL) { return GL_FALSE; } \
    } while(0)

    LOADENTRYPOINT(chooseVisual,          "glXChooseVisual"         );
    LOADENTRYPOINT(copyContext,           "glXCopyContext"          );
    LOADENTRYPOINT(createContext,         "glXCreateContext"        );
    LOADENTRYPOINT(createGLXPixmap,       "glXCreateGLXPixmap"      );
    LOADENTRYPOINT(destroyContext,        "glXDestroyContext"       );
    LOADENTRYPOINT(destroyGLXPixmap,      "glXDestroyGLXPixmap"     );
    LOADENTRYPOINT(getConfig,             "glXGetConfig"            );
    LOADENTRYPOINT(isDirect,              "glXIsDirect"             );
    LOADENTRYPOINT(makeCurrent,           "glXMakeCurrent"          );
    LOADENTRYPOINT(swapBuffers,           "glXSwapBuffers"          );
    LOADENTRYPOINT(useXFont,              "glXUseXFont"             );
    LOADENTRYPOINT(waitGL,                "glXWaitGL"               );
    LOADENTRYPOINT(waitX,                 "glXWaitX"                );
    LOADENTRYPOINT(queryServerString,     "glXQueryServerString"    );
    LOADENTRYPOINT(getClientString,       "glXGetClientString"      );
    LOADENTRYPOINT(queryExtensionsString, "glXQueryExtensionsString");
    LOADENTRYPOINT(chooseFBConfig,        "glXChooseFBConfig"       );
    LOADENTRYPOINT(createNewContext,      "glXCreateNewContext"     );
    LOADENTRYPOINT(createPbuffer,         "glXCreatePbuffer"        );
    LOADENTRYPOINT(createPixmap,          "glXCreatePixmap"         );
    LOADENTRYPOINT(createWindow,          "glXCreateWindow"         );
    LOADENTRYPOINT(destroyPbuffer,        "glXDestroyPbuffer"       );
    LOADENTRYPOINT(destroyPixmap,         "glXDestroyPixmap"        );
    LOADENTRYPOINT(destroyWindow,         "glXDestroyWindow"        );
    LOADENTRYPOINT(getFBConfigAttrib,     "glXGetFBConfigAttrib"    );
    LOADENTRYPOINT(getFBConfigs,          "glXGetFBConfigs"         );
    LOADENTRYPOINT(getSelectedEvent,      "glXGetSelectedEvent"     );
    LOADENTRYPOINT(getVisualFromFBConfig, "glXGetVisualFromFBConfig");
    LOADENTRYPOINT(makeContextCurrent,    "glXMakeContextCurrent"   );
    LOADENTRYPOINT(queryContext,          "glXQueryContext"         );
    LOADENTRYPOINT(queryDrawable,         "glXQueryDrawable"        );
    LOADENTRYPOINT(selectEvent,           "glXSelectEvent"          );
#undef LOADENTRYPOINT

    // These functions are optional.
#define LOADENTRYPOINT(ptr, name) do { \
    vendor->staticDispatch.ptr = vendor->glxvc->getProcAddress((const GLubyte *) name); \
    } while(0)
    LOADENTRYPOINT(importContextEXT,            "glXImportContextEXT"           );
    LOADENTRYPOINT(freeContextEXT,              "glXFreeContextEXT"             );
#undef LOADENTRYPOINT

    return GL_TRUE;
}

static void *VendorGetProcAddressCallback(const char *procName, void *param)
{
    __GLXvendorInfo *vendor = (__GLXvendorInfo *) param;
    return vendor->glxvc->getProcAddress((const GLubyte *) procName);
}

__GLXvendorInfo *__glXLookupVendorByName(const char *vendorName)
{
    __GLXvendorNameHash *pEntry = NULL;
    Bool locked = False;
    size_t vendorNameLen;

    // We'll use the vendor name to construct a DSO name, so make sure it
    // doesn't contain any '/' characters.
    if (strchr(vendorName, '/') != NULL) {
        return NULL;
    }

    vendorNameLen = strlen(vendorName);

    LKDHASH_RDLOCK(__glXVendorNameHash);
    HASH_FIND(hh, _LH(__glXVendorNameHash), vendorName, vendorNameLen, pEntry);

    LKDHASH_UNLOCK(__glXVendorNameHash);

    if (!pEntry) {
        LKDHASH_WRLOCK(__glXVendorNameHash);
        locked = True;
        // Do another lookup to check uniqueness
        HASH_FIND(hh, _LH(__glXVendorNameHash), vendorName, vendorNameLen, pEntry);
        if (!pEntry) {
            __GLXvendorInfo *vendor;
            __PFNGLXMAINPROC glxMainProc;
            char *filename;

            // Previously unseen vendor. dlopen() the new vendor and add it to the
            // hash table.
            pEntry = calloc(1, sizeof(*pEntry) + vendorNameLen + 1);
            if (!pEntry) {
                goto fail;
            }
            vendor = &pEntry->vendor;

            vendor->name = (char *) (pEntry + 1);
            memcpy(vendor->name, vendorName, vendorNameLen + 1);

            filename = ConstructVendorLibraryFilename(vendorName);
            if (filename) {
                vendor->dlhandle = dlopen(filename, RTLD_LAZY);
            }
            free(filename);
            if (vendor->dlhandle == NULL) {
                goto fail;
            }

            glxMainProc = dlsym(vendor->dlhandle, __GLX_MAIN_PROTO_NAME);
            if (!glxMainProc) {
                goto fail;
            }

            vendor->vendorID = __glDispatchNewVendorID();
            assert(vendor->vendorID >= 0);

            vendor->glxvc = (*glxMainProc)(GLX_VENDOR_ABI_VERSION,
                                      &glxExportsTable,
                                      vendor->name,
                                      vendor->vendorID);
            if (!vendor->glxvc) {
                goto fail;
            }

            if (!LookupVendorEntrypoints(vendor)) {
                goto fail;
            }

            vendor->glDispatch = (__GLdispatchTable *)
                __glDispatchCreateTable(
                    VendorGetProcAddressCallback,
                    vendor
                );
            if (!vendor->glDispatch) {
                goto fail;
            }

            /* Initialize the dynamic dispatch table */
            LKDHASH_INIT(vendor->dynDispatchHash);

            HASH_ADD_KEYPTR(hh, _LH(__glXVendorNameHash), vendor->name,
                            strlen(vendor->name), pEntry);
            LKDHASH_UNLOCK(__glXVendorNameHash);

            // Look up the dispatch functions for any GLX extensions that we
            // generated entrypoints for.
            __glvndPthreadFuncs.mutex_lock(&glxGenEntrypointMutex);
            glvndUpdateEntrypoints(
                    (GLVNDentrypointUpdateCallback) __glXFindVendorDispatchAddress,
                    vendor);
            __glvndPthreadFuncs.mutex_unlock(&glxGenEntrypointMutex);
        } else {
            /* Some other thread added a vendor */
            LKDHASH_UNLOCK(__glXVendorNameHash);
        }
    }

    return &pEntry->vendor;

fail:
    if (locked) {
        LKDHASH_UNLOCK(__glXVendorNameHash);
    }
    if (pEntry != NULL) {
        CleanupVendorNameEntry(NULL, pEntry);
        free(pEntry);
    }
    return NULL;
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

    __glvndPthreadFuncs.rwlock_rdlock(&dpyInfo->vendorLock);
    vendor = dpyInfo->vendors[screen];
    __glvndPthreadFuncs.rwlock_unlock(&dpyInfo->vendorLock);

    if (vendor != NULL) {
        return vendor;
    }

    __glvndPthreadFuncs.rwlock_wrlock(&dpyInfo->vendorLock);
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

                // Make sure that the vendor library can support this screen.
                // If it can't, then we'll fall back to the indirect rendering
                // library.
                if (vendor != NULL && !vendor->glxvc->checkSupportsScreen(dpy, screen)) {
                    vendor = NULL;
                }
            }
        }

        if (!vendor) {
            vendor = __glXLookupVendorByName(FALLBACK_VENDOR_NAME);
        }

        dpyInfo->vendors[screen] = vendor;
    }
    __glvndPthreadFuncs.rwlock_unlock(&dpyInfo->vendorLock);

    DBG_PRINTF(10, "Found vendor \"%s\" for screen %d\n",
               (vendor != NULL ? vendor->name : "NULL"), screen);

    return vendor;
}

const __GLXdispatchTableStatic *__glXGetStaticDispatch(Display *dpy, const int screen)
{
    __GLXvendorInfo *vendor = __glXLookupVendorByScreen(dpy, screen);

    if (vendor) {
        return &vendor->staticDispatch;
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

    LKDHASH_INIT(pEntry->info.xidVendorHash);
    __glvndPthreadFuncs.rwlock_init(&pEntry->info.vendorLock, NULL);

    // Check whether the server supports the GLX extension, and record the
    // major opcode if it does.
    pEntry->info.glxSupported = XQueryExtension(dpy, GLX_EXTENSION_NAME,
            &pEntry->info.glxMajorOpcode, &eventBase,
            &pEntry->info.glxFirstError);

    // Check whether the server supports the x11glvnd extension.
    if (XGLVQueryExtension(dpy, &eventBase, &errorBase)) {
        pEntry->info.x11glvndSupported = True;
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

    LKDHASH_TEARDOWN(__GLXvendorXIDMappingHash,
                     pEntry->info.xidVendorHash, NULL, NULL, False);
}

__GLXdisplayInfo *__glXLookupDisplay(Display *dpy)
{
    __GLXdisplayInfoHash *pEntry = NULL;
    __GLXdisplayInfoHash *foundEntry = NULL;

    if (dpy == NULL) {
        return NULL;
    }

    LKDHASH_RDLOCK(__glXDisplayInfoHash);
    HASH_FIND_PTR(_LH(__glXDisplayInfoHash), &dpy, pEntry);
    LKDHASH_UNLOCK(__glXDisplayInfoHash);

    if (pEntry != NULL) {
        return &pEntry->info;
    }

    // Create the new __GLXdisplayInfoHash structure without holding the lock.
    // If we run into an X error, we may wind up in __glXMappingTeardown before
    // we can unlock it again, which would deadlock.
    pEntry = InitDisplayInfoEntry(dpy);
    if (pEntry == NULL) {
        return NULL;
    }

    LKDHASH_WRLOCK(__glXDisplayInfoHash);
    HASH_FIND_PTR(_LH(__glXDisplayInfoHash), &dpy, foundEntry);
    if (foundEntry == NULL) {
        HASH_ADD_PTR(_LH(__glXDisplayInfoHash), dpy, pEntry);
    } else {
        // Another thread already created the hashtable entry.
        free(pEntry);
        pEntry = foundEntry;
    }
    LKDHASH_UNLOCK(__glXDisplayInfoHash);

    return &pEntry->info;
}

void __glXFreeDisplay(Display *dpy)
{
    __GLXdisplayInfoHash *pEntry = NULL;

    LKDHASH_WRLOCK(__glXDisplayInfoHash);
    HASH_FIND_PTR(_LH(__glXDisplayInfoHash), &dpy, pEntry);
    if (pEntry != NULL) {
        HASH_DEL(_LH(__glXDisplayInfoHash), pEntry);
    }
    LKDHASH_UNLOCK(__glXDisplayInfoHash);

    if (pEntry != NULL) {
        CleanupDisplayInfoEntry(NULL, pEntry);
        free(pEntry);
    }
}

/****************************************************************************/
/*
 * Define two hashtables to store the mappings for GLXFBConfig and GLXContext
 * handles to vendor libraries.
 *
 * The same functions are used to access both tables.
 */

typedef struct {
    GLXFBConfig config;
    __GLXvendorInfo *vendor;
    UT_hash_handle hh;
} __GLXvendorConfigMappingHash;

static DEFINE_LKDHASH(__GLXvendorConfigMappingHash, fbconfigHashtable);

void __glXAddVendorFBConfigMapping(Display *dpy, GLXFBConfig config, __GLXvendorInfo *vendor)
{
    __GLXvendorConfigMappingHash *pEntry;

    if (config == NULL) {
        return;
    }

    if (vendor == NULL) {
        return;
    }

    LKDHASH_WRLOCK(fbconfigHashtable);

    HASH_FIND_PTR(_LH(fbconfigHashtable), &config, pEntry);

    if (pEntry == NULL) {
        pEntry = malloc(sizeof(*pEntry));
        pEntry->config = config;
        pEntry->vendor = vendor;
        HASH_ADD_PTR(_LH(fbconfigHashtable), config, pEntry);
    } else {
        // Any GLXContext or GLXFBConfig handles must be unique to a single
        // vendor at a time. If we get two different vendors, then there's
        // either a bug in libGLX or in at least one of the vendor libraries.
        assert(pEntry->vendor == vendor);
    }

    LKDHASH_UNLOCK(fbconfigHashtable);
}

void __glXRemoveVendorFBConfigMapping(Display *dpy, GLXFBConfig config)
{
    __GLXvendorConfigMappingHash *pEntry;

    if (config == NULL) {
        return;
    }

    LKDHASH_WRLOCK(fbconfigHashtable);

    HASH_FIND_PTR(_LH(fbconfigHashtable), &config, pEntry);

    if (pEntry != NULL) {
        HASH_DELETE(hh, _LH(fbconfigHashtable), pEntry);
        free(pEntry);
    }

    LKDHASH_UNLOCK(fbconfigHashtable);
}

int __glXVendorFromFBConfig(Display *dpy, GLXFBConfig config, __GLXvendorInfo **retVendor)
{
    __GLXvendorConfigMappingHash *pEntry;
    __GLXvendorInfo *vendor = NULL;

    __glXThreadInitialize();

    LKDHASH_RDLOCK(fbconfigHashtable);

    HASH_FIND_PTR(_LH(fbconfigHashtable), &config, pEntry);

    if (pEntry != NULL) {
        vendor = pEntry->vendor;
    }

    LKDHASH_UNLOCK(fbconfigHashtable);

    if (retVendor != NULL) {
        *retVendor = vendor;
    }
    return (vendor != NULL ? 0 : -1);
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
    __glXThreadInitialize();

    if (retVendor != NULL) {
        *retVendor = __glXLookupVendorByScreen(dpy, visual->screen);
    }
    return 0;
}



/****************************************************************************/
/*
 * __GLXvendorXIDMappingHash is a hash table which maps XIDs to vendors.
 */


static void AddVendorXIDMapping(Display *dpy, __GLXdisplayInfo *dpyInfo, XID xid, __GLXvendorInfo *vendor)
{
    __GLXvendorXIDMappingHash *pEntry = NULL;

    if (xid == None) {
        return;
    }

    if (vendor == NULL) {
        return;
    }

    LKDHASH_WRLOCK(dpyInfo->xidVendorHash);

    HASH_FIND(hh, _LH(dpyInfo->xidVendorHash), &xid, sizeof(xid), pEntry);

    if (pEntry == NULL) {
        pEntry = malloc(sizeof(*pEntry));
        pEntry->xid = xid;
        pEntry->vendor = vendor;
        HASH_ADD(hh, _LH(dpyInfo->xidVendorHash), xid, sizeof(xid), pEntry);
    } else {
        // Like GLXContext and GLXFBConfig handles, any GLXDrawables must map
        // to a single vendor library.
        assert(pEntry->vendor == vendor);
    }

    LKDHASH_UNLOCK(dpyInfo->xidVendorHash);
}


static void RemoveVendorXIDMapping(Display *dpy, __GLXdisplayInfo *dpyInfo, XID xid)
{
    __GLXvendorXIDMappingHash *pEntry;

    if (xid == None) {
        return;
    }

    LKDHASH_WRLOCK(dpyInfo->xidVendorHash);

    HASH_FIND(hh, _LH(dpyInfo->xidVendorHash), &xid, sizeof(xid), pEntry);

    if (pEntry != NULL) {
        HASH_DELETE(hh, _LH(dpyInfo->xidVendorHash), pEntry);
        free(pEntry);
    }

    LKDHASH_UNLOCK(dpyInfo->xidVendorHash);
}


static void VendorFromXID(Display *dpy, __GLXdisplayInfo *dpyInfo, XID xid,
        __GLXvendorInfo **retVendor)
{
    __GLXvendorXIDMappingHash *pEntry;
    __GLXvendorInfo *vendor = NULL;

    LKDHASH_RDLOCK(dpyInfo->xidVendorHash);

    HASH_FIND(hh, _LH(dpyInfo->xidVendorHash), &xid, sizeof(xid), pEntry);

    if (pEntry) {
        vendor = pEntry->vendor;
        LKDHASH_UNLOCK(dpyInfo->xidVendorHash);
    } else {
        LKDHASH_UNLOCK(dpyInfo->xidVendorHash);

        if (dpyInfo->x11glvndSupported) {
            int screen = XGLVQueryXIDScreenMapping(dpy, xid);
            if (screen >= 0 && screen < ScreenCount(dpy)) {
                vendor = __glXLookupVendorByScreen(dpy, screen);
                if (vendor != NULL) {
                    AddVendorXIDMapping(dpy, dpyInfo, xid, vendor);
                }
            }
        }
    }

    if (retVendor != NULL) {
        *retVendor = vendor;
    }
}


void __glXAddVendorDrawableMapping(Display *dpy, GLXDrawable drawable, __GLXvendorInfo *vendor)
{
    __GLXdisplayInfo *dpyInfo = __glXLookupDisplay(dpy);
    if (dpyInfo != NULL) {
        AddVendorXIDMapping(dpy, dpyInfo, drawable, vendor);
    }
}


void __glXRemoveVendorDrawableMapping(Display *dpy, GLXDrawable drawable)
{
    __GLXdisplayInfo *dpyInfo = __glXLookupDisplay(dpy);
    if (dpyInfo != NULL) {
        RemoveVendorXIDMapping(dpy, dpyInfo, drawable);
    }
}


int __glXVendorFromDrawable(Display *dpy, GLXDrawable drawable, __GLXvendorInfo **retVendor)
{
    __glXThreadInitialize();

    __GLXdisplayInfo *dpyInfo = __glXLookupDisplay(dpy);
    __GLXvendorInfo *vendor = NULL;
    if (dpyInfo != NULL) {
        if (dpyInfo->x11glvndSupported) {
            VendorFromXID(dpy, dpyInfo, drawable, &vendor);
        } else {
            // We'll use the same vendor for every screen in this case.
            vendor = __glXLookupVendorByScreen(dpy, 0);
        }
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
        __glvndPthreadFuncs.rwlock_init(&__glXDispatchIndexHash.lock, NULL);
        __glvndPthreadFuncs.rwlock_init(&fbconfigHashtable.lock, NULL);
        __glvndPthreadFuncs.rwlock_init(&__glXVendorNameHash.lock, NULL);
        __glvndPthreadFuncs.rwlock_init(&__glXDisplayInfoHash.lock, NULL);

        HASH_ITER(hh, _LH(__glXDisplayInfoHash), dpyInfoEntry, dpyInfoTmp) {
            __glvndPthreadFuncs.rwlock_init(&dpyInfoEntry->info.xidVendorHash.lock, NULL);
            __glvndPthreadFuncs.rwlock_init(&dpyInfoEntry->info.vendorLock, NULL);
        }
    } else {
        /* Tear down all hashtables used in this file */
        LKDHASH_TEARDOWN(__GLXdispatchIndexHash,
                         __glXDispatchIndexHash, CleanupDispatchIndexEntry,
                         NULL, False);

        LKDHASH_WRLOCK(__glXDispatchIndexHash);
        __glXNextUnusedHashIndex = 0;
        LKDHASH_UNLOCK(__glXDispatchIndexHash);

        LKDHASH_TEARDOWN(__GLXvendorConfigMappingHash,
                         fbconfigHashtable, NULL, NULL, False);

        LKDHASH_TEARDOWN(__GLXdisplayInfoHash,
                         __glXDisplayInfoHash, CleanupDisplayInfoEntry,
                         NULL, False);
        /*
         * This implicitly unloads vendor libraries that were loaded when
         * they were added to this hashtable.
         */
        LKDHASH_TEARDOWN(__GLXvendorNameHash,
                         __glXVendorNameHash, CleanupVendorNameEntry,
                         NULL, False);

        /* Free any generated entrypoints */
        glvndFreeEntrypoints();
    }

}
