#include <X11/Xlib.h>
#include <GL/glx.h>

#include "libglxabi.h"
#include "libglxnoop.h"

static XVisualInfo* __glXChooseVisualNoop(Display *dpy, int screen,
                                          int *attrib_list)
{
    return NULL;
}


static void __glXCopyContextNoop(Display *dpy, GLXContext src,
                                 GLXContext dst, unsigned long mask)
{
    return;
}


static GLXContext __glXCreateContextNoop(Display *dpy, XVisualInfo *vis,
                                         GLXContext share_list, Bool direct)
{
    return NULL;
}


static GLXPixmap __glXCreateGLXPixmapNoop(Display *dpy,
                                          XVisualInfo *vis,
                                          Pixmap pixmap)
{
    return None;
}


static void __glXDestroyContextNoop(Display *dpy, GLXContext ctx)
{
    return;
}


static void __glXDestroyGLXPixmapNoop(Display *dpy, GLXPixmap pix)
{
    return;
}


static int __glXGetConfigNoop(Display *dpy, XVisualInfo *vis,
                              int attrib, int *value)
{
    return -1;
}


static Bool __glXIsDirectNoop(Display *dpy, GLXContext ctx)
{
    return False;
}


static Bool __glXMakeCurrentNoop(Display *dpy, GLXDrawable drawable,
                                 GLXContext ctx)
{
    return False;
}


static void __glXSwapBuffersNoop(Display *dpy, GLXDrawable drawable)
{
    return;
}


static void __glXUseXFontNoop(Font font, int first, int count, int list_base)
{
    return;
}


static void __glXWaitGLNoop(void)
{
    return;
}


static void __glXWaitXNoop(void)
{
    return;
}


static const char* __glXQueryServerStringNoop(Display *dpy,
                                              int screen, int name)
{
    return NULL;
}


static const char* __glXQueryExtensionsStringNoop(Display *dpy, int screen)
{
    return NULL;
}


static GLXFBConfig* __glXChooseFBConfigNoop(Display *dpy, int screen,
                                            const int *attrib_list,
                                            int *nelements)
{
    return NULL;
}


static GLXContext __glXCreateNewContextNoop(Display *dpy, GLXFBConfig config,
                                            int render_type,
                                            GLXContext share_list, Bool direct)
{
    return NULL;
}


static GLXPbuffer __glXCreatePbufferNoop(Display *dpy, GLXFBConfig config,
                                         const int *attrib_list)
{
    return None;
}


static GLXPixmap __glXCreatePixmapNoop(Display *dpy, GLXFBConfig config,
                                       Pixmap pixmap, const int *attrib_list)
{
    return None;
}


static GLXWindow __glXCreateWindowNoop(Display *dpy, GLXFBConfig config,
                                       Window win, const int *attrib_list)
{
    return None;
}


static void __glXDestroyPbufferNoop(Display *dpy, GLXPbuffer pbuf)
{
    return;
}


static void __glXDestroyPixmapNoop(Display *dpy, GLXPixmap pixmap)
{
    return;
}


static void __glXDestroyWindowNoop(Display *dpy, GLXWindow win)
{
    return;
}


static int __glXGetFBConfigAttribNoop(Display *dpy, GLXFBConfig config,
                                      int attribute, int *value)
{
    return 0;
}


static GLXFBConfig* __glXGetFBConfigsNoop(Display *dpy,
                                          int screen, int *nelements)
{
    return NULL;
}


static void __glXGetSelectedEventNoop(Display *dpy, GLXDrawable draw,
                                      unsigned long *event_mask)
{
    return;
}


static XVisualInfo* __glXGetVisualFromFBConfigNoop(Display *dpy,
                                                   GLXFBConfig config)
{
    return NULL;
}


static Bool __glXMakeContextCurrentNoop(Display *dpy, GLXDrawable draw,
                                        GLXDrawable read, GLXContext ctx)
{
    return False;
}


static int __glXQueryContextNoop(Display *dpy, GLXContext ctx,
                                 int attribute, int *value)
{
    return 0;
}


static void __glXQueryDrawableNoop(Display *dpy, GLXDrawable draw,
                                   int attribute, unsigned int *value)
{
    return;
}


static void __glXSelectEventNoop(Display *dpy, GLXDrawable draw,
                                 unsigned long event_mask)
{
    return;
}


const __GLXdispatchTable __glXDispatchNoop = {
    .chooseVisual          = __glXChooseVisualNoop,
    .copyContext           = __glXCopyContextNoop,
    .createContext         = __glXCreateContextNoop,
    .createGLXPixmap       = __glXCreateGLXPixmapNoop,
    .destroyContext        = __glXDestroyContextNoop,
    .destroyGLXPixmap      = __glXDestroyGLXPixmapNoop,
    .getConfig             = __glXGetConfigNoop,
    .isDirect              = __glXIsDirectNoop,
    .makeCurrent           = __glXMakeCurrentNoop,
    .swapBuffers           = __glXSwapBuffersNoop,
    .useXFont              = __glXUseXFontNoop,
    .waitGL                = __glXWaitGLNoop,
    .waitX                 = __glXWaitXNoop,
    .queryServerString     = __glXQueryServerStringNoop,
    .queryExtensionsString = __glXQueryExtensionsStringNoop,
    .chooseFBConfig        = __glXChooseFBConfigNoop,
    .createNewContext      = __glXCreateNewContextNoop,
    .createPbuffer         = __glXCreatePbufferNoop,
    .createPixmap          = __glXCreatePixmapNoop,
    .createWindow          = __glXCreateWindowNoop,
    .destroyPbuffer        = __glXDestroyPbufferNoop,
    .destroyPixmap         = __glXDestroyPixmapNoop,
    .destroyWindow         = __glXDestroyWindowNoop,
    .getFBConfigAttrib     = __glXGetFBConfigAttribNoop,
    .getFBConfigs          = __glXGetFBConfigsNoop,
    .getSelectedEvent      = __glXGetSelectedEventNoop,
    .getVisualFromFBConfig = __glXGetVisualFromFBConfigNoop,
    .makeContextCurrent    = __glXMakeContextCurrentNoop,
    .queryContext          = __glXQueryContextNoop,
    .queryDrawable         = __glXQueryDrawableNoop,
    .selectEvent           = __glXSelectEventNoop
};


const __GLXdispatchTable *__glXDispatchNoopPtr = &__glXDispatchNoop;
