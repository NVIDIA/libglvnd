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

/* For RTLD_DEFAULT on x86 systems */
#define _GNU_SOURCE 1

#include <X11/Xlib.h>
#include <X11/Xlibint.h>
#include <X11/Xproto.h>
#include <dlfcn.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>

#include "libglxthread.h"
#include "libglxabipriv.h"
#include "libglxmapping.h"
#include "libglxcurrent.h"
#include "libglxstring.h"
#include "utils_misc.h"
#include "trace.h"
#include "GL/glxproto.h"
#include "x11glvnd.h"
#include "libglxgl.h"
#include "glvnd_list.h"

#include "lkdhash.h"

/* current version numbers */
#define GLX_MAJOR_VERSION 1
#define GLX_MINOR_VERSION 4

GLVNDPthreadFuncs __glXPthreadFuncs;

static glvnd_mutex_t clientStringLock = GLVND_MUTEX_INITIALIZER;

/*
 * Hashtable tracking current contexts for the purpose of determining whether
 * glXDestroyContext() should remove the context -> screen mapping immediately,
 * or defer this until the context loses current.
 */
typedef struct __GLXcurrentContextHashRec {
    GLXContext ctx;
    Bool needsUnmap;
    UT_hash_handle hh;
} __GLXcurrentContextHash;

static __GLXcurrentContextHash *currentContextHash = NULL;
static pthread_mutex_t currentContextHashLock;

/**
 * A list of current __GLXAPIState structures. This is used so that we can
 * clean up at process termination or after a fork.
 */
static struct glvnd_list currentAPIStateList;
static glvnd_mutex_t currentAPIStateListMutex = PTHREAD_MUTEX_INITIALIZER;

static __GLXAPIState *CreateAPIState(__GLXvendorInfo *vendor);
static void DestroyAPIState(__GLXAPIState *apiState);
static Bool UpdateCurrentContext(GLXContext newCtx, GLXContext oldCtx);

static void __glXSendError(Display *dpy, unsigned char errorCode,
        XID resourceID, unsigned char minorCode, Bool coreX11error);

/*!
 * A common helper for GLX functions that dispatch based on a drawable.
 *
 * This function will call __glXThreadInitialize and then look up the vendor
 * for a drawable.
 *
 * If it can't find a vendor for the drawable, then it will call __glXSendError
 * to generate an error.
 *
 * Note that if the server doesn't support the x11glvnd extension, then this
 * will return the same vendor library whether or not the drawable is valid.
 * In that case, we'll just rely on the vendor library to report the error if
 * the drawable is not valid.
 *
 * \param dpy The display connection.
 * \param draw The drawable XID.
 * \param minorCode The minor opcode of the function being called.
 * \param errorCode The error code to report if the drawable is invalid.
 * \param coreX11error True if the error is a core X11 error code, or False if
 *      it's a GLX error code.
 */
static __GLXvendorInfo *CommonDispatchDrawable(Display *dpy, GLXDrawable draw,
        unsigned char minorCode, unsigned char errorCode, Bool coreX11error)
{
    __GLXvendorInfo *vendor = NULL;

    if (draw != None) {
        __glXThreadInitialize();
        __glXVendorFromDrawable(dpy, draw, NULL, &vendor);
    }
    if (vendor == NULL) {
        __glXSendError(dpy, errorCode, draw, minorCode, coreX11error);
    }
    return vendor;
}

static __GLXvendorInfo *CommonDispatchContext(Display *dpy, GLXContext context,
        unsigned char minorCode)
{
    __GLXvendorInfo *vendor = NULL;

    if (context != NULL) {
        __glXThreadInitialize();
        __glXVendorFromContext(context, NULL, NULL, &vendor);
    }
    if (vendor == NULL) {
        __glXSendError(dpy, GLXBadContext, 0, minorCode, False);
    }
    return vendor;
}

PUBLIC XVisualInfo* glXChooseVisual(Display *dpy, int screen, int *attrib_list)
{
    __glXThreadInitialize();

    const __GLXdispatchTableStatic *pDispatch = __glXGetStaticDispatch(dpy, screen);

    return pDispatch->chooseVisual(dpy, screen, attrib_list);
}


PUBLIC void glXCopyContext(Display *dpy, GLXContext src, GLXContext dst,
                    unsigned long mask)
{
    /*
     * GLX requires that src and dst are on the same X screen, but the
     * application may have passed invalid input.  Pick the screen
     * from one of the contexts, and then let that vendor's
     * implementation validate that both contexts are on the same
     * screen.
     */
    __GLXvendorInfo *vendor = CommonDispatchContext(dpy, src, X_GLXCopyContext);
    if (vendor != NULL) {
        vendor->staticDispatch.copyContext(dpy, src, dst, mask);
    }
}


PUBLIC GLXContext glXCreateContext(Display *dpy, XVisualInfo *vis,
                            GLXContext share_list, Bool direct)
{
    __glXThreadInitialize();

    const int screen = vis->screen;
    const __GLXdispatchTableStatic *pDispatch = __glXGetStaticDispatch(dpy, screen);
    __GLXvendorInfo *vendor = __glXLookupVendorByScreen(dpy, screen);

    GLXContext context = pDispatch->createContext(dpy, vis, share_list, direct);

    __glXAddScreenContextMapping(dpy, context, screen, vendor);

    return context;
}


PUBLIC GLXContext glXCreateNewContext(Display *dpy, GLXFBConfig config,
                               int render_type, GLXContext share_list,
                               Bool direct)
{
    __glXThreadInitialize();

    const int screen = __glXScreenFromFBConfig(config);
    const __GLXdispatchTableStatic *pDispatch = __glXGetStaticDispatch(dpy, screen);
    __GLXvendorInfo *vendor = __glXLookupVendorByScreen(dpy, screen);

    GLXContext context = pDispatch->createNewContext(dpy, config, render_type,
                                                     share_list, direct);
    __glXAddScreenContextMapping(dpy, context, screen, vendor);

    return context;
}

PUBLIC void glXDestroyContext(Display *dpy, GLXContext context)
{
    __GLXvendorInfo *vendor = CommonDispatchContext(dpy, context, X_GLXDestroyContext);
    if (vendor != NULL) {
        __glXNotifyContextDestroyed(context);
        vendor->staticDispatch.destroyContext(dpy, context);
    }
}

static Bool __glXIsDirect(Display *dpy, __GLXdisplayInfo *dpyInfo, GLXContextID context)
{
    xGLXIsDirectReq *req;
    xGLXIsDirectReply reply;

    assert(dpyInfo->glxSupported);

    LockDisplay(dpy);

    GetReq(GLXIsDirect, req);
    req->reqType = dpyInfo->glxMajorOpcode;
    req->glxCode = X_GLXIsDirect;
    req->context = context;
    _XReply(dpy, (xReply *) &reply, 0, False);

    UnlockDisplay(dpy);
    SyncHandle();

    return reply.isDirect;
}

/**
 * Finds the screen number for a context, using the context's XID. This
 * function sends the request directly, so it doesn't rely on any vendor
 * library.
 *
 * Adapted from Mesa's glXImportContextEXT implementation.
 */
static int __glXGetScreenForContextID(Display *dpy, __GLXdisplayInfo *dpyInfo,
        GLXContextID contextID)
{
    xGLXQueryContextReply reply;
    int *propList;
    int majorVersion, minorVersion;
    int screen = -1;
    int i;

    assert(dpyInfo->glxSupported);

    // Check the version number so that we know which request to send.
    if (!glXQueryVersion(dpy, &majorVersion, &minorVersion)) {
        return -1;
    }

    /* Send the glXQueryContextInfoEXT request */
    LockDisplay(dpy);

    if (majorVersion > 1 || minorVersion >= 3) {
        xGLXQueryContextReq *req;

        GetReq(GLXQueryContext, req);

        req->reqType = dpyInfo->glxMajorOpcode;
        req->glxCode = X_GLXQueryContext;
        req->context = contextID;
    } else {
        xGLXVendorPrivateReq *vpreq;
        xGLXQueryContextInfoEXTReq *req;

        GetReqExtra(GLXVendorPrivate,
                sz_xGLXQueryContextInfoEXTReq - sz_xGLXVendorPrivateReq,
                vpreq);
        req = (xGLXQueryContextInfoEXTReq *) vpreq;
        req->reqType = dpyInfo->glxMajorOpcode;
        req->glxCode = X_GLXVendorPrivateWithReply;
        req->vendorCode = X_GLXvop_QueryContextInfoEXT;
        req->context = contextID;
    }

    _XReply(dpy, (xReply *) &reply, 0, False);

    if (reply.n <= 0) {
        UnlockDisplay(dpy);
        SyncHandle();
        return -1;
    }

    propList = malloc(reply.n * 8);
    if (propList == NULL) {
        UnlockDisplay(dpy);
        SyncHandle();
        return -1;
    }
    _XRead(dpy, (char *) propList, reply.n * 8);

    UnlockDisplay(dpy);
    SyncHandle();

    for (i=0; i<reply.n; i++) {
        int *prop = &propList[i * 2];
        if (prop[0] == GLX_SCREEN) {
            screen = prop[1];
            break;
        }
    }
    free(propList);
    return screen;
}

