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

#ifndef __LIB_EGL_ABI_PRIV__
#define __LIB_EGL_ABI_PRIV__

/*
 * This is a wrapper around libeglabi which defines each vendor's static
 * dispatch table.  Logically this could differ from the API imports provided
 * by the vendor, though in practice they are one and the same.
 */

#include "glvnd/libeglabi.h"

/*!
 * This structure stores function pointers for all functions defined in EGL 1.5.
 */
typedef struct __EGLdispatchTableStaticRec {
    EGLBoolean (* initialize) (EGLDisplay dpy, EGLint *major, EGLint *minor);

    EGLBoolean (* chooseConfig) (EGLDisplay dpy, const EGLint *attrib_list, EGLConfig *configs, EGLint config_size, EGLint *num_config);
    EGLBoolean (* copyBuffers) (EGLDisplay dpy, EGLSurface surface, EGLNativePixmapType target);
    EGLContext (* createContext) (EGLDisplay dpy, EGLConfig config, EGLContext share_context, const EGLint *attrib_list);
    EGLSurface (* createPbufferSurface) (EGLDisplay dpy, EGLConfig config, const EGLint *attrib_list);
    EGLSurface (* createPixmapSurface) (EGLDisplay dpy, EGLConfig config, EGLNativePixmapType pixmap, const EGLint *attrib_list);
    EGLSurface (* createWindowSurface) (EGLDisplay dpy, EGLConfig config, EGLNativeWindowType win, const EGLint *attrib_list);
    EGLBoolean (* destroyContext) (EGLDisplay dpy, EGLContext ctx);
    EGLBoolean (* destroySurface) (EGLDisplay dpy, EGLSurface surface);
    EGLBoolean (* getConfigAttrib) (EGLDisplay dpy, EGLConfig config, EGLint attribute, EGLint *value);
    EGLBoolean (* getConfigs) (EGLDisplay dpy, EGLConfig *configs, EGLint config_size, EGLint *num_config);
    EGLBoolean (* makeCurrent) (EGLDisplay dpy, EGLSurface draw, EGLSurface read, EGLContext ctx);
    EGLBoolean (* queryContext) (EGLDisplay dpy, EGLContext ctx, EGLint attribute, EGLint *value);
    const char *(* queryString) (EGLDisplay dpy, EGLint name);
    EGLBoolean (* querySurface) (EGLDisplay dpy, EGLSurface surface, EGLint attribute, EGLint *value);
    EGLBoolean (* swapBuffers) (EGLDisplay dpy, EGLSurface surface);
    EGLBoolean (* terminate) (EGLDisplay dpy);
    EGLBoolean (* waitGL) (void);
    EGLBoolean (* waitNative) (EGLint engine);
    EGLBoolean (* bindTexImage) (EGLDisplay dpy, EGLSurface surface, EGLint buffer);
    EGLBoolean (* releaseTexImage) (EGLDisplay dpy, EGLSurface surface, EGLint buffer);
    EGLBoolean (* surfaceAttrib) (EGLDisplay dpy, EGLSurface surface, EGLint attribute, EGLint value);
    EGLBoolean (* swapInterval) (EGLDisplay dpy, EGLint interval);

    EGLBoolean (* bindAPI) (EGLenum api);
    EGLSurface (* createPbufferFromClientBuffer) (EGLDisplay dpy, EGLenum buftype, EGLClientBuffer buffer, EGLConfig config, const EGLint *attrib_list);
    EGLBoolean (* releaseThread) (void);
    EGLBoolean (* waitClient) (void);

    EGLint (* getError) (void);

#if 0
    EGLDisplay (* getCurrentDisplay) (void);
    EGLSurface (* getCurrentSurface) (EGLint readdraw);
    EGLDisplay (* getDisplay) (EGLNativeDisplayType display_id);
    EGLContext (* getCurrentContext) (void);
#endif

    // EGL 1.5 functions. A vendor library is not requires to implement these.
    EGLSync (* createSync) (EGLDisplay dpy, EGLenum type, const EGLAttrib *attrib_list);
    EGLBoolean (* destroySync) (EGLDisplay dpy, EGLSync sync);
    EGLint (* clientWaitSync) (EGLDisplay dpy, EGLSync sync, EGLint flags, EGLTime timeout);
    EGLBoolean (* getSyncAttrib) (EGLDisplay dpy, EGLSync sync, EGLint attribute, EGLAttrib *value);
    EGLImage (* createImage) (EGLDisplay dpy, EGLContext ctx, EGLenum target, EGLClientBuffer buffer, const EGLAttrib *attrib_list);
    EGLBoolean (* destroyImage) (EGLDisplay dpy, EGLImage image);
    EGLSurface (* createPlatformWindowSurface) (EGLDisplay dpy, EGLConfig config, void *native_window, const EGLAttrib *attrib_list);
    EGLSurface (* createPlatformPixmapSurface) (EGLDisplay dpy, EGLConfig config, void *native_pixmap, const EGLAttrib *attrib_list);
    EGLBoolean (* waitSync) (EGLDisplay dpy, EGLSync sync, EGLint flags);
    //EGLDisplay (* getPlatformDisplay) (EGLenum platform, void *native_display, const EGLAttrib *attrib_list);

    // Extension functions that libEGL cares about.
    EGLBoolean (* queryDevicesEXT) (EGLint max_devices, EGLDeviceEXT *devices, EGLint *num_devices);


    EGLint (* debugMessageControlKHR) (EGLDEBUGPROCKHR callback, const EGLAttrib *attrib_list);
    EGLBoolean (* queryDebugKHR) (EGLint attribute, EGLAttrib* value);
    EGLint (* labelObjectKHR) (EGLDisplay display, EGLenum objectType, EGLObjectKHR object, EGLLabelKHR label);
} __EGLdispatchTableStatic;

#endif
