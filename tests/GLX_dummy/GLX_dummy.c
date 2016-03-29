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
#include <X11/Xlib.h>
#include <X11/Xlibint.h>
#include <GL/glx.h>
#include <GL/glxint.h>

#include "GLX_dummy.h"
#include "glvnd/libglxabi.h"
#include "utils_misc.h"
#include "trace.h"
#include "compiler.h"


static const __GLXapiExports *apiExports = NULL;

/*
 * Dummy context structure.
 */
typedef struct __GLXcontextRec {
    GLint beginHit;
    GLint vertex3fvHit;
    GLint endHit;
} __GLXcontext;

static const int FBCONFIGS_PER_SCREEN = 10;

static void dummy_glXExampleExtensionFunction(Display *dpy, int screen, int *retval);
static void dispatch_glXExampleExtensionFunction(Display *dpy, int screen, int *retval);

enum
{
    DI_glXExampleExtensionFunction,
    DI_COUNT,
};
static struct {
    const char *name;
    void *addr;
    void *dispatchAddress;
    int index;
} glxExtensionProcs[] = {
#define PROC_ENTRY(name) { #name, dummy_##name, dispatch_##name, -1 }
    PROC_ENTRY(glXExampleExtensionFunction),
#undef PROC_ENTRY
};

static GLXFBConfig GetFBConfigFromScreen(Display *dpy, int screen, int index)
{
    // Pick an arbitrary base address.
    uintptr_t baseConfig = (uintptr_t) &FBCONFIGS_PER_SCREEN;
    baseConfig += (screen * FBCONFIGS_PER_SCREEN);
    return (GLXFBConfig) (baseConfig + index);
}

static int GetScreenFromFBConfig(Display *dpy, GLXFBConfig config)
{
    uintptr_t screen = ((uintptr_t) config) - ((uintptr_t) &FBCONFIGS_PER_SCREEN);
    screen = screen / FBCONFIGS_PER_SCREEN;
    if (screen < (uintptr_t) ScreenCount(dpy)) {
        return (int) screen;
    } else {
        return -1;
    }
}

static GLXDrawable CommonCreateDrawable(Display *dpy, int screen)
{
    // Just hand back a fresh XID
    if (screen >= 0) {
        XID id;
        LockDisplay(dpy);
        id = XAllocID(dpy);
        UnlockDisplay(dpy);
        return id;
    } else {
        return None;
    }
}

