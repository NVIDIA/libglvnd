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
	GLX_dummy

TESTS_ENVIRONMENT = \
	TOP_BUILDDIR=$(top_builddir) \
	ABS_TOP_BUILDDIR=$(abs_top_builddir) \
	DISPLAY=:0

TESTS = \
	init_test_env.sh \
	testglxgetprocaddress.sh \
	testglxmcbasic.sh \
	testglxmcloop.sh \
	testglxmcthreads.sh \
	testglxmclate.sh \
	fini_test_env.sh

check_PROGRAMS = \
	testglxgetprocaddress \
	testglxmakecurrent

X11GLVND_DIR = $(top_builddir)/src/x11glvnd

testglxmakecurrent_SOURCES = \
	testglxmakecurrent.c \
	test_utils.c

# Disable annoying "unused" errors
AM_CFLAGS =                                  \
	-Wno-error=unused-variable               \
	-Wno-error=unused-label                  \
	-I$(top_builddir)/include                \
	-I$(top_builddir)/src/util               \
	-I$(top_builddir)/src/util/trace         \
	-I$(top_builddir)/src/util/glvnd_pthread


testglxgetprocaddress_LDADD = $(top_builddir)/src/GLX/libGLX.la

testglxmakecurrent_LDADD = -lX11
testglxmakecurrent_LDADD += $(top_builddir)/src/GLX/libGLX.la
testglxmakecurrent_LDADD += $(top_builddir)/src/OpenGL/libOpenGL.la
testglxmakecurrent_LDADD += $(top_builddir)/src/util/trace/libtrace.la
testglxmakecurrent_LDADD += $(top_builddir)/src/util/glvnd_pthread/libglvnd_pthread.la
