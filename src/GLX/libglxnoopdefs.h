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
#include <GL/glx.h>

/*
 * Header with templates for generating noop functions. This is sourced by
 * libglxnoop.c and the libGL filter library code.
 */

#include "libglxabipriv.h"
#include "libglxnoop.h"

GLXNOOP XVisualInfo* NOOP_FUNC(ChooseVisual)(Display *dpy, int screen,
                                          int *attrib_list)
{
    return NULL;
}


GLXNOOP void NOOP_FUNC(CopyContext)(Display *dpy, GLXContext src,
                                 GLXContext dst, unsigned long mask)
{
    return;
}


GLXNOOP GLXContext NOOP_FUNC(CreateContext)(Display *dpy, XVisualInfo *vis,
                                         GLXContext share_list, Bool direct)
{
    return NULL;
}


GLXNOOP GLXPixmap NOOP_FUNC(CreateGLXPixmap)(Display *dpy,
                                          XVisualInfo *vis,
                                          Pixmap pixmap)
{
    return None;
}


GLXNOOP void NOOP_FUNC(DestroyContext)(Display *dpy, GLXContext ctx)
{
    return;
}


GLXNOOP void NOOP_FUNC(DestroyGLXPixmap)(Display *dpy, GLXPixmap pix)
{
    return;
}


GLXNOOP int NOOP_FUNC(GetConfig)(Display *dpy, XVisualInfo *vis,
                              int attrib, int *value)
{
    return -1;
}


GLXNOOP Bool NOOP_FUNC(IsDirect)(Display *dpy, GLXContext ctx)
{
    return False;
}


GLXNOOP Bool NOOP_FUNC(MakeCurrent)(Display *dpy, GLXDrawable drawable,
                                 GLXContext ctx)
{
    return False;
}


GLXNOOP void NOOP_FUNC(SwapBuffers)(Display *dpy, GLXDrawable drawable)
{
    return;
}


GLXNOOP void NOOP_FUNC(UseXFont)(Font font, int first, int count, int list_base)
{
    return;
}


GLXNOOP void NOOP_FUNC(WaitGL)(void)
{
    return;
}


GLXNOOP void NOOP_FUNC(WaitX)(void)
{
    return;
}


GLXNOOP const char* NOOP_FUNC(QueryServerString)(Display *dpy,
                                              int screen, int name)
{
    return NULL;
}

GLXNOOP const char *NOOP_FUNC(GetClientString)(Display *dpy, int name)
{
    return NULL;
}


GLXNOOP const char* NOOP_FUNC(QueryExtensionsString)(Display *dpy, int screen)
{
    return NULL;
}


GLXNOOP GLXFBConfig* NOOP_FUNC(ChooseFBConfig)(Display *dpy, int screen,
                                            const int *attrib_list,
                                            int *nelements)
{
    return NULL;
}


GLXNOOP GLXContext NOOP_FUNC(CreateNewContext)(Display *dpy, GLXFBConfig config,
                                            int render_type,
                                            GLXContext share_list, Bool direct)
{
    return NULL;
}


GLXNOOP GLXPbuffer NOOP_FUNC(CreatePbuffer)(Display *dpy, GLXFBConfig config,
                                         const int *attrib_list)
{
    return None;
}


GLXNOOP GLXPixmap NOOP_FUNC(CreatePixmap)(Display *dpy, GLXFBConfig config,
                                       Pixmap pixmap, const int *attrib_list)
{
    return None;
}


GLXNOOP GLXWindow NOOP_FUNC(CreateWindow)(Display *dpy, GLXFBConfig config,
                                       Window win, const int *attrib_list)
{
    return None;
}


GLXNOOP void NOOP_FUNC(DestroyPbuffer)(Display *dpy, GLXPbuffer pbuf)
{
    return;
}


GLXNOOP void NOOP_FUNC(DestroyPixmap)(Display *dpy, GLXPixmap pixmap)
{
    return;
}


GLXNOOP void NOOP_FUNC(DestroyWindow)(Display *dpy, GLXWindow win)
{
    return;
}


GLXNOOP int NOOP_FUNC(GetFBConfigAttrib)(Display *dpy, GLXFBConfig config,
                                      int attribute, int *value)
{
    return 0;
}


GLXNOOP GLXFBConfig* NOOP_FUNC(GetFBConfigs)(Display *dpy,
                                          int screen, int *nelements)
{
    return NULL;
}


GLXNOOP void NOOP_FUNC(GetSelectedEvent)(Display *dpy, GLXDrawable draw,
                                      unsigned long *event_mask)
{
    return;
}


GLXNOOP XVisualInfo* NOOP_FUNC(GetVisualFromFBConfig)(Display *dpy,
                                                   GLXFBConfig config)
{
    return NULL;
}


GLXNOOP Bool NOOP_FUNC(MakeContextCurrent)(Display *dpy, GLXDrawable draw,
                                        GLXDrawable read, GLXContext ctx)
{
    return False;
}


GLXNOOP int NOOP_FUNC(QueryContext)(Display *dpy, GLXContext ctx,
                                 int attribute, int *value)
{
    return 0;
}


GLXNOOP void NOOP_FUNC(QueryDrawable)(Display *dpy, GLXDrawable draw,
                                   int attribute, unsigned int *value)
{
    return;
}


GLXNOOP void NOOP_FUNC(SelectEvent)(Display *dpy, GLXDrawable draw,
                                 unsigned long event_mask)
{
    return;
}

GLXNOOP GLXContext NOOP_FUNC(GetCurrentContext)(void)
{
    return None;
}

GLXNOOP GLXDrawable NOOP_FUNC(GetCurrentDrawable)(void)
{
    return None;
}

GLXNOOP GLXDrawable NOOP_FUNC(GetCurrentReadDrawable)(void)
{
    return None;
}

GLXNOOP __GLXextFuncPtr NOOP_FUNC(GetProcAddress)(const GLubyte *procName)
{
    return NULL;
}

GLXNOOP __GLXextFuncPtr NOOP_FUNC(GetProcAddressARB)(const GLubyte *procName)
{
    return NULL;
}

GLXNOOP Bool NOOP_FUNC(QueryExtension)(Display *dpy,
                                       int *error_base,
                                       int *event_base)
{
    return False;
}

GLXNOOP Bool NOOP_FUNC(QueryVersion)(Display *dpy,
                                     int *major,
                                     int *minor)
{
    return False;
}