static GLXContext glXImportContextEXT(Display *dpy, GLXContextID contextID)
{
    __GLXdisplayInfo *dpyInfo;
    int screen;
    __GLXvendorInfo *vendor;

    dpyInfo = __glXLookupDisplay(dpy);
    if (dpyInfo == NULL || !dpyInfo->glxSupported) {
        return NULL;
    }

    /* The GLX_EXT_import_context spec says:
    *
    *     "If <contextID> does not refer to a valid context, then a BadContext
    *     error is generated; if <contextID> refers to direct rendering
    *     context then no error is generated but glXImportContextEXT returns
    *     NULL."
    *
    * If contextID is None, generate BadContext on the client-side.  Other
    * sorts of invalid contexts will be detected by the server in the
    * __glXIsDirect call.
    */
    if (contextID == None) {
        __glXSendError(dpy, GLXBadContext, contextID, X_GLXIsDirect, False);
        return NULL;
    }

    if (__glXIsDirect(dpy, dpyInfo, contextID)) {
        return NULL;
    }

    // Find the screen number for the context. We can't rely on a vendor
    // library yet, so send the request manually.
    screen = __glXGetScreenForContextID(dpy, dpyInfo, contextID);
    if (screen < 0) {
        return NULL;
    }

    vendor = __glXLookupVendorByScreen(dpy, screen);
    if (vendor != NULL && vendor->staticDispatch.importContextEXT != NULL) {
        GLXContext context = vendor->staticDispatch.importContextEXT(dpy, contextID);
        __glXAddScreenContextMapping(dpy, context, screen, vendor);
        return context;
    } else {
        return NULL;
    }
}

static void glXFreeContextEXT(Display *dpy, GLXContext context)
{
    __GLXvendorInfo *vendor = NULL;

    __glXThreadInitialize();

    __glXVendorFromContext(context, NULL, NULL, &vendor);
    if (vendor != NULL && vendor->staticDispatch.freeContextEXT != NULL) {
        __glXNotifyContextDestroyed(context);
        vendor->staticDispatch.freeContextEXT(dpy, context);
    }
}


PUBLIC GLXPixmap glXCreateGLXPixmap(Display *dpy, XVisualInfo *vis, Pixmap pixmap)
{
    __glXThreadInitialize();

    const int screen = vis->screen;
    const __GLXdispatchTableStatic *pDispatch = __glXGetStaticDispatch(dpy, screen);
    __GLXvendorInfo *vendor = __glXLookupVendorByScreen(dpy, screen);

    GLXPixmap pmap = pDispatch->createGLXPixmap(dpy, vis, pixmap);

    __glXAddScreenDrawableMapping(dpy, pmap, screen, vendor);

    return pmap;
}


PUBLIC void glXDestroyGLXPixmap(Display *dpy, GLXPixmap pix)
{
    __GLXvendorInfo *vendor = CommonDispatchDrawable(dpy, pix,
            X_GLXDestroyGLXPixmap, GLXBadPixmap, False);
    if (vendor != NULL) {
        __glXRemoveScreenDrawableMapping(dpy, pix);
        vendor->staticDispatch.destroyGLXPixmap(dpy, pix);
    }
}


PUBLIC int glXGetConfig(Display *dpy, XVisualInfo *vis, int attrib, int *value)
{
    __glXThreadInitialize();

    const __GLXdispatchTableStatic *pDispatch;

    if (!dpy || !vis || !value) {
        return GLX_BAD_VALUE;
    }

    pDispatch = __glXGetStaticDispatch(dpy, vis->screen);

    return pDispatch->getConfig(dpy, vis, attrib, value);
}

PUBLIC GLXContext glXGetCurrentContext(void)
{
    __glXThreadInitialize();

    return __glXGetCurrentContext();
}


PUBLIC GLXDrawable glXGetCurrentDrawable(void)
{
    __glXThreadInitialize();

    __GLXAPIState *apiState = __glXGetCurrentAPIState();
    if (apiState != NULL) {
        return apiState->currentDraw;
    } else {
        return None;
    }
}

PUBLIC GLXDrawable glXGetCurrentReadDrawable(void)
{
    __glXThreadInitialize();

    __GLXAPIState *apiState = __glXGetCurrentAPIState();
    if (apiState != NULL) {
        return apiState->currentRead;
    } else {
        return None;
    }
}

PUBLIC Display *glXGetCurrentDisplay(void)
{
    __glXThreadInitialize();

    __GLXAPIState *apiState = __glXGetCurrentAPIState();
    if (apiState != NULL) {
        return apiState->currentDisplay;
    } else {
        return NULL;
    }
}

__GLXvendorInfo *__glXGetCurrentDynDispatch(void)
{
    __GLXAPIState *apiState = __glXGetCurrentAPIState();

    if (apiState != NULL) {
        return apiState->currentVendor;
    } else {
        return NULL;
    }
}

PUBLIC Bool glXIsDirect(Display *dpy, GLXContext context)
{
    __GLXvendorInfo *vendor = CommonDispatchContext(dpy, context, X_GLXIsDirect);
    if (vendor != NULL) {
        return vendor->staticDispatch.isDirect(dpy, context);
    } else {
        return False;
    }
}

void DisplayClosed(Display *dpy)
{
    __GLXAPIState *apiState;
    __glXFreeDisplay(dpy);

    apiState = __glXGetCurrentAPIState();
    if (apiState != NULL && apiState->currentDisplay == dpy) {
        // Clear out the current context, but don't call into the vendor
        // library or do anything that might require a valid display.
        __glDispatchLoseCurrent();
        __glXPthreadFuncs.mutex_lock(&currentContextHashLock);
        UpdateCurrentContext(NULL, apiState->currentContext);
        __glXPthreadFuncs.mutex_unlock(&currentContextHashLock);
        DestroyAPIState(apiState);
    }

    __glXPthreadFuncs.mutex_lock(&currentAPIStateListMutex);
    glvnd_list_for_each_entry(apiState, &currentAPIStateList, entry) {
        /*
         * Stub out any references to this display in any other API states.
         */
        if (apiState->currentDisplay == dpy) {
            apiState->currentDisplay = NULL;
        }
    }
    __glXPthreadFuncs.mutex_unlock(&currentAPIStateListMutex);
}

static void ThreadDestroyed(__GLdispatchAPIState *apiState)
{
    __GLXAPIState *glxState = (__GLXAPIState *) apiState;

    /*
     * If a GLX context is current in this thread, remove it from the
     * current context hash before destroying the thread.
     */
    __glXPthreadFuncs.mutex_lock(&currentContextHashLock);
    UpdateCurrentContext(NULL, glxState->currentContext);
    __glXPthreadFuncs.mutex_unlock(&currentContextHashLock);

    // Free the API state struct.
    DestroyAPIState(glxState);
}

static __GLXAPIState *CreateAPIState(__GLXvendorInfo *vendor)
{
    __GLXAPIState *apiState = calloc(1, sizeof(*apiState));

    assert(apiState);

    apiState->glas.tag = GLDISPATCH_API_GLX;
    apiState->glas.threadDestroyedCallback = ThreadDestroyed;
    apiState->currentVendor = vendor;

    __glXPthreadFuncs.mutex_lock(&currentAPIStateListMutex);
    glvnd_list_add(&apiState->entry, &currentAPIStateList);
    __glXPthreadFuncs.mutex_unlock(&currentAPIStateListMutex);

    return apiState;
}

