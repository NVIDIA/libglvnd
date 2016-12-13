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

if ENABLE_GLES
SUBDIRS += GLESv1
SUBDIRS += GLESv2
endif

EXTRA_DIST = generate

clean-local:
	rm -f generate/*.pyc
