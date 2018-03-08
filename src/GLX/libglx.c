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
#include "utils_misc.h"
#include "trace.h"
#include "GL/glxproto.h"
#include "libglxgl.h"
#include "glvnd_list.h"
#include "app_error_check.h"

#include "lkdhash.h"

/* current version numbers */
#define GLX_MAJOR_VERSION 1
#define GLX_MINOR_VERSION 4
#define GLX_VERSION_STRING "1.4"

/*
 * Older versions of glxproto.h contained a typo where "Attribs" was misspelled.
 * The typo was fixed in the xorgproto version of glxproto.h, breaking the API.
 * Work around that here.
 */
#if !defined(X_GLXCreateContextAttribsARB) && \
    defined(X_GLXCreateContextAtrribsARB)
#define X_GLXCreateContextAttribsARB X_GLXCreateContextAtrribsARB
#endif

static glvnd_mutex_t clientStringLock = GLVND_MUTEX_INITIALIZER;

/**
 * This structure keeps track of a rendering context.
 *
 * It's used both to keep track of which vendor owns each context and for
 * whether a context is current to any thread.
 */
struct __GLXcontextInfoRec {
    GLXContext context;
    __GLXvendorInfo *vendor;
    int currentCount;
    Bool deleted;
    UT_hash_handle hh;
};

static __GLXcontextInfo *glxContextHash = NULL;

/**
 * The mutex used to protect the \c glxContextHash hash. Any thread must
 * take this mutex before it accesses the \c glxContextHash, or before it
 * modifies any field in a __GLXcontextInfo structure.
 *
 * Note that a \c __GLXcontextInfo struct will stay valid for as long as a context
 * is. That is, it's only freed when the context is deleted and no longer
 * current to any thread.
 *
 * Also note that the \c context and \c vendor values are never modified for
 * the life of the structure. Thus, it's safe to access them for the current
 * thread's current context without having to take the \c glxContextHashLock
 * mutex.
 */
static glvnd_mutex_t glxContextHashLock;

/**
 * A list of current __GLXThreadState structures. This is used so that we can
 * clean up at process termination or after a fork.
 */
static struct glvnd_list currentThreadStateList;
static glvnd_mutex_t currentThreadStateListMutex = GLVND_MUTEX_INITIALIZER;

static __GLXThreadState *CreateThreadState(__GLXvendorInfo *vendor);
static void DestroyThreadState(__GLXThreadState *threadState);

/*!
 * Updates the current context.
 *
 * If the old context was flagged for deletion and is no longer current to any
 * thread, then it will also remove the context from the context hashtable.
 *
 * \note glxContextHashLock must be locked before calling this
 * function.
 *
 * \param[in] newCtxInfo The new context to make current, or \c NULL to just
 * release the current context.
 * \param[in] oldCtxInfo The previous current context, or \c NULL if no context
 * was current before.
 */
static void UpdateCurrentContext(__GLXcontextInfo *newCtxInfo, __GLXcontextInfo *oldCtxInfo);

/**
 * Removes and frees an entry from the glxContextHash table.
 *
 * The caller must take the \c glxContextHashLock mutex before calling this
 * function.
 *
 * \param ctx The context to free.
 */
static void FreeContextInfo(__GLXcontextInfo *ctx);

/**
 * Checks whether a rendering context should be deleted.
 *
 * If the context is marked for deletion, and is not current to any thread,
 * then it will remove and free the __GLXcontextInfo struct.
 */
static void CheckContextDeleted(__GLXcontextInfo *ctx);

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
        vendor = __glXVendorFromDrawable(dpy, draw);
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
        vendor = __glXVendorFromContext(context);
    }
    if (vendor == NULL) {
        __glXSendError(dpy, GLXBadContext, 0, minorCode, False);
    }
    return vendor;
}

static __GLXvendorInfo *CommonDispatchFBConfig(Display *dpy, GLXFBConfig config,
        unsigned char minorCode)
{
    __GLXvendorInfo *vendor = NULL;

    if (config != NULL) {
        __glXThreadInitialize();
        vendor = __glXVendorFromFBConfig(dpy, config);
    }
    if (vendor == NULL) {
        __glXSendError(dpy, GLXBadFBConfig, 0, minorCode, False);
    }
    return vendor;
}

PUBLIC XVisualInfo* glXChooseVisual(Display *dpy, int screen, int *attrib_list)
{
    __GLXvendorInfo *vendor = __glXGetDynDispatch(dpy, screen);
    if (vendor != NULL) {
        return vendor->staticDispatch.chooseVisual(dpy, screen, attrib_list);
    } else {
        return NULL;
    }
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
    __GLXvendorInfo *vendor = __glXGetDynDispatch(dpy, vis->screen);
    if (vendor != NULL) {
        GLXContext context = vendor->staticDispatch.createContext(dpy, vis, share_list, direct);
        if (__glXAddVendorContextMapping(dpy, context, vendor) != 0) {
            vendor->staticDispatch.destroyContext(dpy, context);
            context = NULL;
        }
        return context;
    } else {
        return NULL;
    }
}


PUBLIC GLXContext glXCreateNewContext(Display *dpy, GLXFBConfig config,
                               int render_type, GLXContext share_list,
                               Bool direct)
{
    GLXContext context = NULL;
    __GLXvendorInfo *vendor = CommonDispatchFBConfig(dpy, config, X_GLXCreateNewContext);
    if (vendor != NULL) {
        context = vendor->staticDispatch.createNewContext(dpy, config, render_type,
                                                     share_list, direct);
        if (__glXAddVendorContextMapping(dpy, context, vendor) != 0) {
            vendor->staticDispatch.destroyContext(dpy, context);
            context = NULL;
        }
    }
    return context;
}

