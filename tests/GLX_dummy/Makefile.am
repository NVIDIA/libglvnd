noinst_HEADERS = GLX_dummy.h
check_LTLIBRARIES = libGLX_dummy.la libGLX_patchentry.la

check-local: libGLX_dummy_copy

.PHONY : libGLX_dummy_copy

# HACK to get multiple copies of the dummy library for testing
libGLX_dummy_copy : libGLX_dummy.la
	cp .libs/libGLX_dummy.so.0.0.0 .libs/libGLX_dummy_0.so.0
	cp .libs/libGLX_dummy.so.0.0.0 .libs/libGLX_dummy_1.so.0

COMMON_CFLAGS = \
	-I$(top_srcdir)/src/GLX        \
	-I$(top_srcdir)/src/GLdispatch \
	-I$(top_srcdir)/src/util       \
	-I$(top_srcdir)/src/util/trace \
	-I$(top_srcdir)/include        \
	-Wno-error=unused-function

COMMON_SOURCES = \
	GLX_dummy.c

# The -rpath option forces libtool to create a shared library instead of just a
# static library.
COMMON_LDFLAGS = \
	-shared \
	-rpath /nowhere

libGLX_dummy_la_CFLAGS = $(COMMON_CFLAGS)
libGLX_dummy_la_SOURCES = $(COMMON_SOURCES)
libGLX_dummy_la_LIBADD = $(top_builddir)/src/util/trace/libtrace.la
libGLX_dummy_la_LIBADD += $(top_builddir)/src/util/libutils_misc.la
libGLX_dummy_la_LDFLAGS = $(COMMON_LDFLAGS)

libGLX_patchentry_la_CFLAGS = $(COMMON_CFLAGS) -DPATCH_ENTRYPOINTS
libGLX_patchentry_la_SOURCES = $(COMMON_SOURCES)
libGLX_patchentry_la_LIBADD = $(top_builddir)/src/util/trace/libtrace.la
libGLX_patchentry_la_LIBADD += $(top_builddir)/src/util/libutils_misc.la
libGLX_patchentry_la_LDFLAGS = $(COMMON_LDFLAGS)
