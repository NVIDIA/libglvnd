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

#ifndef __LIB_GLX_ABI_PRIV__
#define __LIB_GLX_ABI_PRIV__

/*
 * This is a wrapper around libglxabi which defines each vendor's static
 * dispatch table.  Logically this could differ from the API imports provided
 * by the vendor, though in practice they are one and the same.
 */

#include "glvnd/libglxabi.h"

#include <GL/glxext.h>

/*!
 * This structure stores function pointers for all functions defined in GLX 1.4.
 */
typedef struct __GLXdispatchTableStaticRec {
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

    const char*  (*getClientString)     (Display *dpy,
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

    PFNGLXIMPORTCONTEXTEXTPROC importContextEXT;
    PFNGLXFREECONTEXTEXTPROC freeContextEXT;
    PFNGLXCREATECONTEXTATTRIBSARBPROC createContextAttribsARB;
} __GLXdispatchTableStatic;

#endif