static XVisualInfo*  dummy_glXChooseVisual          (Display *dpy,
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

static void          dummy_glXCopyContext           (Display *dpy,
                                                 GLXContext src,
                                                 GLXContext dst,
                                                 unsigned long mask)
{
    // nop
}

static GLXContext CommonCreateContext(Display *dpy, int screen)
{
    if (screen >= 0) {
        __GLXcontext *context = malloc(sizeof(*context));
        context->beginHit = 0;
        context->vertex3fvHit = 0;
        context->endHit = 0;
        return context;
    } else {
        return NULL;
    }
}

static GLXContext    dummy_glXCreateContext         (Display *dpy,
                                                 XVisualInfo *vis,
                                                 GLXContext share_list,
                                                 Bool direct)
{
    return CommonCreateContext(dpy, vis->screen);
}

static GLXContext    dummy_glXCreateNewContext      (Display *dpy,
                                                 GLXFBConfig config,
                                                 int render_type,
                                                 GLXContext share_list,
                                                 Bool direct)
{
    return CommonCreateContext(dpy, GetScreenFromFBConfig(dpy, config));
}

static GLXPixmap     dummy_glXCreateGLXPixmap       (Display *dpy,
                                                 XVisualInfo *vis,
                                                 Pixmap pixmap)
{
    return CommonCreateDrawable(dpy, vis->screen);
}

static void          dummy_glXDestroyContext        (Display *dpy,
                                              GLXContext ctx)
{
    free(ctx);
}

static void          dummy_glXDestroyGLXPixmap      (Display *dpy,
                                                 GLXPixmap pix)
{
    // nop
}

static int           dummy_glXGetConfig             (Display *dpy,
                                                 XVisualInfo *vis,
                                                 int attrib,
                                                 int *value)
{
    return 0;
}

static Bool          dummy_glXIsDirect              (Display *dpy,
                                                 GLXContext ctx)
{
    return False;
}

static Bool          dummy_glXMakeCurrent           (Display *dpy,
                                              GLXDrawable drawable,
                                              GLXContext ctx)
{
    // This doesn't do anything, but fakes success
    return True;
}

static void          dummy_glXSwapBuffers           (Display *dpy,
                                                 GLXDrawable drawable)
{
    // nop
}

static void          dummy_glXUseXFont              (Font font,
                                                 int first,
                                                 int count,
                                                 int list_base)
{
    // nop
}

static void          dummy_glXWaitGL                (void)
{
    // nop
}

static void          dummy_glXWaitX                 (void)
{
    // nop
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


static const char*   dummy_glXGetClientString     (Display *dpy,
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

static const char*   dummy_glXQueryServerString     (Display *dpy,
                                                 int screen,
                                                 int name)
{
    return dummy_glXGetClientString(dpy, name);
}

static const char*   dummy_glXQueryExtensionsString (Display *dpy,
                                                 int screen)
{
    return dummy_glXQueryServerString(dpy, screen, GLX_EXTENSIONS);
}

static GLXFBConfig*  dummy_glXGetFBConfigs          (Display *dpy,
                                                 int screen,
                                                 int *nelements)
{
    GLXFBConfig *configs = NULL;
    int i;

    // Pick an arbitrary base address.
    configs = malloc(sizeof(GLXFBConfig) * FBCONFIGS_PER_SCREEN);
    if (configs != NULL) {
        for (i=0; i<FBCONFIGS_PER_SCREEN; i++) {
            configs[i] = GetFBConfigFromScreen(dpy, screen, i);
        }
    }
    *nelements = FBCONFIGS_PER_SCREEN;
    return configs;
}

static GLXFBConfig*  dummy_glXChooseFBConfig        (Display *dpy,
                                              int screen,
                                              const int *attrib_list,
                                              int *nelements)
{
    return dummy_glXGetFBConfigs(dpy, screen, nelements);
}

static GLXPbuffer    dummy_glXCreatePbuffer         (Display *dpy,
                                                 GLXFBConfig config,
                                                 const int *attrib_list)
{
    return CommonCreateDrawable(dpy, GetScreenFromFBConfig(dpy, config));
}

static GLXPixmap     dummy_glXCreatePixmap          (Display *dpy,
                                                 GLXFBConfig config,
                                                 Pixmap pixmap,
                                                 const int *attrib_list)
{
    return CommonCreateDrawable(dpy, GetScreenFromFBConfig(dpy, config));
}

static GLXWindow     dummy_glXCreateWindow          (Display *dpy,
                                                 GLXFBConfig config,
                                                 Window win,
                                                 const int *attrib_list)
{
    return CommonCreateDrawable(dpy, GetScreenFromFBConfig(dpy, config));
}

static void          dummy_glXDestroyPbuffer        (Display *dpy,
                                                 GLXPbuffer pbuf)
{
    // nop
}

static void          dummy_glXDestroyPixmap         (Display *dpy,
                                                 GLXPixmap pixmap)
{
    // nop
}

static void          dummy_glXDestroyWindow         (Display *dpy,
                                                 GLXWindow win)
{
    // nop
}

static int           dummy_glXGetFBConfigAttrib     (Display *dpy,
                                                 GLXFBConfig config,
                                                 int attribute,
                                                 int *value)
{
    return 0;
}

static void          dummy_glXGetSelectedEvent      (Display *dpy,
                                                 GLXDrawable draw,
                                                 unsigned long *event_mask)
{
    // nop
}

static XVisualInfo*  dummy_glXGetVisualFromFBConfig (Display *dpy,
                                                 GLXFBConfig config)
{
    int screen = GetScreenFromFBConfig(dpy, config);
    if (screen >= 0) {
        return dummy_glXChooseVisual(dpy, screen, NULL);
    } else {
        return NULL;
    }
}

static Bool          dummy_glXMakeContextCurrent    (Display *dpy, GLXDrawable draw,
                                              GLXDrawable read, GLXContext ctx)
{
    // This doesn't do anything, but fakes success
    return True;
}

static int           dummy_glXQueryContext          (Display *dpy,
                                                 GLXContext ctx,
                                                 int attribute,
                                                 int *value)
{
    return 0;
}

static void          dummy_glXQueryDrawable         (Display *dpy,
                                                 GLXDrawable draw,
                                                 int attribute,
                                                 unsigned int *value)
{
    // nop
}

static void          dummy_glXSelectEvent           (Display *dpy,
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
    GLXContext ctx = apiExports->getCurrentContext();
    assert(ctx);

    ctx->beginHit++;
}

static void dummy_glVertex3fv(GLfloat *v)
{
    GLXContext ctx = apiExports->getCurrentContext();
    assert(ctx);

    ctx->vertex3fvHit++;
}

static void dummy_glEnd (void)
{
    GLXContext ctx = apiExports->getCurrentContext();
    assert(ctx);

    ctx->endHit++;
}

static void dummy_glMakeCurrentTestResults(GLint req,
                                        GLboolean *saw,
                                        void **ret)
{
    GLXContext ctx = apiExports->getCurrentContext();
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
            // FIXME: This is used from testglxnscreens to check that the
            // correct vendor library is loaded from each display. Originally,
            // it used the vendor name passed to __glx_Main, but libGLX doesn't
            // provide the vendor name anymore.
            *ret = NULL;
        }
        break;
    case GL_MC_LAST_REQ:
    default:
        *ret = NULL;
        break;
    }
}

static void dummy_glXExampleExtensionFunction(Display *dpy, int screen, int *retval)
{
    // Indicate that we've called the real function, and not a dispatch stub
    *retval = 1;
}

static void dispatch_glXExampleExtensionFunction(Display *dpy,
                                                int screen,
                                                int *retval)
{
    typedef void (*ExampleExtensionFunctionPtr)(Display *dpy,
                                                int screen,
                                                int *retval);
    __GLXvendorInfo *dynDispatch;
    ExampleExtensionFunctionPtr func;
    const int index = glxExtensionProcs[DI_glXExampleExtensionFunction].index;

    dynDispatch = apiExports->getDynDispatch(dpy, screen);
    if (!dynDispatch) {
        return;
    }

    func = (ExampleExtensionFunctionPtr)
        apiExports->fetchDispatchEntry(dynDispatch, index);
    if (func) {
        func(dpy, screen, retval);
    }
}

/*
 * Note we only fill in real implementations for a few core GL functions.
 * The rest will dispatch to the NOP stub.
 */
static const struct {
    const char *name;
    void *addr;
} procAddresses[] = {
#define PROC_ENTRY(name) { #name, (void *)dummy_ ## name }
    PROC_ENTRY(glBegin),
    PROC_ENTRY(glEnd),
    PROC_ENTRY(glVertex3fv),
    PROC_ENTRY(glMakeCurrentTestResults),

    PROC_ENTRY(glXChooseVisual),
    PROC_ENTRY(glXCopyContext),
    PROC_ENTRY(glXCreateContext),
    PROC_ENTRY(glXCreateGLXPixmap),
    PROC_ENTRY(glXDestroyContext),
    PROC_ENTRY(glXDestroyGLXPixmap),
    PROC_ENTRY(glXGetConfig),
    PROC_ENTRY(glXIsDirect),
    PROC_ENTRY(glXMakeCurrent),
    PROC_ENTRY(glXSwapBuffers),
    PROC_ENTRY(glXUseXFont),
    PROC_ENTRY(glXWaitGL),
    PROC_ENTRY(glXWaitX),
    PROC_ENTRY(glXQueryServerString),
    PROC_ENTRY(glXGetClientString),
    PROC_ENTRY(glXQueryExtensionsString),
    PROC_ENTRY(glXChooseFBConfig),
    PROC_ENTRY(glXCreateNewContext),
    PROC_ENTRY(glXCreatePbuffer),
    PROC_ENTRY(glXCreatePixmap),
    PROC_ENTRY(glXCreateWindow),
    PROC_ENTRY(glXDestroyPbuffer),
    PROC_ENTRY(glXDestroyPixmap),
    PROC_ENTRY(glXDestroyWindow),
    PROC_ENTRY(glXGetFBConfigAttrib),
    PROC_ENTRY(glXGetFBConfigs),
    PROC_ENTRY(glXGetSelectedEvent),
    PROC_ENTRY(glXGetVisualFromFBConfig),
    PROC_ENTRY(glXMakeContextCurrent),
    PROC_ENTRY(glXQueryContext),
    PROC_ENTRY(glXQueryDrawable),
    PROC_ENTRY(glXSelectEvent),
#undef PROC_ENTRY
};

static Bool          dummyCheckSupportsScreen    (Display *dpy, int screen)
{
    return True;
}

static void         *dummyGetProcAddress         (const GLubyte *procName)
{
    int i;

    for (i = 0; i < ARRAY_LEN(procAddresses); i++) {
        if (!strcmp(procAddresses[i].name, (const char *)procName)) {
            return procAddresses[i].addr;
        }
    }

    for (i = 0; i<DI_COUNT; i++) {
        if (!strcmp(glxExtensionProcs[i].name, (const char *)procName)) {
            return glxExtensionProcs[i].addr;
        }
    }

    return NULL;
}

static void         *dummyGetDispatchAddress     (const GLubyte *procName)
{
    int i;
    for (i = 0; i<DI_COUNT; i++) {
        if (!strcmp(glxExtensionProcs[i].name, (const char *)procName)) {
            return glxExtensionProcs[i].dispatchAddress;
        }
    }
    return NULL;
}

static void         dummySetDispatchIndex      (const GLubyte *procName, int index)
{
    int i;
    for (i = 0; i<DI_COUNT; i++) {
        if (!strcmp(glxExtensionProcs[i].name, (const char *)procName)) {
            glxExtensionProcs[i].index = index;
        }
    }
}

#if defined(PATCH_ENTRYPOINTS)
PUBLIC int __glXSawVertex3fv;

static void patch_x86_64(char *writeEntry,
                             const char *execEntry,
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
    *p = (int)(pSawVertex3fv - (execEntry + 6));

    p = (int *)&tmpl[11];
    *p = (int)(pSawVertex3fv - (execEntry + 15));

    memcpy(writeEntry, tmpl, sizeof(tmpl));
#else
    assert(0); // Should not be calling this
#endif
}

static void patch_x86(char *writeEntry,
                          const char *execEntry,
                          int stubSize)
{
#if defined(__i386__)
    uintptr_t *p;
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

    // Patch the address of the __glXSawVertex3fv variable. Note that we patch
    // in an absolute address in this case. Unlike x86-64, x86 does not allow
    // PC-relative addressing for MOV instructions.
    p = (uintptr_t *)&tmpl[1];
    *p = (uintptr_t) &__glXSawVertex3fv;

    p = (uintptr_t *)&tmpl[9];
    *p = (uintptr_t) &__glXSawVertex3fv;

    memcpy(writeEntry, tmpl, sizeof(tmpl));

    // Jump to an intermediate location
    __asm__(
        "\tjmp 0f\n"
        "\t0:\n"
    );
#else
    assert(0); // Should not be calling this
#endif
}

static void patch_armv7_thumb(char *writeEntry,
                                  const char *execEntry,
                                  int stubSize)
{
#if defined(__arm__)
    char *pSawVertex3fv = (char *)&__glXSawVertex3fv;

    // Thumb bytecode
    char tmpl[] = {
        // ldr r0, 1f
        0x48, 0x02,
        // ldr r1, [r0]
        0x68, 0x01,
        // add r1, r1, #1
        0xf1, 0x01, 0x01, 0x01,
        // str r1, [r0]
        0x60, 0x01,
        // bx lr
        0x47, 0x70,
        // 1:
        0x00, 0x00, 0x00, 0x00,
    };

    int offsetAddr = sizeof(tmpl) - 4;

    if (stubSize < sizeof(tmpl)) {
        return;
    }

#if __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
    glvnd_byte_swap16((uint16_t *)tmpl, offsetAddr);
#endif

    *((uint32_t *)(tmpl + offsetAddr)) = (uint32_t)pSawVertex3fv;

    memcpy(writeEntry, tmpl, sizeof(tmpl));

    __builtin___clear_cache((char *) execEntry, (char *) (execEntry + sizeof(tmpl)));
#else
    assert(0); // Should not be calling this
#endif
}

static GLboolean dummyCheckPatchSupported(int type, int stubSize)
{
    switch (type) {
        case __GLDISPATCH_STUB_X86_64:
        case __GLDISPATCH_STUB_X86:
        case __GLDISPATCH_STUB_ARMV7_THUMB:
            return GL_TRUE;
        default:
            return GL_FALSE;
    }
}

static GLboolean dummyInitiatePatch(int type,
                                    int stubSize,
                                    DispatchPatchLookupStubOffset lookupStubOffset)
{
    void *writeAddr;
    const void *execAddr;

    if (!dummyCheckPatchSupported(type, stubSize))
    {
        return GL_FALSE;
    }

    if (lookupStubOffset("Vertex3fv", &writeAddr, &execAddr)) {
        switch (type) {
            case __GLDISPATCH_STUB_X86_64:
                patch_x86_64(writeAddr, execAddr, stubSize);
                break;
            case __GLDISPATCH_STUB_X86:
                patch_x86(writeAddr, execAddr, stubSize);
                break;
            case __GLDISPATCH_STUB_ARMV7_THUMB:
                patch_armv7_thumb(writeAddr, execAddr, stubSize);
                break;
            default:
                assert(0);
        }
    }

    return GL_TRUE;
}

#endif // defined(PATCH_ENTRYPOINTS)

PUBLIC Bool __glx_Main(uint32_t version,
                                  const __GLXapiExports *exports,
                                  __GLXvendorInfo *vendor,
                                  __GLXapiImports *imports)
{
    assert(ARRAY_LEN(glxExtensionProcs) == DI_COUNT);

    if (GLX_VENDOR_ABI_GET_MAJOR_VERSION(version)
            == GLX_VENDOR_ABI_MAJOR_VERSION) {
        if (GLX_VENDOR_ABI_GET_MINOR_VERSION(version)
                >= GLX_VENDOR_ABI_MINOR_VERSION) {
            apiExports = exports;

            imports->isScreenSupported = dummyCheckSupportsScreen;
            imports->getProcAddress = dummyGetProcAddress;
            imports->getDispatchAddress = dummyGetDispatchAddress;
            imports->setDispatchIndex = dummySetDispatchIndex;
#if defined(PATCH_ENTRYPOINTS)
            imports->isPatchSupported = dummyCheckPatchSupported;
            imports->initiatePatch = dummyInitiatePatch;
#endif

            return True;
        }
    }
    return False;
}
