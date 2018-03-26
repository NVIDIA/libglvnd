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
	json

# The *_oldlink variant tests that linking against legacy libGL.so works

# Disable annoying "unused" errors
AM_CFLAGS =                                  \
	-Wno-error=unused-variable               \
	-Wno-error=unused-label                  \
	-I$(top_srcdir)/include                  \
	-I$(top_srcdir)/src/util                 \
	-I$(top_srcdir)/src/util/glvnd_pthread


TESTS += testgldispatch_static.sh
TESTS += testgldispatch_generated.sh
TESTS += testgldispatch_patched.sh
check_PROGRAMS += testgldispatch
testgldispatch_SOURCES = \
	testgldispatch.c
testgldispatch_CFLAGS = \
	-I$(top_srcdir)/include \
	-I$(top_srcdir)/src/GLdispatch
testgldispatch_LDADD = $(top_builddir)/src/GLdispatch/libGLdispatch.la
testgldispatch_LDADD += $(top_builddir)/src/OpenGL/libOpenGL.la
testgldispatch_LDADD += dummy/libpatchentrypoints.la
testgldispatch_LDADD += $(top_builddir)/src/util/libutils_misc.la

# Start of GLX-specific tests.
# Notes that the TESTS_GLX variable must be defined outside the conditional, so
# that we can include the test scripts in the EXTRA_DIST package. Otherwise,
# the scripts would be missing when you run "make dist" or "make distcheck".

TESTS_GLX =
TESTS_GLX += testglxcreatecontext.sh
TESTS_GLX += testglxmcbasic.sh
TESTS_GLX += testglxmcloop.sh
TESTS_GLX += testglxmcthreads.sh
TESTS_GLX += testglxmclate.sh
TESTS_GLX += testglxmcoldlink.sh
TESTS_GLX += testglxgetprocaddress.sh
TESTS_GLX += testglxgetprocaddress_genentry.sh
TESTS_GLX += testglxgetclientstr.sh
TESTS_GLX += testglxqueryversion.sh
TESTS_GLX += testpatchentrypoints.sh

if ENABLE_GLX

TESTS += $(TESTS_GLX)

check_PROGRAMS += testglxcreatecontext
testglxcreatecontext_LDADD = -lX11
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
testglxmakecurrent_LDADD = -lX11 @LIB_DL@
testglxmakecurrent_LDADD += $(top_builddir)/src/GLX/libGLX.la
testglxmakecurrent_LDADD += $(top_builddir)/src/OpenGL/libOpenGL.la
testglxmakecurrent_LDADD += $(top_builddir)/src/util/libglvnd_pthread.la
testglxmakecurrent_LDADD += $(top_builddir)/src/util/libtrace.la


check_PROGRAMS += testglxmakecurrent_oldlink
testglxmakecurrent_oldlink_SOURCES = $(TESTGLXMAKECURRENT_SOURCES_COMMON)
testglxmakecurrent_oldlink_LDADD = -lX11 @LIB_DL@
testglxmakecurrent_oldlink_LDADD += $(top_builddir)/src/GL/libGL.la
testglxmakecurrent_oldlink_LDADD += $(top_builddir)/src/util/libglvnd_pthread.la
testglxmakecurrent_oldlink_LDADD += $(top_builddir)/src/util/libtrace.la


check_PROGRAMS += testglxgetprocaddress
testglxgetprocaddress_LDADD = -lX11
testglxgetprocaddress_LDADD += $(top_builddir)/src/GLX/libGLX.la


check_PROGRAMS += testglxgetclientstr
testglxgetclientstr_LDADD = -lX11
testglxgetclientstr_LDADD += $(top_builddir)/src/GLX/libGLX.la
testglxgetclientstr_LDADD += $(top_builddir)/src/OpenGL/libOpenGL.la
testglxgetclientstr_LDADD += $(top_builddir)/src/util/libtrace.la


check_PROGRAMS += testglxqueryversion
testglxqueryversion_LDADD = -lX11
testglxqueryversion_LDADD += $(top_builddir)/src/GLX/libGLX.la
testglxqueryversion_LDADD += $(top_builddir)/src/OpenGL/libOpenGL.la


check_PROGRAMS += testpatchentrypoints
testpatchentrypoints_SOURCES = \
	testpatchentrypoints.c \
	test_utils.c

testpatchentrypoints_LDADD = -lX11 @LIB_DL@
testpatchentrypoints_LDADD += $(top_builddir)/src/GLX/libGLX.la
testpatchentrypoints_LDADD += $(top_builddir)/src/OpenGL/libOpenGL.la
testpatchentrypoints_LDADD += $(top_builddir)/src/util/libtrace.la

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
