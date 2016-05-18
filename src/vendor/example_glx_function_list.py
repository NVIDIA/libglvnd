#!/usr/bin/python

# Copyright (c) 2016, NVIDIA CORPORATION.
#
# Permission is hereby granted, free of charge, to any person obtaining a
# copy of this software and/or associated documentation files (the
# "Materials"), to deal in the Materials without restriction, including
# without limitation the rights to use, copy, modify, merge, publish,
# distribute, sublicense, and/or sell copies of the Materials, and to
# permit persons to whom the Materials are furnished to do so, subject to
# the following conditions:
#
# The above copyright notice and this permission notice shall be included
# unaltered in all copies or substantial portions of the Materials.
# Any additions, deletions, or changes to the original source files
# must be clearly indicated in accompanying documentation.
#
# If only executable code is distributed, then the accompanying
# documentation must state that "this software is based in part on the
# work of the Khronos Group."
#
# THE MATERIALS ARE PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
# EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
# MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
# IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
# CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
# TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
# MATERIALS OR THE USE OR OTHER DEALINGS IN THE MATERIALS.

"""
The GLX function list, used by gen_glx_dispatch.py.

GLX Function values:
- "method"
    How to select a vendor library. See "Method values" below.

- "add_mapping"
    Specifies that the return value should be added to one of libGLX's
    mappings. See "add_mapping values" below.

- "remove_mapping"
    Specifies that the return value should be removed to one of libGLX's
    mappings.

    Allowed values are "context", "drawable", and "config".

- "remove_key"
    The parameter that contains the value to remove from libGLX's mapping.
    If unspecified, the default is the same as the "key" field.

- "prefix"
    If True, the function should be exported from the library. This is used for
    generating dispatch stubs for libGLX itself -- vendor libraries generally
    should not use it.

- "static"
    If this is True, then the function will be declared static.

- "public"
    If True, the function should be exported from the library. Vendor libraries
    generally should not use this.

- "display"
    The name of the parameter that contains the Display pointer.
    May be "<current>" to specify the current display.
    If not specified, it will look for a parameter with the correct type.

- "extension"
    The name of an extension macro to check for before defining the function.

- "inheader"
    If True, then this function should be declared in the generated header
    file.

method values:
- "custom"
    The dispatch stub will be hand-written instead of generated.

- "none"
    No dispatch function exists at all, but the function should still have an
    entry in the index array. This is for other functions that a stub may need
    to call that are implemented in libGLX itself.

- "current"
    Use the current context.

- "drawable", "context", "config"
    Look up by a GLXDrawable, GLXContext, or GLXFBConfig parameter.
    The parameter is given by the "key" field. If unspecified, it will use the
    first parameter of the correct type that it finds.

- "screen"
    Get a screen number from a parameter.
    The parameter name is given by the "key" field. If unspecifid, the default
    parameter name is "screen".

- "xvisinfo"
    Use an XVisualInfo pointer from a parameter.
    The parameter name is given by the "key" field. If unspecifid, it will look
    for the first XVisualInfo parameter.

add_mapping values:
- "context", "drawable", "config"
    Adds the returned GLXContext, GLXDrawable, or GLXConfig.

- "config_list"
    Adds an array of GLXFBConfig values. In this case, you also need to specify
    a pointer to the item count in the "nelements" field.
"""

def _glxFunc(name, method, opcode=None, error=None, prefix="dispatch_",
        display=None, key=None, extension=None, retval=None, static=None,
        public=False, inheader=None, add_mapping=None, nelements=None,
        remove_mapping=None, remove_key=None):

    if (static == None):
        static = (method not in ("none", "custom"))
    if (inheader == None):
        inheader = (method != "none" and not static)
    values = {
        "method" : method,
        "opcode" : opcode,
        "error" : error,
        "key" : key,
        "prefix" : prefix,
        "display" : display,
        "extension" : extension,
        "retval" : retval,
        "static" : static,
        "public" : public,
        "inheader" : inheader,
        "add_mapping" : add_mapping,
        "nelements" : nelements,
        "remove_mapping" : remove_mapping,
        "remove_key" : remove_key,
    }
    return (name, values)

