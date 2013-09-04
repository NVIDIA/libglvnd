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

#include "libglxthread.h"
#include "libglxabipriv.h"
#include "libglxmapping.h"
#include "libglxcurrent.h"
#include "utils_misc.h"
#include "trace.h"
#include "GL/glxproto.h"
#include "x11glvnd.h"

#include "lkdhash.h"

/* current version numbers */
#define GLX_MAJOR_VERSION 1
#define GLX_MINOR_VERSION 4

#define GLX_EXTENSION_NAME "GLX"

GLVNDPthreadFuncs __glXPthreadFuncs;

PUBLIC XVisualInfo* glXChooseVisual(Display *dpy, int screen, int *attrib_list)
{
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
    const int screen = vis->screen;
    const __GLXdispatchTableStatic *pDispatch = __glXGetStaticDispatch(dpy, screen);

    GLXContext context = pDispatch->glx14ep.createContext(dpy, vis, share_list, direct);

    __glXAddScreenContextMapping(context, screen);

    return context;
}


PUBLIC void glXDestroyContext(Display *dpy, GLXContext context)
{
    const int screen = __glXScreenFromContext(context);
    const __GLXdispatchTableStatic *pDispatch = __glXGetStaticDispatch(dpy, screen);

    __glXRemoveScreenContextMapping(context, screen);

    pDispatch->glx14ep.destroyContext(dpy, context);
}


PUBLIC GLXPixmap glXCreateGLXPixmap(Display *dpy, XVisualInfo *vis, Pixmap pixmap)
{
    const int screen = vis->screen;
    const __GLXdispatchTableStatic *pDispatch = __glXGetStaticDispatch(dpy, screen);

    GLXPixmap pmap = pDispatch->glx14ep.createGLXPixmap(dpy, vis, pixmap);

    __glXAddScreenDrawableMapping(pmap, screen);

    return pmap;
}


PUBLIC void glXDestroyGLXPixmap(Display *dpy, GLXPixmap pix)
{
    const int screen = __glXScreenFromDrawable(dpy, pix);
    const __GLXdispatchTableStatic *pDispatch = __glXGetStaticDispatch(dpy, screen);

    __glXRemoveScreenDrawableMapping(pix, screen);

    pDispatch->glx14ep.destroyGLXPixmap(dpy, pix);
}


PUBLIC int glXGetConfig(Display *dpy, XVisualInfo *vis, int attrib, int *value)
{
    const __GLXdispatchTableStatic *pDispatch = __glXGetStaticDispatch(dpy, vis->screen);

    return pDispatch->glx14ep.getConfig(dpy, vis, attrib, value);
}

PUBLIC GLXContext glXGetCurrentContext(void)
{
    return __glXGetCurrentContext();
}


PUBLIC GLXDrawable glXGetCurrentDrawable(void)
{
    __GLXAPIState *apiState = __glXGetCurrentAPIState();
    return apiState->currentDraw;
}


PUBLIC Bool glXIsDirect(Display *dpy, GLXContext context)
{
    const int screen = __glXScreenFromContext(context);
    const __GLXdispatchTableStatic *pDispatch = __glXGetStaticDispatch(dpy, screen);

    return pDispatch->glx14ep.isDirect(dpy, context);
}

static DEFINE_INITIALIZED_LKDHASH(__GLXAPIState, __glXAPIStateHash);

