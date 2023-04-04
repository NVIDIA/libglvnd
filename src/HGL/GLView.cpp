/*
 * Copyright 2006-2023, Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *        Jérôme Duval, korli@users.berlios.de
 *        Philippe Houdoin, philippe.houdoin@free.fr
 *        Stefano Ceccherini, burton666@libero.it
 *        X512, danger_mail@list.ru
 *        Alexander von Gluck IV, kallisti5@unixzen.com
 */

#include <kernel/image.h>

#define EGL_EGL_PROTOTYPES 0
#include <GLView.h>
#include <EGL/egl.h>

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dlfcn.h>
#include <algorithm>

#include <DirectWindow.h>
#include <Referenceable.h>

#include <private/interface/DirectWindowPrivate.h>
#include <private/shared/AutoDeleter.h>
#include <private/shared/AutoLocker.h>
#include <private/shared/PthreadMutexLocker.h>

#include "BitmapHook.h"


struct glview_direct_info {
    direct_buffer_info* direct_info;
    bool direct_connected;
    bool enable_direct_mode;

    glview_direct_info();
    ~glview_direct_info();
};

#define LIBEGL_HOOK_LIST(REQUIRED) \
    REQUIRED(PFNEGLGETPROCADDRESSPROC, eglGetProcAddress) \
    REQUIRED(PFNEGLGETDISPLAYPROC, eglGetDisplay) \
    REQUIRED(PFNEGLINITIALIZEPROC, eglInitialize) \
    REQUIRED(PFNEGLCHOOSECONFIGPROC, eglChooseConfig) \
    REQUIRED(PFNEGLBINDAPIPROC, eglBindAPI) \
    REQUIRED(PFNEGLCREATECONTEXTPROC, eglCreateContext) \
    REQUIRED(PFNEGLDESTROYCONTEXTPROC, eglDestroyContext) \
    REQUIRED(PFNEGLMAKECURRENTPROC, eglMakeCurrent) \
    REQUIRED(PFNEGLTERMINATEPROC, eglTerminate) \
    REQUIRED(PFNEGLSWAPBUFFERSPROC, eglSwapBuffers) \
    REQUIRED(PFNEGLCREATEWINDOWSURFACEPROC, eglCreateWindowSurface) \
    REQUIRED(PFNEGLCREATEPBUFFERSURFACEPROC, eglCreatePbufferSurface) \
    REQUIRED(PFNEGLDESTROYSURFACEPROC, eglDestroySurface) \
    REQUIRED(PFNEGLGETCURRENTCONTEXTPROC, eglGetCurrentContext) \
    REQUIRED(PFNEGLGETCURRENTDISPLAYPROC, eglGetCurrentDisplay)


class BGLView::Display : public BReferenceable {
public:
#define DISPATCH_TABLE_ENTRY(x, y) x y {};
    LIBEGL_HOOK_LIST(DISPATCH_TABLE_ENTRY)
#undef DISPATCH_TABLE_ENTRY

    EGLDisplay eglDpy {};
    EGLConfig eglCfg {};

private:
    status_t Init();

    pthread_mutex_t fLock = PTHREAD_RECURSIVE_MUTEX_INITIALIZER;
    status_t fStatus = B_NO_INIT;
    void *libEgl {};

protected:
    virtual    void FirstReferenceAcquired();
    virtual    void LastReferenceReleased();

public:
    Display();
    virtual ~Display() = default;

    status_t InitCheck();
};


struct BGLView::Renderer {
    BGLView *view;

    BReference<Display> display;

    EGLContext eglCtx = EGL_NO_CONTEXT;
    EGLSurface eglSurf = EGL_NO_SURFACE;
    uint32_t width = 0;
    uint32_t height = 0;

    class BitmapHook: public ::BitmapHook {
    private:
        inline Renderer *Base() {return (Renderer*)((char*)this - offsetof(Renderer, fBmpHook));}

    public:
        virtual ~BitmapHook() {};
        void GetSize(uint32_t &width, uint32_t &height) override;
        BBitmap *SetBitmap(BBitmap *bmp) override;
    } fBmpHook;
    pthread_mutex_t fLock = PTHREAD_RECURSIVE_MUTEX_INITIALIZER;
    ObjectDeleter<BBitmap> fBitmap;

    ~Renderer();
    status_t Init();
    void SwapBuffers();
};


BGLView::Display BGLView::sDisplay;


BGLView::Display::Display()
{
    fReferenceCount = 0;
}

status_t BGLView::Display::InitCheck()
{
    PthreadMutexLocker lock(&fLock);
    return fStatus;
}

