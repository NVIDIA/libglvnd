/*
 * Copyright (c) 2013, NVIDIA CORPORATION.
 */

#if !defined(__LIB_GLX_ABI_H)
#define __LIB_GLX_ABI_H

#include <GL/glx.h>

/*
 * Definition of ABI exported by libGLX.so to libGLX_VENDOR.so
 * libraries.  This structure stores function pointers for all
 * functions defined in GLX 1.4.
 */

typedef struct {

    XVisualInfo* (*chooseVisual)          (Display *dpy,
                                           int screen,
                                           int *attrib_list);

    void         (*copyContext)           (Display *dpy,
                                           GLXContext src,
                                           GLXContext dst,
                                           unsigned long mask);

    GLXContext   (*createContext)         (Display *dpy,
                                           XVisualInfo *vis,
                                           GLXContext share_list,
                                           Bool direct);

    GLXPixmap    (*createGLXPixmap)       (Display *dpy,
                                           XVisualInfo *vis,
                                           Pixmap pixmap);

    void         (*destroyContext)        (Display *dpy,
                                           GLXContext ctx);

    void         (*destroyGLXPixmap)      (Display *dpy,
                                           GLXPixmap pix);

    int          (*getConfig)             (Display *dpy,
                                           XVisualInfo *vis,
                                           int attrib,
                                           int *value);

    Bool         (*isDirect)              (Display *dpy,
                                           GLXContext ctx);

    Bool         (*makeCurrent)           (Display *dpy,
                                           GLXDrawable drawable,
                                           GLXContext ctx);

    void         (*swapBuffers)           (Display *dpy,
                                           GLXDrawable drawable);

    void         (*useXFont)              (Font font,
                                           int first,
                                           int count,
                                           int list_base);

    void         (*waitGL)                (void);

    void         (*waitX)                 (void);

    const char*  (*queryServerString)     (Display *dpy,
                                           int screen,
                                           int name);

    const char*  (*queryExtensionsString) (Display *dpy,
                                           int screen);

    GLXFBConfig* (*chooseFBConfig)        (Display *dpy,
                                           int screen,
                                           const int *attrib_list,
                                           int *nelements);

    GLXContext   (*createNewContext)      (Display *dpy,
                                           GLXFBConfig config,
                                           int render_type,
                                           GLXContext share_list,
                                           Bool direct);

    GLXPbuffer   (*createPbuffer)         (Display *dpy,
                                           GLXFBConfig config,
                                           const int *attrib_list);

    GLXPixmap    (*createPixmap)          (Display *dpy,
                                           GLXFBConfig config,
                                           Pixmap pixmap,
                                           const int *attrib_list);

    GLXWindow    (*createWindow)          (Display *dpy,
                                           GLXFBConfig config,
                                           Window win,
                                           const int *attrib_list);

    void         (*destroyPbuffer)        (Display *dpy,
                                           GLXPbuffer pbuf);

    void         (*destroyPixmap)         (Display *dpy,
                                           GLXPixmap pixmap);

    void         (*destroyWindow)         (Display *dpy,
                                           GLXWindow win);

    int          (*getFBConfigAttrib)     (Display *dpy,
                                           GLXFBConfig config,
                                           int attribute,
                                           int *value);

    GLXFBConfig* (*getFBConfigs)          (Display *dpy,
                                           int screen,
                                           int *nelements);

    void         (*getSelectedEvent)      (Display *dpy,
                                           GLXDrawable draw,
                                           unsigned long *event_mask);

    XVisualInfo* (*getVisualFromFBConfig) (Display *dpy,
                                           GLXFBConfig config);

    Bool         (*makeContextCurrent)    (Display *dpy, GLXDrawable draw,
                                           GLXDrawable read, GLXContext ctx);

    int          (*queryContext)          (Display *dpy,
                                           GLXContext ctx,
                                           int attribute,
                                           int *value);

    void         (*queryDrawable)         (Display *dpy,
                                           GLXDrawable draw,
                                           int attribute,
                                           unsigned int *value);

    void         (*selectEvent)           (Display *dpy,
                                           GLXDrawable draw,
                                           unsigned long event_mask);

} __GLXdispatchTable;

#endif /* __LIB_GLX_ABI_H */
