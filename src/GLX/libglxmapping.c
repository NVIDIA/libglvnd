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
#include "libglxthread.h"
#include "libglxproto.h"
#include "utils_misc.h"
#include "glvnd_genentry.h"
#include "trace.h"
#include "winsys_dispatch.h"

#include "lkdhash.h"

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

/****************************************************************************/

/**
 * __glXVendorNameHash is a hash table mapping a vendor name to vendor info.
 *
 * Note that the lock for the vendor name hashtable is also used to control
 * access to the GLX dispatch index list and the generated GLX dispatch stubs.
 */
typedef struct __GLXvendorNameHashRec {
    __GLXvendorInfo vendor;

    /**
     * The imports table for this vendor. This is allocated and zeroed by
     * libGLX.so, so that we can add functions to the end without breaking
     * backward compatibility.
     */
    __GLXapiImports imports;
    __GLdispatchPatchCallbacks patchCallbacks;

    UT_hash_handle hh;
} __GLXvendorNameHash;

static DEFINE_INITIALIZED_LKDHASH(__GLXvendorNameHash, __glXVendorNameHash);

typedef struct __GLXdisplayInfoHashRec {
    __GLXdisplayInfo info;
    UT_hash_handle hh;
} __GLXdisplayInfoHash;

static DEFINE_INITIALIZED_LKDHASH(__GLXdisplayInfoHash, __glXDisplayInfoHash);

struct __GLXvendorXIDMappingHashRec {
    XID xid;
    __GLXvendorInfo *vendor;
    UT_hash_handle hh;
};

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

    .addVendorDrawableMapping = __glXAddVendorDrawableMapping,
    .removeVendorDrawableMapping = __glXRemoveVendorDrawableMapping,
    .vendorFromDrawable = __glXVendorFromDrawable,
};

/*!
 * Looks for a GLX dispatch function.
 *
 * This function queries each loaded vendor to determine if there is
 * a vendor-implemented dispatch function. The dispatch function
 * uses the vendor <-> API library ABI to determine the screen given
 * the parameters of the function and dispatch to the correct vendor's
 * implementation.
 *
 * If no vendor provides a dispatch function, then instead we'll generate a
 * dispatch stub. We'll plug a real dispatch function into that stub later.
 */
__GLXextFuncPtr __glXGetGLXDispatchAddress(const GLubyte *procName)
{
    int index;
    __GLXextFuncPtr addr = NULL;
    Bool isGLX;
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

    // First, check if we've already found a dispatch stub. Note that this
    // generally shouldn't happen, because we cache the results of
    // glXGetProcAddress.

    // The vendor name hashtable's lock is also used for the dispatch index
    // list and the generated GLX entrypoints.
    LKDHASH_WRLOCK(__glXVendorNameHash);
    index = __glvndWinsysDispatchFindIndex((const char *) procName);
    if (index >= 0) {
        addr = (__GLXextFuncPtr) __glvndWinsysDispatchGetDispatch(index);

        LKDHASH_UNLOCK(__glXVendorNameHash);
        return addr;
    }

    // We haven't seen this function before, so we need to find or generate a
    // dispatch stub.

    // First, look for a GLX dispatch function from any vendor.
    HASH_ITER(hh, _LH(__glXVendorNameHash), pEntry, tmp) {
        addr = pEntry->vendor.glxvc->getDispatchAddress((const GLubyte *) procName);
        if (addr != NULL) {
            break;
        }
    }

    if (addr != NULL) {
        isGLX = True;
    } else {
        // Look to see if any vendor provides an implementation function. If
        // it does, then that means this is really a GL function that happens
        // to start with "glX".
        HASH_ITER(hh, _LH(__glXVendorNameHash), pEntry, tmp) {
            addr = pEntry->vendor.glxvc->getProcAddress((const GLubyte *) procName);
            if (addr != NULL) {
                break;
            }
        }

        if (addr != NULL) {
            // This is a GL function, so get a dispatch stub from
            // libGLdispatch.
            addr = __glDispatchGetProcAddress((const char *) procName);
            isGLX = False;
        } else {
            // None of the vendor libraries provide the function in either
            // form. That probably means it's a GLX extension function from a
            // vendor that hasn't been loaded yet. Generate a GLX entrypoint
            // stub. We'll plug in the real GLX dispatch function if and when
            // we load a vendor library that supports it.
            addr = (__GLXextFuncPtr) glvndGenerateEntrypoint((const char *) procName);
            isGLX = True;
        }
    }

    if (addr != NULL && isGLX) {
        index = __glvndWinsysDispatchAllocIndex((const char *) procName, addr);
        if (index >= 0) {
            HASH_ITER(hh, _LH(__glXVendorNameHash), pEntry, tmp) {
                pEntry->vendor.glxvc->setDispatchIndex(procName, index);
            }
        } else {
            addr = NULL;
        }
    }

    LKDHASH_UNLOCK(__glXVendorNameHash);

    return addr;
}

