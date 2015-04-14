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

#include "lkdhash.h"

/* current version numbers */
#define GLX_MAJOR_VERSION 1
#define GLX_MINOR_VERSION 4

#define GLX_EXTENSION_NAME "GLX"

GLVNDPthreadFuncs __glXPthreadFuncs;

static glvnd_key_t tsdContextKey;
static Bool tsdContextKeyInitialized;
static glvnd_once_t threadCreateTSDContextOnceControl = GLVND_ONCE_INIT;

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

static DEFINE_INITIALIZED_LKDHASH(__GLXcurrentContextHash,
                                  __glXCurrentContextHash);

static Bool UpdateCurrentContext(GLXContext newCtx,
                                 GLXContext tsdCtx,
                                 Bool needsUnmapNew,
                                 Bool *needsUnmapOld);


static void ThreadDestroyed(void *tsdCtx)
{
    Bool needsUnmap;
    /*
     * If a GLX context is current in this thread, remove it from the
     * current context hash before destroying the thread.
     *
     * The TSD key associated with this destructor contains a pointer
     * to the current context.
     */
    LKDHASH_WRLOCK(__glXPthreadFuncs, __glXCurrentContextHash);
    UpdateCurrentContext(NULL, tsdCtx, False, &needsUnmap);
    LKDHASH_UNLOCK(__glXPthreadFuncs, __glXCurrentContextHash);

    if (needsUnmap) {
        __glXRemoveScreenContextMapping(tsdCtx);
    }

    /*
     * Call into GLdispatch to lose current to this thread.  XXX
     * should we check for ownership of the API state before doing
     * this?
     */
    __glDispatchLoseCurrent();
}

static void ThreadCreateTSDContextOnce(void)
{
    int ret = __glXPthreadFuncs.key_create(&tsdContextKey, ThreadDestroyed);
    assert(!ret);

    tsdContextKeyInitialized = True;
}

PUBLIC XVisualInfo* glXChooseVisual(Display *dpy, int screen, int *attrib_list)
{
    __glXThreadInitialize();

    const __GLXdispatchTableStatic *pDispatch = __glXGetStaticDispatch(dpy, screen);

    return pDispatch->glx14ep.chooseVisual(dpy, screen, attrib_list);
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
    const int screen = __glXScreenFromContext(src);
    const __GLXdispatchTableStatic *pDispatch = __glXGetStaticDispatch(dpy, screen);

    pDispatch->glx14ep.copyContext(dpy, src, dst, mask);
}


PUBLIC GLXContext glXCreateContext(Display *dpy, XVisualInfo *vis,
                            GLXContext share_list, Bool direct)
{
    __glXThreadInitialize();

    const int screen = vis->screen;
    const __GLXdispatchTableStatic *pDispatch = __glXGetStaticDispatch(dpy, screen);

    GLXContext context = pDispatch->glx14ep.createContext(dpy, vis, share_list, direct);

    __glXAddScreenContextMapping(context, screen);

    return context;
}


PUBLIC void glXDestroyContext(Display *dpy, GLXContext context)
{
    __glXThreadInitialize();

    const int screen = __glXScreenFromContext(context);
    const __GLXdispatchTableStatic *pDispatch = __glXGetStaticDispatch(dpy, screen);

    __glXNotifyContextDestroyed(context);

    pDispatch->glx14ep.destroyContext(dpy, context);
}


PUBLIC GLXPixmap glXCreateGLXPixmap(Display *dpy, XVisualInfo *vis, Pixmap pixmap)
{
    __glXThreadInitialize();

    const int screen = vis->screen;
    const __GLXdispatchTableStatic *pDispatch = __glXGetStaticDispatch(dpy, screen);

    GLXPixmap pmap = pDispatch->glx14ep.createGLXPixmap(dpy, vis, pixmap);

    __glXAddScreenDrawableMapping(pmap, screen);

    return pmap;
}


PUBLIC void glXDestroyGLXPixmap(Display *dpy, GLXPixmap pix)
{
    __glXThreadInitialize();

    const int screen = __glXScreenFromDrawable(dpy, pix);
    const __GLXdispatchTableStatic *pDispatch = __glXGetStaticDispatch(dpy, screen);

    __glXRemoveScreenDrawableMapping(pix);

    pDispatch->glx14ep.destroyGLXPixmap(dpy, pix);
}


