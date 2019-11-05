# Copyright (c) 2013, NVIDIA CORPORATION.
#
# Permission is hereby granted, free of charge, to any person obtaining a
# copy of this software and/or associated documentation files (the
# "Materials"), to deal in the Materials without restriction, including
# without limitation the rights to use, copy, modify, merge, publish,
# distribute, sublicense, and/or sell copies of the Materials, and to
# permit persons to whom the Materials are furnished to do so, subject to
# the following conditions:
#
# The above copyright notice and this permission notice shall be included
# unaltered in all copies or substantial portions of the Materials.
# Any additions, deletions, or changes to the original source files
# must be clearly indicated in accompanying documentation.
#
# If only executable code is distributed, then the accompanying
# documentation must state that "this software is based in part on the
# work of the Khronos Group."
#
# THE MATERIALS ARE PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
# EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
# MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
# IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
# CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
# TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
# MATERIALS OR THE USE OR OTHER DEALINGS IN THE MATERIALS.

SUBDIRS = \
	dummy

noinst_HEADERS = \
	test_utils.h \
	egl_test_utils.h

TESTS_ENVIRONMENT = \
	TOP_SRCDIR=$(top_srcdir) \
	TOP_BUILDDIR=$(top_builddir) \
	ABS_TOP_BUILDDIR=$(abs_top_builddir)

TESTS =
check_PROGRAMS =

EXTRA_DIST = $(TESTS) \
	glxenv.sh \
	eglenv.sh \
	json \
	meson.build

CFLAGS_COMMON = \
	-I$(top_srcdir)/include                  \
	-I$(top_srcdir)/src/util                 \
	-I$(top_srcdir)/src/util/glvnd_pthread

AM_CFLAGS = $(CFLAGS_COMMON)

TESTS += testgldispatch_static.sh
TESTS += testgldispatch_static_thr.sh
TESTS += testgldispatch_generated.sh
TESTS += testgldispatch_generated_thr.sh
TESTS += testgldispatch_patched.sh
TESTS += testgldispatch_patched_thr.sh
check_PROGRAMS += testgldispatch
testgldispatch_SOURCES = \
	testgldispatch.c
testgldispatch_CFLAGS = \
	-I$(top_srcdir)/include \
	-I$(top_srcdir)/src/GLdispatch \
	$(PTHREAD_CFLAGS)
testgldispatch_LDADD = $(top_builddir)/src/GLdispatch/libGLdispatch.la
testgldispatch_LDADD += $(top_builddir)/src/OpenGL/libOpenGL.la
testgldispatch_LDADD += dummy/libpatchentrypoints.la
testgldispatch_LDADD += $(top_builddir)/src/util/libutils_misc.la
testgldispatch_LDADD += $(PTHREAD_LIBS)

TESTS += testgldispatchthread.sh
check_PROGRAMS += testgldispatchthread
testgldispatchthread_SOURCES = \
	testgldispatchthread.c
testgldispatchthread_CFLAGS = \
	-I$(top_srcdir)/include \
	-I$(top_srcdir)/src/GLdispatch \
	$(PTHREAD_CFLAGS)
testgldispatchthread_LDADD = $(top_builddir)/src/GLdispatch/libGLdispatch.la

# Start of GLX-specific tests.
# Notes that the TESTS_GLX variable must be defined outside the conditional, so
# that we can include the test scripts in the EXTRA_DIST package. Otherwise,
# the scripts would be missing when you run "make dist" or "make distcheck".

TESTS_GLX =
TESTS_GLX += testglxcreatecontext.sh
TESTS_GLX += testglxmcbasic.sh
TESTS_GLX += testglxmcloop.sh
TESTS_GLX += testglxmcthreads.sh
TESTS_GLX += testglxmcoldlink.sh
TESTS_GLX += testglxgetprocaddress.sh
TESTS_GLX += testglxgetprocaddress_genentry.sh
TESTS_GLX += testglxgetclientstr.sh
TESTS_GLX += testglxqueryversion.sh

if ENABLE_GLX

TESTS += $(TESTS_GLX)

check_PROGRAMS += testglxcreatecontext
testglxcreatecontext_CFLAGS = $(CFLAGS_COMMON) $(X11_CFLAGS)
testglxcreatecontext_LDADD = $(X11_LIBS)
testglxcreatecontext_LDADD += $(top_builddir)/src/GLX/libGLX.la
testglxcreatecontext_LDADD += $(top_builddir)/src/OpenGL/libOpenGL.la
testglxcreatecontext_SOURCES = \
	testglxcreatecontext.c \
	test_utils.c


TESTGLXMAKECURRENT_SOURCES_COMMON = \
	testglxmakecurrent.c \
	test_utils.c

check_PROGRAMS += testglxmakecurrent
testglxmakecurrent_SOURCES = $(TESTGLXMAKECURRENT_SOURCES_COMMON)
testglxmakecurrent_CFLAGS = $(CFLAGS_COMMON) $(X11_CFLAGS)
testglxmakecurrent_LDADD = $(X11_LIBS) @LIB_DL@
testglxmakecurrent_LDADD += $(top_builddir)/src/GLX/libGLX.la
testglxmakecurrent_LDADD += $(top_builddir)/src/OpenGL/libOpenGL.la
testglxmakecurrent_LDADD += $(top_builddir)/src/util/libglvnd_pthread.la
testglxmakecurrent_LDADD += $(top_builddir)/src/util/libtrace.la
testglxmakecurrent_LDADD += $(top_builddir)/src/util/libutils_misc.la