/* NOTE this assumes the __glXAPIStateHash lock is taken! */
static __GLXAPIState *CreateAPIState(glvnd_thread_t tid)
{
    __GLXAPIState *apiState = calloc(1, sizeof(*apiState));

    assert(apiState);

    apiState->glas.tag = GLDISPATCH_API_GLX;
    apiState->glas.id = malloc(sizeof(glvnd_thread_t));
    *((glvnd_thread_t *)apiState->glas.id) = tid;

    HASH_ADD_KEYPTR(hh, _LH(__glXAPIStateHash), apiState->glas.id,
                    sizeof(glvnd_thread_t), apiState);

    return apiState;
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

static Bool MakeContextCurrentInternal(Display *dpy,
                                       GLXDrawable draw,
                                       GLXDrawable read,
                                       GLXContext context,
                                       const __GLXdispatchTableStatic **ppDispatch)
{
    __GLXAPIState *apiState;
    __GLXvendorInfo *oldVendor, *newVendor;
    Bool ret;
    int screen;

    DBG_PRINTF(0, "dpy = %p, draw = %x, read = %x, context = %p\n",
               dpy, (unsigned)draw, (unsigned)read, context);

    apiState = __glXGetCurrentAPIState();
    oldVendor = apiState->currentVendor;
    screen = __glXScreenFromContext(context);
    newVendor = __glXLookupVendorByScreen(dpy, screen);

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
        if (draw != None || read != None) {
            return False;
        }

        __glDispatchLoseCurrent();

        /* Update the current display and drawable(s) in this apiState */
        apiState->currentDisplay = dpy;
        apiState->currentDraw = draw;
        apiState->currentRead = read;
        apiState->currentVendor = NULL;

        /* Update the GLX dispatch table */
        apiState->currentStaticDispatch = NULL;
        apiState->currentDynDispatch = NULL;

        /* Update the GL dispatch table */
        apiState->glas.dispatch = NULL;

        /* Update the current context */
        apiState->glas.context = NULL;
        return True;
    } else {
        /* Update the current display and drawable(s) in this apiState */
        apiState->currentDisplay = dpy;
        apiState->currentDraw = draw;
        apiState->currentRead = read;
        apiState->currentVendor = newVendor;

        /* Update the GLX dispatch table */
        apiState->currentStaticDispatch = *ppDispatch;
        apiState->currentDynDispatch = newVendor->dynDispatch;

        /* Update the GL dispatch table */
        apiState->glas.dispatch = newVendor->glDispatch;

        DBG_PRINTF(0, "GL dispatch = %p\n", apiState->glas.dispatch);

        /* Update the current context */
        apiState->glas.context = context;

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
         * Call into GLdispatch to set up the current state.
         */
        __glDispatchMakeCurrent(&apiState->glas);
    }

    return True;
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
    const __GLXdispatchTableStatic *pDispatch;
    Bool ret;
    GLXDrawable oldDraw, oldRead;
    GLXContext oldContext;

    SaveCurrentValues(&oldDraw, &oldRead, &oldContext);

    ret = MakeContextCurrentInternal(dpy,
                                     drawable,
                                     drawable,
                                     context,
                                     &pDispatch);
    if (ret) {
        assert(!context || pDispatch);
        if (pDispatch) {
            ret = pDispatch->glx14ep.makeCurrent(dpy, drawable, context);
            if (!ret) {
                // Restore the original current values
                ret = MakeContextCurrentInternal(dpy,
                                                 oldDraw,
                                                 oldRead,
                                                 oldContext,
                                                 &pDispatch);
                assert(ret);
                if (pDispatch) {
                    ret = pDispatch->glx14ep.makeContextCurrent(dpy,
                                                                oldDraw,
                                                                oldRead,
                                                                oldContext);
                    assert(ret);
                }
            }
        }
    }

    return ret;
}


PUBLIC Bool glXQueryExtension(Display *dpy, int *error_base, int *event_base)
{
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
    const int screen = __glXScreenFromDrawable(dpy, drawable);
    const __GLXdispatchTableStatic *pDispatch = __glXGetStaticDispatch(dpy, screen);

    pDispatch->glx14ep.swapBuffers(dpy, drawable);
}


PUBLIC void glXUseXFont(Font font, int first, int count, int list_base)
{
    const __GLXdispatchTableStatic *pDispatch = __glXGetCurrentDispatch();

    pDispatch->glx14ep.useXFont(font, first, count, list_base);
}


PUBLIC void glXWaitGL(void)
{
    const __GLXdispatchTableStatic *pDispatch = __glXGetCurrentDispatch();

    pDispatch->glx14ep.waitGL();
}


PUBLIC void glXWaitX(void)
{
    const __GLXdispatchTableStatic *pDispatch = __glXGetCurrentDispatch();

    pDispatch->glx14ep.waitX();
}

#define GLX_CLIENT_STRING_LAST_ATTRIB GLX_EXTENSIONS
#define CLIENT_STRING_BUFFER_SIZE 256