PUBLIC int glXGetConfig(Display *dpy, XVisualInfo *vis, int attrib, int *value)
{
    __glXThreadInitialize();

    const __GLXdispatchTableStatic *pDispatch;

    if (!dpy || !vis || !value) {
        return GLX_BAD_VALUE;
    }

    pDispatch = __glXGetStaticDispatch(dpy, vis->screen);

    return pDispatch->glx14ep.getConfig(dpy, vis, attrib, value);
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
    return apiState->currentDraw;
}


PUBLIC Bool glXIsDirect(Display *dpy, GLXContext context)
{
    __glXThreadInitialize();

    const int screen = __glXScreenFromContext(context);
    const __GLXdispatchTableStatic *pDispatch = __glXGetStaticDispatch(dpy, screen);

    return pDispatch->glx14ep.isDirect(dpy, context);
}

static DEFINE_INITIALIZED_LKDHASH(__GLXAPIState, __glXAPIStateHash);

void DisplayClosed(Display *dpy)
{
    __GLXAPIState *apiState, *tmp;
    __glXFreeDisplay(dpy);
    LKDHASH_WRLOCK(__glXPthreadFuncs, __glXAPIStateHash);
    HASH_ITER(hh, _LH(__glXAPIStateHash), apiState, tmp) {
        /*
         * Stub out any references to this display in the API states.
         */
        if (apiState->currentDisplay == dpy) {
            apiState->currentDisplay = NULL;
        }
    }
    LKDHASH_UNLOCK(__glXPthreadFuncs, __glXAPIStateHash);
}

/* NOTE this assumes the __glXAPIStateHash lock is taken! */
static __GLXAPIState *CreateAPIState(glvnd_thread_t tid)
{
    __GLXAPIState *apiState = calloc(1, sizeof(*apiState));

    assert(apiState);

    apiState->glas.tag = GLDISPATCH_API_GLX;
    apiState->glas.id = malloc(sizeof(glvnd_thread_t));
    apiState->glas.vendorID = -1;

    *((glvnd_thread_t *)apiState->glas.id) = tid;

    HASH_ADD_KEYPTR(hh, _LH(__glXAPIStateHash), apiState->glas.id,
                    sizeof(glvnd_thread_t), apiState);

    return apiState;
}

/* NOTE this assumes the __glXAPIStateHash lock is taken! */
static void CleanupAPIStateEntry(void *unused, __GLXAPIState *apiState)
{
    free(apiState->glas.id);
}

/* NOTE this assumes the __glXAPIStateHash lock is taken! */
static __GLXAPIState *LookupAPIState(glvnd_thread_t tid)
{
    __GLXAPIState *apiState;

    HASH_FIND(hh, _LH(__glXAPIStateHash), &tid,
              sizeof(glvnd_thread_t), apiState);

    return apiState;
}

__GLXAPIState *__glXGetAPIState(glvnd_thread_t tid)
{
    __GLXAPIState *apiState;

    LKDHASH_RDLOCK(__glXPthreadFuncs, __glXAPIStateHash);

    apiState = LookupAPIState(tid);
    if (!apiState) {
        LKDHASH_UNLOCK(__glXPthreadFuncs, __glXAPIStateHash);
        LKDHASH_WRLOCK(__glXPthreadFuncs, __glXAPIStateHash);
        apiState = LookupAPIState(tid);
        if (!apiState) {
            apiState = CreateAPIState(tid);
        }
    }

    LKDHASH_UNLOCK(__glXPthreadFuncs, __glXAPIStateHash);

    return apiState;
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
    LKDHASH_RDLOCK(__glXPthreadFuncs, __glXCurrentContextHash);

    HASH_FIND(hh, _LH(__glXCurrentContextHash), &ctx, sizeof(ctx), pEntry);

    if (pEntry) {
        canUnmap = False;
        pEntry->needsUnmap = True;
    }

    if (canUnmap) {
        /*
         * Note: this implies a lock ordering: the current context
         * hash lock must be taken before the screen pointer hash lock!
         */
        __glXRemoveScreenContextMapping(ctx);
    }

    LKDHASH_UNLOCK(__glXPthreadFuncs, __glXCurrentContextHash);

}

