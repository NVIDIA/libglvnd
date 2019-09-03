
nobase_include_HEADERS = \
	glvnd/GLdispatchABI.h \
	glvnd/libglxabi.h \
	glvnd/libeglabi.h

noinst_HEADERS = \
	c99_compat.h \
	compiler.h \
	glheader.h \
	glvnd_list.h \
	lkdhash.h

GL_HEADER_FILES = \
	GL/gl.h \
	GL/glcorearb.h \
	GL/glext.h \
	KHR/khrplatform.h

GLES_HEADER_FILES = \
	GLES/egl.h \
	GLES/gl.h \
	GLES/glext.h \
	GLES/glplatform.h \
	GLES2/gl2ext.h \
	GLES2/gl2.h \
	GLES2/gl2platform.h \
	GLES3/gl31.h \
	GLES3/gl32.h \
	GLES3/gl3.h \
	GLES3/gl3platform.h

GLX_HEADER_FILES = \
	GL/glx.h \
	GL/glxext.h

EGL_HEADER_FILES = \
	EGL/egl.h \
	EGL/eglext.h \
	EGL/eglplatform.h

if ENABLE_EGL
nobase_include_HEADERS += $(GL_HEADER_FILES)
else
noinst_HEADERS += $(GL_HEADER_FILES)
endif

if ENABLE_GLES
nobase_include_HEADERS += $(GLES_HEADER_FILES)
else
noinst_HEADERS += $(GLES_HEADER_FILES)
endif

if ENABLE_GLX
nobase_include_HEADERS += $(GLX_HEADER_FILES)
else
noinst_HEADERS += $(GLX_HEADER_FILES)
endif

if ENABLE_EGL
nobase_include_HEADERS += $(EGL_HEADER_FILES)
else
noinst_HEADERS += $(EGL_HEADER_FILES)
endif

