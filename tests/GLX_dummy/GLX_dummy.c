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
#include "trace.h"
#include "compiler.h"


static char *thisVendorName;
static __GLXapiExports apiExports;

/*
 * Dummy context structure.
 */
typedef struct __GLXcontextRec {
    GLint beginHit;
    GLint vertex3fvHit;
    GLint endHit;
} __GLXcontext;

static XVisualInfo*  dummyChooseVisual          (Display *dpy,
                                                 int screen,
                                                 int *attrib_list)
{
    XVisualInfo *ret_visual;
    XVisualInfo matched_visual;

    // XXX Just get a visual which can be used to open a window.
    // Ignore the attribs; we're not going to be doing
    // any actual rendering in this test.
    if (XMatchVisualInfo(dpy, screen,
                         DefaultDepth(dpy, screen),
                         TrueColor,
                         &matched_visual) == 0) {
        return NULL;
    }

    ret_visual = malloc(sizeof(XVisualInfo));
    memcpy(ret_visual, &matched_visual, sizeof(XVisualInfo));

    return ret_visual;
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
    __GLXcontext *context = malloc(sizeof(*context));
    context->beginHit = 0;
    context->vertex3fvHit = 0;
    context->endHit = 0;
    return context;
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

/*
 * Macro magic to construct a long extension string
 */
#define EXT_STR0 "GLX_bogusextensionstring "
#define EXT_STR1 EXT_STR0 EXT_STR0
#define EXT_STR2 EXT_STR1 EXT_STR1
#define EXT_STR3 EXT_STR2 EXT_STR2
#define EXT_STR4 EXT_STR3 EXT_STR3
#define LONG_EXT_STR EXT_STR4 EXT_STR4


static const char*   dummyGetClientString     (Display *dpy,
                                               int name)
{
    /* Used for client string unit test */
    static const char glxVendor[] = "testlib";
    static const char glxVersion[] = "0.0 GLX_makecurrent";

    /* Use a reallly long extension string to test bounds-checking */
    static const char glxExtensions[] = LONG_EXT_STR;

    switch (name) {
    case GLX_VENDOR:
        return glxVendor;
    case GLX_VERSION:
        return glxVersion;
    case GLX_EXTENSIONS:
        return glxExtensions;
    default:
        return NULL;
    }
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
    GLXContext ctx = apiExports.getCurrentContext();
    assert(ctx);

    ctx->beginHit++;
}

static void dummy_glVertex3fv(GLfloat *v)
{
    GLXContext ctx = apiExports.getCurrentContext();
    assert(ctx);

    ctx->vertex3fvHit++;
}

static void dummy_glEnd (void)
{
    GLXContext ctx = apiExports.getCurrentContext();
    assert(ctx);

    ctx->endHit++;
}

static void dummy_glMakeCurrentTestResults(GLint req,
                                        GLboolean *saw,
                                        void **ret)
{
    GLXContext ctx = apiExports.getCurrentContext();
    assert(ctx);

    *saw = GL_TRUE;
    switch (req) {
    case GL_MC_FUNCTION_COUNTS:
        {
            GLint *data = (GLint *)malloc(3 * sizeof(GLint));
            data[0] = ctx->beginHit;
            data[1] = ctx->vertex3fvHit;
            data[2] = ctx->endHit;
            *ret = (void *)data;
        }
        break;
    case GL_MC_VENDOR_STRING:
        {
            *ret = thisVendorName ? strdup(thisVendorName) : NULL;
        }
        break;
    case GL_MC_LAST_REQ:
    default:
        *ret = NULL;
        break;
    }
}

static void dummyExampleExtensionFunction(Display *dpy, int screen, int *retval)
{
    // Indicate that we've called the real function, and not a dispatch stub
    *retval = 1;
}

typedef void (*ExampleExtensionFunctionPtr)(Display *dpy,
                                            int screen,
                                            int *retval);

static int dummyExampleExtensionFunctionIndex = -1;

static void dispatch_glXExampleExtensionFunction(Display *dpy,
                                                int screen,
                                                int *retval)
{
    __GLXvendorInfo *dynDispatch;
    ExampleExtensionFunctionPtr func;
    const int index = dummyExampleExtensionFunctionIndex;

    dynDispatch = apiExports.getDynDispatch(dpy, screen);
    if (!dynDispatch) {
        return;
    }

    func = (ExampleExtensionFunctionPtr)
        apiExports.fetchDispatchEntry(dynDispatch, index);
    if (func) {
        func(dpy, screen, retval);
    }
}

#define GL_PROC_ENTRY(x) { (void *)dummy_gl ## x, "gl" #x }
#define GLX_PROC_ENTRY(x) { (void *)dummy ## x, "glX" #x }

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
    GLX_PROC_ENTRY(ExampleExtensionFunction)
};


static void dummyNopStub (void)
{
    // nop
}

// XXX non-entry point ABI functions
static void         *dummyGetProcAddress         (const GLubyte *procName)
{
    int i;
    for (i = 0; i < ARRAY_LEN(procAddresses); i++) {
        if (!strcmp(procAddresses[i].name, (const char *)procName)) {
            return procAddresses[i].addr;
        }
    }

    return (void *)dummyNopStub;
}