static GLXContext glXCreateContextAttribsARB(Display *dpy, GLXFBConfig config,
        GLXContext share_list, Bool direct, const int *attrib_list)
{
    GLXContext context = NULL;
    __GLXvendorInfo *vendor = NULL;

    if (attrib_list != NULL) {
        // See if the caller passed in a GLX_SCREEN attribute, and if so, use
        // that to select a vendor library. This is needed for
        // GLX_EXT_no_config_context, where we won't have a GLXFBConfig handle.
        int i;
        for (i=0; attrib_list[i] != None; i += 2) {
            if (attrib_list[i] == GLX_SCREEN) {
                int screen = attrib_list[i + 1];
                vendor = __glXGetDynDispatch(dpy, screen);
                if (vendor == NULL) {
                    __glXSendError(dpy, BadValue, 0,
                            X_GLXCreateContextAttribsARB, True);
                    return None;
                }
            }
        }
    }

    if (vendor == NULL) {
        // We didn't get a GLX_SCREEN attribute, so look at the config instead.
        vendor = CommonDispatchFBConfig(dpy, config, X_GLXCreateContextAttribsARB);
    }

    if (vendor != NULL && vendor->staticDispatch.createContextAttribsARB != NULL) {
        context = vendor->staticDispatch.createContextAttribsARB(dpy, config, share_list, direct, attrib_list);
        if (context != NULL) {
            if (__glXAddVendorContextMapping(dpy, context, vendor) != 0) {
                vendor->staticDispatch.destroyContext(dpy, context);
                context = NULL;
            }
        }
    }

    return context;
}

