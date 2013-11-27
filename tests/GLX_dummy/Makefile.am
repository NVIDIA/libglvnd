noinst_HEADERS = GLX_dummy.h
lib_LTLIBRARIES = libGLX_dummy.la

check-local: libGLX_dummy_copy

.PHONY : libGLX_dummy_copy

# HACK to get multiple copies of the dummy library for testing
libGLX_dummy_copy : libGLX_dummy.la
	cp .libs/libGLX_dummy.so.0.0.0 .libs/libGLX_dummy_0.so.0
	cp .libs/libGLX_dummy.so.0.0.0 .libs/libGLX_dummy_1.so.0


libGLX_dummy_la_CFLAGS =           \
	-I$(top_srcdir)/src/GLX        \
	-I$(top_srcdir)/src/GLdispatch \
	-I$(top_srcdir)/src/util       \
	-I$(top_srcdir)/src/util/trace \
	-I$(top_srcdir)/include        \
	-Wno-error=unused-function
libGLX_dummy_la_SOURCES = GLX_dummy.c
libGLX_dummy_la_LIBADD = $(top_builddir)/src/util/trace/libtrace.la