GLX_FUNCTIONS = (
    # GLX core functions. These are all implemented in libGLX, but are listed
    # here to keep track of the dispatch indexes for them.
    _glxFunc("glXChooseFBConfig", "none"),
    _glxFunc("glXCreateContext", "none"),
    _glxFunc("glXCreateGLXPixmap", "none"),
    _glxFunc("glXCreateNewContext", "none"),
    _glxFunc("glXCreatePbuffer", "none"),
    _glxFunc("glXCreatePixmap", "none"),
    _glxFunc("glXCreateWindow", "none"),
    _glxFunc("glXDestroyGLXPixmap", "none"),
    _glxFunc("glXDestroyPbuffer", "none"),
    _glxFunc("glXDestroyPixmap", "none"),
    _glxFunc("glXDestroyWindow", "none"),
    _glxFunc("glXGetConfig", "none"),
    _glxFunc("glXChooseVisual", "none"),
    _glxFunc("glXCopyContext", "none"),
    _glxFunc("glXGetFBConfigAttrib", "none"),
    _glxFunc("glXGetFBConfigs", "none"),
    _glxFunc("glXGetSelectedEvent", "none"),
    _glxFunc("glXGetVisualFromFBConfig", "none"),
    _glxFunc("glXIsDirect", "none"),
    _glxFunc("glXQueryExtensionsString", "none"),
    _glxFunc("glXQueryServerString", "none"),
    _glxFunc("glXQueryContext", "none"),
    _glxFunc("glXQueryDrawable", "none"),
    _glxFunc("glXSelectEvent", "none"),
    _glxFunc("glXSwapBuffers", "none"),
    _glxFunc("glXUseXFont", "none"),
    _glxFunc("glXWaitGL", "none"),
    _glxFunc("glXWaitX", "none"),
    _glxFunc("glXDestroyContext", "none"),
    _glxFunc("glXGetClientString", "none"),
    _glxFunc("glXGetCurrentContext", "none"),
    _glxFunc("glXGetCurrentDisplay", "none"),
    _glxFunc("glXGetCurrentDrawable", "none"),
    _glxFunc("glXGetCurrentReadDrawable", "none"),
    _glxFunc("glXGetProcAddress", "none"),
    _glxFunc("glXGetProcAddressARB", "none"),
    _glxFunc("glXMakeContextCurrent", "none"),
    _glxFunc("glXMakeCurrent", "none"),
    _glxFunc("glXQueryExtension", "none"),
    _glxFunc("glXQueryVersion", "none"),

    # GLX_ARB_create_context
    _glxFunc("glXCreateContextAttribsARB", "config", "X_GLXCreateContextAtrribsARB", add_mapping="context"),

    # GLX_EXT_import_context
    _glxFunc("glXImportContextEXT", "none"), # Implemented in libGLX
    _glxFunc("glXFreeContextEXT", "none"), # Implemented in libGLX
    _glxFunc("glXGetContextIDEXT", "context", None),
    _glxFunc("glXGetCurrentDisplayEXT", "current"),
    _glxFunc("glXQueryContextInfoEXT", "context", "X_GLXVendorPrivateWithReply"),

    # GLX_EXT_texture_from_pixmap
    _glxFunc("glXBindTexImageEXT", "drawable", "X_GLXVendorPrivate", error="GLXBadPixmap"),
    _glxFunc("glXReleaseTexImageEXT", "drawable", "X_GLXVendorPrivate", error="GLXBadPixmap"),

    # GLX_EXT_swap_control
    _glxFunc("glXSwapIntervalEXT", "drawable", "X_GLXVendorPrivate", error="GLXBadDrawable"),

    # GLX_SGI_video_sync
    _glxFunc("glXGetVideoSyncSGI", "current"),
    _glxFunc("glXWaitVideoSyncSGI", "current"),

    # GLX_SGI_swap_control
    _glxFunc("glXSwapIntervalSGI", "current"),

    # GLX_SGIX_swap_barrier
    _glxFunc("glXBindSwapBarrierSGIX", "drawable"),
    _glxFunc("glXQueryMaxSwapBarriersSGIX", "screen"),

    # GLX_SGIX_video_resize
    _glxFunc("glXBindChannelToWindowSGIX", "screen"),
    _glxFunc("glXChannelRectSGIX", "screen"),
    _glxFunc("glXQueryChannelRectSGIX", "screen"),
    _glxFunc("glXQueryChannelDeltasSGIX", "screen"),
    _glxFunc("glXChannelRectSyncSGIX", "screen"),

    # GLX_SGIX_fbconfig
    _glxFunc("glXCreateContextWithConfigSGIX", "config", "X_GLXVendorPrivateWithReply", add_mapping="context"),
    _glxFunc("glXGetFBConfigAttribSGIX", "config", "X_GLXVendorPrivateWithReply"),
    _glxFunc("glXChooseFBConfigSGIX", "screen", add_mapping="config_list", nelements=3),
    _glxFunc("glXCreateGLXPixmapWithConfigSGIX", "config", "X_GLXVendorPrivateWithReply", add_mapping="drawable"),
    _glxFunc("glXGetVisualFromFBConfigSGIX", "config", "X_GLXVendorPrivateWithReply"),
    _glxFunc("glXGetFBConfigFromVisualSGIX", "xvisinfo"),

    # GLX_SGIX_pbuffer
    _glxFunc("glXCreateGLXPbufferSGIX", "config", "X_GLXVendorPrivateWithReply", add_mapping="drawable"),
    _glxFunc("glXDestroyGLXPbufferSGIX", "drawable", "X_GLXVendorPrivateWithReply", remove_mapping="drawable"),
    _glxFunc("glXQueryGLXPbufferSGIX", "drawable", "X_GLXVendorPrivateWithReply"),
    _glxFunc("glXSelectEventSGIX", "drawable", "X_GLXVendorPrivateWithReply"),
    _glxFunc("glXGetSelectedEventSGIX", "drawable", "X_GLXVendorPrivateWithReply"),
)