/*!
 * Updates the current context.  This function handles:
 *
 * - Adding the new current context newCtx to the process-global
 *   __glXCurrentContextHash and updating this thread's TSD entry to
 *   contain this context
 * - Removing the old current context oldCtx from __glXCurrentContextHash
 *
 * \param[in] ctx The context to make current
 * \param[in] tsdCtx If this function is called from the tsdContextKey
 * destructor, this should point to the old context to use.  Otherwise, this
 * should be set to NULL.
 * \param[in] needsUnmapNew True when the new context's screen mapping
 * should be removed when it is no longer current (this can happen if the make
 * current operation failed and we are restoring a context which will be
 * destroyed when it loses current) or False otherwise.
 * \param[out] needsUnmapOld If non-NULL, points to a boolean which is set to
 * indicate whether the old context's screen mapping needs to be removed
 * (because the old context is about to be destroyed).
 *
 * Returns True on success, False otherwise.
 *
 * Note: __glXCurrentContextHash must be write-locked before calling this
 * function!
 *
 * Returns True on success.
 */
static Bool UpdateCurrentContext(GLXContext newCtx,
                                 GLXContext tsdCtx,
                                 Bool needsUnmapNew,
                                 Bool *needsUnmapOld)
{
    __GLXcurrentContextHash *pOldEntry,
                            *pTmpEntry,
                            *pNewEntry;
    GLXContext oldCtx;

    if (needsUnmapOld) {
        *needsUnmapOld = False;
    }

    // Attempt the allocation first, so we can bail out early
    // on failure.
    if (newCtx) {
        pNewEntry = malloc(sizeof(*pNewEntry));
        if (!pNewEntry) {
            return False;
        }
        pNewEntry->ctx = newCtx;
        pNewEntry->needsUnmap = needsUnmapNew;
    } else {
        pNewEntry = NULL;
    }

    // Initialize the TSD entry (if we haven't already)
    __glXPthreadFuncs.once(&threadCreateTSDContextOnceControl,
                           ThreadCreateTSDContextOnce);

    if (!tsdContextKeyInitialized) {
        return False;
    }

    // Update the TSD entry to reflect the correct current context.  It's
    // possible that this may be called from tsdContextKey's destructor.  In
    // that case, the NULL value has already been associated with the key and
    // pthread_getspecific() will return the wrong value, so use the value
    // passed in as tsdCtx instead.  In the case where this is called from a
    // destructor and tsdCtx == NULL, oldCtx will (correctly) be set to NULL.
    if (tsdCtx) {
        oldCtx = tsdCtx;
    } else {
        oldCtx = (GLXContext)__glXPthreadFuncs.getspecific(tsdContextKey);
    }
    __glXPthreadFuncs.setspecific(tsdContextKey, newCtx);

    if (oldCtx) {
        // Remove the old context from the hash table, if not NULL
        HASH_FIND(hh, _LH(__glXCurrentContextHash), &oldCtx, sizeof(oldCtx), pOldEntry);

        assert(pOldEntry);

        if (needsUnmapOld) {
            *needsUnmapOld = pOldEntry->needsUnmap;
        }
        HASH_DELETE(hh, _LH(__glXCurrentContextHash), pOldEntry);
        free(pOldEntry);
    }

    if (pNewEntry) {
        HASH_FIND(hh, _LH(__glXCurrentContextHash), &newCtx, sizeof(newCtx), pTmpEntry);
        assert(!pTmpEntry);
        HASH_ADD(hh, _LH(__glXCurrentContextHash), ctx, sizeof(newCtx), pNewEntry);
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

    HASH_FIND(hh, _LH(__glXCurrentContextHash), &ctx, sizeof(ctx), pEntry);

    return !!pEntry && (current != ctx);
}

/*!
 * Given the Make{Context,}Current arguments passed in, tries to find the
 * screen of an appropriate vendor to notify when an X error occurs.  It's
 * possible none of the arguments in this list will produce a valid screen
 * number, so this will fall back to screen 0 if all else fails.
 */
int FindAnyValidScreenFromMakeCurrent(Display *dpy,
                                      GLXDrawable draw,
                                      GLXDrawable read,
                                      GLXContext context)
{
    int screen;

    screen = __glXScreenFromContext(__glXGetCurrentContext());

    if (screen < 0) {
        screen = __glXScreenFromContext(context);
    }

    if (screen < 0) {
        screen = __glXScreenFromDrawable(dpy, draw);
    }

    if (screen < 0) {
        screen = __glXScreenFromDrawable(dpy, read);
    }

    if (screen < 0) {
        /* If no screens were found, fall back to 0 */
        screen = 0;
    }

    return screen;
}

void NotifyVendorOfXError(int screen,
                          Display *dpy,
                          char errorOpcode,
                          char minorOpcode,
                          XID resid)
{
    /*
     * XXX: For now, libglvnd doesn't handle generating X errors directly.
     * Instead, it tries to pass the error off to an available vendor library
     * so the vendor can handle generating the X error.
     */
    const __GLXdispatchTableStatic *pDispatch =
        __glXGetStaticDispatch(dpy, screen);

    if (pDispatch->glxvc.notifyError) {
        pDispatch->glxvc.notifyError(dpy, errorOpcode, minorOpcode, resid);
    }
}

static Bool MakeContextCurrentInternal(Display *dpy,
                                       GLXDrawable draw,
                                       GLXDrawable read,
                                       GLXContext context,
                                       char callerOpcode,
                                       const __GLXdispatchTableStatic **ppDispatch)
{
    __GLXAPIState *apiState;
    __GLXvendorInfo *oldVendor, *newVendor;
    Bool ret;
    int screen;

    DBG_PRINTF(0, "dpy = %p, draw = %x, read = %x, context = %p\n",
               dpy, (unsigned)draw, (unsigned)read, context);

    assert(callerOpcode == X_GLXMakeCurrent ||
           callerOpcode == X_GLXMakeContextCurrent);

    apiState = __glXGetCurrentAPIState();
    oldVendor = apiState->currentVendor;
    screen = __glXScreenFromContext(context);

    if (context && (screen < 0)) {
        /*
         * XXX: We can run into this corner case if a GLX client calls
         * glXDestroyContext() on a current context, loses current to this
         * context (causing it to be freed), then tries to make current to the
         * context again.  This is incorrect application behavior, but we should
         * attempt to handle this failure gracefully.
         */
        return False;
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
        int errorScreen =
            FindAnyValidScreenFromMakeCurrent(dpy, draw, read, context);
        NotifyVendorOfXError(errorScreen, dpy, BadMatch, callerOpcode, 0);
        return False;
    }

    /*
     * If we have a valid screen number, there must be a valid vendor associated
     * with that screen.
     */
    newVendor = __glXLookupVendorByScreen(dpy, screen);
    assert(!context || newVendor);

    if (oldVendor != newVendor) {
        // Lose current on the old context before proceeding
        if (oldVendor) {
            const __GLXdispatchTableStatic *oldDispatch =
                oldVendor->staticDispatch;
            assert(oldDispatch);
            ret = oldDispatch->glx14ep.makeCurrent(dpy,
                                                   None,
                                                   NULL);
            if (!ret) {
                return False;
            }
        }
    }

    /* Save the new dispatch table for use by caller */
    assert(!newVendor || newVendor->staticDispatch);
    *ppDispatch = newVendor ? newVendor->staticDispatch : NULL;

    if (!context) {
        /*
         * Call into GLdispatch to lose current and update the context and GL
         * dispatch table
         */
        __glDispatchLoseCurrent();

        /* Update the current display and drawable(s) in this apiState */
        apiState->currentDisplay = dpy;
        apiState->currentDraw = draw;
        apiState->currentRead = read;
        apiState->currentVendor = NULL;

        /* Update the GLX dispatch table */
        apiState->currentStaticDispatch = NULL;
        apiState->currentDynDispatch = NULL;

        return True;
    } else {
        assert(newVendor);

        /* Update the current display and drawable(s) in this apiState */
        apiState->currentDisplay = dpy;
        apiState->currentDraw = draw;
        apiState->currentRead = read;
        apiState->currentVendor = newVendor;

        /* Update the GLX dispatch table */
        apiState->currentStaticDispatch = *ppDispatch;
        apiState->currentDynDispatch = newVendor->dynDispatch;

        DBG_PRINTF(0, "GL dispatch = %p\n", apiState->glas.dispatch);


        /*
         * XXX It is possible that these drawables were never seen by
         * this libGLX.so instance before now.  Since the screen is
         * known from the context, and the drawable must be on the
         * same screen if MakeCurrent passed, then record the mapping
         * of this drawable to the context's screen.
         */
        __glXAddScreenDrawableMapping(draw, screen);
        __glXAddScreenDrawableMapping(read, screen);

        /*
         * Call into GLdispatch to set up the current context and
         * GL dispatch table.
         */
        ret = __glDispatchMakeCurrent(
            &apiState->glas,
            newVendor->glDispatch,
            (void *)context,
            newVendor->vendorID,
            newVendor->staticDispatch->glxvc.patchCallbacks
        );

        return ret;
    }
}

static void SaveCurrentValues(GLXDrawable *pDraw,
                              GLXDrawable *pRead,
                              GLXContext *pContext)
{
    *pDraw = glXGetCurrentDrawable();
    *pRead = glXGetCurrentReadDrawable();
    *pContext = glXGetCurrentContext();
}

PUBLIC Bool glXMakeCurrent(Display *dpy, GLXDrawable drawable, GLXContext context)
{
    __glXThreadInitialize();

    const __GLXdispatchTableStatic *pDispatch;
    Bool tmpRet, ret;
    GLXDrawable oldDraw, oldRead;
    GLXContext oldContext;
    Bool oldContextNeedsUnmap;

    SaveCurrentValues(&oldDraw, &oldRead, &oldContext);

    LKDHASH_WRLOCK(__glXPthreadFuncs, __glXCurrentContextHash);

    if (IsContextCurrentToAnyOtherThread(context)) {
        // XXX throw BadAccess?
        LKDHASH_UNLOCK(__glXPthreadFuncs, __glXCurrentContextHash);
        return False;
    }

    if (oldContext != context) {
        if (!UpdateCurrentContext(context, NULL, False, &oldContextNeedsUnmap)) {
            /*
             * Fail here. Continuing on would mess up our accounting.
             */
            LKDHASH_UNLOCK(__glXPthreadFuncs, __glXCurrentContextHash);
            return False;
        }
    }

    ret = MakeContextCurrentInternal(dpy,
                                     drawable,
                                     drawable,
                                     context,
                                     X_GLXMakeCurrent,
                                     &pDispatch);
    if (ret) {
        assert(!context || pDispatch);
        if (pDispatch) {
            ret = pDispatch->glx14ep.makeCurrent(dpy, drawable, context);
            if (!ret) {
                // Restore the original current values
                tmpRet = MakeContextCurrentInternal(dpy,
                                                    oldDraw,
                                                    oldRead,
                                                    oldContext,
                                                    X_GLXMakeCurrent,
                                                    &pDispatch);
                assert(tmpRet);
                if (pDispatch) {
                    tmpRet = pDispatch->glx14ep.makeContextCurrent(dpy,
                                                                oldDraw,
                                                                oldRead,
                                                                oldContext);
                    assert(tmpRet);
                }
            }
        }
    }

    if (oldContext != context) {
        /*
         * If the make current operation failed, restore the original context.
         */
        if (!ret) {
            tmpRet = UpdateCurrentContext(oldContext, NULL, oldContextNeedsUnmap, NULL);
            assert(tmpRet);
        } else if (oldContextNeedsUnmap) {
            __glXRemoveScreenContextMapping(oldContext);
        }
    }

    LKDHASH_UNLOCK(__glXPthreadFuncs, __glXCurrentContextHash);

    return ret;
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

    int extMajor, extEvent, extError;
    Bool ret;

    ret = XQueryExtension(dpy, GLX_EXTENSION_NAME, &extMajor, &extEvent, &extError);

    if (ret == False) {
        /* No extension! */
        return False;
    }

    LockDisplay(dpy);
    GetReq(GLXQueryVersion, req);
    req->reqType = extMajor;
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
    __glXThreadInitialize();

    const int screen = __glXScreenFromDrawable(dpy, drawable);
    const __GLXdispatchTableStatic *pDispatch = __glXGetStaticDispatch(dpy, screen);

    pDispatch->glx14ep.swapBuffers(dpy, drawable);
}


PUBLIC void glXUseXFont(Font font, int first, int count, int list_base)
{
    __glXThreadInitialize();

    const __GLXdispatchTableStatic *pDispatch = __glXGetCurrentDispatch();

    pDispatch->glx14ep.useXFont(font, first, count, list_base);
}


PUBLIC void glXWaitGL(void)
{
    __glXThreadInitialize();

    const __GLXdispatchTableStatic *pDispatch = __glXGetCurrentDispatch();

    pDispatch->glx14ep.waitGL();
}


PUBLIC void glXWaitX(void)
{
    __glXThreadInitialize();

    const __GLXdispatchTableStatic *pDispatch = __glXGetCurrentDispatch();

    pDispatch->glx14ep.waitX();
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
        result[screen] = pDispatch->glx14ep.getClientString(dpy, name);
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
        newLen = strlen(newString);
        if (newLen > 0) {
            buf = (char *) realloc(currentString, newLen + 1);
            if (buf == NULL) {
                free(currentString);
                return NULL;
            }
            memcpy(buf, newString, newLen + 1);
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

    if (ret > 0) {
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
        return pDispatch->glx14ep.getClientString(dpy, name);
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

    return pDispatch->glx14ep.queryServerString(dpy, screen, name);
}


PUBLIC const char *glXQueryExtensionsString(Display *dpy, int screen)
{
    __glXThreadInitialize();

    const __GLXdispatchTableStatic *pDispatch = __glXGetStaticDispatch(dpy, screen);

    return pDispatch->glx14ep.queryExtensionsString(dpy, screen);
}


PUBLIC Display *glXGetCurrentDisplay(void)
{
    __glXThreadInitialize();

    __GLXAPIState *apiState = __glXGetCurrentAPIState();
    return apiState->currentDisplay;
}


PUBLIC GLXFBConfig *glXChooseFBConfig(Display *dpy, int screen,
                                      const int *attrib_list, int *nelements)
{
    __glXThreadInitialize();

    const __GLXdispatchTableStatic *pDispatch = __glXGetStaticDispatch(dpy, screen);
    GLXFBConfig *fbconfigs =
        pDispatch->glx14ep.chooseFBConfig(dpy, screen, attrib_list, nelements);
    int i;

    if (fbconfigs != NULL) {
        for (i = 0; i < *nelements; i++) {
            __glXAddScreenFBConfigMapping(fbconfigs[i], screen);
        }
    }

    return fbconfigs;
}


PUBLIC GLXContext glXCreateNewContext(Display *dpy, GLXFBConfig config,
                               int render_type, GLXContext share_list,
                               Bool direct)
{
    __glXThreadInitialize();

    const int screen = __glXScreenFromFBConfig(config);
    const __GLXdispatchTableStatic *pDispatch = __glXGetStaticDispatch(dpy, screen);

    GLXContext context = pDispatch->glx14ep.createNewContext(dpy, config, render_type,
                                                     share_list, direct);
    __glXAddScreenContextMapping(context, screen);

    return context;
}


PUBLIC GLXPbuffer glXCreatePbuffer(Display *dpy, GLXFBConfig config,
                            const int *attrib_list)
{
    __glXThreadInitialize();

    const int screen = __glXScreenFromFBConfig(config);
    const __GLXdispatchTableStatic *pDispatch = __glXGetStaticDispatch(dpy, screen);

    GLXPbuffer pbuffer = pDispatch->glx14ep.createPbuffer(dpy, config, attrib_list);

    __glXAddScreenDrawableMapping(pbuffer, screen);

    return pbuffer;
}


PUBLIC GLXPixmap glXCreatePixmap(Display *dpy, GLXFBConfig config,
                          Pixmap pixmap, const int *attrib_list)
{
    __glXThreadInitialize();

    const int screen = __glXScreenFromFBConfig(config);
    const __GLXdispatchTableStatic *pDispatch = __glXGetStaticDispatch(dpy, screen);

    GLXPixmap glxPixmap =
        pDispatch->glx14ep.createPixmap(dpy, config, pixmap, attrib_list);

    __glXAddScreenDrawableMapping(glxPixmap, screen);

    return glxPixmap;
}


PUBLIC GLXWindow glXCreateWindow(Display *dpy, GLXFBConfig config,
                          Window win, const int *attrib_list)
{
    __glXThreadInitialize();

    const int screen = __glXScreenFromFBConfig(config);
    const __GLXdispatchTableStatic *pDispatch = __glXGetStaticDispatch(dpy, screen);

    GLXWindow glxWindow =
        pDispatch->glx14ep.createWindow(dpy, config, win, attrib_list);

    __glXAddScreenDrawableMapping(glxWindow, screen);

    return glxWindow;
}


PUBLIC void glXDestroyPbuffer(Display *dpy, GLXPbuffer pbuf)
{
    __glXThreadInitialize();

    const int screen = __glXScreenFromDrawable(dpy, pbuf);
    const __GLXdispatchTableStatic *pDispatch = __glXGetStaticDispatch(dpy, screen);

    __glXRemoveScreenDrawableMapping(pbuf);

    pDispatch->glx14ep.destroyPbuffer(dpy, pbuf);
}


PUBLIC void glXDestroyPixmap(Display *dpy, GLXPixmap pixmap)
{
    __glXThreadInitialize();

    const int screen = __glXScreenFromDrawable(dpy, pixmap);
    const __GLXdispatchTableStatic *pDispatch = __glXGetStaticDispatch(dpy, screen);

    __glXRemoveScreenDrawableMapping(pixmap);

    pDispatch->glx14ep.destroyPixmap(dpy, pixmap);
}


PUBLIC void glXDestroyWindow(Display *dpy, GLXWindow win)
{
    __glXThreadInitialize();

    const int screen = __glXScreenFromDrawable(dpy, win);
    const __GLXdispatchTableStatic *pDispatch = __glXGetStaticDispatch(dpy, screen);

    __glXRemoveScreenDrawableMapping(win);

    pDispatch->glx14ep.destroyWindow(dpy, win);
}


PUBLIC GLXDrawable glXGetCurrentReadDrawable(void)
{
    __glXThreadInitialize();

    __GLXAPIState *apiState = __glXGetCurrentAPIState();
    return apiState->currentRead;
}


PUBLIC int glXGetFBConfigAttrib(Display *dpy, GLXFBConfig config,
                         int attribute, int *value)
{
    __glXThreadInitialize();

    const int screen = __glXScreenFromFBConfig(config);
    const __GLXdispatchTableStatic *pDispatch = __glXGetStaticDispatch(dpy, screen);

    return pDispatch->glx14ep.getFBConfigAttrib(dpy, config, attribute, value);
}


PUBLIC GLXFBConfig *glXGetFBConfigs(Display *dpy, int screen, int *nelements)
{
    __glXThreadInitialize();

    const __GLXdispatchTableStatic *pDispatch = __glXGetStaticDispatch(dpy, screen);
    GLXFBConfig *fbconfigs = pDispatch->glx14ep.getFBConfigs(dpy, screen, nelements);
    int i;

    if (fbconfigs != NULL) {
        for (i = 0; i < *nelements; i++) {
            __glXAddScreenFBConfigMapping(fbconfigs[i], screen);
        }
    }

    return fbconfigs;
}


PUBLIC void glXGetSelectedEvent(Display *dpy, GLXDrawable draw,
                         unsigned long *event_mask)
{
    __glXThreadInitialize();

    const int screen = __glXScreenFromDrawable(dpy, draw);
    const __GLXdispatchTableStatic *pDispatch = __glXGetStaticDispatch(dpy, screen);

    pDispatch->glx14ep.getSelectedEvent(dpy, draw, event_mask);
}


PUBLIC XVisualInfo *glXGetVisualFromFBConfig(Display *dpy, GLXFBConfig config)
{
    __glXThreadInitialize();

    const int screen = __glXScreenFromFBConfig(config);
    const __GLXdispatchTableStatic *pDispatch = __glXGetStaticDispatch(dpy, screen);

    return pDispatch->glx14ep.getVisualFromFBConfig(dpy, config);
}

PUBLIC Bool glXMakeContextCurrent(Display *dpy, GLXDrawable draw,
                                  GLXDrawable read, GLXContext context)
{
    __glXThreadInitialize();

    const __GLXdispatchTableStatic *pDispatch;
    Bool tmpRet, ret;
    GLXDrawable oldDraw, oldRead;
    GLXContext oldContext;
    Bool oldContextNeedsUnmap;

    SaveCurrentValues(&oldDraw, &oldRead, &oldContext);

    LKDHASH_WRLOCK(__glXPthreadFuncs, __glXCurrentContextHash);

    if (IsContextCurrentToAnyOtherThread(context)) {
        // XXX throw BadAccess?
        LKDHASH_UNLOCK(__glXPthreadFuncs, __glXCurrentContextHash);
        return False;
    }

    if (oldContext != context) {
        if (!UpdateCurrentContext(context, NULL, False, &oldContextNeedsUnmap)) {
            /*
             * Fail here. Continuing on would mess up our accounting.
             */
            LKDHASH_UNLOCK(__glXPthreadFuncs, __glXCurrentContextHash);
            return False;
        }
    }

    ret = MakeContextCurrentInternal(dpy,
                                     draw,
                                     read,
                                     context,
                                     X_GLXMakeContextCurrent,
                                     &pDispatch);
    if (ret) {
        assert(!context || pDispatch);
        if (pDispatch) {
            ret = pDispatch->glx14ep.makeContextCurrent(dpy,
                                                        draw,
                                                        read,
                                                        context);
            if (!ret) {
                // Restore the original current values
                tmpRet = MakeContextCurrentInternal(dpy,
                                                    oldDraw,
                                                    oldRead,
                                                    oldContext,
                                                    X_GLXMakeContextCurrent,
                                                    &pDispatch);
                assert(tmpRet);
                if (pDispatch) {
                    tmpRet = pDispatch->glx14ep.makeContextCurrent(dpy,
                                                                oldDraw,
                                                                oldRead,
                                                                oldContext);
                    assert(tmpRet);
                }
            }
        }
    }

    if (oldContext != context) {
        /*
         * If the make current operation failed, restore the original context.
         */
        if (!ret) {
            tmpRet = UpdateCurrentContext(oldContext, NULL, oldContextNeedsUnmap, NULL);
            assert(tmpRet);
        } else if (oldContextNeedsUnmap) {
            __glXRemoveScreenContextMapping(oldContext);
        }
    }

    LKDHASH_UNLOCK(__glXPthreadFuncs, __glXCurrentContextHash);

    return ret;
}


PUBLIC int glXQueryContext(Display *dpy, GLXContext context, int attribute, int *value)
{
    __glXThreadInitialize();

    const int screen = __glXScreenFromContext(context);
    const __GLXdispatchTableStatic *pDispatch = __glXGetStaticDispatch(dpy, screen);

    return pDispatch->glx14ep.queryContext(dpy, context, attribute, value);
}


PUBLIC void glXQueryDrawable(Display *dpy, GLXDrawable draw,
                      int attribute, unsigned int *value)
{
    __glXThreadInitialize();

    const int screen = __glXScreenFromDrawable(dpy, draw);
    const __GLXdispatchTableStatic *pDispatch = __glXGetStaticDispatch(dpy, screen);

    return pDispatch->glx14ep.queryDrawable(dpy, draw, attribute, value);
}


PUBLIC void glXSelectEvent(Display *dpy, GLXDrawable draw, unsigned long event_mask)
{
    __glXThreadInitialize();

    const int screen = __glXScreenFromDrawable(dpy, draw);
    const __GLXdispatchTableStatic *pDispatch = __glXGetStaticDispatch(dpy, screen);

    pDispatch->glx14ep.selectEvent(dpy, draw, event_mask);
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
PUBLIC __GLXextFuncPtr __glXGetCachedProcAddress(const GLubyte *procName)
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

    /*
     * If *that* doesn't work, request a NOP stub from GLdispatch.  This should
     * always succeed if the function name begins with "gl".
     */
    addr = __glDispatchGetProcAddress((const char *)procName);
    assert(addr || procName[0] != 'g' || procName[1] != 'l');

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
            if (newVal < 0) {
                newVal = 0;
            }
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
        __glXRemoveScreenContextMapping(pEntry->ctx);
    }
}

static void __glXAPITeardown(Bool doReset)
{
    /* Clear the current TSD context */
    __glXPthreadFuncs.setspecific(tsdContextKey, NULL);

    /*
     * XXX: This will leave dangling screen-context mappings, but they will be
     * cleared separately in __glXMappingTeardown().
     */
    LKDHASH_TEARDOWN(__glXPthreadFuncs, __GLXcurrentContextHash,
                     __glXCurrentContextHash, CurrentContextHashCleanup, NULL, doReset);

    LKDHASH_TEARDOWN(__glXPthreadFuncs, __GLXAPIState,
                     __glXAPIStateHash, CleanupAPIStateEntry,
                     NULL, doReset);

    if (doReset) {
        /*
         * XXX: We should be able to get away with just resetting the proc address
         * hash lock, and not throwing away cached addresses.
         */
        __glXPthreadFuncs.rwlock_init(&__glXProcAddressHash.lock, NULL);
    } else {
        LKDHASH_TEARDOWN(__glXPthreadFuncs, __GLXprocAddressHash,
                         __glXProcAddressHash, CleanupProcAddressEntry,
                         NULL, False);
    }
}

static void __glXResetOnFork(void)
{
    /* Reset all GLX API state */
    __glXAPITeardown(True);

    /* Reset all mapping state */
    __glXMappingTeardown(True);

    /* Reset GLdispatch */
    __glDispatchReset();
}

#if defined(USE_ATTRIBUTE_CONSTRUCTOR)
void __attribute__ ((constructor)) __glXInit(void)
#else
void _init(void)
#endif
{

    /* Initialize GLdispatch; this will also initialize our pthreads imports */
    __glDispatchInit(&__glXPthreadFuncs);

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

__GLXdispatchTableDynamic *__glXGetCurrentDynDispatch(void)
{
    __GLXAPIState *apiState = __glXGetCurrentAPIState();

    return apiState->currentDynDispatch;
}
