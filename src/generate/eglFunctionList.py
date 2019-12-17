"""
Contains a list of EGL functions to generate dispatch functions for.

This is used from gen_egl_dispatch.py.

EGL_FUNCTIONS is a sequence of (name, eglData) pairs, where name is the name
of the function, and eglData is a dictionary containing data about that
function.

The values in the eglData dictionary are:
- method (string):
    How to select a vendor library. See "Method values" below.

- prefix (string):
    This string is prepended to the name of the dispatch function. If
    unspecified, the default is "" (an empty string).

- static (boolean)
  If True, this function should be declared static.

- "public" (boolean)
    If True, the function should be exported from the library. Vendor libraries
    generally should not use this.

- extension (string):
    If specified, this is the name of a macro to check for before defining a
    function. Used for checking for extension macros and such.

- retval (string):
    If specified, this is a C expression with the default value to return if we
    can't find a function to call. By default, it will try to guess from the
    return type: EGL_NO_whatever for the various handle types, NULL for
    pointers, and zero for everything else.

method values:
- "custom"
    The dispatch stub will be hand-written instead of generated.

- "none"
    No dispatch function exists at all, but the function should still have an
    entry in the index array. This is for other functions that a stub may need
    to call that are implemented in libEGL itself.

- "display"
    Select a vendor from an EGLDisplay argument.

- "device"
    Select a vendor from an EGLDeviceEXT argument.

- "current"
    Select the vendor that owns the current context.
"""

def _eglFunc(name, method, inheader, static=False, public=False, prefix="", extension=None, retval=None):
    """
    A convenience function to define an entry in the EGL function list.
    """
    values = {
        "method" : method,
        "prefix" : prefix,
        "extension" : extension,
        "retval" : retval,
        "static" : static,
        "public" : public,
        "inheader" : inheader,
    }
    return (name, values)

def _eglCore(name, method, **kwargs):
    return _eglFunc(name, method, public=True, inheader=False, **kwargs)

def _eglExt(name, method, static=None, **kwargs):
    if (static is None):
        static = (method != "custom")
    inheader = not static
    return _eglFunc(name, method, static=static, inheader=inheader, public=False, **kwargs)

EGL_FUNCTIONS = (
    # EGL_VERSION_1_0
    _eglCore("eglChooseConfig",                      "display"),
    _eglCore("eglCopyBuffers",                       "display"),
    _eglCore("eglCreateContext",                     "display"),
    _eglCore("eglCreatePbufferSurface",              "display"),
    _eglCore("eglCreatePixmapSurface",               "display"),
    _eglCore("eglCreateWindowSurface",               "display"),
    _eglCore("eglDestroyContext",                    "display"),
    _eglCore("eglDestroySurface",                    "display"),
    _eglCore("eglGetConfigAttrib",                   "display"),
    _eglCore("eglGetConfigs",                        "display"),
    _eglCore("eglQueryContext",                      "display"),
    _eglCore("eglQuerySurface",                      "display"),
    _eglCore("eglSwapBuffers",                       "display"),
    _eglCore("eglWaitGL",                            "current", retval="EGL_TRUE"),
    _eglCore("eglWaitNative",                        "current", retval="EGL_TRUE"),
    _eglCore("eglTerminate",                         "display"),
    _eglCore("eglInitialize",                        "display"),

    _eglCore("eglGetCurrentDisplay",                 "custom"),
    _eglCore("eglGetCurrentSurface",                 "custom"),
    _eglCore("eglGetDisplay",                        "custom"),
    _eglCore("eglGetError",                          "custom"),
    _eglCore("eglGetProcAddress",                    "custom"),
    _eglCore("eglMakeCurrent",                       "custom"),
    _eglCore("eglQueryString",                       "custom"),

    # EGL_VERSION_1_1
    _eglCore("eglBindTexImage",                      "display"),
    _eglCore("eglReleaseTexImage",                   "display"),
    _eglCore("eglSurfaceAttrib",                     "display"),
    _eglCore("eglSwapInterval",                      "display"),

    # EGL_VERSION_1_2
    _eglCore("eglCreatePbufferFromClientBuffer",     "display"),
    _eglCore("eglWaitClient",                        "current", retval="EGL_TRUE"),
    _eglCore("eglBindAPI",                           "custom"),
    _eglCore("eglQueryAPI",                          "custom"),
    _eglCore("eglReleaseThread",                     "custom"),

    # EGL_VERSION_1_4
    _eglCore("eglGetCurrentContext",                 "custom"),

    # EGL_VERSION_1_5
    _eglCore("eglCreateSync",                        "display"),
    _eglCore("eglDestroySync",                       "display"),
    _eglCore("eglClientWaitSync",                    "display"),
    _eglCore("eglGetSyncAttrib",                     "display"),
    _eglCore("eglCreateImage",                       "display"),
    _eglCore("eglDestroyImage",                      "display"),
    _eglCore("eglCreatePlatformWindowSurface",       "display"),
    _eglCore("eglCreatePlatformPixmapSurface",       "display"),
    _eglCore("eglWaitSync",                          "display"),
    _eglCore("eglGetPlatformDisplay",                "custom"),

    # EGL_EXT_platform_base
    _eglExt("eglCreatePlatformWindowSurfaceEXT",    "display"),
    _eglExt("eglCreatePlatformPixmapSurfaceEXT",    "display"),
    _eglExt("eglGetPlatformDisplayEXT",             "custom"),

    # EGL_EXT_device_enumeration
    _eglExt("eglQueryDevicesEXT",                   "custom"),

    # EGL_KHR_debug
    _eglExt("eglDebugMessageControlKHR",            "custom"),
    _eglExt("eglQueryDebugKHR",                     "custom"),
    _eglExt("eglLabelObjectKHR",                    "custom"),
)