static void DestroyAPIState(__GLXAPIState *apiState)
{
    // Free the API state struct.
    __glXPthreadFuncs.mutex_lock(&currentAPIStateListMutex);
    glvnd_list_del(&apiState->entry);
    __glXPthreadFuncs.mutex_unlock(&currentAPIStateListMutex);

    free(apiState);
}

/*
 * Notifies libglvnd that the given context has been marked for destruction
 * by glXDestroyContext(), and removes any context -> screen mappings if
 * necessary.
 */
void __glXNotifyContextDestroyed(GLXContext ctx)
{
    Bool canUnmap = True;
    __GLXcurrentContextHash *pEntry = NULL;
    __glXPthreadFuncs.mutex_lock(&currentContextHashLock);

    HASH_FIND(hh, currentContextHash, &ctx, sizeof(ctx), pEntry);

    if (pEntry) {
        canUnmap = False;
        pEntry->needsUnmap = True;
    }

    if (canUnmap) {
        /*
         * Note: this implies a lock ordering: the current context
         * hash lock must be taken before the screen pointer hash lock!
         */
        __glXRemoveScreenContextMapping(NULL, ctx);
    }

    __glXPthreadFuncs.mutex_unlock(&currentContextHashLock);

}

/*!
 * Updates the current context.
 *
 * If both newCtx and oldCtx are not NULL, then it will replace the old context
 * with the new context in the currentContextHash table.
 *
 * This function will remove the old context from the currentContextHash
 * hashtable, and add the new context to it.
 *
 * If the old context was flagged for deletion, then it will also remove the
 * context from the (context, screen) mapping.
 *
 * \note that the only case where this function can fail is if \p oldCtx is NULL
 * and \p newCtx is not NULL, because it has to allocate a new hashtable entry.
 *
 * \note currentContextHashLock must be locked before calling this
 * function.
 *
 * \param[in] newCtx The new context to make current, or \c NULL to just
 * release the current context.
 * \param[in] oldCtx The previous current context, or \c NULL if no context was
 * current before.
 *
 * \return True on success, False otherwise.
 */
static Bool UpdateCurrentContext(GLXContext newCtx,
                                 GLXContext oldCtx)
{
    __GLXcurrentContextHash *pEntry;

    if (oldCtx != NULL) {
        Bool needsUnmap;

        HASH_FIND(hh, currentContextHash, &oldCtx, sizeof(oldCtx), pEntry);
        assert(pEntry);

        HASH_DELETE(hh, currentContextHash, pEntry);
        needsUnmap = pEntry->needsUnmap;

        if (newCtx != NULL)
        {
            // Switching to a new context. Reuse the same structure so that we
            // don't have to worry about malloc failing.
            pEntry->ctx = newCtx;
            pEntry->needsUnmap = False;
            HASH_ADD(hh, currentContextHash, ctx, sizeof(newCtx), pEntry);
        }
        else
        {
            // We're releasing the current context, so free the structure.
            free(pEntry);
        }

        if (needsUnmap) {
            __glXRemoveScreenContextMapping(NULL, oldCtx);
        }
    } else {
        // We're adding a current context but we didn't have one before, so
        // allocate a new hash entry.
        assert(newCtx != NULL);

        pEntry = malloc(sizeof(*pEntry));
        if (!pEntry) {
            return False;
        }
        pEntry->ctx = newCtx;
        pEntry->needsUnmap = False;
        HASH_ADD(hh, currentContextHash, ctx, sizeof(newCtx), pEntry);
    }

    return True;
}

/*
 * Note: __glXCurrentContextHash must be (read or write)-locked before calling
 * this function!
 */
static Bool IsContextCurrentToAnyOtherThread(GLXContext ctx)
{
    GLXContext current = glXGetCurrentContext();
    __GLXcurrentContextHash *pEntry = NULL;

    HASH_FIND(hh, currentContextHash, &ctx, sizeof(ctx), pEntry);

    return !!pEntry && (current != ctx);
}

static void __glXSendError(Display *dpy, unsigned char errorCode,
        XID resourceID, unsigned char minorCode, Bool coreX11error)
{
    __GLXdisplayInfo *dpyInfo = NULL;
    xError error;

    if (dpy == NULL) {
        return;
    }

    dpyInfo = __glXLookupDisplay(dpy);

    if (dpyInfo == NULL || !dpyInfo->glxSupported) {
        return;
    }

    LockDisplay(dpy);

    error.type = X_Error;
    error.errorCode = errorCode;
    error.sequenceNumber = dpy->request;
    error.resourceID = resourceID;
    error.minorCode = minorCode;
    error.majorCode = dpyInfo->glxMajorOpcode;
    if (!coreX11error) {
        error.errorCode += dpyInfo->glxFirstError;
    }

    _XError(dpy, &error);

    UnlockDisplay(dpy);
}

static void NotifyXError(Display *dpy, unsigned char errorCode,
        XID resourceID, unsigned char minorCode, Bool coreX11error,
        __GLXvendorInfo *vendor)
{
    if (vendor != NULL && vendor->glxvc->notifyError != NULL) {
        Bool ret = vendor->glxvc->notifyError(dpy, errorCode, resourceID,
                minorCode, coreX11error);
        if (!ret) {
            return;
        }
    }
    __glXSendError(dpy, errorCode, resourceID, minorCode, coreX11error);
}

static Bool InternalLoseCurrent(void)
{
    __GLXAPIState *apiState = __glXGetCurrentAPIState();
    Bool ret;

    if (apiState == NULL) {
        return True;
    }

    ret = apiState->currentVendor->staticDispatch.makeCurrent(apiState->currentDisplay, None, NULL);
    if (!ret) {
        return False;
    }

    __glDispatchLoseCurrent();

    // Remove the context from the current context map.
    UpdateCurrentContext(NULL, apiState->currentContext);
    DestroyAPIState(apiState);

    return True;
}

/**
 * Calls into the vendor library to set the current context, and then updates
 * the API state fields to match.
 *
 * This function does *not* call into libGLdispatch, so it can only switch
 * to another context with the same vendor.
 *
 * If this function succeeds, then it will update the current display, context,
 * and drawables in \p apiState.
 *
 * If it fails, then it will leave \p apiState unmodified. It's up to the
 * vendor library to ensure that the old context is still current in that case.
 */
static Bool InternalMakeCurrentVendor(
        Display *dpy, GLXDrawable draw, GLXDrawable read,
        GLXContext context, char callerOpcode,
        __GLXAPIState *apiState,
        __GLXvendorInfo *vendor)
{
    Bool ret;

    assert(apiState->currentVendor == vendor);

    if (callerOpcode == X_GLXMakeCurrent && draw == read) {
        ret = vendor->staticDispatch.makeCurrent(dpy, draw, context);
    } else {
        ret = vendor->staticDispatch.makeContextCurrent(dpy,
                                                    draw,
                                                    read,
                                                    context);
    }

    if (ret) {
        apiState->currentDisplay = dpy;
        apiState->currentDraw = draw;
        apiState->currentRead = read;
        apiState->currentContext = context;
    }

    return ret;
}

/**
 * Makes a context current. This function handles both the vendor library and
 * libGLdispatch.
 *
 * There must not be a current API state in libGLdispatch when this function is
 * called.
 *
 * If this function fails, then it will release the context and dispatch state
 * before returning.
 */
static Bool InternalMakeCurrentDispatch(
        Display *dpy, GLXDrawable draw, GLXDrawable read,
        GLXContext context, char callerOpcode,
        __GLXvendorInfo *vendor)
{
    __GLXAPIState *apiState;
    Bool ret;

    assert(__glXGetCurrentAPIState() == NULL);

    if (!UpdateCurrentContext(context, NULL)) {
        return False;
    }

    apiState = CreateAPIState(vendor);
    if (apiState == NULL) {
        UpdateCurrentContext(NULL, context);
        return False;
    }

    ret = __glDispatchMakeCurrent(
        &apiState->glas,
        vendor->glDispatch,
        vendor->vendorID,
        vendor->glxvc->patchCallbacks
    );

    if (ret) {
        // Call into the vendor library.
        ret = InternalMakeCurrentVendor(dpy, draw, read, context, callerOpcode,
                apiState, vendor);
        if (!ret) {
            __glDispatchLoseCurrent();
        }
    }

    if (!ret) {
        DestroyAPIState(apiState);
        UpdateCurrentContext(NULL, context);
    }

    return ret;
}