check_PROGRAMS += testglxmakecurrent_oldlink
testglxmakecurrent_oldlink_SOURCES = $(TESTGLXMAKECURRENT_SOURCES_COMMON)
testglxmakecurrent_oldlink_CFLAGS = $(CFLAGS_COMMON) $(X11_CFLAGS)
testglxmakecurrent_oldlink_LDADD = $(X11_LIBS) @LIB_DL@
testglxmakecurrent_oldlink_LDADD += $(top_builddir)/src/GL/libGL.la
testglxmakecurrent_oldlink_LDADD += $(top_builddir)/src/util/libglvnd_pthread.la
testglxmakecurrent_oldlink_LDADD += $(top_builddir)/src/util/libtrace.la
testglxmakecurrent_oldlink_LDADD += $(top_builddir)/src/util/libutils_misc.la


check_PROGRAMS += testglxgetprocaddress
testglxgetprocaddress_CFLAGS = \
	$(CFLAGS_COMMON) \
	$(X11_CFLAGS) \
	-I$(top_srcdir)/include \
	-I$(top_srcdir)/src/GLdispatch
testglxgetprocaddress_LDADD = $(X11_LIBS)
testglxgetprocaddress_LDADD += $(top_builddir)/src/GLX/libGLX.la
testglxgetprocaddress_LDADD += $(top_builddir)/src/GLdispatch/libGLdispatch.la


check_PROGRAMS += testglxgetprocaddress_genentry
testglxgetprocaddress_genentry_LDADD = -lX11
testglxgetprocaddress_genentry_LDADD += $(top_builddir)/src/GLX/libGLX.la


check_PROGRAMS += testglxgetclientstr
testglxgetclientstr_CFLAGS = $(CFLAGS_COMMON) $(X11_CFLAGS)
testglxgetclientstr_LDADD = $(X11_LIBS)
testglxgetclientstr_LDADD += $(top_builddir)/src/GLX/libGLX.la
testglxgetclientstr_LDADD += $(top_builddir)/src/OpenGL/libOpenGL.la
testglxgetclientstr_LDADD += $(top_builddir)/src/util/libtrace.la
testglxgetclientstr_LDADD += $(top_builddir)/src/util/libutils_misc.la


check_PROGRAMS += testglxqueryversion
testglxqueryversion_CFLAGS = $(CFLAGS_COMMON) $(X11_CFLAGS)
testglxqueryversion_LDADD = $(X11_LIBS)
testglxqueryversion_LDADD += $(top_builddir)/src/GLX/libGLX.la
testglxqueryversion_LDADD += $(top_builddir)/src/OpenGL/libOpenGL.la

endif # ENABLE_GLX


# Start of EGL-specific tests.

TESTS_EGL =
TESTS_EGL += testegldisplay.sh
TESTS_EGL += testegldevice.sh
TESTS_EGL += testeglgetprocaddress.sh
TESTS_EGL += testeglmakecurrent.sh
TESTS_EGL += testeglerror.sh
TESTS_EGL += testegldebug.sh

if ENABLE_EGL

TESTS += $(TESTS_EGL)

check_PROGRAMS += testegldisplay
testegldisplay_SOURCES = \
	testegldisplay.c \
	egl_test_utils.c
testegldisplay_LDADD = $(top_builddir)/src/EGL/libEGL.la

check_PROGRAMS += testegldevice
testegldevice_SOURCES = \
	testegldevice.c \
	egl_test_utils.c
testegldevice_LDADD = $(top_builddir)/src/EGL/libEGL.la

check_PROGRAMS += testeglgetprocaddress
testeglgetprocaddress_SOURCES = \
	testeglgetprocaddress.c \
	egl_test_utils.c
testeglgetprocaddress_LDADD = $(top_builddir)/src/EGL/libEGL.la

check_PROGRAMS += testeglmakecurrent
testeglmakecurrent_SOURCES = \
	testeglmakecurrent.c \
	egl_test_utils.c
testeglmakecurrent_LDADD = $(top_builddir)/src/EGL/libEGL.la
testeglmakecurrent_LDADD += $(top_builddir)/src/OpenGL/libOpenGL.la

check_PROGRAMS += testeglerror
testeglerror_SOURCES = \
	testeglerror.c \
	egl_test_utils.c
testeglerror_LDADD = $(top_builddir)/src/EGL/libEGL.la
testeglerror_LDADD += $(top_builddir)/src/OpenGL/libOpenGL.la


check_PROGRAMS += testegldebug
testegldebug_SOURCES = \
	testegldebug.c \
	egl_test_utils.c
testegldebug_LDADD = $(top_builddir)/src/EGL/libEGL.la

endif # ENABLE_EGL

EXTRA_DIST += $(TESTS_GLX) $(TESTS_EGL)
