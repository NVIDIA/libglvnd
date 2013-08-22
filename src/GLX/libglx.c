/*
 * Copyright (c) 2013, NVIDIA CORPORATION.
 */

#include <X11/Xlib.h>

#include "libglxabi.h"
#include "libglxmapping.h"


XVisualInfo* glXChooseVisual(Display *dpy, int screen, int *attrib_list)
{
    const __GLXdispatchTable *pDispatch = __glXGetDispatch(dpy, screen);

    return pDispatch->chooseVisual(dpy, screen, attrib_list);
}


void glXCopyContext(Display *dpy, GLXContext src, GLXContext dst,
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
    const __GLXdispatchTable *pDispatch = __glXGetDispatch(dpy, screen);

    pDispatch->copyContext(dpy, src, dst, mask);
}


GLXContext glXCreateContext(Display *dpy, XVisualInfo *vis,
                            GLXContext share_list, Bool direct)
{
    const int screen = vis->screen;
    const __GLXdispatchTable *pDispatch = __glXGetDispatch(dpy, screen);

    GLXContext context = pDispatch->createContext(dpy, vis, share_list, direct);

    __glXAddScreenContextMapping(context, screen);

    return context;
}


void glXDestroyContext(Display *dpy, GLXContext context)
{
    const int screen = __glXScreenFromContext(context);
    const __GLXdispatchTable *pDispatch = __glXGetDispatch(dpy, screen);

    __glXRemoveScreenContextMapping(context, screen);

    pDispatch->destroyContext(dpy, context);
}


GLXPixmap glXCreateGLXPixmap(Display *dpy, XVisualInfo *vis, Pixmap pixmap)
{
    const int screen = vis->screen;
    const __GLXdispatchTable *pDispatch = __glXGetDispatch(dpy, screen);

    GLXPixmap pmap = pDispatch->createGLXPixmap(dpy, vis, pixmap);

    __glXAddScreenDrawableMapping(pmap, screen);

    return pmap;
}


void glXDestroyGLXPixmap(Display *dpy, GLXPixmap pix)
{
    const int screen = __glXScreenFromDrawable(pix);
    const __GLXdispatchTable *pDispatch = __glXGetDispatch(dpy, screen);

    __glXRemoveScreenDrawableMapping(pix, screen);

    pDispatch->destroyGLXPixmap(dpy, pix);
}


int glXGetConfig(Display *dpy, XVisualInfo *vis, int attrib, int *value)
{
    const __GLXdispatchTable *pDispatch = __glXGetDispatch(dpy, vis->screen);

    return pDispatch->getConfig(dpy, vis, attrib, value);
}


GLXContext glXGetCurrentContext(void)
{
    /* XXX return the current context from TLS */
    return 0;
}


GLXDrawable glXGetCurrentDrawable(void)
{
    /* XXX return the current drawable from TLS */
    return 0;
}


Bool glXIsDirect(Display *dpy, GLXContext context)
{
    const int screen = __glXScreenFromContext(context);
    const __GLXdispatchTable *pDispatch = __glXGetDispatch(dpy, screen);

    return pDispatch->isDirect(dpy, context);
}


Bool glXMakeCurrent(Display *dpy, GLXDrawable drawable, GLXContext context)
{
    const int screen = __glXScreenFromContext(context);
    const __GLXdispatchTable *pDispatch = __glXGetDispatch(dpy, screen);

    Bool ret = pDispatch->makeCurrent(dpy, drawable, context);

    if (ret) {
        /*
         * XXX record in TLS that drawable is current
         * XXX record in TLS that context is current
         * XXX record in TLS that dpy is current
         * XXX record in TLS the vendor, to be able to can dispatch based on TLS
         *
         * XXX It is possible that this drawable was never seen by
         * this libGLX.so instance before now.  Since the screen is
         * known from the context, and the drawable must be on the
         * same screen if MakeCurrent passed, then record the mapping
         * of this drawable to the context's screen.
         */
        __glXAddScreenDrawableMapping(drawable, screen);
    }

    return ret;
}


Bool glXQueryExtension(Display *dpy, int *error_base, int *event_base)
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


Bool glXQueryVersion(Display *dpy, int *major, int *minor)
{
    /*
     * There isn't enough information to dispatch to a vendor's
     * implementation, so handle the request here.
     */

    /* XXX pack protocol for QueryVersion */

    return False;
}