/**
 * A common function to handle glXMakeCurrent and glXMakeContextCurrent.
 */
static Bool CommonMakeCurrent(Display *dpy, GLXDrawable draw,
                                  GLXDrawable read, GLXContext context,
                                  char callerOpcode)
{
    __GLXAPIState *apiState;
    __GLXvendorInfo *oldVendor, *newVendor;
    Display *oldDpy;
    GLXDrawable oldDraw, oldRead;
    GLXContext oldContext;
    Bool ret;

    __glXThreadInitialize();
    apiState = __glXGetCurrentAPIState();

    if (apiState != NULL) {
        oldVendor = apiState->currentVendor;
        oldDpy = apiState->currentDisplay;
        oldDraw = apiState->currentDraw;
        oldRead = apiState->currentRead;
        oldContext = apiState->currentContext;

        assert(oldContext != NULL);

        if (dpy == oldDpy && context == oldContext
                && draw == oldDraw && read == oldRead) {
            // The current display, context, and drawables are the same, so just
            // return.
            return True;
        }
    } else {
        // We don't have a current context already.
        oldVendor = NULL;
        oldDpy = NULL;
        oldDraw = oldRead = None;
        oldContext = NULL;
    }

    /*
     * If <ctx> is NULL and <draw> and <read> are not None, or if <draw> or
     * <read> are set to None and <ctx> is not NULL, then a BadMatch error will
     * be generated. GLX 1.4 section 3.3.7 (p. 27).
     *
     * However, GLX_ARB_create_context specifies that GL 3.0+ contexts may be
     * made current without a default framebuffer, so the "or if..." part above
     * is ignored here.
     */
    if (!context && (draw != None || read != None)) {
        // Notify the vendor library and send the X error. Since we don't have
        // a new context, instead notify the vendor library that owns the
        // current context (if there is one).

        NotifyXError(dpy, BadMatch, 0, callerOpcode, True, oldVendor);
        return False;
    }

    if (oldContext == NULL && context == NULL) {
        // If both the old and new contexts are NULL, then there's nothing to
        // do. Just return early.
        return True;
    }


    __glXPthreadFuncs.mutex_lock(&currentContextHashLock);

    if (context != NULL) {
        if (IsContextCurrentToAnyOtherThread(context)) {
            __glXPthreadFuncs.mutex_unlock(&currentContextHashLock);
            NotifyXError(dpy, BadAccess, 0, callerOpcode, True, oldVendor);
            return False;
        }

        if (__glXVendorFromContext(context, NULL, NULL, &newVendor) != 0) {
            /*
             * We can run into this corner case if a GLX client calls
             * glXDestroyContext() on a current context, loses current to this
             * context (causing it to be freed), then tries to make current to the
             * context again.  This is incorrect application behavior, but we should
             * attempt to handle this failure gracefully.
             */
            __glXPthreadFuncs.mutex_unlock(&currentContextHashLock);
            NotifyXError(dpy, GLXBadContext, 0, callerOpcode, False, oldVendor);
            return False;
        }
        assert(newVendor != NULL);
    } else {
        newVendor = NULL;
    }

    if (oldVendor == newVendor) {
        assert(apiState != NULL);

        /*
         * We're switching between two contexts that use the same vendor. That
         * means the dispatch table is also the same, which is the only thing
         * that libGLdispatch cares about. Call into the vendor library to
         * switch contexts, but don't call into libGLdispatch.
         */
        ret = InternalMakeCurrentVendor(dpy, draw, read, context, callerOpcode,
                apiState, newVendor);
        if (ret) {
            UpdateCurrentContext(context, oldContext);
        }
    } else if (newVendor == NULL) {
        /*
         * We have a current context and we're releasing it.
         */
        assert(context == NULL);
        ret = InternalLoseCurrent();

    } else if (oldVendor == NULL) {
        /*
         * We don't have a current context, so we only need to make the new one
         * current.
         */
        ret = InternalMakeCurrentDispatch(dpy, draw, read, context, callerOpcode,
                newVendor);
    } else {
        /*
         * We're switching between contexts with different vendors.
         *
         * This gets tricky because we have to call into both vendor libraries
         * and libGLdispatch. Any of those can fail, and if it does, then we
         * have to make sure libGLX, libGLdispatch, and the vendor libraries
         * all agree on what the current context is.
         *
         * To do that, we'll first release the current context, and then make
         * the new context current.
         */
        ret = InternalLoseCurrent();

        if (ret) {
            ret = InternalMakeCurrentDispatch(dpy, draw, read, context, callerOpcode,
                    newVendor);
            if (!ret) {
                /*
                 * Try to restore the old context. Note that this can fail if
                 * the old context was marked for deletion. If that happens,
                 * then we'll end up with no current context instead, but we
                 * should at least still be in a consistent state.
                 */
                InternalMakeCurrentDispatch(oldDpy, oldDraw, oldRead, oldContext,
                        callerOpcode, oldVendor);
            }
        }
    }

    __glXPthreadFuncs.mutex_unlock(&currentContextHashLock);
    return ret;
}

PUBLIC Bool glXMakeCurrent(Display *dpy, GLXDrawable drawable, GLXContext context)
{
    return CommonMakeCurrent(dpy, drawable, drawable, context, X_GLXMakeCurrent);
}

PUBLIC Bool glXMakeContextCurrent(Display *dpy, GLXDrawable draw,
                                  GLXDrawable read, GLXContext context)
{
    return CommonMakeCurrent(dpy, draw, read, context, X_GLXMakeContextCurrent);
}

PUBLIC Bool glXQueryExtension(Display *dpy, int *error_base, int *event_base)
{
    __glXThreadInitialize();

    /*
     * There isn't enough information to dispatch to a vendor's
     * implementation, so handle the request here.
     */
    int major, event, error;
    Bool ret = XQueryExtension(dpy, "GLX", &major, &event, &error);
    if (ret) {
        if (error_base) {
            *error_base = error;
        }
        if (event_base) {
            *event_base = event;
        }
    }
    return ret;
}


PUBLIC Bool glXQueryVersion(Display *dpy, int *major, int *minor)
{
    __glXThreadInitialize();

    /*
     * There isn't enough information to dispatch to a vendor's
     * implementation, so handle the request here.
     *
     * Adapted from mesa's
     *
     * gallium/state_trackers/egl/x11/glxinit.c:QueryVersion()
     *
     * TODO: Mesa's GLX state tracker uses xcb-glx rather than Xlib to perform
     * the query. Should we do the same here?
     */
    xGLXQueryVersionReq *req;
    xGLXQueryVersionReply reply;
    __GLXdisplayInfo *dpyInfo = NULL;
    Bool ret;

    dpyInfo = __glXLookupDisplay(dpy);
    if (dpyInfo == NULL || !dpyInfo->glxSupported) {
        return False;
    }

    LockDisplay(dpy);
    GetReq(GLXQueryVersion, req);
    req->reqType = dpyInfo->glxMajorOpcode;
    req->glxCode = X_GLXQueryVersion;
    req->majorVersion = GLX_MAJOR_VERSION;
    req->minorVersion = GLX_MINOR_VERSION;

    ret = _XReply(dpy, (xReply *)&reply, 0, False);
    UnlockDisplay(dpy);
    SyncHandle();

    if (!ret) {
        return False;
    }

    if (reply.majorVersion != GLX_MAJOR_VERSION) {
        /* Server does not support same major as client */
        return False;
    }

    if (major) {
        *major = reply.majorVersion;
    }
    if (minor) {
        *minor = reply.minorVersion;
    }

    return True;
}


