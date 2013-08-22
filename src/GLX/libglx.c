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

#include "compiler.h"
#include "libglxabi.h"
#include "GL/glxproto.h"

/* current version numbers */
#define GLX_MAJOR_VERSION 1
#define GLX_MINOR_VERSION 4

#define GLX_EXTENSION_NAME "GLX"

typedef __GLXapiImports __GLXdispatchTableStatic;

static __GLXdispatchTableStatic *GetStaticDispatch(void)
{
    // XXX this is a placeholder for a real strategy to get
    // the dispatch table
    return NULL;
}

PUBLIC XVisualInfo* glXChooseVisual(Display *dpy, int screen, int *attrib_list)
{
    const __GLXdispatchTableStatic *pDispatch = GetStaticDispatch();

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
    const __GLXdispatchTableStatic *pDispatch = GetStaticDispatch();

    pDispatch->glx14ep.copyContext(dpy, src, dst, mask);
}


PUBLIC GLXContext glXCreateContext(Display *dpy, XVisualInfo *vis,
                            GLXContext share_list, Bool direct)
{
    const __GLXdispatchTableStatic *pDispatch = GetStaticDispatch();

    GLXContext context = pDispatch->glx14ep.createContext(dpy, vis, share_list, direct);

    /* TODO: Add a mapping */

    return context;
}


PUBLIC void glXDestroyContext(Display *dpy, GLXContext context)
{
    const __GLXdispatchTableStatic *pDispatch = GetStaticDispatch();

    /* TODO: Remove a mapping */

    pDispatch->glx14ep.destroyContext(dpy, context);
}


PUBLIC GLXPixmap glXCreateGLXPixmap(Display *dpy, XVisualInfo *vis, Pixmap pixmap)
{
    const __GLXdispatchTableStatic *pDispatch = GetStaticDispatch();

    GLXPixmap pmap = pDispatch->glx14ep.createGLXPixmap(dpy, vis, pixmap);

    /* TODO: Add a mapping */

    return pmap;
}


PUBLIC void glXDestroyGLXPixmap(Display *dpy, GLXPixmap pix)
{
    const __GLXdispatchTableStatic *pDispatch = GetStaticDispatch();

    /* TODO: Remove a mapping */

    pDispatch->glx14ep.destroyGLXPixmap(dpy, pix);
}


PUBLIC int glXGetConfig(Display *dpy, XVisualInfo *vis, int attrib, int *value)
{
    const __GLXdispatchTableStatic *pDispatch = GetStaticDispatch();

    return pDispatch->glx14ep.getConfig(dpy, vis, attrib, value);
}

PUBLIC GLXContext glXGetCurrentContext(void)
{
    return NULL; // TODO
}


PUBLIC GLXDrawable glXGetCurrentDrawable(void)
{
    return None; // TODO
}


PUBLIC Bool glXIsDirect(Display *dpy, GLXContext context)
{
    const __GLXdispatchTableStatic *pDispatch = GetStaticDispatch();

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
    const __GLXdispatchTableStatic *pDispatch = GetStaticDispatch();

    pDispatch->glx14ep.swapBuffers(dpy, drawable);
}


PUBLIC void glXUseXFont(Font font, int first, int count, int list_base)
{
    const __GLXdispatchTableStatic *pDispatch = GetStaticDispatch();

    pDispatch->glx14ep.useXFont(font, first, count, list_base);
}


PUBLIC void glXWaitGL(void)
{
    const __GLXdispatchTableStatic *pDispatch = GetStaticDispatch();

    pDispatch->glx14ep.waitGL();
}


PUBLIC void glXWaitX(void)
{
    const __GLXdispatchTableStatic *pDispatch = GetStaticDispatch();

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
    const __GLXdispatchTableStatic *pDispatch = GetStaticDispatch();

    return pDispatch->glx14ep.queryServerString(dpy, screen, name);
}


PUBLIC const char *glXQueryExtensionsString(Display *dpy, int screen)
{
    const __GLXdispatchTableStatic *pDispatch = GetStaticDispatch();

    return pDispatch->glx14ep.queryExtensionsString(dpy, screen);
}


PUBLIC Display *glXGetCurrentDisplay(void)
{
    return NULL;
}


PUBLIC GLXFBConfig *glXChooseFBConfig(Display *dpy, int screen,
                                      const int *attrib_list, int *nelements)
{
    const __GLXdispatchTableStatic *pDispatch = GetStaticDispatch();
    GLXFBConfig *fbconfigs =
        pDispatch->glx14ep.chooseFBConfig(dpy, screen, attrib_list, nelements);

    /* TODO: add mappings for each fbconfig */

    return fbconfigs;
}


PUBLIC GLXContext glXCreateNewContext(Display *dpy, GLXFBConfig config,
                               int render_type, GLXContext share_list,
                               Bool direct)
{
    const __GLXdispatchTableStatic *pDispatch = GetStaticDispatch();

    GLXContext context = pDispatch->glx14ep.createNewContext(dpy, config, render_type,
                                                     share_list, direct);
    /* TODO: Add a mapping */

    return context;
}


