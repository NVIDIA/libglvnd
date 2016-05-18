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


def _glxFunc(name, method, opcode=None, error=None, prefix="",
        display=None, key=None, extension=None, retval=None, static=False,
        public=True, inheader=None, add_mapping=None, nelements=None,
        remove_mapping=None, remove_key=None):

    if (inheader == None):
        # The public functions are already declared in glx.h.
        inheader = (not public)
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

# This list contains all of the GLX functions that are implemented in libGLX
# itself. In addition to generating some of the dispatch functions, this list
# is also used to populate the GLX dispatch index list.

GLX_FUNCTIONS = (
    _glxFunc("glXChooseFBConfig", "screen", "X_GLXGetFBConfigs", add_mapping="config_list", nelements=3),
    _glxFunc("glXCreateContext", "xvisinfo", "X_GLXCreateContext", add_mapping="context"),
    _glxFunc("glXCreateGLXPixmap", "xvisinfo", "X_GLXCreateGLXPixmap", add_mapping="drawable"),
    _glxFunc("glXCreateNewContext", "config", "X_GLXCreateNewContext", add_mapping="context"),
    _glxFunc("glXCreatePbuffer", "config", "X_GLXCreatePbuffer", add_mapping="drawable"),
    _glxFunc("glXCreatePixmap", "config", "X_GLXCreatePixmap", add_mapping="drawable"),
    _glxFunc("glXCreateWindow", "config", "X_GLXCreateWindow", add_mapping="drawable"),
    _glxFunc("glXDestroyGLXPixmap", "drawable", "X_GLXDestroyGLXPixmap", error="GLXBadPixmap", remove_mapping="drawable"),
    _glxFunc("glXDestroyPbuffer", "drawable", "X_GLXDestroyPbuffer", error="GLXBadPbuffer", remove_mapping="drawable"),
    _glxFunc("glXDestroyPixmap", "drawable", "X_GLXDestroyPixmap", error="GLXBadPixmap", remove_mapping="drawable"),
    _glxFunc("glXDestroyWindow", "drawable", "X_GLXDestroyWindow", error="GLXBadWindow", remove_mapping="drawable"),
    _glxFunc("glXGetConfig", "xvisinfo", "X_GLXGetFBConfigs"),
    _glxFunc("glXChooseVisual", "screen", "X_GLXGetVisualConfigs"),
    _glxFunc("glXCopyContext", "context", "X_GLXCopyContext"),
    _glxFunc("glXGetFBConfigAttrib", "config", "X_GLXGetFBConfigs"),
    _glxFunc("glXGetFBConfigs", "screen", "X_GLXGetFBConfigs", add_mapping="config_list", nelements=2),
    _glxFunc("glXGetSelectedEvent", "drawable", "X_GLXGetDrawableAttributes", error="GLXBadDrawable"),
    _glxFunc("glXGetVisualFromFBConfig", "config", "X_GLXGetFBConfigs"),
    _glxFunc("glXIsDirect", "context", "X_GLXIsDirect"),
    _glxFunc("glXQueryExtensionsString", "screen", "X_GLXQueryServerString"),
    _glxFunc("glXQueryServerString", "screen", "X_GLXQueryServerString"),
    _glxFunc("glXQueryContext", "context", "X_GLXQueryContext"),
    _glxFunc("glXQueryDrawable", "drawable", "X_GLXGetDrawableAttributes", error="GLXBadDrawable"),
    _glxFunc("glXSelectEvent", "drawable", "X_GLXChangeDrawableAttributes", error="GLXBadDrawable"),
    _glxFunc("glXSwapBuffers", "drawable", "X_GLXSwapBuffers", error="GLXBadDrawable"),
    _glxFunc("glXUseXFont", "current", "X_GLXUseXFont"),
    _glxFunc("glXWaitGL", "current", "X_GLXWaitGL"),
    _glxFunc("glXWaitX", "current", "X_GLXWaitX"),

    _glxFunc("glXDestroyContext", "custom"),
    _glxFunc("glXGetClientString", "custom"),
    _glxFunc("glXGetCurrentContext", "custom"),
    _glxFunc("glXGetCurrentDisplay", "custom"),
    _glxFunc("glXGetCurrentDrawable", "custom"),
    _glxFunc("glXGetCurrentReadDrawable", "custom"),
    _glxFunc("glXGetProcAddress", "custom"),
    _glxFunc("glXGetProcAddressARB", "custom"),
    _glxFunc("glXMakeContextCurrent", "custom"),
    _glxFunc("glXMakeCurrent", "custom"),
    _glxFunc("glXQueryExtension", "custom"),
    _glxFunc("glXQueryVersion", "custom"),
    _glxFunc("glXImportContextEXT", "custom", public=False),
    _glxFunc("glXFreeContextEXT", "custom", public=False),

)