PUBLIC void glXSwapBuffers(Display *dpy, GLXDrawable drawable)
{
    __GLXvendorInfo *vendor = CommonDispatchDrawable(dpy, drawable,
            X_GLXSwapBuffers, GLXBadDrawable, False);
    if (vendor != NULL) {
        vendor->staticDispatch.swapBuffers(dpy, drawable);
    }
}


PUBLIC void glXUseXFont(Font font, int first, int count, int list_base)
{
    __glXThreadInitialize();

    const __GLXdispatchTableStatic *pDispatch = __glXGetCurrentDispatch();

    pDispatch->useXFont(font, first, count, list_base);
}


PUBLIC void glXWaitGL(void)
{
    __glXThreadInitialize();

    const __GLXdispatchTableStatic *pDispatch = __glXGetCurrentDispatch();

    pDispatch->waitGL();
}


PUBLIC void glXWaitX(void)
{
    __glXThreadInitialize();

    const __GLXdispatchTableStatic *pDispatch = __glXGetCurrentDispatch();

    pDispatch->waitX();
}

/**
 * Queries a client string for each screen in a display.
 *
 * The returned array will have one element for each screen. The caller must
 * free the array by calling free.
 *
 * \param dpy The display connection.
 * \param name The string to query (GLX_VENDOR, GLX_VERSION, or GLX_EXTENSION).
 * \return An array of strings, or NULL on error.
 */
static const char **GetVendorClientStrings(Display *dpy, int name)
{
    int num_screens = XScreenCount(dpy);
    const char **result = malloc(num_screens * sizeof(const char *));
    int screen;
    if (result == NULL) {
        return NULL;
    }

    for (screen = 0; screen < num_screens; screen++) {
        const __GLXdispatchTableStatic *pDispatch = __glXGetStaticDispatch(dpy, screen);
        result[screen] = pDispatch->getClientString(dpy, name);
        if (result[screen] == NULL) {
            free(result);
            return NULL;
        }
    }
    return result;
}

/**
 * Merges two GLX_EXTENSIONS strings.
 *
 * If \p newString is a subset of \c currentString, then \c currentString will
 * be returned unmodified. Otherwise, \c currentString will be re-allocated
 * with enough space to hold the union of both string.
 *
 * If an error occurrs, then \c currentString will be freed before returning.
 *
 * \param currentString The current string, which must have been allocated with malloc.
 * \param newString The extension string to add.
 * \return A new extension string.
 */
static char *MergeExtensionStrings(char *currentString, const char *newString)
{
    size_t origLen;
    size_t newLen;
    const char *name;
    size_t nameLen;
    char *buf, *ptr;

    // Calculate the length of the new string.
    origLen = newLen = strlen(currentString);

    // The code below assumes that currentString is not empty, so if it is
    // empty, then just copy the new string.
    if (origLen == 0) {
        buf = currentString;
        if (newString[0] != '\0') {
            buf = strdup(newString);
            free(currentString);
        }
        return buf;
    }

    name = newString;
    nameLen = 0;
    while (FindNextExtensionName(&name, &nameLen)) {
        if (!IsExtensionInString(currentString, name, nameLen)) {
            newLen += nameLen + 1;
        }
    }
    if (origLen == newLen) {
        // No new extensions to add.
        return currentString;
    }

    buf = (char *) realloc(currentString, newLen + 1);
    if (buf == NULL) {
        free(currentString);
        return NULL;
    }

    ptr = buf + origLen;
    name = newString;
    nameLen = 0;
    while (FindNextExtensionName(&name, &nameLen)) {
        if (!IsExtensionInString(currentString, name, nameLen)) {
            *ptr++ = ' ';
            memcpy(ptr, name, nameLen);
            ptr += nameLen;
        }
    }
    *ptr = '\0';
    assert((size_t) (ptr - buf) == newLen);
    return buf;
}

/**
 * Merges two GLX_VERSION strings.
 *
 * The merged string will specify the higher version number of \p currentString
 * and \p newString, up to the version specified by \c GLX_MAJOR_VERSION and
 * \c GLX_MINOR_VERSION.
 *
 * \param currentString The current string, which must have been allocated with malloc.
 * \param newString The version string to merge.
 * \return A new version string.
 */
static char *MergeVersionStrings(char *currentString, const char *newString)
{
    int major, minor;
    const char *vendorInfo;
    int newMajor, newMinor;
    const char *newVendorInfo;
    char *buf;
    int ret;

    if (ParseClientVersionString(currentString, &major, &minor, &vendorInfo) != 0) {
        return currentString;
    }
    if (ParseClientVersionString(newString, &newMajor, &newMinor, &newVendorInfo) != 0) {
        return currentString;
    }

    // Report the highest version number of any vendor library, but no higher
    // than what this version of libglvnd supports.
    if (newMajor > major || (newMajor == major && newMinor > minor)) {
        major = newMajor;
        minor = newMinor;
    }
    if (major > GLX_MAJOR_VERSION || (major == GLX_MAJOR_VERSION && minor > GLX_MINOR_VERSION)) {
        major = GLX_MAJOR_VERSION;
        minor = GLX_MINOR_VERSION;
    }

    if (vendorInfo != NULL && newVendorInfo != NULL) {
        ret = glvnd_asprintf(&buf, "%d.%d %s, %s", major, minor, vendorInfo, newVendorInfo);
    } else if (vendorInfo != NULL || newVendorInfo != NULL) {
        const char *info = (vendorInfo != NULL ? vendorInfo : newVendorInfo);
        ret = glvnd_asprintf(&buf, "%d.%d %s", major, minor, info);
    } else {
        ret = glvnd_asprintf(&buf, "%d.%d", major, minor);
    }
    free(currentString);

    if (ret >= 0) {
        return buf;
    } else {
        return NULL;
    }
}

PUBLIC const char *glXGetClientString(Display *dpy, int name)
{
    __glXThreadInitialize();

    __GLXdisplayInfo *dpyInfo = NULL;
    int num_screens = XScreenCount(dpy);
    int screen;
    int index = name - 1;
    const char **vendorStrings = NULL;

    if (num_screens == 1) {
        // There's only one screen, so we don't have to mess around with
        // merging the strings from multiple vendors.

        const __GLXdispatchTableStatic *pDispatch = __glXGetStaticDispatch(dpy, 0);
        return pDispatch->getClientString(dpy, name);
    }

    if (name != GLX_VENDOR && name != GLX_VERSION && name != GLX_EXTENSIONS) {
        return NULL;
    }

    dpyInfo = __glXLookupDisplay(dpy);
    if (dpyInfo == NULL) {
        return NULL;
    }

    __glXPthreadFuncs.mutex_lock(&clientStringLock);

    if (dpyInfo->clientStrings[index] != NULL) {
        goto done;
    }

    vendorStrings = GetVendorClientStrings(dpy, name);
    if (vendorStrings == NULL) {
        goto done;
    }

    dpyInfo->clientStrings[index] = strdup(vendorStrings[0]);
    if (dpyInfo->clientStrings[index] == NULL) {
        goto done;
    }
    for (screen = 1; screen < num_screens; screen++) {
        if (name == GLX_VENDOR) {
            char *newBuf;
            if (glvnd_asprintf(&newBuf, "%s, %s", dpyInfo->clientStrings[index], vendorStrings[screen]) < 0) {
                newBuf = NULL;
            }
            free(dpyInfo->clientStrings[index]);
            dpyInfo->clientStrings[index] = newBuf;
        } else if (name == GLX_VERSION) {
            dpyInfo->clientStrings[index] = MergeVersionStrings(dpyInfo->clientStrings[index], vendorStrings[screen]);
        } else if (name == GLX_EXTENSIONS) {
            dpyInfo->clientStrings[index] = MergeExtensionStrings(dpyInfo->clientStrings[index], vendorStrings[screen]);
        } else {
            assert(!"Can't happen: Invalid string name");
            free(dpyInfo->clientStrings[index]);
            dpyInfo->clientStrings[index] = NULL;
        }
        if (dpyInfo->clientStrings[index] == NULL) {
            goto done;
        }
    }

done:
    __glXPthreadFuncs.mutex_unlock(&clientStringLock);
    if (vendorStrings != NULL) {
        free(vendorStrings);
    }
    return dpyInfo->clientStrings[index];
}