static GLVNDentrypointStub GLXEntrypointUpdateCallback(const char *procName, void *param)
{
    __GLXvendorInfo *vendor = (__GLXvendorInfo *) param;
    __GLXextFuncPtr addr = NULL;

    addr = vendor->glxvc->getDispatchAddress((const GLubyte *) procName);
    if (addr == NULL) {
        // If we didn't find a GLX dispatch function, then check for a normal
        // OpenGL function. This should handle any case where a GL extension
        // function starts with "glX".
        addr = vendor->glxvc->getProcAddress((const GLubyte *) procName);
        if (addr != NULL) {
            addr = __glDispatchGetProcAddress(procName);
        }
    }
    return (GLVNDentrypointStub) addr;
}

__GLXextFuncPtr __glXFetchDispatchEntry(__GLXvendorInfo *vendor,
                                        int index)
{
    __GLXextFuncPtr addr = NULL;
    const GLubyte *procName = NULL;

    addr = (__GLXextFuncPtr) __glvndWinsysVendorDispatchLookupFunc(vendor->dynDispatch, index);
    if (addr != NULL) {
        return addr;
    }

    // Not seen before by this vendor: query the vendor for the right
    // address to use.

    LKDHASH_RDLOCK(__glXVendorNameHash);
    procName = (const GLubyte *) __glvndWinsysDispatchGetName(index);
    LKDHASH_UNLOCK(__glXVendorNameHash);

    // This should have a valid entry point associated with it.
    if (procName == NULL) {
        assert(procName);
        return NULL;
    }

    // Get the real address.
    addr = vendor->glxvc->getProcAddress(procName);
    if (addr != NULL) {
        // Record the address in the vendor's hashtable. Note that if this
        // fails, it's not fatal. It just means we'll have to call
        // getProcAddress again the next time we need this function.
        __glvndWinsysVendorDispatchAddFunc(vendor->dynDispatch, index, addr);
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

    if (vendor->dynDispatch != NULL) {
        __glvndWinsysVendorDispatchDestroy(vendor->dynDispatch);
        vendor->dynDispatch = NULL;
    }

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
    LOADENTRYPOINT(createContextAttribsARB,     "glXCreateContextAttribsARB"    );
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
            int i, count;
            Bool success;

            // Previously unseen vendor. dlopen() the new vendor and add it to the
            // hash table.
            pEntry = calloc(1, sizeof(*pEntry) + vendorNameLen + 1);
            if (!pEntry) {
                goto fail;
            }
            vendor = &pEntry->vendor;

            vendor->glxvc = &pEntry->imports;
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

            vendor->glDispatch = (__GLdispatchTable *)
                __glDispatchCreateTable(
                    VendorGetProcAddressCallback,
                    vendor
                );
            if (!vendor->glDispatch) {
                goto fail;
            }

            /* Initialize the dynamic dispatch table */
            vendor->dynDispatch = __glvndWinsysVendorDispatchCreate();
            if (vendor->dynDispatch == NULL) {
                goto fail;
            }

            success = (*glxMainProc)(GLX_VENDOR_ABI_VERSION,
                                      &glxExportsTable,
                                      vendor, &pEntry->imports);
            if (!success) {
                goto fail;
            }

            // Make sure all the required functions are there.
            if (pEntry->imports.isScreenSupported == NULL
                    || pEntry->imports.getProcAddress == NULL
                    || pEntry->imports.getDispatchAddress == NULL
                    || pEntry->imports.setDispatchIndex == NULL)
            {
                goto fail;
            }

            if (!LookupVendorEntrypoints(vendor)) {
                goto fail;
            }

            // Check to see whether this vendor library can support entrypoint
            // patching.
            if (pEntry->imports.isPatchSupported != NULL
                    && pEntry->imports.initiatePatch != NULL) {
                pEntry->patchCallbacks.isPatchSupported = pEntry->imports.isPatchSupported;
                pEntry->patchCallbacks.initiatePatch = pEntry->imports.initiatePatch;
                pEntry->patchCallbacks.releasePatch = pEntry->imports.releasePatch;
                pEntry->patchCallbacks.threadAttach = pEntry->imports.patchThreadAttach;
                pEntry->vendor.patchCallbacks = &pEntry->patchCallbacks;
            }

            HASH_ADD_KEYPTR(hh, _LH(__glXVendorNameHash), vendor->name,
                            strlen(vendor->name), pEntry);

            // Look up the dispatch functions for any GLX extensions that we
            // generated entrypoints for.
            glvndUpdateEntrypoints(GLXEntrypointUpdateCallback, vendor);

            // Tell the vendor the index of all of the GLX dispatch stubs.
            count = __glvndWinsysDispatchGetCount();
            for (i=0; i<count; i++) {
                const char *procName = __glvndWinsysDispatchGetName(i);
                vendor->glxvc->setDispatchIndex((const GLubyte *) procName, i);
            }
        }
        LKDHASH_UNLOCK(__glXVendorNameHash);
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
        char envName[40];
        const char *specifiedVendorName;

        snprintf(envName, sizeof(envName), "__GLX_FORCE_VENDOR_LIBRARY_%d", screen);
        specifiedVendorName = getenv(envName);

        if (specifiedVendorName == NULL) {
            specifiedVendorName = getenv("__GLX_VENDOR_LIBRARY_NAME");
        }

        if (specifiedVendorName) {
            vendor = __glXLookupVendorByName(specifiedVendorName);
        }

        if (!vendor) {
            if (dpyInfo->libglvndExtensionSupported) {
                char *queriedVendorNames =
                    __glXQueryServerString(dpyInfo, screen, GLX_VENDOR_NAMES_EXT);
                if (queriedVendorNames != NULL) {
                    char *name, *saveptr;
                    for (name = strtok_r(queriedVendorNames, " ", &saveptr);
                            name != NULL;
                            name = strtok_r(NULL, " ", &saveptr)) {
                        vendor = __glXLookupVendorByName(name);

                        // Make sure that the vendor library can support this screen.
                        if (vendor != NULL && !vendor->glxvc->isScreenSupported(dpy, screen)) {
                            vendor = NULL;
                        }

                        if (vendor != NULL) {
                            break;
                        }
                    }
                    free(queriedVendorNames);
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
    int eventBase;

    size = sizeof(*pEntry) + ScreenCount(dpy) * sizeof(__GLXvendorInfo *);
    pEntry = (__GLXdisplayInfoHash *) malloc(size);
    if (pEntry == NULL) {
        return NULL;
    }

    memset(pEntry, 0, size);
    pEntry->info.dpy = dpy;
    pEntry->info.vendors = (__GLXvendorInfo **) (pEntry + 1);

    LKDHASH_INIT(pEntry->info.xidVendorHash);
    __glvndPthreadFuncs.rwlock_init(&pEntry->info.vendorLock, NULL);

    // Check whether the server supports the GLX extension, and record the
    // major opcode if it does.
    pEntry->info.glxSupported = XQueryExtension(dpy, GLX_EXTENSION_NAME,
            &pEntry->info.glxMajorOpcode, &eventBase,
            &pEntry->info.glxFirstError);

    if (pEntry->info.glxSupported) {
        int screen;

        // Check to see if the server supports the GLX_EXT_libglvnd extension.
        // Note that it has to be supported on every screen to use it.
        pEntry->info.libglvndExtensionSupported = True;
        for (screen = 0;
                screen < ScreenCount(dpy) && pEntry->info.libglvndExtensionSupported;
                screen++) {
            char *extensions = __glXQueryServerString(&pEntry->info, screen, GLX_EXTENSIONS);
            if (extensions != NULL) {
                if (!IsTokenInString(extensions, GLX_EXT_LIBGLVND_NAME,
                            strlen(GLX_EXT_LIBGLVND_NAME), " ")) {
                    pEntry->info.libglvndExtensionSupported = False;
                }
                free(extensions);
            } else {
                pEntry->info.libglvndExtensionSupported = False;
            }
        }
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

static int OnDisplayClosed(Display *dpy, XExtCodes *codes)
{
    __GLXdisplayInfoHash *pEntry = NULL;

    LKDHASH_WRLOCK(__glXDisplayInfoHash);

    HASH_FIND_PTR(_LH(__glXDisplayInfoHash), &dpy, pEntry);
    if (pEntry != NULL) {
        __glXDisplayClosed(&pEntry->info);
        HASH_DEL(_LH(__glXDisplayInfoHash), pEntry);
    }
    LKDHASH_UNLOCK(__glXDisplayInfoHash);

    CleanupDisplayInfoEntry(NULL, pEntry);
    free(pEntry);

    return 0;
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
        XExtCodes *extCodes = XAddExtension(dpy);
        if (extCodes == NULL) {
            CleanupDisplayInfoEntry(NULL, pEntry);
            free(pEntry);
            LKDHASH_UNLOCK(__glXDisplayInfoHash);
            return NULL;
        }

        XESetCloseDisplay(dpy, extCodes->extension, OnDisplayClosed);
        HASH_ADD_PTR(_LH(__glXDisplayInfoHash), info.dpy, pEntry);
    } else {
        // Another thread already created the hashtable entry.
        CleanupDisplayInfoEntry(NULL, pEntry);
        free(pEntry);
        pEntry = foundEntry;
    }
    LKDHASH_UNLOCK(__glXDisplayInfoHash);

    return &pEntry->info;
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

int __glXAddVendorFBConfigMapping(Display *dpy, GLXFBConfig config, __GLXvendorInfo *vendor)
{
    __GLXvendorConfigMappingHash *pEntry;

    if (config == NULL) {
        return 0;
    }

    if (vendor == NULL) {
        return -1;
    }

    LKDHASH_WRLOCK(fbconfigHashtable);

    HASH_FIND_PTR(_LH(fbconfigHashtable), &config, pEntry);

    if (pEntry == NULL) {
        pEntry = malloc(sizeof(*pEntry));
        if (pEntry == NULL) {
            LKDHASH_UNLOCK(fbconfigHashtable);
            return -1;
        }
        pEntry->config = config;
        pEntry->vendor = vendor;
        HASH_ADD_PTR(_LH(fbconfigHashtable), config, pEntry);
    } else {
        // Any GLXContext or GLXFBConfig handles must be unique to a single
        // vendor at a time. If we get two different vendors, then there's
        // either a bug in libGLX or in at least one of the vendor libraries.
        if (pEntry->vendor != vendor) {
            LKDHASH_UNLOCK(fbconfigHashtable);
            return -1;
        }
    }

    LKDHASH_UNLOCK(fbconfigHashtable);
    return 0;
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

__GLXvendorInfo *__glXVendorFromFBConfig(Display *dpy, GLXFBConfig config)
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

    return vendor;
}



/****************************************************************************/
/*
 * __GLXvendorXIDMappingHash is a hash table which maps XIDs to vendors.
 */


static int AddVendorXIDMapping(Display *dpy, __GLXdisplayInfo *dpyInfo, XID xid, __GLXvendorInfo *vendor)
{
    __GLXvendorXIDMappingHash *pEntry = NULL;

    if (xid == None) {
        return 0;
    }

    if (vendor == NULL) {
        return -1;
    }

    LKDHASH_WRLOCK(dpyInfo->xidVendorHash);

    HASH_FIND(hh, _LH(dpyInfo->xidVendorHash), &xid, sizeof(xid), pEntry);

    if (pEntry == NULL) {
        pEntry = malloc(sizeof(*pEntry));
        if (pEntry == NULL) {
            LKDHASH_UNLOCK(dpyInfo->xidVendorHash);
            return -1;
        }
        pEntry->xid = xid;
        pEntry->vendor = vendor;
        HASH_ADD(hh, _LH(dpyInfo->xidVendorHash), xid, sizeof(xid), pEntry);
    } else {
        // Like GLXContext and GLXFBConfig handles, any GLXDrawables must map
        // to a single vendor library.
        if (pEntry->vendor != vendor) {
            LKDHASH_UNLOCK(dpyInfo->xidVendorHash);
            return -1;
        }
    }

    LKDHASH_UNLOCK(dpyInfo->xidVendorHash);
    return 0;
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

        if (dpyInfo->libglvndExtensionSupported) {
            int screen = __glXGetDrawableScreen(dpyInfo, xid);
            if (screen >= 0 && screen < ScreenCount(dpy)) {
                vendor = __glXLookupVendorByScreen(dpy, screen);
                if (vendor != NULL) {
                    // Note that if this fails, it's not necessarily a problem.
                    // We can just query it again next time.
                    AddVendorXIDMapping(dpy, dpyInfo, xid, vendor);
                }
            }
        }
    }

    if (retVendor != NULL) {
        *retVendor = vendor;
    }
}


int __glXAddVendorDrawableMapping(Display *dpy, GLXDrawable drawable, __GLXvendorInfo *vendor)
{
    __GLXdisplayInfo *dpyInfo = __glXLookupDisplay(dpy);
    if (dpyInfo != NULL) {
        return AddVendorXIDMapping(dpy, dpyInfo, drawable, vendor);
    } else {
        return -1;
    }
}


void __glXRemoveVendorDrawableMapping(Display *dpy, GLXDrawable drawable)
{
    __GLXdisplayInfo *dpyInfo = __glXLookupDisplay(dpy);
    if (dpyInfo != NULL) {
        RemoveVendorXIDMapping(dpy, dpyInfo, drawable);
    }
}


__GLXvendorInfo *__glXVendorFromDrawable(Display *dpy, GLXDrawable drawable)
{
    __glXThreadInitialize();

    __GLXdisplayInfo *dpyInfo = __glXLookupDisplay(dpy);
    __GLXvendorInfo *vendor = NULL;
    if (dpyInfo != NULL) {
        if (dpyInfo->libglvndExtensionSupported) {
            VendorFromXID(dpy, dpyInfo, drawable, &vendor);
        } else {
            // We'll use the same vendor for every screen in this case.
            vendor = __glXLookupVendorByScreen(dpy, 0);
        }
    }

    return vendor;
}

void __glXMappingInit(void)
{
    int i;

    __glvndWinsysDispatchInit();

    // Add all of the GLX dispatch stubs that are defined in libGLX itself.
    for (i=0; LOCAL_GLX_DISPATCH_FUNCTIONS[i].name != NULL; i++) {
        // TODO: Is there any way to recover from a malloc failure here?
        __glvndWinsysDispatchAllocIndex(
                LOCAL_GLX_DISPATCH_FUNCTIONS[i].name,
                LOCAL_GLX_DISPATCH_FUNCTIONS[i].addr);
    }
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
        __glvndPthreadFuncs.rwlock_init(&fbconfigHashtable.lock, NULL);
        __glvndPthreadFuncs.rwlock_init(&__glXVendorNameHash.lock, NULL);
        __glvndPthreadFuncs.rwlock_init(&__glXDisplayInfoHash.lock, NULL);

        HASH_ITER(hh, _LH(__glXDisplayInfoHash), dpyInfoEntry, dpyInfoTmp) {
            __glvndPthreadFuncs.rwlock_init(&dpyInfoEntry->info.xidVendorHash.lock, NULL);
            __glvndPthreadFuncs.rwlock_init(&dpyInfoEntry->info.vendorLock, NULL);
        }
    } else {
        __GLXvendorNameHash *pEntry, *tmp;

        /* Tear down all hashtables used in this file */
        __glvndWinsysDispatchCleanup();

        // If a GLX vendor library has patched the OpenGL entrypoints, then
        // unpatch them before we unload the vendors.
        LKDHASH_RDLOCK(__glXVendorNameHash);
        HASH_ITER(hh, _LH(__glXVendorNameHash), pEntry, tmp) {
            __glDispatchForceUnpatch(pEntry->vendor.vendorID);
        }
        LKDHASH_UNLOCK(__glXVendorNameHash);

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
