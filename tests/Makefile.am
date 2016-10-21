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

TESTS = \
	testglxgetprocaddress.sh \
	testglxgetprocaddress_genentry.sh \
	testglxmcbasic.sh \
	testglxmcloop.sh \
	testglxmcthreads.sh \
	testglxmclate.sh \
	testglxmcoldlink.sh \
	testglxgetclientstr.sh \
	testglxqueryversion.sh \
	testpatchentrypoints.sh \
	testpatchentrypoints_gldispatch.sh \
	testegldisplay.sh \
	testegldevice.sh \
	testeglgetprocaddress.sh

EXTRA_DIST = $(TESTS) \
	glxenv.sh \
	eglenv.sh \
	json

check_PROGRAMS = \
	testglxgetprocaddress \
	testglxmakecurrent \
	testglxmakecurrent_oldlink \
	testglxgetclientstr \
	testglxqueryversion \
	testpatchentrypoints \
	testpatchentrypoints_gldispatch \
	testegldisplay \
	testegldevice \
	testeglgetprocaddress

# The *_oldlink variant tests that linking against legacy libGL.so works

TESTGLXMAKECURRENT_SOURCES_COMMON = \
	testglxmakecurrent.c \
	test_utils.c

testglxmakecurrent_SOURCES = \
	$(TESTGLXMAKECURRENT_SOURCES_COMMON)

testglxmakecurrent_oldlink_SOURCES = \
	$(TESTGLXMAKECURRENT_SOURCES_COMMON)

testglxmakecurrent_oldlink_LDADD = -lX11 -ldl
testglxmakecurrent_oldlink_LDADD += $(top_builddir)/src/GL/libGL.la
testglxmakecurrent_oldlink_LDADD += $(top_builddir)/src/util/libglvnd_pthread.la
testglxmakecurrent_oldlink_LDADD += $(top_builddir)/src/util/libtrace.la

# Disable annoying "unused" errors
AM_CFLAGS =                                  \
	-Wno-error=unused-variable               \
	-Wno-error=unused-label                  \
	-I$(top_srcdir)/include                  \
	-I$(top_srcdir)/src/util                 \
	-I$(top_srcdir)/src/util/glvnd_pthread


testglxgetprocaddress_LDADD = -lX11
testglxgetprocaddress_LDADD += $(top_builddir)/src/GLX/libGLX.la

testglxmakecurrent_LDADD = -lX11 -ldl
testglxmakecurrent_LDADD += $(top_builddir)/src/GLX/libGLX.la
testglxmakecurrent_LDADD += $(top_builddir)/src/OpenGL/libOpenGL.la
testglxmakecurrent_LDADD += $(top_builddir)/src/util/libglvnd_pthread.la
testglxmakecurrent_LDADD += $(top_builddir)/src/util/libtrace.la

testglxgetclientstr_LDADD = -lX11
testglxgetclientstr_LDADD += $(top_builddir)/src/GLX/libGLX.la
testglxgetclientstr_LDADD += $(top_builddir)/src/OpenGL/libOpenGL.la
testglxgetclientstr_LDADD += $(top_builddir)/src/util/libtrace.la

testglxqueryversion_LDADD = -lX11
testglxqueryversion_LDADD += $(top_builddir)/src/GLX/libGLX.la
testglxqueryversion_LDADD += $(top_builddir)/src/OpenGL/libOpenGL.la

testpatchentrypoints_SOURCES = \
	testpatchentrypoints.c \
	test_utils.c

testpatchentrypoints_LDADD = -lX11 -ldl
testpatchentrypoints_LDADD += $(top_builddir)/src/GLX/libGLX.la
testpatchentrypoints_LDADD += $(top_builddir)/src/OpenGL/libOpenGL.la
testpatchentrypoints_LDADD += $(top_builddir)/src/util/libtrace.la

testpatchentrypoints_gldispatch_SOURCES = \
	testpatchentrypoints_gldispatch.c
testpatchentrypoints_gldispatch_CFLAGS = \
	-I$(top_srcdir)/include \
	-I$(top_srcdir)/src/GLdispatch
testpatchentrypoints_gldispatch_LDADD = $(top_builddir)/src/GLdispatch/libGLdispatch.la
testpatchentrypoints_gldispatch_LDADD += $(top_builddir)/src/OpenGL/libOpenGL.la
testpatchentrypoints_gldispatch_LDADD += dummy/libpatchentrypoints.la
testpatchentrypoints_gldispatch_LDADD += $(top_builddir)/src/util/libutils_misc.la

testegldisplay_SOURCES = \
	testegldisplay.c \
	egl_test_utils.c
testegldisplay_LDADD = $(top_builddir)/src/EGL/libEGL.la

testegldevice_SOURCES = \
	testegldevice.c \
	egl_test_utils.c
testegldevice_LDADD = $(top_builddir)/src/EGL/libEGL.la

testeglgetprocaddress_SOURCES = \
	testeglgetprocaddress.c \
	egl_test_utils.c
testeglgetprocaddress_LDADD = $(top_builddir)/src/EGL/libEGL.la