PUBLIC const char *glXQueryServerString(Display *dpy, int screen, int name)
{
    __glXThreadInitialize();

    const __GLXdispatchTableStatic *pDispatch = __glXGetStaticDispatch(dpy, screen);

    return pDispatch->queryServerString(dpy, screen, name);
}


PUBLIC const char *glXQueryExtensionsString(Display *dpy, int screen)
{
    __glXThreadInitialize();

    const __GLXdispatchTableStatic *pDispatch = __glXGetStaticDispatch(dpy, screen);

    return pDispatch->queryExtensionsString(dpy, screen);
}


PUBLIC GLXFBConfig *glXChooseFBConfig(Display *dpy, int screen,
                                      const int *attrib_list, int *nelements)
{
    __glXThreadInitialize();

    const __GLXdispatchTableStatic *pDispatch = __glXGetStaticDispatch(dpy, screen);
    __GLXvendorInfo *vendor = __glXLookupVendorByScreen(dpy, screen);
    GLXFBConfig *fbconfigs =
        pDispatch->chooseFBConfig(dpy, screen, attrib_list, nelements);
    int i;

    if (fbconfigs != NULL) {
        for (i = 0; i < *nelements; i++) {
            __glXAddScreenFBConfigMapping(dpy, fbconfigs[i], screen, vendor);
        }
    }

    return fbconfigs;
}


PUBLIC GLXPbuffer glXCreatePbuffer(Display *dpy, GLXFBConfig config,
                            const int *attrib_list)
{
    __glXThreadInitialize();

    const int screen = __glXScreenFromFBConfig(config);
    const __GLXdispatchTableStatic *pDispatch = __glXGetStaticDispatch(dpy, screen);
    __GLXvendorInfo *vendor = __glXLookupVendorByScreen(dpy, screen);

    GLXPbuffer pbuffer = pDispatch->createPbuffer(dpy, config, attrib_list);

    __glXAddScreenDrawableMapping(dpy, pbuffer, screen, vendor);

    return pbuffer;
}


PUBLIC GLXPixmap glXCreatePixmap(Display *dpy, GLXFBConfig config,
                          Pixmap pixmap, const int *attrib_list)
{
    __glXThreadInitialize();

    const int screen = __glXScreenFromFBConfig(config);
    const __GLXdispatchTableStatic *pDispatch = __glXGetStaticDispatch(dpy, screen);
    __GLXvendorInfo *vendor = __glXLookupVendorByScreen(dpy, screen);

    GLXPixmap glxPixmap =
        pDispatch->createPixmap(dpy, config, pixmap, attrib_list);

    __glXAddScreenDrawableMapping(dpy, glxPixmap, screen, vendor);

    return glxPixmap;
}


PUBLIC GLXWindow glXCreateWindow(Display *dpy, GLXFBConfig config,
                          Window win, const int *attrib_list)
{
    __glXThreadInitialize();

    const int screen = __glXScreenFromFBConfig(config);
    const __GLXdispatchTableStatic *pDispatch = __glXGetStaticDispatch(dpy, screen);
    __GLXvendorInfo *vendor = __glXLookupVendorByScreen(dpy, screen);

    GLXWindow glxWindow =
        pDispatch->createWindow(dpy, config, win, attrib_list);

    __glXAddScreenDrawableMapping(dpy, glxWindow, screen, vendor);

    return glxWindow;
}


PUBLIC void glXDestroyPbuffer(Display *dpy, GLXPbuffer pbuf)
{
    __GLXvendorInfo *vendor = CommonDispatchDrawable(dpy, pbuf,
            X_GLXDestroyPbuffer, GLXBadPbuffer, False);
    if (vendor != NULL) {
        __glXRemoveScreenDrawableMapping(dpy, pbuf);
        vendor->staticDispatch.destroyPbuffer(dpy, pbuf);
    }
}


PUBLIC void glXDestroyPixmap(Display *dpy, GLXPixmap pixmap)
{
    __GLXvendorInfo *vendor = CommonDispatchDrawable(dpy, pixmap,
            X_GLXDestroyPixmap, GLXBadPixmap, False);
    if (vendor != NULL) {
        __glXRemoveScreenDrawableMapping(dpy, pixmap);
        vendor->staticDispatch.destroyPixmap(dpy, pixmap);
    }
}


PUBLIC void glXDestroyWindow(Display *dpy, GLXWindow win)
{
    __GLXvendorInfo *vendor = CommonDispatchDrawable(dpy, win,
            X_GLXDestroyWindow, GLXBadWindow, False);
    if (vendor != NULL) {
        __glXRemoveScreenDrawableMapping(dpy, win);
        vendor->staticDispatch.destroyWindow(dpy, win);
    }
}

PUBLIC int glXGetFBConfigAttrib(Display *dpy, GLXFBConfig config,
                         int attribute, int *value)
{
    __glXThreadInitialize();

    const int screen = __glXScreenFromFBConfig(config);
    const __GLXdispatchTableStatic *pDispatch = __glXGetStaticDispatch(dpy, screen);

    return pDispatch->getFBConfigAttrib(dpy, config, attribute, value);
}


PUBLIC GLXFBConfig *glXGetFBConfigs(Display *dpy, int screen, int *nelements)
{
    __glXThreadInitialize();

    const __GLXdispatchTableStatic *pDispatch = __glXGetStaticDispatch(dpy, screen);
    GLXFBConfig *fbconfigs = pDispatch->getFBConfigs(dpy, screen, nelements);
    __GLXvendorInfo *vendor = __glXLookupVendorByScreen(dpy, screen);
    int i;

    if (fbconfigs != NULL) {
        for (i = 0; i < *nelements; i++) {
            __glXAddScreenFBConfigMapping(dpy, fbconfigs[i], screen, vendor);
        }
    }

    return fbconfigs;
}


PUBLIC void glXGetSelectedEvent(Display *dpy, GLXDrawable draw,
                         unsigned long *event_mask)
{
    // glXGetSelectedEvent uses the glXGetDrawableAttributes protocol.
    __GLXvendorInfo *vendor = CommonDispatchDrawable(dpy, draw,
            X_GLXGetDrawableAttributes, GLXBadDrawable, False);
    if (vendor != NULL) {
        vendor->staticDispatch.getSelectedEvent(dpy, draw, event_mask);
    }
}


PUBLIC XVisualInfo *glXGetVisualFromFBConfig(Display *dpy, GLXFBConfig config)
{
    __glXThreadInitialize();

    const int screen = __glXScreenFromFBConfig(config);
    const __GLXdispatchTableStatic *pDispatch = __glXGetStaticDispatch(dpy, screen);

    return pDispatch->getVisualFromFBConfig(dpy, config);
}


PUBLIC int glXQueryContext(Display *dpy, GLXContext context, int attribute, int *value)
{
    __GLXvendorInfo *vendor = CommonDispatchContext(dpy, context, X_GLXQueryContext);
    if (vendor != NULL) {
        return vendor->staticDispatch.queryContext(dpy, context, attribute, value);
    } else {
        return GLX_BAD_CONTEXT;
    }
}


PUBLIC void glXQueryDrawable(Display *dpy, GLXDrawable draw,
                      int attribute, unsigned int *value)
{
    __GLXvendorInfo *vendor = CommonDispatchDrawable(dpy, draw,
            X_GLXGetDrawableAttributes, GLXBadDrawable, False);
    if (vendor != NULL) {
        vendor->staticDispatch.queryDrawable(dpy, draw, attribute, value);
    }
}


PUBLIC void glXSelectEvent(Display *dpy, GLXDrawable draw, unsigned long event_mask)
{
    __GLXvendorInfo *vendor = CommonDispatchDrawable(dpy, draw,
            X_GLXChangeDrawableAttributes, GLXBadDrawable, False);
    if (vendor != NULL) {
        vendor->staticDispatch.selectEvent(dpy, draw, event_mask);
    }
}

typedef struct {
    GLubyte *procName;
    __GLXextFuncPtr addr;
    UT_hash_handle hh;
} __GLXprocAddressHash;

static DEFINE_INITIALIZED_LKDHASH(__GLXprocAddressHash, __glXProcAddressHash);

