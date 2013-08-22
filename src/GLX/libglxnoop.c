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

#include "libglxabipriv.h"
#include "libglxnoop.h"

#define GLXNOOP static __attribute__((unused))
#define NOOP_FUNC(func) __glX ## func ## Noop
#include "libglxnoopdefs.h"

const __GLXdispatchTableStatic __glXDispatchNoop = {
    .glx14ep = {
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
        .getClientString       = __glXGetClientStringNoop,
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
    }
};


const __GLXdispatchTableStatic *__glXDispatchNoopPtr = &__glXDispatchNoop;
