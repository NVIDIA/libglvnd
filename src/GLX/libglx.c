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

#include "libglxabipriv.h"
#include "libglxmapping.h"
#include "libglxcurrent.h"
#include "utils_misc.h"
#include "GL/glxproto.h"

/* current version numbers */
#define GLX_MAJOR_VERSION 1
#define GLX_MINOR_VERSION 4

#define GLX_EXTENSION_NAME "GLX"

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

PUBLIC Bool glXMakeCurrent(Display *dpy, GLXDrawable drawable, GLXContext context)
{
    return False;
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
    return False; // TODO
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
    return NULL;
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
    return False;
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

static __GLXextFuncPtr getCachedProcAddress(const GLubyte *procName)
{
    /*
     * TODO
     *
     * If this is the first time GetProcAddress has been called,
     * initialize a procname -> address hash table with locally-exported
     * functions.
     *
     * Then, try to look up the function address in the hash table.
     */
    return NULL;
}

static void cacheProcAddress(const GLubyte *procName, __GLXextFuncPtr addr)
{
    /* TODO: save the mapping in the local hash table */
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
     * TODO: If *that* doesn't work, request a NOP stub from the core
     * dispatcher.
     */

    /* Store the resulting proc address. */
done:
    if (addr) {
        cacheProcAddress(procName, addr);
    }

    return addr;
}


void __attribute__ ((constructor)) __glXInit(void)
{

    /* TODO Initialize pthreads imports */

    /*
     * Check if we need to pre-load any vendors specified via environment
     * variable.
     */
    const char *preloadedVendor = getenv("__GLX_VENDOR_LIBRARY_NAME");

    if (preloadedVendor) {
        __glXLookupVendorByName(preloadedVendor);
    }


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