#define LOCAL_FUNC_TABLE_ENTRY(func) \
    { (GLubyte *)#func, (__GLXextFuncPtr)(func) },

/*
 * This helper function initializes the __GLXprocAddressHash with the
 * dispatch functions implemented above.
 */
void cacheInitializeOnce(void)
{
    size_t i;
    __GLXprocAddressHash *pEntry;
    const struct {
        const GLubyte *procName;
        __GLXextFuncPtr addr;
    } localFuncTable[] = {
        LOCAL_FUNC_TABLE_ENTRY(glXChooseFBConfig)
        LOCAL_FUNC_TABLE_ENTRY(glXChooseVisual)
        LOCAL_FUNC_TABLE_ENTRY(glXCopyContext)
        LOCAL_FUNC_TABLE_ENTRY(glXCreateContext)
        LOCAL_FUNC_TABLE_ENTRY(glXCreateGLXPixmap)
        LOCAL_FUNC_TABLE_ENTRY(glXCreateNewContext)
        LOCAL_FUNC_TABLE_ENTRY(glXCreatePbuffer)
        LOCAL_FUNC_TABLE_ENTRY(glXCreatePixmap)
        LOCAL_FUNC_TABLE_ENTRY(glXCreateWindow)
        LOCAL_FUNC_TABLE_ENTRY(glXDestroyContext)
        LOCAL_FUNC_TABLE_ENTRY(glXDestroyGLXPixmap)
        LOCAL_FUNC_TABLE_ENTRY(glXDestroyPbuffer)
        LOCAL_FUNC_TABLE_ENTRY(glXDestroyPixmap)
        LOCAL_FUNC_TABLE_ENTRY(glXDestroyWindow)
        LOCAL_FUNC_TABLE_ENTRY(glXGetClientString)
        LOCAL_FUNC_TABLE_ENTRY(glXGetConfig)
        LOCAL_FUNC_TABLE_ENTRY(glXGetCurrentContext)
        LOCAL_FUNC_TABLE_ENTRY(glXGetCurrentDisplay)
        LOCAL_FUNC_TABLE_ENTRY(glXGetCurrentDrawable)
        LOCAL_FUNC_TABLE_ENTRY(glXGetCurrentReadDrawable)
        LOCAL_FUNC_TABLE_ENTRY(glXGetFBConfigAttrib)
        LOCAL_FUNC_TABLE_ENTRY(glXGetFBConfigs)
        LOCAL_FUNC_TABLE_ENTRY(glXGetProcAddress)
        LOCAL_FUNC_TABLE_ENTRY(glXGetProcAddressARB)
        LOCAL_FUNC_TABLE_ENTRY(glXGetSelectedEvent)
        LOCAL_FUNC_TABLE_ENTRY(glXGetVisualFromFBConfig)
        LOCAL_FUNC_TABLE_ENTRY(glXIsDirect)
        LOCAL_FUNC_TABLE_ENTRY(glXMakeContextCurrent)
        LOCAL_FUNC_TABLE_ENTRY(glXMakeCurrent)
        LOCAL_FUNC_TABLE_ENTRY(glXQueryContext)
        LOCAL_FUNC_TABLE_ENTRY(glXQueryDrawable)
        LOCAL_FUNC_TABLE_ENTRY(glXQueryExtension)
        LOCAL_FUNC_TABLE_ENTRY(glXQueryExtensionsString)
        LOCAL_FUNC_TABLE_ENTRY(glXQueryServerString)
        LOCAL_FUNC_TABLE_ENTRY(glXQueryVersion)
        LOCAL_FUNC_TABLE_ENTRY(glXSelectEvent)
        LOCAL_FUNC_TABLE_ENTRY(glXSwapBuffers)
        LOCAL_FUNC_TABLE_ENTRY(glXUseXFont)
        LOCAL_FUNC_TABLE_ENTRY(glXWaitGL)
        LOCAL_FUNC_TABLE_ENTRY(glXWaitX)

        LOCAL_FUNC_TABLE_ENTRY(glXImportContextEXT)
        LOCAL_FUNC_TABLE_ENTRY(glXFreeContextEXT)
    };

    LKDHASH_WRLOCK(__glXPthreadFuncs, __glXProcAddressHash);

    // Initialize the hash table with our locally-exported functions

    for (i = 0; i < ARRAY_LEN(localFuncTable); i++) {
        pEntry = malloc(sizeof(*pEntry));
        if (!pEntry) {
            assert(pEntry);
            break;
        }
        pEntry->procName =
            (GLubyte *)strdup((const char *)localFuncTable[i].procName);
        pEntry->addr = localFuncTable[i].addr;
        HASH_ADD_KEYPTR(hh, _LH(__glXProcAddressHash), pEntry->procName,
                        strlen((const char *)pEntry->procName), pEntry);
    }
    LKDHASH_UNLOCK(__glXPthreadFuncs, __glXProcAddressHash);

}

static void CleanupProcAddressEntry(void *unused, __GLXprocAddressHash *pEntry)
{
    free(pEntry->procName);
}

/*
 * This function is called externally by the libGL wrapper library to
 * retrieve libGLX entrypoints.
 */
static __GLXextFuncPtr __glXGetCachedProcAddress(const GLubyte *procName)
{
    /*
     * If this is the first time GetProcAddress has been called,
     * initialize the hash table with locally-exported functions.
     */
    static glvnd_once_t cacheInitializeOnceControl = GLVND_ONCE_INIT;
    __GLXprocAddressHash *pEntry = NULL;

    __glXPthreadFuncs.once(&cacheInitializeOnceControl, cacheInitializeOnce);

    LKDHASH_RDLOCK(__glXPthreadFuncs, __glXProcAddressHash);
    HASH_FIND(hh, _LH(__glXProcAddressHash), procName,
              strlen((const char *)procName), pEntry);
    LKDHASH_UNLOCK(__glXPthreadFuncs, __glXProcAddressHash);

    if (pEntry) {
        return pEntry->addr;
    }

    return NULL;
}

PUBLIC __GLXextFuncPtr __glXGLLoadGLXFunction(const char *name,
        __GLXextFuncPtr *ptr, glvnd_mutex_t *mutex)
{
    __GLXextFuncPtr func;

    if (mutex != NULL) {
        __glXPthreadFuncs.mutex_lock(mutex);
    }

    func = *ptr;
    if (func == NULL) {
        func = glXGetProcAddress((const GLubyte *) name);
        *ptr = func;
    }

    if (mutex != NULL) {
        __glXPthreadFuncs.mutex_unlock(mutex);
    }
    return func;
}


static void cacheProcAddress(const GLubyte *procName, __GLXextFuncPtr addr)
{
    __GLXprocAddressHash *pEntry = malloc(sizeof(*pEntry));

    if (!pEntry) {
        assert(pEntry);
        return;
    }

    pEntry->procName = (GLubyte *)strdup((const char *)procName);

    if (pEntry->procName == NULL) {
        assert(pEntry->procName);
        free(pEntry);
        return;
    }

    pEntry->addr = addr;

    LKDHASH_WRLOCK(__glXPthreadFuncs, __glXProcAddressHash);
    HASH_ADD_KEYPTR(hh, _LH(__glXProcAddressHash), pEntry->procName,
                    strlen((const char*)pEntry->procName),
                    pEntry);
    LKDHASH_UNLOCK(__glXPthreadFuncs, __glXProcAddressHash);
}

PUBLIC __GLXextFuncPtr glXGetProcAddressARB(const GLubyte *procName)
{
    __glXThreadInitialize();

    return glXGetProcAddress(procName);
}

PUBLIC __GLXextFuncPtr glXGetProcAddress(const GLubyte *procName)
{
    __glXThreadInitialize();

    __GLXextFuncPtr addr = NULL;

    /*
     * Easy case: First check if we already know this address from
     * a previous GetProcAddress() call or by virtue of being a function
     * exported by libGLX.
     */
    addr = __glXGetCachedProcAddress(procName);
    if (addr) {
        return addr;
    }

    /*
     * If that doesn't work, try requesting a dispatch function
     * from one of the loaded vendor libraries.
     */
    addr = __glXGetGLXDispatchAddress(procName);
    if (addr) {
        goto done;
    }

    /* If that doesn't work, then try to generate a stub function. */
    addr = __glXGenerateGLXEntrypoint(procName);

    /* Store the resulting proc address. */
done:
    if (addr) {
        cacheProcAddress(procName, addr);
    }

    return addr;
}

int AtomicIncrement(int volatile *val)
{
#if defined(HAVE_SYNC_INTRINSICS)
    return __sync_add_and_fetch(val, 1);
#elif defined(USE_X86_ASM) || defined(USE_X86_64_ASM)
    int result;
    int delta = 1;

    __asm __volatile__ ("lock; xaddl %0, %1"
                        : "=r" (result), "=m" (*val)
                        : "0" (delta), "m" (*val));

    return result + delta;
#else
#error "Not implemented"
#endif
}

int AtomicSwap(int volatile *val, int newVal)
{
#if defined(HAVE_SYNC_INTRINSICS)
    return __sync_lock_test_and_set(val, newVal);
#elif defined(USE_X86_ASM) || defined(USE_X86_64_ASM)
    int result;

    __asm __volatile__ ("xchgl %0, %1"
                        : "=r" (result), "=m" (*val)
                        : "0" (newVal), "m" (*val));

    return result;
#else
#error "Not implemented"
#endif
}

int AtomicCompareAndSwap(int volatile *val, int oldVal, int newVal)
{
#if defined(HAVE_SYNC_INTRINSICS)
    return __sync_val_compare_and_swap(val, oldVal, newVal);
#elif defined(USE_X86_ASM) || defined(USE_X86_64_ASM)
    int result;

    __asm __volatile__ ("lock; cmpxchgl %2, %1"
                        : "=a" (result), "=m" (*val)
                        : "r" (newVal), "m" (*val), "0" (oldVal));

    return result;
#else
#error "Not implemented"
#endif
}

int AtomicDecrementClampAtZero(int volatile *val)
{
    int oldVal, newVal;

    oldVal = *val;
    newVal = oldVal;

    do {
        if (oldVal <= 0) {
            assert(oldVal == 0);
        } else {
            newVal = oldVal - 1;
            oldVal = AtomicCompareAndSwap(val, oldVal, newVal);
        }
    } while ((oldVal > 0) && (newVal != oldVal - 1));

    return newVal;
}

static void __glXResetOnFork(void);

/*
 * Perform checks that need to occur when entering any GLX entrypoint.
 * Currently, this only detects whether a fork occurred since the last
 * entrypoint was called, and performs recovery as needed.
 */
void __glXThreadInitialize(void)
{
    volatile static int g_threadsInCheck = 0;
    volatile static int g_lastPid = -1;

    int lastPid;
    int pid = getpid();

    AtomicIncrement(&g_threadsInCheck);

    lastPid = AtomicSwap(&g_lastPid, pid);

    if ((lastPid != -1) &&
        (lastPid != pid)) {

        DBG_PRINTF(0, "Fork detected\n");

        __glXResetOnFork();

        // Force g_threadsInCheck to 0 to unblock other threads waiting here.
        g_threadsInCheck = 0;
    } else {
        AtomicDecrementClampAtZero(&g_threadsInCheck);
        while (g_threadsInCheck > 0) {
            // Wait for other threads to finish checking for a fork.
            //
            // If a fork happens while g_threadsInCheck > 0 the _first_ thread
            // to enter __glXThreadInitialize() will see the fork, handle it, and force
            // g_threadsInCheck to 0, unblocking any other threads stuck here.
            sched_yield();
        }
    }

    __glDispatchCheckMultithreaded();
}

void CurrentContextHashCleanup(void *unused, __GLXcurrentContextHash *pEntry)
{
    if (pEntry->needsUnmap) {
        __glXRemoveScreenContextMapping(NULL, pEntry->ctx);
    }
}

static void __glXAPITeardown(Bool doReset)
{
    __GLXAPIState *apiState, *apiStateTemp;
    __GLXcurrentContextHash *currContext, *currContextTemp;

    /*
     * XXX: This will leave dangling screen-context mappings, but they will be
     * cleared separately in __glXMappingTeardown().
     */
    __glXPthreadFuncs.mutex_lock(&currentContextHashLock);
    HASH_ITER(hh, currentContextHash, currContext, currContextTemp) {
        HASH_DEL(currentContextHash, currContext);
        free(currContext);
    }
    assert(currentContextHash == NULL);
    __glXPthreadFuncs.mutex_unlock(&currentContextHashLock);

    glvnd_list_for_each_entry_safe(apiState, apiStateTemp, &currentAPIStateList, entry) {
        glvnd_list_del(&apiState->entry);
        free(apiState);
    }

    if (doReset) {
        /*
         * XXX: We should be able to get away with just resetting the proc address
         * hash lock, and not throwing away cached addresses.
         */
        __glXPthreadFuncs.rwlock_init(&__glXProcAddressHash.lock, NULL);
        __glXPthreadFuncs.mutex_init(&currentAPIStateListMutex, NULL);
    } else {
        LKDHASH_TEARDOWN(__glXPthreadFuncs, __GLXprocAddressHash,
                         __glXProcAddressHash, CleanupProcAddressEntry,
                         NULL, False);
    }
}

static void __glXResetOnFork(void)
{
    /* Reset GLdispatch */
    __glDispatchReset();

    /* Reset all GLX API state */
    __glXAPITeardown(True);

    /* Reset all mapping state */
    __glXMappingTeardown(True);
}

#if defined(USE_ATTRIBUTE_CONSTRUCTOR)
void __attribute__ ((constructor)) __glXInit(void)
#else
void _init(void)
#endif
{
    glvnd_mutexattr_t mutexAttribs;

    if (__glDispatchGetABIVersion() != GLDISPATCH_ABI_VERSION) {
        fprintf(stderr, "libGLdispatch ABI version is incompatible with libGLX.\n");
        abort();
    }

    /* Initialize GLdispatch; this will also initialize our pthreads imports */
    __glDispatchInit();
    glvndSetupPthreads(RTLD_DEFAULT, &__glXPthreadFuncs);

    glvnd_list_init(&currentAPIStateList);

    /*
     * currentContextHashLock must be a recursive mutex, because we'll have it
     * locked when we call into the vendor library's glXMakeCurrent
     * implementation. If the vendor library generates an X error, then that
     * will often result in a call to exit. In that case, the teardown code
     * will try to lock the mutex again so that it can clean up the current
     * context list.
     */
    __glXPthreadFuncs.mutexattr_init(&mutexAttribs);
    __glXPthreadFuncs.mutexattr_settype(&mutexAttribs, PTHREAD_MUTEX_RECURSIVE);
    __glXPthreadFuncs.mutex_init(&currentContextHashLock, &mutexAttribs);
    __glXPthreadFuncs.mutexattr_destroy(&mutexAttribs);

    {
        /*
         * Check if we need to pre-load any vendors specified via environment
         * variable.
         */
        const char *preloadedVendor = getenv("__GLX_VENDOR_LIBRARY_NAME");

        if (preloadedVendor) {
            __glXLookupVendorByName(preloadedVendor);
        }
    }

    /* TODO install fork handlers using __register_atfork */

    /* Register our XCloseDisplay() callback */
    XGLVRegisterCloseDisplayCallback(DisplayClosed);

    DBG_PRINTF(0, "Loading GLX...\n");

}

#if defined(USE_ATTRIBUTE_CONSTRUCTOR)
void __attribute__ ((destructor)) __glXFini(void)
#else
void _fini(void)
#endif
{
    /* Check for a fork before going further. */
    __glXThreadInitialize();

    /*
     * If libGLX owns the current API state, lose current
     * in GLdispatch before going further.
     */
    __GLdispatchAPIState *glas =
        __glDispatchGetCurrentAPIState();

    if (glas && glas->tag == GLDISPATCH_API_GLX) {
        __glDispatchLoseCurrent();
    }


    /* Unregister all XCloseDisplay() callbacks */
    XGLVUnregisterCloseDisplayCallbacks();

    /* Tear down all GLX API state */
    __glXAPITeardown(False);

    /* Tear down all mapping state */
    __glXMappingTeardown(False);

    /* Tear down GLdispatch if necessary */
    __glDispatchFini();
}