PUBLIC void glXDestroyContext(Display *dpy, GLXContext context)
{
    __GLXvendorInfo *vendor;

    if (context == NULL) {
        // Some drivers will just return without generating an error if the app
        // passes NULL for a context, and unfortunately there are some broken
        // applications that depend on that behavior.
        glvndAppErrorCheckReportError("glXDestroyContext called with NULL for context\n");
        return;
    }

    vendor = CommonDispatchContext(dpy, context, X_GLXDestroyContext);
    if (vendor != NULL) {
        __glXRemoveVendorContextMapping(dpy, context);
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
    if (vendor != NULL && vendor->staticDispatch.importContextEXT != NULL
            && vendor->staticDispatch.freeContextEXT) {
        GLXContext context = vendor->staticDispatch.importContextEXT(dpy, contextID);
        if (__glXAddVendorContextMapping(dpy, context, vendor) != 0) {
            vendor->staticDispatch.freeContextEXT(dpy, context);
            context = NULL;
        }
        return context;
    } else {
        return NULL;
    }
}

static void glXFreeContextEXT(Display *dpy, GLXContext context)
{
    __GLXvendorInfo *vendor = NULL;

    __glXThreadInitialize();

    vendor = __glXVendorFromContext(context);
    if (vendor != NULL && vendor->staticDispatch.freeContextEXT != NULL) {
        __glXRemoveVendorContextMapping(dpy, context);
        vendor->staticDispatch.freeContextEXT(dpy, context);
    }
}


PUBLIC GLXPixmap glXCreateGLXPixmap(Display *dpy, XVisualInfo *vis, Pixmap pixmap)
{
    __GLXvendorInfo *vendor = __glXGetDynDispatch(dpy, vis->screen);
    if (vendor != NULL) {
        GLXPixmap pmap = vendor->staticDispatch.createGLXPixmap(dpy, vis, pixmap);
        if (__glXAddVendorDrawableMapping(dpy, pmap, vendor) != 0) {
            vendor->staticDispatch.destroyGLXPixmap(dpy, pmap);
            pmap = None;
        }
        return pmap;
    } else {
        return None;
    }
}


PUBLIC void glXDestroyGLXPixmap(Display *dpy, GLXPixmap pix)
{
    __GLXvendorInfo *vendor = CommonDispatchDrawable(dpy, pix,
            X_GLXDestroyGLXPixmap, GLXBadPixmap, False);
    if (vendor != NULL) {
        __glXRemoveVendorDrawableMapping(dpy, pix);
        vendor->staticDispatch.destroyGLXPixmap(dpy, pix);
    }
}


PUBLIC int glXGetConfig(Display *dpy, XVisualInfo *vis, int attrib, int *value)
{
    __GLXvendorInfo *vendor;

    __glXThreadInitialize();

    if (!dpy || !vis || !value) {
        return GLX_BAD_VALUE;
    }

    vendor = __glXLookupVendorByScreen(dpy, vis->screen);
    if (vendor != NULL) {
        return vendor->staticDispatch.getConfig(dpy, vis, attrib, value);
    } else {
        return GLX_BAD_VALUE;
    }
}

PUBLIC GLXContext glXGetCurrentContext(void)
{
    __glXThreadInitialize();

    __GLXThreadState *threadState = __glXGetCurrentThreadState();
    if (threadState != NULL) {
        // The current thread has a thread state pointer if and only if it has a
        // current context, and the currentContext pointer is assigned before
        // the threadState pointer is put into TLS, so it will never be NULL.
        assert(threadState->currentContext != NULL);
        return threadState->currentContext->context;
    } else {
        return NULL;
    }
}


PUBLIC GLXDrawable glXGetCurrentDrawable(void)
{
    __glXThreadInitialize();

    __GLXThreadState *threadState = __glXGetCurrentThreadState();
    if (threadState != NULL) {
        return threadState->currentDraw;
    } else {
        return None;
    }
}

PUBLIC GLXDrawable glXGetCurrentReadDrawable(void)
{
    __glXThreadInitialize();

    __GLXThreadState *threadState = __glXGetCurrentThreadState();
    if (threadState != NULL) {
        return threadState->currentRead;
    } else {
        return None;
    }
}

PUBLIC Display *glXGetCurrentDisplay(void)
{
    __glXThreadInitialize();

    __GLXThreadState *threadState = __glXGetCurrentThreadState();
    if (threadState != NULL) {
        return threadState->currentDisplay;
    } else {
        return NULL;
    }
}

__GLXvendorInfo *__glXGetCurrentDynDispatch(void)
{
    __glXThreadInitialize();

    __GLXThreadState *threadState = __glXGetCurrentThreadState();

    if (threadState != NULL) {
        return threadState->currentVendor;
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

void __glXDisplayClosed(__GLXdisplayInfo *dpyInfo)
{
    __GLXThreadState *threadState;

    threadState = __glXGetCurrentThreadState();
    if (threadState != NULL && threadState->currentDisplay == dpyInfo->dpy) {
        // Clear out the current context, but don't call into the vendor
        // library or do anything that might require a valid display.
        __glDispatchLoseCurrent();
        __glvndPthreadFuncs.mutex_lock(&glxContextHashLock);
        UpdateCurrentContext(NULL, threadState->currentContext);
        __glvndPthreadFuncs.mutex_unlock(&glxContextHashLock);
        DestroyThreadState(threadState);
    }

    __glvndPthreadFuncs.mutex_lock(&currentThreadStateListMutex);
    glvnd_list_for_each_entry(threadState, &currentThreadStateList, entry) {
        /*
         * Stub out any references to this display in any other thread states.
         */
        if (threadState->currentDisplay == dpyInfo->dpy) {
            threadState->currentDisplay = NULL;
        }
    }
    __glvndPthreadFuncs.mutex_unlock(&currentThreadStateListMutex);
}

static void ThreadDestroyed(__GLdispatchThreadState *threadState)
{
    __GLXThreadState *glxState = (__GLXThreadState *) threadState;

    // Clear out the current context.
    __glvndPthreadFuncs.mutex_lock(&glxContextHashLock);
    UpdateCurrentContext(NULL, glxState->currentContext);
    __glvndPthreadFuncs.mutex_unlock(&glxContextHashLock);

    // Free the thread state struct.
    DestroyThreadState(glxState);
}

static __GLXThreadState *CreateThreadState(__GLXvendorInfo *vendor)
{
    __GLXThreadState *threadState = calloc(1, sizeof(*threadState));

    assert(threadState);

    threadState->glas.tag = GLDISPATCH_API_GLX;
    threadState->glas.threadDestroyedCallback = ThreadDestroyed;
    threadState->currentVendor = vendor;

    __glvndPthreadFuncs.mutex_lock(&currentThreadStateListMutex);
    glvnd_list_add(&threadState->entry, &currentThreadStateList);
    __glvndPthreadFuncs.mutex_unlock(&currentThreadStateListMutex);

    return threadState;
}

static void DestroyThreadState(__GLXThreadState *threadState)
{
    // Free the thread state struct.
    __glvndPthreadFuncs.mutex_lock(&currentThreadStateListMutex);
    glvnd_list_del(&threadState->entry);
    __glvndPthreadFuncs.mutex_unlock(&currentThreadStateListMutex);

    free(threadState);
}

/*
 * Notifies libglvnd that the given context has been marked for destruction
 * by glXDestroyContext(), and removes any context -> screen mappings if
 * necessary.
 */
void __glXRemoveVendorContextMapping(Display *dpy, GLXContext context)
{
    __GLXcontextInfo *ctxInfo;
    __glvndPthreadFuncs.mutex_lock(&glxContextHashLock);

    HASH_FIND_PTR(glxContextHash, &context, ctxInfo);
    if (ctxInfo != NULL) {
        ctxInfo->deleted = True;
        CheckContextDeleted(ctxInfo);
    }
    __glvndPthreadFuncs.mutex_unlock(&glxContextHashLock);
}

int __glXAddVendorContextMapping(Display *dpy, GLXContext context, __GLXvendorInfo *vendor)
{
    __GLXcontextInfo *ctxInfo;

    __glvndPthreadFuncs.mutex_lock(&glxContextHashLock);

    HASH_FIND_PTR(glxContextHash, &context, ctxInfo);
    if (ctxInfo == NULL) {
        ctxInfo = (__GLXcontextInfo *) malloc(sizeof(__GLXcontextInfo));
        if (ctxInfo == NULL) {
            __glvndPthreadFuncs.mutex_unlock(&glxContextHashLock);
            return -1;
        }
        ctxInfo->context = context;
        ctxInfo->vendor = vendor;
        ctxInfo->currentCount = 0;
        ctxInfo->deleted = False;
        HASH_ADD_PTR(glxContextHash, context, ctxInfo);
    } else {
        if (ctxInfo->vendor != vendor) {
            __glvndPthreadFuncs.mutex_unlock(&glxContextHashLock);
            return -1;
        }
    }

    __glvndPthreadFuncs.mutex_unlock(&glxContextHashLock);
    return 0;
}

__GLXvendorInfo *__glXVendorFromContext(GLXContext context)
{
    __GLXcontextInfo *ctxInfo;
    __GLXvendorInfo *vendor = NULL;

    __glvndPthreadFuncs.mutex_lock(&glxContextHashLock);
    HASH_FIND_PTR(glxContextHash, &context, ctxInfo);
    if (ctxInfo != NULL) {
        vendor = ctxInfo->vendor;
    }
    __glvndPthreadFuncs.mutex_unlock(&glxContextHashLock);

    return vendor;
}

static void FreeContextInfo(__GLXcontextInfo *ctx)
{
    if (ctx != NULL) {
        HASH_DELETE(hh, glxContextHash, ctx);
        free(ctx);
    }
}

static void UpdateCurrentContext(__GLXcontextInfo *newCtxInfo, __GLXcontextInfo *oldCtxInfo)
{
    if (newCtxInfo == oldCtxInfo) {
        return;
    }
    if (newCtxInfo != NULL) {
        newCtxInfo->currentCount++;
    }
    if (oldCtxInfo != NULL) {
        assert(oldCtxInfo->currentCount > 0);
        oldCtxInfo->currentCount--;
        CheckContextDeleted(oldCtxInfo);
    }
}

static void CheckContextDeleted(__GLXcontextInfo *ctx)
{
    if (ctx->deleted && ctx->currentCount == 0) {
        FreeContextInfo(ctx);
    }
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
    __GLXThreadState *threadState = __glXGetCurrentThreadState();
    Bool ret;

    if (threadState == NULL) {
        return True;
    }

    ret = threadState->currentVendor->staticDispatch.makeCurrent(threadState->currentDisplay, None, NULL);
    if (!ret) {
        return False;
    }

    __glDispatchLoseCurrent();

    // Remove the context from the current context map.
    UpdateCurrentContext(NULL, threadState->currentContext);
    DestroyThreadState(threadState);

    return True;
}

/**
 * Calls into the vendor library to set the current context, and then updates
 * the thread state fields to match.
 *
 * This function does *not* call into libGLdispatch, so it can only switch
 * to another context with the same vendor.
 *
 * If this function succeeds, then it will update the current display, context,
 * and drawables in \p threadState.
 *
 * If it fails, then it will leave \p threadState unmodified. It's up to the
 * vendor library to ensure that the old context is still current in that case.
 */
static Bool InternalMakeCurrentVendor(
        Display *dpy, GLXDrawable draw, GLXDrawable read,
        __GLXcontextInfo *ctxInfo, char callerOpcode,
        __GLXThreadState *threadState,
        __GLXvendorInfo *vendor)
{
    Bool ret;

    assert(threadState->currentVendor == vendor);

    if (callerOpcode == X_GLXMakeCurrent && draw == read) {
        ret = vendor->staticDispatch.makeCurrent(dpy, draw, ctxInfo->context);
    } else {
        ret = vendor->staticDispatch.makeContextCurrent(dpy,
                                                    draw,
                                                    read,
                                                    ctxInfo->context);
    }

    if (ret) {
        threadState->currentDisplay = dpy;
        threadState->currentDraw = draw;
        threadState->currentRead = read;
        threadState->currentContext = ctxInfo;
    }

    return ret;
}

/**
 * Makes a context current. This function handles both the vendor library and
 * libGLdispatch.
 *
 * There must not be a current thread state in libGLdispatch when this function
 * is called.
 *
 * If this function fails, then it will release the context and dispatch state
 * before returning.
 */
static Bool InternalMakeCurrentDispatch(
        Display *dpy, GLXDrawable draw, GLXDrawable read,
        __GLXcontextInfo *ctxInfo, char callerOpcode,
        __GLXvendorInfo *vendor)
{
    __GLXThreadState *threadState;
    Bool ret;

    assert(__glXGetCurrentThreadState() == NULL);

    UpdateCurrentContext(ctxInfo, NULL);

    threadState = CreateThreadState(vendor);
    if (threadState == NULL) {
        UpdateCurrentContext(NULL, ctxInfo);
        return False;
    }

    ret = __glDispatchMakeCurrent(
        &threadState->glas,
        vendor->glDispatch,
        vendor->vendorID,
        vendor->patchCallbacks
    );

    if (ret) {
        // Call into the vendor library.
        ret = InternalMakeCurrentVendor(dpy, draw, read, ctxInfo, callerOpcode,
                threadState, vendor);
        if (!ret) {
            __glDispatchLoseCurrent();
        }
    }

    if (!ret) {
        DestroyThreadState(threadState);
        UpdateCurrentContext(NULL, ctxInfo);
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
    __GLXThreadState *threadState;
    __GLXvendorInfo *oldVendor, *newVendor;
    Display *oldDpy;
    GLXDrawable oldDraw, oldRead;
    __GLXcontextInfo *oldCtxInfo;
    __GLXcontextInfo *newCtxInfo;
    Bool ret;

    __glXThreadInitialize();
    threadState = __glXGetCurrentThreadState();

    if (threadState != NULL) {
        oldVendor = threadState->currentVendor;
        oldDpy = threadState->currentDisplay;
        oldDraw = threadState->currentDraw;
        oldRead = threadState->currentRead;
        oldCtxInfo = threadState->currentContext;

        assert(oldCtxInfo != NULL);

        if (dpy == oldDpy && context == oldCtxInfo->context
                && draw == oldDraw && read == oldRead) {
            // The current display, context, and drawables are the same, so just
            // return.
            return True;
        }
    } else {
        // We might have a non-GLX context current...
        __GLdispatchThreadState *glas = __glDispatchGetCurrentThreadState();
        if (glas != NULL && glas->tag != GLDISPATCH_API_GLX) {
            NotifyXError(dpy, BadAccess, 0, callerOpcode, True, NULL);
            return False;
        }

        // We don't have a current context already.
        oldVendor = NULL;
        oldDpy = NULL;
        oldDraw = oldRead = None;
        oldCtxInfo = NULL;
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

    if (oldCtxInfo == NULL && context == NULL) {
        // If both the old and new contexts are NULL, then there's nothing to
        // do. Just return early.
        return True;
    }

    __glvndPthreadFuncs.mutex_lock(&glxContextHashLock);

    if (context != NULL) {
        // Look up the new display. This will ensure that we keep track of it
        // and get a callback when it's closed.
        if (__glXLookupDisplay(dpy) == NULL) {
            __glvndPthreadFuncs.mutex_unlock(&glxContextHashLock);
            return False;
        }

        HASH_FIND_PTR(glxContextHash, &context, newCtxInfo);
        if (newCtxInfo == NULL) {
            __glvndPthreadFuncs.mutex_unlock(&glxContextHashLock);

            /*
             * We can run into this corner case if a GLX client calls
             * glXDestroyContext() on a current context, loses current to this
             * context (causing it to be freed), then tries to make current to the
             * context again.  This is incorrect application behavior, but we should
             * attempt to handle this failure gracefully.
             */
            NotifyXError(dpy, GLXBadContext, 0, callerOpcode, False, oldVendor);
            return False;
        }
        newVendor = newCtxInfo->vendor;
        assert(newVendor != NULL);
    } else {
        newCtxInfo = NULL;
        newVendor = NULL;
    }

    if (oldVendor == newVendor) {
        assert(threadState != NULL);

        /*
         * We're switching between two contexts that use the same vendor. That
         * means the dispatch table is also the same, which is the only thing
         * that libGLdispatch cares about. Call into the vendor library to
         * switch contexts, but don't call into libGLdispatch.
         */
        ret = InternalMakeCurrentVendor(dpy, draw, read, newCtxInfo, callerOpcode,
                threadState, newVendor);
        if (ret) {
            UpdateCurrentContext(newCtxInfo, oldCtxInfo);
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
        ret = InternalMakeCurrentDispatch(dpy, draw, read, newCtxInfo, callerOpcode,
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

        // First, check to see if calling InternalLoseCurrent is going to
        // destroy the old context.
        Bool canRestoreOldContext = True;
        if (oldCtxInfo->deleted && oldCtxInfo->currentCount == 1) {
            canRestoreOldContext = False;
        }
        ret = InternalLoseCurrent();

        if (ret) {
            ret = InternalMakeCurrentDispatch(dpy, draw, read, newCtxInfo, callerOpcode,
                    newVendor);
            if (!ret && canRestoreOldContext) {
                /*
                 * Try to restore the old context. Note that this can fail if
                 * the old context was marked for deletion. If that happens,
                 * then we'll end up with no current context instead, but we
                 * should at least still be in a consistent state.
                 */
                InternalMakeCurrentDispatch(oldDpy, oldDraw, oldRead, oldCtxInfo,
                        callerOpcode, oldVendor);
            }
        }
    }

    __glvndPthreadFuncs.mutex_unlock(&glxContextHashLock);
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
    __GLXvendorInfo *vendor = __glXGetCurrentDynDispatch();
    if (vendor != NULL) {
        vendor->staticDispatch.useXFont(font, first, count, list_base);
    }
}


PUBLIC void glXWaitGL(void)
{
    __GLXvendorInfo *vendor = __glXGetCurrentDynDispatch();
    if (vendor != NULL) {
        vendor->staticDispatch.waitGL();
    }
}


PUBLIC void glXWaitX(void)
{
    __GLXvendorInfo *vendor = __glXGetCurrentDynDispatch();
    if (vendor != NULL) {
        vendor->staticDispatch.waitX();
    }
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
        __GLXvendorInfo *vendor = __glXLookupVendorByScreen(dpy, screen);
        if (vendor != NULL) {
            result[screen] = vendor->staticDispatch.getClientString(dpy, name);
        } else {
            result[screen] = NULL;
        }

        if (result[screen] == NULL) {
            free(result);
            return NULL;
        }
    }
    return result;
}

/*!
 * Parses the version string that you'd get from calling glXGetClientString
 * with GLX_VERSION.
 *
 * \param version The version string.
 * \param[out] major The major version number.
 * \param[out] minor The minor version number.
 * \param[out] vendor Returns a pointer to the vendor-specific part of the
 * string, or \c NULL if it there isn't any vendor-specific string.
 * \return Zero on success, or -1 if \c version doesn't match the correct
 * format.
 */
static int ParseClientVersionString(const char *version,
        int *major, int *minor, const char **vendor)
{
    int count;
    const char *ptr;

    count = sscanf(version, "%d.%d", major, minor);
    if (count != 2) {
        return -1;
    }

    // The vendor-specific info should be after the first space character.
    *vendor = NULL;
    ptr = strchr(version, ' ');
    if (ptr != NULL) {
        while (*ptr == ' ') {
            ptr++;
        }
        if (*ptr != '\0') {
            *vendor = ptr;
        }
    }
    return 0;
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

static const char *GetClientStringNoVendor(int name)
{
    switch (name) {
    case GLX_VENDOR:
        return "libglvnd (no display specified)";
    case GLX_VERSION:
        return GLX_VERSION_STRING " (no display specified)";
    case GLX_EXTENSIONS:
        return "";
    default:
        return NULL;
    }
}

PUBLIC const char *glXGetClientString(Display *dpy, int name)
{
    __glXThreadInitialize();

    __GLXdisplayInfo *dpyInfo = NULL;
    int num_screens;
    int screen;
    int index = name - 1;
    const char **vendorStrings = NULL;

    if (dpy == NULL) {
        return GetClientStringNoVendor(name);
    }

    num_screens = XScreenCount(dpy);

    if (num_screens == 1) {
        // There's only one screen, so we don't have to mess around with
        // merging the strings from multiple vendors.
        __GLXvendorInfo *vendor = __glXLookupVendorByScreen(dpy, 0);
        if (vendor != NULL) {
            return vendor->staticDispatch.getClientString(dpy, name);
        } else {
            return NULL;
        }
    }

    if (name != GLX_VENDOR && name != GLX_VERSION && name != GLX_EXTENSIONS) {
        return NULL;
    }

    dpyInfo = __glXLookupDisplay(dpy);
    if (dpyInfo == NULL) {
        return NULL;
    }

    __glvndPthreadFuncs.mutex_lock(&clientStringLock);

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
            dpyInfo->clientStrings[index] = UnionExtensionStrings(dpyInfo->clientStrings[index], vendorStrings[screen]);
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
    __glvndPthreadFuncs.mutex_unlock(&clientStringLock);
    if (vendorStrings != NULL) {
        free(vendorStrings);
    }
    return dpyInfo->clientStrings[index];
}

PUBLIC const char *glXQueryServerString(Display *dpy, int screen, int name)
{
    __GLXvendorInfo *vendor = __glXGetDynDispatch(dpy, screen);
    if (vendor != NULL) {
        return vendor->staticDispatch.queryServerString(dpy, screen, name);
    } else {
        return NULL;
    }
}


PUBLIC const char *glXQueryExtensionsString(Display *dpy, int screen)
{
    __GLXvendorInfo *vendor = __glXGetDynDispatch(dpy, screen);
    if (vendor != NULL) {
        return vendor->staticDispatch.queryExtensionsString(dpy, screen);
    } else {
        return NULL;
    }
}

PUBLIC GLXFBConfig *glXChooseFBConfig(Display *dpy, int screen,
                                      const int *attrib_list, int *nelements)
{
    __GLXvendorInfo *vendor = __glXGetDynDispatch(dpy, screen);
    if (vendor != NULL) {
        GLXFBConfig *fbconfigs =
            vendor->staticDispatch.chooseFBConfig(dpy, screen, attrib_list, nelements);

        if (fbconfigs != NULL) {
            int i;
            Bool success = True;
            for (i = 0; i < *nelements; i++) {
                if (__glXAddVendorFBConfigMapping(dpy, fbconfigs[i], vendor) != 0) {
                    success = False;
                    break;
                }
            }
            if (!success) {
                XFree(fbconfigs);
                fbconfigs = NULL;
                *nelements = 0;
            }
        }
        return fbconfigs;
    } else {
        return NULL;
    }
}


PUBLIC GLXPbuffer glXCreatePbuffer(Display *dpy, GLXFBConfig config,
                            const int *attrib_list)
{
    GLXPbuffer pbuffer = None;
    __GLXvendorInfo *vendor = CommonDispatchFBConfig(dpy, config, X_GLXCreatePbuffer);
    if (vendor != NULL) {
        pbuffer = vendor->staticDispatch.createPbuffer(dpy, config, attrib_list);
        if (__glXAddVendorDrawableMapping(dpy, pbuffer, vendor) != 0) {
            vendor->staticDispatch.destroyPbuffer(dpy, pbuffer);
            pbuffer = None;
        }
    }
    return pbuffer;
}


PUBLIC GLXPixmap glXCreatePixmap(Display *dpy, GLXFBConfig config,
                          Pixmap pixmap, const int *attrib_list)
{
    GLXPixmap glxPixmap = None;
    __GLXvendorInfo *vendor = CommonDispatchFBConfig(dpy, config, X_GLXCreatePixmap);
    if (vendor != NULL) {
        glxPixmap = vendor->staticDispatch.createPixmap(dpy, config, pixmap, attrib_list);
        if (__glXAddVendorDrawableMapping(dpy, glxPixmap, vendor) != 0) {
            vendor->staticDispatch.destroyGLXPixmap(dpy, glxPixmap);
            glxPixmap = None;
        }
    }
    return glxPixmap;
}


PUBLIC GLXWindow glXCreateWindow(Display *dpy, GLXFBConfig config,
                          Window win, const int *attrib_list)
{
    GLXWindow glxWindow = None;
    __GLXvendorInfo *vendor = CommonDispatchFBConfig(dpy, config, X_GLXCreateWindow);
    if (vendor != NULL) {
        glxWindow = vendor->staticDispatch.createWindow(dpy, config, win, attrib_list);
        if (__glXAddVendorDrawableMapping(dpy, glxWindow, vendor) != 0) {
            vendor->staticDispatch.destroyWindow(dpy, glxWindow);
            glxWindow = None;
        }
    }
    return glxWindow;
}


PUBLIC void glXDestroyPbuffer(Display *dpy, GLXPbuffer pbuf)
{
    __GLXvendorInfo *vendor = CommonDispatchDrawable(dpy, pbuf,
            X_GLXDestroyPbuffer, GLXBadPbuffer, False);
    if (vendor != NULL) {
        __glXRemoveVendorDrawableMapping(dpy, pbuf);
        vendor->staticDispatch.destroyPbuffer(dpy, pbuf);
    }
}


PUBLIC void glXDestroyPixmap(Display *dpy, GLXPixmap pixmap)
{
    __GLXvendorInfo *vendor = CommonDispatchDrawable(dpy, pixmap,
            X_GLXDestroyPixmap, GLXBadPixmap, False);
    if (vendor != NULL) {
        __glXRemoveVendorDrawableMapping(dpy, pixmap);
        vendor->staticDispatch.destroyPixmap(dpy, pixmap);
    }
}


PUBLIC void glXDestroyWindow(Display *dpy, GLXWindow win)
{
    __GLXvendorInfo *vendor = CommonDispatchDrawable(dpy, win,
            X_GLXDestroyWindow, GLXBadWindow, False);
    if (vendor != NULL) {
        __glXRemoveVendorDrawableMapping(dpy, win);
        vendor->staticDispatch.destroyWindow(dpy, win);
    }
}

PUBLIC int glXGetFBConfigAttrib(Display *dpy, GLXFBConfig config,
                         int attribute, int *value)
{
    __GLXvendorInfo *vendor = CommonDispatchFBConfig(dpy, config, X_GLXGetFBConfigs);
    if (vendor != NULL) {
        return vendor->staticDispatch.getFBConfigAttrib(dpy, config, attribute, value);
    } else {
        return GLX_BAD_VISUAL;
    }
}


PUBLIC GLXFBConfig *glXGetFBConfigs(Display *dpy, int screen, int *nelements)
{
    __GLXvendorInfo *vendor = __glXGetDynDispatch(dpy, screen);
    if (vendor != NULL) {
        GLXFBConfig *fbconfigs = vendor->staticDispatch.getFBConfigs(dpy, screen, nelements);
        if (fbconfigs != NULL) {
            int i;
            Bool success = True;
            for (i = 0; i < *nelements; i++) {
                if (__glXAddVendorFBConfigMapping(dpy, fbconfigs[i], vendor) != 0) {
                    success = False;
                    break;
                }
            }
            if (!success) {
                XFree(fbconfigs);
                fbconfigs = NULL;
                *nelements = 0;
            }
        }
        return fbconfigs;
    } else {
        return NULL;
    }
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
    __GLXvendorInfo *vendor = CommonDispatchFBConfig(dpy, config, X_GLXGetFBConfigs);
    if (vendor != NULL) {
        return vendor->staticDispatch.getVisualFromFBConfig(dpy, config);
    } else {
        return NULL;
    }
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

const __GLXlocalDispatchFunction LOCAL_GLX_DISPATCH_FUNCTIONS[] =
{
#define LOCAL_FUNC_TABLE_ENTRY(func) \
    { #func, (__GLXextFuncPtr)(func) },
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
        LOCAL_FUNC_TABLE_ENTRY(glXCreateContextAttribsARB)
#undef LOCAL_FUNC_TABLE_ENTRY
    { NULL, NULL }
};

typedef struct {
    GLubyte *procName;
    __GLXextFuncPtr addr;
    UT_hash_handle hh;
} __GLXprocAddressHash;

static DEFINE_INITIALIZED_LKDHASH(__GLXprocAddressHash, __glXProcAddressHash);

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
    __GLXprocAddressHash *pEntry = NULL;

    LKDHASH_RDLOCK(__glXProcAddressHash);
    HASH_FIND(hh, _LH(__glXProcAddressHash), procName,
              strlen((const char *)procName), pEntry);
    LKDHASH_UNLOCK(__glXProcAddressHash);

    if (pEntry) {
        return pEntry->addr;
    }

    return NULL;
}

static void cacheProcAddress(const GLubyte *procName, __GLXextFuncPtr addr)
{
    size_t nameLen = strlen((const char *) procName);
    __GLXprocAddressHash *pEntry;

    LKDHASH_WRLOCK(__glXProcAddressHash);

    HASH_FIND(hh, _LH(__glXProcAddressHash), procName,
              nameLen, pEntry);
    if (pEntry == NULL) {
        pEntry = malloc(sizeof(*pEntry) + nameLen + 1);
        if (pEntry != NULL) {
            pEntry->procName = (GLubyte *) (pEntry + 1);
            memcpy(pEntry->procName, procName, nameLen + 1);
            pEntry->addr = addr;
            HASH_ADD_KEYPTR(hh, _LH(__glXProcAddressHash), pEntry->procName,
                            nameLen, pEntry);
        }
    } else {
        assert(pEntry->addr == addr);
    }
    LKDHASH_UNLOCK(__glXProcAddressHash);
}

PUBLIC __GLXextFuncPtr glXGetProcAddressARB(const GLubyte *procName)
{
    __glXThreadInitialize();

    return glXGetProcAddress(procName);
}

PUBLIC __GLXextFuncPtr glXGetProcAddress(const GLubyte *procName)
{
    __GLXextFuncPtr addr = NULL;

    __glXThreadInitialize();

    /*
     * Easy case: First check if we already know this address from
     * a previous GetProcAddress() call or by virtue of being a function
     * exported by libGLX.
     */
    addr = __glXGetCachedProcAddress(procName);
    if (addr) {
        return addr;
    }

    if (procName[0] == 'g' && procName[1] == 'l' && procName[2] == 'X') {
        // This looks like a GLX function, so try to find a GLX dispatch stub.
        addr = __glXGetGLXDispatchAddress(procName);
    } else {
        addr = __glDispatchGetProcAddress((const char *) procName);
    }

    /* Store the resulting proc address. */
    if (addr) {
        cacheProcAddress(procName, addr);
    }

    return addr;
}

PUBLIC __GLXextFuncPtr __glXGLLoadGLXFunction(const char *name,
        __GLXextFuncPtr *ptr, glvnd_mutex_t *mutex)
{
    __GLXextFuncPtr func;

    __glvndPthreadFuncs.mutex_lock(mutex);

    func = *ptr;
    if (func == NULL) {
        func = glXGetProcAddress((const GLubyte *) name);
        *ptr = func;
    }

    __glvndPthreadFuncs.mutex_unlock(mutex);
    return func;
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

/*!
 * Checks to see if a fork occurred since the last GLX entrypoint was called,
 * and performs recovery if needed.
 */
static void CheckFork(void)
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
}

/*!
 * Handles any common tasks that need to occur at the beginning of any GLX
 * entrypoint.
 */
void __glXThreadInitialize(void)
{
    CheckFork();
    __glDispatchCheckMultithreaded();
}

static void __glXAPITeardown(Bool doReset)
{
    __GLXThreadState *threadState, *threadStateTemp;
    __GLXcontextInfo *currContext, *currContextTemp;

    glvnd_list_for_each_entry_safe(threadState, threadStateTemp, &currentThreadStateList, entry) {
        glvnd_list_del(&threadState->entry);
        free(threadState);
    }

    if (doReset) {
        /*
         * XXX: We should be able to get away with just resetting the proc address
         * hash lock, and not throwing away cached addresses.
         */
        __glvndPthreadFuncs.rwlock_init(&__glXProcAddressHash.lock, NULL);
        __glvndPthreadFuncs.mutex_init(&currentThreadStateListMutex, NULL);

        HASH_ITER(hh, glxContextHash, currContext, currContextTemp) {
            currContext->currentCount = 0;
            CheckContextDeleted(currContext);
        }
    } else {
        LKDHASH_TEARDOWN(__GLXprocAddressHash,
                         __glXProcAddressHash, NULL, NULL, False);

        /*
         * It's possible that another thread could be blocked in a
         * glXMakeCurrent call here, especially if an Xlib I/O error occurred.
         * In that case, the other thead will be holding the context hash lock,
         * so we'd deadlock if we tried to wait for it here. Instead, clean up
         * if the lock is available, but don't try to wait if it isn't.
         */
        if (__glvndPthreadFuncs.mutex_trylock(&glxContextHashLock) == 0) {
            HASH_ITER(hh, glxContextHash, currContext, currContextTemp) {
                FreeContextInfo(currContext);
            }
            assert(glxContextHash == NULL);
            __glvndPthreadFuncs.mutex_unlock(&glxContextHashLock);
        }
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

PUBLIC const __glXGLCoreFunctions __GLXGL_CORE_FUNCTIONS = {
    glXChooseFBConfig,
    glXChooseVisual,
    glXCopyContext,
    glXCreateContext,
    glXCreateGLXPixmap,
    glXCreateNewContext,
    glXCreatePbuffer,
    glXCreatePixmap,
    glXCreateWindow,
    glXDestroyContext,
    glXDestroyGLXPixmap,
    glXDestroyPbuffer,
    glXDestroyPixmap,
    glXDestroyWindow,
    glXGetClientString,
    glXGetConfig,
    glXGetCurrentContext,
    glXGetCurrentDrawable,
    glXGetCurrentReadDrawable,
    glXGetFBConfigAttrib,
    glXGetFBConfigs,
    glXGetProcAddress,
    glXGetProcAddressARB,
    glXGetSelectedEvent,
    glXGetVisualFromFBConfig,
    glXIsDirect,
    glXMakeContextCurrent,
    glXMakeCurrent,
    glXQueryContext,
    glXQueryDrawable,
    glXQueryExtension,
    glXQueryExtensionsString,
    glXQueryServerString,
    glXQueryVersion,
    glXSelectEvent,
    glXSwapBuffers,
    glXUseXFont,
    glXWaitGL,
    glXWaitX,
};

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
    glvndSetupPthreads();
    glvndAppErrorCheckInit();

    glvnd_list_init(&currentThreadStateList);

    /*
     * glxContextHashLock must be a recursive mutex, because we'll have it
     * locked when we call into the vendor library's glXMakeCurrent
     * implementation. If the vendor library generates an X error, then that
     * will often result in a call to exit. In that case, the teardown code
     * will try to lock the mutex again so that it can clean up the current
     * context list.
     */
    __glvndPthreadFuncs.mutexattr_init(&mutexAttribs);
    __glvndPthreadFuncs.mutexattr_settype(&mutexAttribs, PTHREAD_MUTEX_RECURSIVE);
    __glvndPthreadFuncs.mutex_init(&glxContextHashLock, &mutexAttribs);
    __glvndPthreadFuncs.mutexattr_destroy(&mutexAttribs);

    __glXMappingInit();

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

    DBG_PRINTF(0, "Loading GLX...\n");

}

#if defined(USE_ATTRIBUTE_CONSTRUCTOR)
void __attribute__ ((destructor)) __glXFini(void)
#else
void _fini(void)
#endif
{
    /*
     * Note that the dynamic linker may have already called the destructors for
     * the vendor libraries. As a result, we can't do anything here that would
     * try to call into any vendor library.
     */

    /* Check for a fork before going further. */
    CheckFork();

    /*
     * If libGLX owns the current thread state, lose current
     * in GLdispatch before going further.
     */
    __GLdispatchThreadState *glas =
        __glDispatchGetCurrentThreadState();

    if (glas && glas->tag == GLDISPATCH_API_GLX) {
        __glDispatchLoseCurrent();
    }

    /* Tear down all GLX API state */
    __glXAPITeardown(False);

    /* Tear down all mapping state */
    __glXMappingTeardown(False);

    /* Tear down GLdispatch if necessary */
    __glDispatchFini();
}