PUBLIC GLXPbuffer glXCreatePbuffer(Display *dpy, GLXFBConfig config,
                            const int *attrib_list)
{
    const __GLXdispatchTableStatic *pDispatch = GetStaticDispatch();

    GLXPbuffer pbuffer = pDispatch->glx14ep.createPbuffer(dpy, config, attrib_list);

    /* TODO: Add a mapping */

    return pbuffer;
}


PUBLIC GLXPixmap glXCreatePixmap(Display *dpy, GLXFBConfig config,
                          Pixmap pixmap, const int *attrib_list)
{
    const __GLXdispatchTableStatic *pDispatch = GetStaticDispatch();

    GLXPixmap glxPixmap =
        pDispatch->glx14ep.createPixmap(dpy, config, pixmap, attrib_list);

    /* TODO: Add a mapping */

    return glxPixmap;
}


PUBLIC GLXWindow glXCreateWindow(Display *dpy, GLXFBConfig config,
                          Window win, const int *attrib_list)
{
    const __GLXdispatchTableStatic *pDispatch = GetStaticDispatch();

    GLXWindow glxWindow =
        pDispatch->glx14ep.createWindow(dpy, config, win, attrib_list);

    /* TODO: Add a mapping */

    return glxWindow;
}


PUBLIC void glXDestroyPbuffer(Display *dpy, GLXPbuffer pbuf)
{
    const __GLXdispatchTableStatic *pDispatch = GetStaticDispatch();

    /* TODO: Remove a mapping */

    pDispatch->glx14ep.destroyPbuffer(dpy, pbuf);
}


PUBLIC void glXDestroyPixmap(Display *dpy, GLXPixmap pixmap)
{
    const __GLXdispatchTableStatic *pDispatch = GetStaticDispatch();

    /* TODO: Remove a mapping */

    pDispatch->glx14ep.destroyPixmap(dpy, pixmap);
}


PUBLIC void glXDestroyWindow(Display *dpy, GLXWindow win)
{
    const __GLXdispatchTableStatic *pDispatch = GetStaticDispatch();

    /* TODO: Remove a mapping */

    pDispatch->glx14ep.destroyWindow(dpy, win);
}


PUBLIC GLXDrawable glXGetCurrentReadDrawable(void)
{
    return None; // TODO
}


PUBLIC int glXGetFBConfigAttrib(Display *dpy, GLXFBConfig config,
                         int attribute, int *value)
{
    const __GLXdispatchTableStatic *pDispatch = GetStaticDispatch();

    return pDispatch->glx14ep.getFBConfigAttrib(dpy, config, attribute, value);
}


PUBLIC GLXFBConfig *glXGetFBConfigs(Display *dpy, int screen, int *nelements)
{
    const __GLXdispatchTableStatic *pDispatch = GetStaticDispatch();
    GLXFBConfig *fbconfigs = pDispatch->glx14ep.getFBConfigs(dpy, screen, nelements);

    /* TODO: add mappings for each FBconfig */

    return fbconfigs;
}


PUBLIC void glXGetSelectedEvent(Display *dpy, GLXDrawable draw,
                         unsigned long *event_mask)
{
    const __GLXdispatchTableStatic *pDispatch = GetStaticDispatch();

    pDispatch->glx14ep.getSelectedEvent(dpy, draw, event_mask);
}


PUBLIC XVisualInfo *glXGetVisualFromFBConfig(Display *dpy, GLXFBConfig config)
{
    const __GLXdispatchTableStatic *pDispatch = GetStaticDispatch();

    return pDispatch->glx14ep.getVisualFromFBConfig(dpy, config);
}

PUBLIC Bool glXMakeContextCurrent(Display *dpy, GLXDrawable draw,
                           GLXDrawable read, GLXContext context)
{
    return False;
}


PUBLIC int glXQueryContext(Display *dpy, GLXContext context, int attribute, int *value)
{
    const __GLXdispatchTableStatic *pDispatch = GetStaticDispatch();

    return pDispatch->glx14ep.queryContext(dpy, context, attribute, value);
}


PUBLIC void glXQueryDrawable(Display *dpy, GLXDrawable draw,
                      int attribute, unsigned int *value)
{
    const __GLXdispatchTableStatic *pDispatch = GetStaticDispatch();

    return pDispatch->glx14ep.queryDrawable(dpy, draw, attribute, value);
}


PUBLIC void glXSelectEvent(Display *dpy, GLXDrawable draw, unsigned long event_mask)
{
    const __GLXdispatchTableStatic *pDispatch = GetStaticDispatch();

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
     * TODO: If that doesn't work, try requesting a dispatch function
     * from one of the loaded vendor libraries.
     */

    /*
     * TODO: If *that* doesn't work, request a NOP stub from the core
     * dispatcher.
     */

    /* Store the resulting proc address. */
    if (addr) {
        cacheProcAddress(procName, addr);
    }

    return addr;
}


void __attribute__ ((constructor)) __glXInit(void)
{

    /* TODO Initialize pthreads imports */

    /*
     * TODO Check if we need to pre-load any vendors specified via environment
     * variable.
     */
}

void __attribute__ ((destructor)) __glXFini(void)
{
    // TODO teardown code here
}
