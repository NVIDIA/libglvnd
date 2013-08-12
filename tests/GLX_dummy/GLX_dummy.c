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

#include <config.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <GL/glx.h>
#include <GL/glxint.h>

#include "GLX_dummy.h"
#include "libglxabi.h"
#include "utils_misc.h"
#include "compiler.h"


static char *thisVendorName;
static __GLXapiExports apiExports;

static XVisualInfo*  dummyChooseVisual          (Display *dpy,
                                                 int screen,
                                                 int *attrib_list)
{
    return NULL;
}

static void          dummyCopyContext           (Display *dpy,
                                                 GLXContext src,
                                                 GLXContext dst,
                                                 unsigned long mask)
{
    // nop
}

static GLXContext    dummyCreateContext         (Display *dpy,
                                                 XVisualInfo *vis,
                                                 GLXContext share_list,
                                                 Bool direct)
{
    return NULL;
}

static GLXPixmap     dummyCreateGLXPixmap       (Display *dpy,
                                                 XVisualInfo *vis,
                                                 Pixmap pixmap)
{
    return None;
}

static void          dummyDestroyContext        (Display *dpy,
                                              GLXContext ctx)
{
    free(ctx);
}

static void          dummyDestroyGLXPixmap      (Display *dpy,
                                                 GLXPixmap pix)
{
    // nop
}

static int           dummyGetConfig             (Display *dpy,
                                                 XVisualInfo *vis,
                                                 int attrib,
                                                 int *value)
{
    return 0;
}

static Bool          dummyIsDirect              (Display *dpy,
                                                 GLXContext ctx)
{
    return False;
}

static Bool          dummyMakeCurrent           (Display *dpy,
                                              GLXDrawable drawable,
                                              GLXContext ctx)
{
    // This doesn't do anything, but fakes success
    return True;
}

static void          dummySwapBuffers           (Display *dpy,
                                                 GLXDrawable drawable)
{
    // nop
}

static void          dummyUseXFont              (Font font,
                                                 int first,
                                                 int count,
                                                 int list_base)
{
    // nop
}

static void          dummyWaitGL                (void)
{
    // nop
}

static void          dummyWaitX                 (void)
{
    // nop
}

static const char*   dummyQueryServerString     (Display *dpy,
                                                 int screen,
                                                 int name)
{
    return NULL;
}

static const char*   dummyGetClientString     (Display *dpy,
                                               int name)
{
    return NULL;
}

static const char*   dummyQueryExtensionsString (Display *dpy,
                                                 int screen)
{
    return NULL;
}

static GLXFBConfig*  dummyChooseFBConfig        (Display *dpy,
                                              int screen,
                                              const int *attrib_list,
                                              int *nelements)
{
    return NULL;
}

static GLXContext    dummyCreateNewContext      (Display *dpy,
                                                 GLXFBConfig config,
                                                 int render_type,
                                                 GLXContext share_list,
                                                 Bool direct)
{
    return NULL;
}

static GLXPbuffer    dummyCreatePbuffer         (Display *dpy,
                                                 GLXFBConfig config,
                                                 const int *attrib_list)
{
    return None;
}

static GLXPixmap     dummyCreatePixmap          (Display *dpy,
                                                 GLXFBConfig config,
                                                 Pixmap pixmap,
                                                 const int *attrib_list)
{
    return None;
}

static GLXWindow     dummyCreateWindow          (Display *dpy,
                                                 GLXFBConfig config,
                                                 Window win,
                                                 const int *attrib_list)
{
    return None;
}

static void          dummyDestroyPbuffer        (Display *dpy,
                                                 GLXPbuffer pbuf)
{
    // nop
}

static void          dummyDestroyPixmap         (Display *dpy,
                                                 GLXPixmap pixmap)
{
    // nop
}

static void          dummyDestroyWindow         (Display *dpy,
                                                 GLXWindow win)
{
    // nop
}

static int           dummyGetFBConfigAttrib     (Display *dpy,
                                                 GLXFBConfig config,
                                                 int attribute,
                                                 int *value)
{
    return 0;
}

static GLXFBConfig*  dummyGetFBConfigs          (Display *dpy,
                                                 int screen,
                                                 int *nelements)
{
    return NULL;
}

static void          dummyGetSelectedEvent      (Display *dpy,
                                                 GLXDrawable draw,
                                                 unsigned long *event_mask)
{
    // nop
}

static XVisualInfo*  dummyGetVisualFromFBConfig (Display *dpy,
                                                 GLXFBConfig config)
{
    return NULL;
}

static Bool          dummyMakeContextCurrent    (Display *dpy, GLXDrawable draw,
                                              GLXDrawable read, GLXContext ctx)
{
    // This doesn't do anything, but fakes success
    return True;
}

static int           dummyQueryContext          (Display *dpy,
                                                 GLXContext ctx,
                                                 int attribute,
                                                 int *value)
{
    return 0;
}

static void          dummyQueryDrawable         (Display *dpy,
                                                 GLXDrawable draw,
                                                 int attribute,
                                                 unsigned int *value)
{
    // nop
}