void BGLView::Display::FirstReferenceAcquired()
{
    PthreadMutexLocker lock(&fLock);
    fStatus = Init();
}

void BGLView::Display::LastReferenceReleased()
{
    PthreadMutexLocker lock(&fLock);

    if (fStatus < B_OK)
        return;

    fStatus = B_NO_INIT;

    if (eglDpy == eglGetCurrentDisplay())
        eglMakeCurrent(eglDpy, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);

    eglTerminate(eglDpy); eglDpy = NULL;
    dlclose(libEgl); libEgl = NULL;
}

status_t BGLView::Display::Init()
{
    libEgl = dlopen("libEGL.so.1", RTLD_LOCAL);
    if (libEgl == NULL) {
        fprintf(stderr, "[!] libEGL.so.1 not found\n");
        return B_ERROR;
    }

#define DISPATCH_TABLE_ENTRY(x, y) \
    *(void**)&y = dlsym(libEgl, #y); \
    if (y == NULL) { \
        fprintf(stderr, "[!] libEGL symbol " #y " not found\n"); \
        return B_ERROR; \
    }
    LIBEGL_HOOK_LIST(DISPATCH_TABLE_ENTRY)
#undef DISPATCH_TABLE_ENTRY

    eglDpy = eglGetDisplay(EGL_DEFAULT_DISPLAY);
    if (eglDpy == EGL_NO_DISPLAY) {
        fprintf(stderr, "[!] eglGetDisplay failed to obtain EGL_DEFAULT_DISPLAY\n");
        return B_ERROR;
    }

    EGLint major, minor;
    EGLBoolean result = eglInitialize(eglDpy, &major, &minor);
    if (result != EGL_TRUE) {
        fprintf(stderr, "[!] eglInitialize failed for EGL_DEFAULT_DISPLAY\n");
        return B_ERROR;
    }

    EGLint numConfigs;
    static const EGLint configAttribs[] = {
        EGL_SURFACE_TYPE, EGL_WINDOW_BIT,
        EGL_BLUE_SIZE, 8,
        EGL_GREEN_SIZE, 8,
        EGL_RED_SIZE, 8,
        EGL_DEPTH_SIZE, 8,
        EGL_RENDERABLE_TYPE, EGL_OPENGL_BIT,
        EGL_NONE
    };
    result = eglChooseConfig(eglDpy, configAttribs, &eglCfg, 1, &numConfigs);
    if (result != EGL_TRUE || numConfigs <= 0) {
        fprintf(stderr, "[!] eglChooseDisplay failed\n");
        return B_ERROR;
    }

    return B_OK;
}

void BGLView::Renderer::BitmapHook::GetSize(uint32_t &width, uint32_t &height)
{
    PthreadMutexLocker lock(&Base()->fLock);
    width = Base()->width;
    height = Base()->height;
}

BBitmap *BGLView::Renderer::BitmapHook::SetBitmap(BBitmap *bmp)
{
    PthreadMutexLocker lock(&Base()->fLock);
    BBitmap* oldBmp = Base()->fBitmap.Detach();
    Base()->fBitmap.SetTo(bmp);
    BMessenger(Base()->view).SendMessage(B_INVALIDATE);
    return oldBmp;
}

status_t BGLView::Renderer::Init()
{
    display = &BGLView::sDisplay;
    if (display->InitCheck() < B_OK)
        return display->InitCheck();

    EGLContext shareCtx = EGL_NO_CONTEXT;
    if ((view->fOptions & BGL_SHARE_CONTEXT) != 0) {
        shareCtx = display->eglGetCurrentContext();
    }

    EGLBoolean result = display->eglBindAPI(EGL_OPENGL_API);
    if (result != EGL_TRUE) {
        fprintf(stderr, "[!] eglBindAPI failed\n");
        return B_ERROR;
    }

    eglCtx = display->eglCreateContext(display->eglDpy, display->eglCfg, shareCtx, NULL);
    if (eglCtx == NULL) {
        fprintf(stderr, "[!] eglCreateContext failed\n");
        return B_ERROR;
    }

    BRect viewFrame = view->Frame();
    width = (uint32_t)viewFrame.Width() + 1;
    height = (uint32_t)viewFrame.Height() + 1;

    eglSurf = display->eglCreateWindowSurface(display->eglDpy, display->eglCfg,
            (EGLNativeWindowType)&fBmpHook, NULL);
    if (eglSurf == NULL) {
        fprintf(stderr, "[!] eglCreateWindowSurface failed\n");
        return B_ERROR;
    }

    return B_OK;
}

BGLView::Renderer::~Renderer()
{
    if (display->InitCheck() < B_OK)
        return;

    if (display->eglDpy == display->eglGetCurrentDisplay()
            && eglCtx == display->eglGetCurrentContext())
        display->eglMakeCurrent(display->eglDpy, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);

    if (eglSurf != EGL_NO_SURFACE) {
        display->eglDestroySurface(display->eglDpy, eglSurf);
        eglSurf = EGL_NO_SURFACE;
    }

    if (eglCtx != EGL_NO_CONTEXT) {
        display->eglDestroyContext(display->eglDpy, eglCtx);
        eglCtx = EGL_NO_CONTEXT;
    }
}

void BGLView::Renderer::SwapBuffers()
{
    display->eglSwapBuffers(display->eglDpy, eglSurf);
}

// #pragma mark -

BGLView::BGLView(BRect rect, const char* name, ulong resizingMode, ulong mode,
    ulong options)
    :
    BView(rect, name, B_FOLLOW_ALL_SIDES, mode | B_WILL_DRAW | B_FRAME_EVENTS),
    fGc(NULL),
    fOptions(options),
    fDrawLock("BGLView draw lock"),
    fDisplayLock("BGLView display lock"),
    fClipInfo(NULL),
    fRenderer(new Renderer{.view = this})
{
    if (fRenderer->Init() < B_OK) {
        delete fRenderer;
        fRenderer = NULL;
    }
}

// BeOS compatibility: contrary to others BView's contructors,
// BGLView one wants a non-const name argument.
BGLView::BGLView(BRect rect, char* name, ulong resizingMode, ulong mode,
    ulong options)
    :
    BView(rect, name, B_FOLLOW_ALL_SIDES, mode | B_WILL_DRAW | B_FRAME_EVENTS),
    fGc(NULL),
    fOptions(options),
    fDrawLock("BGLView draw lock"),
    fDisplayLock("BGLView display lock"),
    fClipInfo(NULL),
    fRenderer(new Renderer{.view = this})
{
    if (fRenderer->Init() < B_OK) {
        delete fRenderer;
        fRenderer = NULL;
    }
}

BGLView::~BGLView()
{
    delete fRenderer;
    delete fClipInfo;
}

void
BGLView::LockGL()
{
    fDisplayLock.Lock();
    if (fDisplayLock.CountLocks() == 1) {
        fRenderer->display->eglMakeCurrent(fRenderer->display->eglDpy, fRenderer->eglSurf,
            fRenderer->eglSurf, fRenderer->eglCtx);
    }
}

void
BGLView::UnlockGL()
{
    thread_id lockerThread = fDisplayLock.LockingThread();
    thread_id callerThread = find_thread(NULL);

    if (lockerThread != B_ERROR && lockerThread != callerThread) {
        printf("UnlockGL is called from wrong thread, lockerThread: %d, callerThread: %d\n",
            (int)lockerThread, (int)callerThread);
    }

    if (fDisplayLock.CountLocks() == 1) {
        fRenderer->display->eglMakeCurrent(fRenderer->display->eglDpy, EGL_NO_SURFACE,
            EGL_NO_SURFACE, EGL_NO_CONTEXT);
    }
    fDisplayLock.Unlock();
}

void
BGLView::SwapBuffers()
{
    SwapBuffers(false);
}

void
BGLView::SwapBuffers(bool vSync)
{
    _LockDraw();
    fRenderer->SwapBuffers();
    _UnlockDraw();
}


BView*
BGLView::EmbeddedView()
{
    return NULL;
}

void*
BGLView::GetGLProcAddress(const char* procName)
{
    return (void*)fRenderer->display->eglGetProcAddress(procName);
}

status_t
BGLView::CopyPixelsOut(BPoint source, BBitmap* dest)
{
    if (!dest || !dest->Bounds().IsValid())
        return B_BAD_VALUE;

    return ENOSYS;
}

status_t
BGLView::CopyPixelsIn(BBitmap* source, BPoint dest)
{
    if (!source || !source->Bounds().IsValid())
        return B_BAD_VALUE;

    return ENOSYS;
}

/*! Mesa's GLenum is not ulong but uint, so we can't use GLenum
    without breaking this method signature.
    Instead, we have to use the effective BeOS's SGI OpenGL GLenum type:
    unsigned long.
 */
void
BGLView::ErrorCallback(unsigned long errorCode)
{
    char msg[32];
    sprintf(msg, "GL: Error code $%04lx.", errorCode);
    // TODO: under BeOS R5, it call debugger(msg);
    fprintf(stderr, "%s\n", msg);
}

void
BGLView::Draw(BRect updateRect)
{
    BRegion region(updateRect);
    PthreadMutexLocker lock(&fRenderer->fLock);
    if (fRenderer->fBitmap.IsSet()) {
        DrawBitmap(fRenderer->fBitmap.Get(), B_ORIGIN);
        region.Exclude(fRenderer->fBitmap->Bounds());
    }
    FillRegion(&region, B_SOLID_LOW);
}

void
BGLView::AttachedToWindow()
{
    BView::AttachedToWindow();

    {
        PthreadMutexLocker lock(&fRenderer->fLock);
        fRenderer->width = Bounds().IntegerWidth() + 1;
        fRenderer->height = Bounds().IntegerHeight() + 1;
    }

    // Set default OpenGL viewport:
    LockGL();
    glViewport(0, 0, Bounds().IntegerWidth() + 1, Bounds().IntegerHeight() + 1);
    UnlockGL();
}

void BGLView::GetPreferredSize(float* _width, float* _height)
{
    if (_width)
        *_width = 0;
    if (_height)
        *_height = 0;
}

void
BGLView::DirectConnected(direct_buffer_info* info)
{
}


void
BGLView::EnableDirectMode(bool enabled)
{
}

void BGLView::FrameResized(float width, float height)
{
    BView::FrameResized(width, height);
    PthreadMutexLocker lock(&fRenderer->fLock);
    fRenderer->width = (uint32_t)width + 1;
    fRenderer->height = (uint32_t)height + 1;
}


// forward to base class
void BGLView::AllAttached() {
    BView::AllAttached();
}

void BGLView::DetachedFromWindow() {
    BView::DetachedFromWindow();
}

void BGLView::AllDetached() {
    BView::AllDetached();
}

status_t BGLView::Perform(perform_code d, void* arg) {
    return BView::Perform(d, arg);
}

status_t BGLView::Archive(BMessage* data, bool deep) const {
    return BView::Archive(data, deep);
}

void BGLView::MessageReceived(BMessage* msg) {
    BView::MessageReceived(msg);
}

void BGLView::SetResizingMode(uint32 mode) {
    BView::SetResizingMode(mode);
}

void BGLView::Show() {
    BView::Show();
}

void BGLView::Hide() {
    BView::Hide();
}

BHandler* BGLView::ResolveSpecifier(BMessage* msg, int32 index, BMessage* specifier,
        int32 form, const char* property) {
    return BView::ResolveSpecifier(msg, index, specifier, form, property);
}

status_t BGLView::GetSupportedSuites(BMessage* data) {
    return BView::GetSupportedSuites(data);
}

void BGLView::_LockDraw()
{
    if (!fClipInfo || !fClipInfo->enable_direct_mode)
        return;

    fDrawLock.Lock();
}

void BGLView::_UnlockDraw()
{
    if (!fClipInfo || !fClipInfo->enable_direct_mode)
        return;

    fDrawLock.Unlock();
}

//---- virtual reserved methods ----------

void BGLView::_ReservedGLView1() {}
void BGLView::_ReservedGLView2() {}
void BGLView::_ReservedGLView3() {}
void BGLView::_ReservedGLView4() {}
void BGLView::_ReservedGLView5() {}
void BGLView::_ReservedGLView6() {}
void BGLView::_ReservedGLView7() {}
void BGLView::_ReservedGLView8() {}

// #pragma mark -

const char* color_space_name(color_space space)
{
#define C2N(a)    case a:    return #a

    switch (space) {
    C2N(B_RGB24);
    C2N(B_RGB32);
    C2N(B_RGBA32);
    C2N(B_RGB32_BIG);
    C2N(B_RGBA32_BIG);
    C2N(B_GRAY8);
    C2N(B_GRAY1);
    C2N(B_RGB16);
    C2N(B_RGB15);
    C2N(B_RGBA15);
    C2N(B_CMAP8);
    default:
        return "Unknown!";
    };

#undef C2N
};

glview_direct_info::glview_direct_info()
{
    // TODO: See direct_window_data() in app_server's ServerWindow.cpp
    direct_info = (direct_buffer_info*)calloc(1, DIRECT_BUFFER_INFO_AREA_SIZE);
    direct_connected = false;
    enable_direct_mode = false;
}

glview_direct_info::~glview_direct_info()
{
    free(direct_info);
}