PUBLIC const char *glXGetClientString(Display *dpy, int name)
{
    int num_screens = XScreenCount(dpy);
    int screen;
    size_t n = CLIENT_STRING_BUFFER_SIZE - 1;
    int index = name - 1;

    glvnd_mutex_t clientStringLock = GLVND_MUTEX_INITIALIZER;
    static struct {
        int initialized;
        char string[CLIENT_STRING_BUFFER_SIZE];
    } clientStringData[GLX_CLIENT_STRING_LAST_ATTRIB];

    __glXPthreadFuncs.mutex_lock(&clientStringLock);

    if (clientStringData[index].initialized) {
        __glXPthreadFuncs.mutex_unlock(&clientStringLock);
        return clientStringData[index].string;
    }

    for (screen = 0; (screen < num_screens) && (n > 0); screen++) {
        const __GLXdispatchTableStatic *pDispatch = __glXGetStaticDispatch(dpy, screen);

        const char *screenClientString = pDispatch->glx14ep.getClientString(dpy,
                                                                            name);
        if (!screenClientString) {
            // Error!
            return NULL;
        }
        strncat(clientStringData[index].string, screenClientString, n);
        n = CLIENT_STRING_BUFFER_SIZE -
            (1 + strlen(clientStringData[index].string));
    }

    clientStringData[index].initialized = 1;

    __glXPthreadFuncs.mutex_unlock(&clientStringLock);
    return clientStringData[index].string;
}


PUBLIC const char *glXQueryServerString(Display *dpy, int screen, int name)
{
    const __GLXdispatchTableStatic *pDispatch = __glXGetStaticDispatch(dpy, screen);

    return pDispatch->glx14ep.queryServerString(dpy, screen, name);
}


PUBLIC const char *glXQueryExtensionsString(Display *dpy, int screen)
{
    const __GLXdispatchTableStatic *pDispatch = __glXGetStaticDispatch(dpy, screen);

    return pDispatch->glx14ep.queryExtensionsString(dpy, screen);
}


PUBLIC Display *glXGetCurrentDisplay(void)
{
    __GLXAPIState *apiState = __glXGetCurrentAPIState();
    return apiState->currentDisplay;
}


PUBLIC GLXFBConfig *glXChooseFBConfig(Display *dpy, int screen,
                                      const int *attrib_list, int *nelements)
{
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
    const int screen = __glXScreenFromFBConfig(config);
    const __GLXdispatchTableStatic *pDispatch = __glXGetStaticDispatch(dpy, screen);

    GLXPbuffer pbuffer = pDispatch->glx14ep.createPbuffer(dpy, config, attrib_list);

    __glXAddScreenDrawableMapping(pbuffer, screen);

    return pbuffer;
}


PUBLIC GLXPixmap glXCreatePixmap(Display *dpy, GLXFBConfig config,
                          Pixmap pixmap, const int *attrib_list)
{
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
    const int screen = __glXScreenFromFBConfig(config);
    const __GLXdispatchTableStatic *pDispatch = __glXGetStaticDispatch(dpy, screen);

    GLXWindow glxWindow =
        pDispatch->glx14ep.createWindow(dpy, config, win, attrib_list);

    __glXAddScreenDrawableMapping(glxWindow, screen);

    return glxWindow;
}


PUBLIC void glXDestroyPbuffer(Display *dpy, GLXPbuffer pbuf)
{
    const int screen = __glXScreenFromDrawable(dpy, pbuf);
    const __GLXdispatchTableStatic *pDispatch = __glXGetStaticDispatch(dpy, screen);

    __glXRemoveScreenDrawableMapping(pbuf, screen);

    pDispatch->glx14ep.destroyPbuffer(dpy, pbuf);
}


PUBLIC void glXDestroyPixmap(Display *dpy, GLXPixmap pixmap)
{
    const int screen = __glXScreenFromDrawable(dpy, pixmap);
    const __GLXdispatchTableStatic *pDispatch = __glXGetStaticDispatch(dpy, screen);

    __glXRemoveScreenDrawableMapping(pixmap, screen);

    pDispatch->glx14ep.destroyPixmap(dpy, pixmap);
}


PUBLIC void glXDestroyWindow(Display *dpy, GLXWindow win)
{
    const int screen = __glXScreenFromDrawable(dpy, win);
    const __GLXdispatchTableStatic *pDispatch = __glXGetStaticDispatch(dpy, screen);

    __glXRemoveScreenDrawableMapping(win, screen);

    pDispatch->glx14ep.destroyWindow(dpy, win);
}


