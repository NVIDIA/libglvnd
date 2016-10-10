noinst_HEADERS = \
	patchentrypoints.h \
	GLX_dummy.h

check_LTLIBRARIES = \
	libpatchentrypoints.la \
	libGLX_dummy.la
 
libpatchentrypoints_la_CFLAGS = \
	-I$(top_srcdir)/src/GLdispatch \
	-I$(top_srcdir)/src/util       \
	-I$(top_srcdir)/include
libpatchentrypoints_la_SOURCES = \
	patchentrypoints.c

libGLX_dummy_la_CFLAGS = \
	-I$(top_srcdir)/src/GLX        \
	-I$(top_srcdir)/src/GLdispatch \
	-I$(top_srcdir)/src/util       \
	-I$(top_srcdir)/include
libGLX_dummy_la_SOURCES = \
	GLX_dummy.c
libGLX_dummy_la_LIBADD = $(top_builddir)/src/util/libtrace.la
libGLX_dummy_la_LIBADD += $(top_builddir)/src/util/libutils_misc.la
libGLX_dummy_la_LIBADD += libpatchentrypoints.la
# The -rpath option forces libtool to create a shared library instead of just a
# static library.
libGLX_dummy_la_LDFLAGS = \
	-shared \
	-rpath /nowhere

