noinst_HEADERS = GLX_dummy.h
check_LTLIBRARIES = libGLX_dummy.la


libGLX_dummy_la_CFLAGS = \
	-I$(top_srcdir)/src/GLX        \
	-I$(top_srcdir)/src/GLdispatch \
	-I$(top_srcdir)/src/util       \
	-I$(top_srcdir)/include        \
	-Wno-error=unused-function
libGLX_dummy_la_SOURCES = \
	GLX_dummy.c
libGLX_dummy_la_LIBADD = $(top_builddir)/src/util/libtrace.la
libGLX_dummy_la_LIBADD += $(top_builddir)/src/util/libutils_misc.la
# The -rpath option forces libtool to create a shared library instead of just a
# static library.
libGLX_dummy_la_LDFLAGS = \
	-shared \
	-rpath /nowhere

