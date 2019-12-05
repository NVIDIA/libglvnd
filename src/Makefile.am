SUBDIRS =
SUBDIRS += util
SUBDIRS += GLdispatch
SUBDIRS += OpenGL

if ENABLE_EGL
SUBDIRS += EGL
endif

if ENABLE_GLX
SUBDIRS += GLX
SUBDIRS += GL
endif

if ENABLE_GLES1
SUBDIRS += GLESv1
endif
if ENABLE_GLES2
SUBDIRS += GLESv2
endif

EXTRA_DIST = \
	generate/eglFunctionList.py \
	generate/genCommon.py \
	generate/gen_egl_dispatch.py \
	generate/gen_gldispatch_mapi.py \
	generate/gen_libOpenGL_exports.py \
	generate/gen_libgl_glxstubs.py \
	generate/xml/egl.xml \
	generate/xml/gl.xml \
	generate/xml/gl_other.xml \
	generate/xml/glx.xml \
	generate/xml/glx_other.xml \
	generate/meson.build \
	meson.build

clean-local:
	rm -f generate/*.pyc