PUBLIC GLXDrawable glXGetCurrentReadDrawable(void)
{
    __GLXAPIState *apiState = __glXGetCurrentAPIState();
    return apiState->currentRead;
}


PUBLIC int glXGetFBConfigAttrib(Display *dpy, GLXFBConfig config,
                         int attribute, int *value)
{
    const int screen = __glXScreenFromFBConfig(config);
    const __GLXdispatchTableStatic *pDispatch = __glXGetStaticDispatch(dpy, screen);

    return pDispatch->glx14ep.getFBConfigAttrib(dpy, config, attribute, value);
}


PUBLIC GLXFBConfig *glXGetFBConfigs(Display *dpy, int screen, int *nelements)
{
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
    const int screen = __glXScreenFromDrawable(dpy, draw);
    const __GLXdispatchTableStatic *pDispatch = __glXGetStaticDispatch(dpy, screen);

    pDispatch->glx14ep.getSelectedEvent(dpy, draw, event_mask);
}


PUBLIC XVisualInfo *glXGetVisualFromFBConfig(Display *dpy, GLXFBConfig config)
{
    const int screen = __glXScreenFromFBConfig(config);
    const __GLXdispatchTableStatic *pDispatch = __glXGetStaticDispatch(dpy, screen);

    return pDispatch->glx14ep.getVisualFromFBConfig(dpy, config);
}

PUBLIC Bool glXMakeContextCurrent(Display *dpy, GLXDrawable draw,
                           GLXDrawable read, GLXContext context)
{
    const __GLXdispatchTableStatic *pDispatch;
    Bool ret;
    GLXDrawable oldDraw, oldRead;
    GLXContext oldContext;

    SaveCurrentValues(&oldDraw, &oldRead, &oldContext);

    ret = MakeContextCurrentInternal(dpy,
                                     draw,
                                     read,
                                     context,
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
                ret = MakeContextCurrentInternal(dpy,
                                                 oldDraw,
                                                 oldRead,
                                                 oldContext,
                                                 &pDispatch);
                assert(ret);
                if (pDispatch) {
                    ret = pDispatch->glx14ep.makeContextCurrent(dpy,
                                                                oldDraw,
                                                                oldRead,
                                                                oldContext);
                    assert(ret);
                }
            }
        }
    }

    return ret;
}


PUBLIC int glXQueryContext(Display *dpy, GLXContext context, int attribute, int *value)
{
    const int screen = __glXScreenFromContext(context);
    const __GLXdispatchTableStatic *pDispatch = __glXGetStaticDispatch(dpy, screen);

    return pDispatch->glx14ep.queryContext(dpy, context, attribute, value);
}


PUBLIC void glXQueryDrawable(Display *dpy, GLXDrawable draw,
                      int attribute, unsigned int *value)
{
    const int screen = __glXScreenFromDrawable(dpy, draw);
    const __GLXdispatchTableStatic *pDispatch = __glXGetStaticDispatch(dpy, screen);

    return pDispatch->glx14ep.queryDrawable(dpy, draw, attribute, value);
}


PUBLIC void glXSelectEvent(Display *dpy, GLXDrawable draw, unsigned long event_mask)
{
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

static __GLXextFuncPtr getCachedProcAddress(const GLubyte *procName)
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

PUBLIC __GLXextFuncPtr glXGetProcAddress(const GLubyte *procName)
{
    __GLXextFuncPtr addr = NULL;

    /*
     * Easy case: First check if we already know this address from
     * a previous GetProcAddress() call or by virtue of being a function
     * exported by libGLX.
     */
    addr = getCachedProcAddress(procName);
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


void __attribute__ ((constructor)) __glXInit(void)
{

    /* Initialize pthreads imports */
    glvndSetupPthreads(RTLD_DEFAULT, &__glXPthreadFuncs);

    /* Initialize GLdispatch */
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

    DBG_PRINTF(0, "Loading GLX...\n");

}

void __attribute__ ((destructor)) __glXFini(void)
{
    // TODO teardown code here
}

__GLXdispatchTableDynamic *__glXGetCurrentDynDispatch(void)
{
    __GLXAPIState *apiState = __glXGetCurrentAPIState();

    return apiState->currentDynDispatch;
}
