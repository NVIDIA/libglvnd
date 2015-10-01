#!/usr/bin/python

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

def _eglFunc(name, method, static=False, public=False, inheader=None, prefix="", extension=None, retval=None):
    """
    A convenience function to define an entry in the EGL function list.
    """
    if (inheader == None):
        inheader = (not public)
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
    return _eglFunc(name, method, public=True, **kwargs)

def _eglExt(name, method, **kwargs):
    return _eglFunc(name, method, public=False, **kwargs)

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

    # TODO: Most of these extensions should be provided by the vendor
    # libraries, not by libEGL. They're here now to make testing everything
    # else easier.

    # EGL_ANDROID_blob_cache
    _eglExt("eglSetBlobCacheFuncsANDROID",          "display"),

    # EGL_ANDROID_native_fence_sync
    _eglExt("eglDupNativeFenceFDANDROID",           "display"),

    # EGL_ANGLE_query_surface_pointer
    _eglExt("eglQuerySurfacePointerANGLE",          "display"),

    # EGL_EXT_device_query
    _eglExt("eglQueryDeviceAttribEXT",              "device"),
    _eglExt("eglQueryDeviceStringEXT",              "device"),
    _eglExt("eglQueryDisplayAttribEXT",             "display"),

    # EGL_EXT_output_base
    _eglExt("eglGetOutputLayersEXT",                "display"),
    _eglExt("eglGetOutputPortsEXT",                 "display"),
    _eglExt("eglOutputLayerAttribEXT",              "display"),
    _eglExt("eglQueryOutputLayerAttribEXT",         "display"),
    _eglExt("eglQueryOutputLayerStringEXT",         "display"),
    _eglExt("eglOutputPortAttribEXT",               "display"),
    _eglExt("eglQueryOutputPortAttribEXT",          "display"),
    _eglExt("eglQueryOutputPortStringEXT",          "display"),

    # EGL_EXT_stream_consumer_egloutput
    _eglExt("eglStreamConsumerOutputEXT",           "display"),

    # EGL_EXT_swap_buffers_with_damage
    _eglExt("eglSwapBuffersWithDamageEXT",          "display"),

    # EGL_HI_clientpixmap
    _eglExt("eglCreatePixmapSurfaceHI",             "display"),

    # EGL_KHR_cl_event2
    _eglExt("eglCreateSync64KHR",                   "display"),

    # EGL_KHR_fence_sync
    _eglExt("eglCreateSyncKHR",                     "display"),
    _eglExt("eglDestroySyncKHR",                    "display"),
    _eglExt("eglClientWaitSyncKHR",                 "display"),
    _eglExt("eglGetSyncAttribKHR",                  "display"),

    # EGL_KHR_image
    _eglExt("eglCreateImageKHR",                    "display"),
    _eglExt("eglDestroyImageKHR",                   "display"),

    # EGL_KHR_image_base
    # eglCreateImageKHR already defined in EGL_KHR_image
    # eglDestroyImageKHR already defined in EGL_KHR_image

    # EGL_KHR_lock_surface
    _eglExt("eglLockSurfaceKHR",                    "display"),
    _eglExt("eglUnlockSurfaceKHR",                  "display"),

    # EGL_KHR_lock_surface3
    _eglExt("eglQuerySurface64KHR",                 "display"),
    # eglLockSurfaceKHR already defined in EGL_KHR_lock_surface
    # eglUnlockSurfaceKHR already defined in EGL_KHR_lock_surface

    # EGL_KHR_partial_update
    _eglExt("eglSetDamageRegionKHR",                "display"),

    # EGL_KHR_reusable_sync
    _eglExt("eglSignalSyncKHR",                     "display"),
    # eglCreateSyncKHR already defined in EGL_KHR_fence_sync
    # eglDestroySyncKHR already defined in EGL_KHR_fence_sync
    # eglClientWaitSyncKHR already defined in EGL_KHR_fence_sync
    # eglGetSyncAttribKHR already defined in EGL_KHR_fence_sync

    # EGL_KHR_stream
    _eglExt("eglCreateStreamKHR",                   "display"),
    _eglExt("eglDestroyStreamKHR",                  "display"),
    _eglExt("eglStreamAttribKHR",                   "display"),
    _eglExt("eglQueryStreamKHR",                    "display"),
    _eglExt("eglQueryStreamu64KHR",                 "display"),

    # EGL_KHR_stream_consumer_gltexture
    _eglExt("eglStreamConsumerGLTextureExternalKHR","display"),
    _eglExt("eglStreamConsumerAcquireKHR",          "display"),
    _eglExt("eglStreamConsumerReleaseKHR",          "display"),

    # EGL_KHR_stream_cross_process_fd
    _eglExt("eglGetStreamFileDescriptorKHR",        "display"),
    _eglExt("eglCreateStreamFromFileDescriptorKHR", "display"),

    # EGL_KHR_stream_fifo
    _eglExt("eglQueryStreamTimeKHR",                "display"),

    # EGL_KHR_stream_producer_eglsurface
    _eglExt("eglCreateStreamProducerSurfaceKHR",    "display"),

    # EGL_KHR_swap_buffers_with_damage
    _eglExt("eglSwapBuffersWithDamageKHR",          "display"),

    # EGL_KHR_wait_sync
    _eglExt("eglWaitSyncKHR",                       "display"),

    # EGL_MESA_drm_image
    _eglExt("eglCreateDRMImageMESA",                "display"),
    _eglExt("eglExportDRMImageMESA",                "display"),

    # EGL_MESA_image_dma_buf_export
    _eglExt("eglExportDMABUFImageQueryMESA",        "display"),
    _eglExt("eglExportDMABUFImageMESA",             "display"),

    # EGL_NOK_swap_region
    _eglExt("eglSwapBuffersRegionNOK",              "display"),

    # EGL_NOK_swap_region2
    _eglExt("eglSwapBuffersRegion2NOK",             "display"),

    # EGL_NV_native_query
    _eglExt("eglQueryNativeDisplayNV",              "display"),
    _eglExt("eglQueryNativeWindowNV",               "display"),
    _eglExt("eglQueryNativePixmapNV",               "display"),

    # EGL_NV_post_sub_buffer
    _eglExt("eglPostSubBufferNV",                   "display"),

    # EGL_NV_stream_sync
    _eglExt("eglCreateStreamSyncNV",                "display"),
)