void glXSwapBuffers(Display *dpy, GLXDrawable drawable)
{
    const int screen = __glXScreenFromDrawable(drawable);
    const __GLXdispatchTable *pDispatch = __glXGetDispatch(dpy, screen);

    pDispatch->swapBuffers(dpy, drawable);
}


void glXUseXFont(Font font, int first, int count, int list_base)
{
    const __GLXdispatchTable *pDispatch = __glXGetDispatchFromTLS();

    pDispatch->useXFont(font, first, count, list_base);
}


void glXWaitGL(void)
{
    const __GLXdispatchTable *pDispatch = __glXGetDispatchFromTLS();

    pDispatch->waitGL();
}


void glXWaitX(void)
{
    const __GLXdispatchTable *pDispatch = __glXGetDispatchFromTLS();

    pDispatch->waitX();
}


const char *glXGetClientString(Display *dpy, int name)
{
    /*
     * XXX do the following:
     * Enumerate all of the X screens on the X server.
     * Query the vendor for each X screen.
     * Get the client string for each vendor.
     * Take the union of client strings across all vendors.
     */

    return NULL;
}


const char *glXQueryServerString(Display *dpy, int screen, int name)
{
    const __GLXdispatchTable *pDispatch = __glXGetDispatch(dpy, screen);

    return pDispatch->queryServerString(dpy, screen, name);
}


const char *glXQueryExtensionsString(Display *dpy, int screen)
{
    const __GLXdispatchTable *pDispatch = __glXGetDispatch(dpy, screen);

    return pDispatch->queryExtensionsString(dpy, screen);
}


Display *glXGetCurrentDisplay(void)
{
    /* XXX return the current Display from TLS */
    return NULL;
}


GLXFBConfig *glXChooseFBConfig(Display *dpy, int screen,
                               const int *attrib_list, int *nelements)
{
    const __GLXdispatchTable *pDispatch = __glXGetDispatch(dpy, screen);
    GLXFBConfig *fbconfigs =
        pDispatch->chooseFBConfig(dpy, screen, attrib_list, nelements);
    int i;

    if (fbconfigs != NULL) {
        for (i = 0; i < *nelements; i++) {
            __glXAddScreenFBConfigMapping(fbconfigs[i], screen);
        }
    }

    return fbconfigs;
}


GLXContext glXCreateNewContext(Display *dpy, GLXFBConfig config,
                               int render_type, GLXContext share_list,
                               Bool direct)
{
    const int screen = __glXScreenFromFBConfig(config);
    const __GLXdispatchTable *pDispatch = __glXGetDispatch(dpy, screen);

    GLXContext context = pDispatch->createNewContext(dpy, config, render_type,
                                                     share_list, direct);
    __glXAddScreenContextMapping(context, screen);

    return context;
}


GLXPbuffer glXCreatePbuffer(Display *dpy, GLXFBConfig config,
                            const int *attrib_list)
{
    const int screen = __glXScreenFromFBConfig(config);
    const __GLXdispatchTable *pDispatch = __glXGetDispatch(dpy, screen);

    GLXPbuffer pbuffer = pDispatch->createPbuffer(dpy, config, attrib_list);

    __glXAddScreenDrawableMapping(pbuffer, screen);

    return pbuffer;
}


GLXPixmap glXCreatePixmap(Display *dpy, GLXFBConfig config,
                          Pixmap pixmap, const int *attrib_list)
{
    const int screen = __glXScreenFromFBConfig(config);
    const __GLXdispatchTable *pDispatch = __glXGetDispatch(dpy, screen);

    GLXPixmap glxPixmap =
        pDispatch->createPixmap(dpy, config, pixmap, attrib_list);

    __glXAddScreenDrawableMapping(glxPixmap, screen);

    return glxPixmap;
}


GLXWindow glXCreateWindow(Display *dpy, GLXFBConfig config,
                          Window win, const int *attrib_list)
{
    const int screen = __glXScreenFromFBConfig(config);
    const __GLXdispatchTable *pDispatch = __glXGetDispatch(dpy, screen);

    GLXWindow glxWindow =
        pDispatch->createWindow(dpy, config, win, attrib_list);

    __glXAddScreenDrawableMapping(glxWindow, screen);

    return glxWindow;
}


void glXDestroyPbuffer(Display *dpy, GLXPbuffer pbuf)
{
    const int screen = __glXScreenFromDrawable(pbuf);
    const __GLXdispatchTable *pDispatch = __glXGetDispatch(dpy, screen);

    __glXRemoveScreenDrawableMapping(pbuf, screen);

    pDispatch->destroyPbuffer(dpy, pbuf);
}


