lib_LTLIBRARIES = libGLX_dummy.la

check-local: libGLX_dummy_copy

.PHONY : libGLX_dummy_copy

# HACK to get multiple copies of the dummy library for testing
libGLX_dummy_copy : libGLX_dummy.la
	cp .libs/libGLX_dummy.so.0.0.0 .libs/libGLX_dummy_0.so.0
	cp .libs/libGLX_dummy.so.0.0.0 .libs/libGLX_dummy_1.so.0

GLX_ABI_DIR = $(top_builddir)/src/GLX

libGLX_dummy_la_CFLAGS =             \
	-I$(GLX_ABI_DIR)                 \
	-I$(top_builddir)/src/util       \
	-I$(top_builddir)/src/util/trace \
	-I$(top_builddir)/include        \
	-Wno-error=unused-function
libGLX_dummy_la_SOURCES = GLX_dummy.c
libGLX_dummy_la_LIBADD = $(top_builddir)/src/util/trace/libtrace.la