static void         *dummyGetDispatchAddress     (const GLubyte *procName)
{
    if (!strcmp((const char *)procName, "glXExampleExtensionFunction")) {
        return dispatch_glXExampleExtensionFunction;
    }
    return NULL;
}

static void         dummySetDispatchIndex      (const GLubyte *procName, int index)
{
    // nop
    if (!strcmp((const char *)procName, "glXExampleExtensionFunction")) {
        dummyExampleExtensionFunctionIndex = index;
    }
}

#if defined(PATCH_ENTRYPOINTS)
PUBLIC int __glXSawVertex3fv;

static void patch_x86_64_tls(char *entry,
                             int stubSize)
{
#if defined(__x86_64__)
    char *pSawVertex3fv = (char *)&__glXSawVertex3fv;
    int *p;
    char tmpl[] = {
        0x8b, 0x05, 0x0, 0x0, 0x0, 0x0,  // mov 0x0(%rip), %eax
        0x83, 0xc0, 0x01,                // add $0x1, %eax
        0x89, 0x05, 0x0, 0x0, 0x0, 0x0,  // mov %eax, 0x0(%rip)
        0xc3,                            // ret
    };

    STATIC_ASSERT(sizeof(int) == 0x4);

    if (stubSize < sizeof(tmpl)) {
        return;
    }

    p = (int *)&tmpl[2];
    *p = (int)(pSawVertex3fv - (entry + 6));

    p = (int *)&tmpl[11];
    *p = (int)(pSawVertex3fv - (entry + 15));

    memcpy(entry, tmpl, sizeof(tmpl));
#else
    assert(0); // Should not be calling this
#endif
}

static void patch_x86_tls(char *entry,
                          int stubSize)
{
#if defined(__i386__)
    char *pSawVertex3fv = (char *)&__glXSawVertex3fv;
    int *p;
    char tmpl[] = {
        0xa1, 0x0, 0x0, 0x0, 0x0,   // mov 0x0, %eax
        0x83, 0xc0, 0x01,           // add $0x1, %eax
        0xa3, 0x0, 0x0, 0x0, 0x0,   // mov %eax, 0x0
        0xc3                        // ret
    };

    STATIC_ASSERT(sizeof(int) == 0x4);

    if (stubSize < sizeof(tmpl)) {
        return;
    }

    p = (int *)&tmpl[1];
    *p = (int)(pSawVertex3fv - (entry + 5));

    p = (int *)&tmpl[9];
    *p = (int)(pSawVertex3fv - (entry + 13));

    memcpy(entry, tmpl, sizeof(tmpl));

    // Jump to an intermediate location
    __asm__(
        "\tjmp 0f\n"
        "\t0:\n"
    );
#else
    assert(0); // Should not be calling this
#endif
}

static int dummyStubType;
static int dummyStubSize;
static GLint64 dummyStubGeneration;
static char *dummyVertex3fvAddrs[3];

static GLboolean dummyInitiatePatch(int type,
                                    int stubSize,
                                    GLint64 stubGeneration,
                                    GLboolean *needOffsets)
{
    assert(needOffsets);
    switch (type) {
        case __GLDISPATCH_STUB_X86_64_TLS:
        case __GLDISPATCH_STUB_X86_TLS:
        case __GLDISPATCH_STUB_X86_64_TSD:
            dummyStubType = type;
            dummyStubSize = stubSize;
            dummyStubGeneration = stubGeneration;
            *needOffsets = GL_TRUE;
            return GL_TRUE;
        default:
            *needOffsets = GL_FALSE;
            return GL_FALSE;
    }
}

/* Only interested in glVertex3fv() for testing purposes. */
static void dummyGetOffsetHook(
    void *(*lookupStubOffset)(const char *funcName)
)
{
    char **pNewAddr;

    // Seek to the first empty entry
    for (pNewAddr = dummyVertex3fvAddrs;
         *pNewAddr; pNewAddr++) {
    }

    assert((pNewAddr - dummyVertex3fvAddrs) < 3);

    *pNewAddr = (char *)lookupStubOffset("Vertex3fv");
}

static void dummyFinalizePatch(void)
{
    char **pAddr;
    for (pAddr = dummyVertex3fvAddrs; *pAddr; pAddr++) {
        switch (dummyStubType) {
            case __GLDISPATCH_STUB_X86_64_TLS:
            case __GLDISPATCH_STUB_X86_64_TSD:
                patch_x86_64_tls(*pAddr, dummyStubSize);
                break;
            case __GLDISPATCH_STUB_X86_TLS:
                patch_x86_tls(*pAddr, dummyStubSize);
                break;
            default:
                assert(0);
        }
    }
}

static void dummyReleasePatch(void)
{
}

static const __GLdispatchPatchCallbacks dummyPatchCallbacks =
{
    .initiatePatch = dummyInitiatePatch,
    .getOffsetHook = dummyGetOffsetHook,
    .finalizePatch = dummyFinalizePatch,
    .releasePatch = dummyReleasePatch,
};
#endif // defined(PATCH_ENTRYPOINTS)

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
        .getDispatchAddress = dummyGetDispatchAddress,
        .setDispatchIndex = dummySetDispatchIndex,
#if defined(PATCH_ENTRYPOINTS)
        .patchCallbacks = &dummyPatchCallbacks,
#else
        .patchCallbacks = NULL,
#endif
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