static void          dummySelectEvent           (Display *dpy,
                                                 GLXDrawable draw,
                                                 unsigned long event_mask)
{
    // nop
}

/*
 * Some immediate-mode GL functions which will be part of the static dispatch
 * table.
 */
static void dummy_glBegin (void)
{
    // TODO
}

static void dummy_glVertex3fv(GLfloat *v)
{
    // TODO
}

static void dummy_glEnd (void)
{
    // TODO
}

static void dummy_glMakeCurrentTestResults(GLint req,
                                        GLboolean *saw,
                                        void **ret)
{
    *saw = GL_TRUE;
    *ret = NULL;
}

#define GL_PROC_ENTRY(x) { (void *)dummy_gl ## x, "gl" #x }

/*
 * Note we only fill in real implementations for a few core GL functions.
 * The rest will dispatch to the NOP stub.
 */
static struct {
    void *addr;
    const char *name;
} procAddresses[] = {
    GL_PROC_ENTRY(Begin),
    GL_PROC_ENTRY(End),
    GL_PROC_ENTRY(Vertex3fv),
    GL_PROC_ENTRY(MakeCurrentTestResults),
};


static void dummyNopStub (void)
{
    // nop
}

// XXX non-entry point ABI functions
static void         *dummyGetProcAddress         (const GLubyte *procName, void *data)
{
    int i;
    for (i = 0; i < ARRAY_LEN(procAddresses); i++) {
        if (!strcmp(procAddresses[i].name, (const char *)procName)) {
            return procAddresses[i].addr;
        }
    }

    return (void *)dummyNopStub;
}

static void dummyDestroyDispatchData(void *data)
{
    // nop
}

static void         *dummyGetDispatchAddress     (const GLubyte *procName)
{
    return NULL;
}

static void         dummySetDispatchIndex      (const GLubyte *procName, int index)
{
    // nop
}

static GLboolean    dummyGetDispatchProto   (const GLubyte *procName,
                                          char ***function_names,
                                          char **parameter_signature)
{
    // We only export one extension function here
    // TODO: Maybe a good idea to test a bunch of different protos?
    if (!strcmp((const char *)procName, "glMakeCurrentTestResults")) {
        *function_names = malloc(2 * sizeof(char *));
        (*function_names)[0] = strdup("glMakeCurrentTestResults");
        (*function_names)[1] = NULL;
        *parameter_signature = strdup("ipp");
        return GL_TRUE;
    }

    return GL_FALSE;
}


static const __GLXapiImports dummyImports =
{
    /* Entry points */
    .glx14ep = {
        .chooseVisual = dummyChooseVisual,
        .copyContext = dummyCopyContext,
        .createContext = dummyCreateContext,
        .createGLXPixmap = dummyCreateGLXPixmap,
        .destroyContext = dummyDestroyContext,
        .destroyGLXPixmap = dummyDestroyGLXPixmap,
        .getConfig = dummyGetConfig,
        .isDirect = dummyIsDirect,
        .makeCurrent = dummyMakeCurrent,
        .swapBuffers = dummySwapBuffers,
        .useXFont = dummyUseXFont,
        .waitGL = dummyWaitGL,
        .waitX = dummyWaitX,
        .queryServerString = dummyQueryServerString,
        .getClientString = dummyGetClientString,
        .queryExtensionsString = dummyQueryExtensionsString,
        .chooseFBConfig = dummyChooseFBConfig,
        .createNewContext = dummyCreateNewContext,
        .createPbuffer = dummyCreatePbuffer,
        .createPixmap = dummyCreatePixmap,
        .createWindow = dummyCreateWindow,
        .destroyPbuffer = dummyDestroyPbuffer,
        .destroyPixmap = dummyDestroyPixmap,
        .destroyWindow = dummyDestroyWindow,
        .getFBConfigAttrib = dummyGetFBConfigAttrib,
        .getFBConfigs = dummyGetFBConfigs,
        .getSelectedEvent = dummyGetSelectedEvent,
        .getVisualFromFBConfig = dummyGetVisualFromFBConfig,
        .makeContextCurrent = dummyMakeContextCurrent,
        .queryContext = dummyQueryContext,
        .queryDrawable = dummyQueryDrawable,
        .selectEvent = dummySelectEvent,
    },

    /* Non-entry points */
    .glxvc = {
        .getProcAddress = dummyGetProcAddress,
        .destroyDispatchData = dummyDestroyDispatchData,
        .getDispatchAddress = dummyGetDispatchAddress,
        .setDispatchIndex = dummySetDispatchIndex,
        .getDispatchProto = dummyGetDispatchProto
    }
};

PUBLIC __GLX_MAIN_PROTO(version, exports, vendorName)
{
    thisVendorName = strdup(vendorName);
    if (version <= GLX_VENDOR_ABI_VERSION) {
        memcpy(&apiExports, exports, sizeof(*exports));
        return &dummyImports;
    } else {
        return NULL;
    }
}