void glXDestroyPixmap(Display *dpy, GLXPixmap pixmap)
{
    const int screen = __glXScreenFromDrawable(pixmap);
    const __GLXdispatchTable *pDispatch = __glXGetDispatch(dpy, screen);

    __glXRemoveScreenDrawableMapping(pixmap, screen);

    pDispatch->destroyPixmap(dpy, pixmap);
}


void glXDestroyWindow(Display *dpy, GLXWindow win)
{
    const int screen = __glXScreenFromDrawable(win);
    const __GLXdispatchTable *pDispatch = __glXGetDispatch(dpy, screen);

    __glXRemoveScreenDrawableMapping(win, screen);

    pDispatch->destroyWindow(dpy, win);
}


GLXDrawable glXGetCurrentReadDrawable(void)
{
    /* XXX return read drawable from TLS */
    return None;
}


int glXGetFBConfigAttrib(Display *dpy, GLXFBConfig config,
                         int attribute, int *value)
{
    const int screen = __glXScreenFromFBConfig(config);
    const __GLXdispatchTable *pDispatch = __glXGetDispatch(dpy, screen);

    return pDispatch->getFBConfigAttrib(dpy, config, attribute, value);
}


GLXFBConfig *glXGetFBConfigs(Display *dpy, int screen, int *nelements)
{
    const __GLXdispatchTable *pDispatch = __glXGetDispatch(dpy, screen);
    GLXFBConfig *fbconfigs = pDispatch->getFBConfigs(dpy, screen, nelements);
    int i;

    if (fbconfigs != NULL) {
        for (i = 0; i < *nelements; i++) {
            __glXAddScreenFBConfigMapping(fbconfigs[i], screen);
        }
    }

    return fbconfigs;
}


void glXGetSelectedEvent(Display *dpy, GLXDrawable draw,
                         unsigned long *event_mask)
{
    const int screen = __glXScreenFromDrawable(draw);
    const __GLXdispatchTable *pDispatch = __glXGetDispatch(dpy, screen);

    pDispatch->getSelectedEvent(dpy, draw, event_mask);
}


XVisualInfo *glXGetVisualFromFBConfig(Display *dpy, GLXFBConfig config)
{
    const int screen = __glXScreenFromFBConfig(config);
    const __GLXdispatchTable *pDispatch = __glXGetDispatch(dpy, screen);

    return pDispatch->getVisualFromFBConfig(dpy, config);
}


Bool glXMakeContextCurrent(Display *dpy, GLXDrawable draw,
                           GLXDrawable read, GLXContext context)
{
    const int screen = __glXScreenFromContext(context);
    const __GLXdispatchTable *pDispatch = __glXGetDispatch(dpy, screen);

    Bool ret = pDispatch->makeContextCurrent(dpy, draw, read, context);

    if (ret) {
        /*
         * XXX record in TLS that drawable(s) is current
         * XXX record in TLS that context is current
         * XXX record in TLS that dpy is current
         * XXX record in TLS the vendor, to be able to can dispatch based on TLS
         *
         * XXX It is possible that these drawables were never seen by
         * this libGLX.so instance before now.  Since the screen is
         * known from the context, and the drawable must be on the
         * same screen if MakeCurrent passed, then record the mapping
         * of this drawable to the context's screen.
         */

        __glXAddScreenDrawableMapping(draw, screen);
        __glXAddScreenDrawableMapping(read, screen);
    }

    return ret;
}


int glXQueryContext(Display *dpy, GLXContext context, int attribute, int *value)
{
    const int screen = __glXScreenFromContext(context);
    const __GLXdispatchTable *pDispatch = __glXGetDispatch(dpy, screen);

    return pDispatch->queryContext(dpy, context, attribute, value);
}


void glXQueryDrawable(Display *dpy, GLXDrawable draw,
                      int attribute, unsigned int *value)
{
    const int screen = __glXScreenFromDrawable(draw);
    const __GLXdispatchTable *pDispatch = __glXGetDispatch(dpy, screen);

    return pDispatch->queryDrawable(dpy, draw, attribute, value);
}


void glXSelectEvent(Display *dpy, GLXDrawable draw, unsigned long event_mask)
{
    const int screen = __glXScreenFromDrawable(draw);
    const __GLXdispatchTable *pDispatch = __glXGetDispatch(dpy, screen);

    pDispatch->selectEvent(dpy, draw, event_mask);
}


__GLXextFuncPtr glXGetProcAddress(const GLubyte *procName)
{
    /* XXX implement me */
    return NULL;
}
