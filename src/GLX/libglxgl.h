#if !defined(__LIB_GLX_GL_H)
#define __LIB_GLX_GL_H

#include <GL/glx.h>
#include "glvnd_pthread.h"

/*
 * Glue header defining the ABI between libGLX and the libGL wrapper library.
 */

typedef struct {
    GLXFBConfig * (* ptr_glXChooseFBConfig) (Display *dpy, int screen, const int *attrib_list, int *nelements);
    XVisualInfo * (* ptr_glXChooseVisual) (Display *dpy, int screen, int *attribList);
    void (* ptr_glXCopyContext) (Display *dpy, GLXContext src, GLXContext dst, unsigned long mask);
    GLXContext (* ptr_glXCreateContext) (Display *dpy, XVisualInfo *vis, GLXContext shareList, Bool direct);
    GLXPixmap (* ptr_glXCreateGLXPixmap) (Display *dpy, XVisualInfo *visual, Pixmap pixmap);
    GLXContext (* ptr_glXCreateNewContext) (Display *dpy, GLXFBConfig config, int render_type, GLXContext share_list, Bool direct);
    GLXPbuffer (* ptr_glXCreatePbuffer) (Display *dpy, GLXFBConfig config, const int *attrib_list);
    GLXPixmap (* ptr_glXCreatePixmap) (Display *dpy, GLXFBConfig config, Pixmap pixmap, const int *attrib_list);
    GLXWindow (* ptr_glXCreateWindow) (Display *dpy, GLXFBConfig config, Window win, const int *attrib_list);
    void (* ptr_glXDestroyContext) (Display *dpy, GLXContext ctx);
    void (* ptr_glXDestroyGLXPixmap) (Display *dpy, GLXPixmap pixmap);
    void (* ptr_glXDestroyPbuffer) (Display *dpy, GLXPbuffer pbuf);
    void (* ptr_glXDestroyPixmap) (Display *dpy, GLXPixmap pixmap);
    void (* ptr_glXDestroyWindow) (Display *dpy, GLXWindow win);
    const char * (* ptr_glXGetClientString) (Display *dpy, int name);
    int (* ptr_glXGetConfig) (Display *dpy, XVisualInfo *visual, int attrib, int *value);
    GLXContext (* ptr_glXGetCurrentContext) (void);
    GLXDrawable (* ptr_glXGetCurrentDrawable) (void);
    GLXDrawable (* ptr_glXGetCurrentReadDrawable) (void);
    int (* ptr_glXGetFBConfigAttrib) (Display *dpy, GLXFBConfig config, int attribute, int *value);
    GLXFBConfig * (* ptr_glXGetFBConfigs) (Display *dpy, int screen, int *nelements);
    __GLXextFuncPtr (* ptr_glXGetProcAddress) (const GLubyte *procName);
    __GLXextFuncPtr (* ptr_glXGetProcAddressARB) (const GLubyte *procName);
    void (* ptr_glXGetSelectedEvent) (Display *dpy, GLXDrawable draw, unsigned long *event_mask);
    XVisualInfo * (* ptr_glXGetVisualFromFBConfig) (Display *dpy, GLXFBConfig config);
    Bool (* ptr_glXIsDirect) (Display *dpy, GLXContext ctx);
    Bool (* ptr_glXMakeContextCurrent) (Display *dpy, GLXDrawable draw, GLXDrawable read, GLXContext ctx);
    Bool (* ptr_glXMakeCurrent) (Display *dpy, GLXDrawable drawable, GLXContext ctx);
    int (* ptr_glXQueryContext) (Display *dpy, GLXContext ctx, int attribute, int *value);
    void (* ptr_glXQueryDrawable) (Display *dpy, GLXDrawable draw, int attribute, unsigned int *value);
    Bool (* ptr_glXQueryExtension) (Display *dpy, int *errorb, int *event);
    const char * (* ptr_glXQueryExtensionsString) (Display *dpy, int screen);
    const char * (* ptr_glXQueryServerString) (Display *dpy, int screen, int name);
    Bool (* ptr_glXQueryVersion) (Display *dpy, int *maj, int *min);
    void (* ptr_glXSelectEvent) (Display *dpy, GLXDrawable draw, unsigned long event_mask);
    void (* ptr_glXSwapBuffers) (Display *dpy, GLXDrawable drawable);
    void (* ptr_glXUseXFont) (Font font, int first, int count, int list);
    void (* ptr_glXWaitGL) (void);
    void (* ptr_glXWaitX) (void);
} __glXGLCoreFunctions;

/**
 * Called from libGL.so to load a GLX function.
 *
 * \p ptr should point to the variable that will hold the function pointer. If
 * the value is already non-NULL, then __glXGLLoadGLXFunction will assume that
 * the function has already been loaded and will just return \c (*ptr).
 *
 * To avoid problems with multiple threads trying to load the same function at
 * the same time, __glXGLLoadGLXFunction will lock \p mutex before it tries to
 * read or write \p ptr.
 *
 * Also see src/generate/gen_libgl_glxstubs.py for where this is used.
 *
 * \param name The name of the function to load.
 * \param[out] ptr A pointer to store the function in.
 * \param ptr A mutex to lock before accessing \p ptr, or NULL.
 * \return A pointer to the requested function.
 */
extern __GLXextFuncPtr __glXGLLoadGLXFunction(const char *name, __GLXextFuncPtr *ptr, glvnd_mutex_t *mutex);

extern const __glXGLCoreFunctions __GLXGL_CORE_FUNCTIONS;

#endif // !defined(__LIB_GLX_GL_H)
