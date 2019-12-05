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

noinst_HEADERS = \
	glvnd_genentry.h \
	libglxabipriv.h \
	libglxcurrent.h \
	libglxmapping.h \
	libglxgl.h \
	libglxthread.h \
	libglxproto.h

lib_LTLIBRARIES = libGLX.la

UTIL_DIR = ../util
UTHASH_DIR = ../util/uthash/src
GL_DISPATCH_DIR = ../GLdispatch

# Warning settings
# Include paths
libGLX_la_CFLAGS =  -I$(srcdir)/$(UTHASH_DIR)
libGLX_la_CFLAGS += -I$(srcdir)/$(UTIL_DIR)
libGLX_la_CFLAGS += -I$(srcdir)/$(GL_DISPATCH_DIR)
libGLX_la_CFLAGS += -I$(top_srcdir)/include
libGLX_la_CFLAGS += $(GLPROTO_CFLAGS) $(X11_CFLAGS) $(XEXT_CFLAGS)

# Required library flags
libGLX_la_CFLAGS += $(PTHREAD_CFLAGS)

# Required libraries
libGLX_la_LIBADD = @LIB_DL@
libGLX_la_LIBADD += $(X11_LIBS)
libGLX_la_LIBADD += $(XEXT_LIBS)
libGLX_la_LIBADD += $(GL_DISPATCH_DIR)/libGLdispatch.la
libGLX_la_LIBADD += $(UTIL_DIR)/libtrace.la
libGLX_la_LIBADD += $(UTIL_DIR)/libglvnd_pthread.la
libGLX_la_LIBADD += $(UTIL_DIR)/libutils_misc.la
libGLX_la_LIBADD += $(UTIL_DIR)/libapp_error_check.la
libGLX_la_LIBADD += $(UTIL_DIR)/libwinsys_dispatch.la

libGLX_la_LDFLAGS = -shared -Wl,-Bsymbolic -version-info 0 $(LINKER_FLAG_NO_UNDEFINED)

GENERATE_DISPATCH_STUBS_SCRIPT = $(top_srcdir)/src/GLX/gen_glx_stubs.py
EXTRA_DIST = $(GENERATE_DISPATCH_STUBS_SCRIPT)

if HAVE_PYTHON
g_glx_dispatch_stub_list.h: $(GENERATE_DISPATCH_STUBS_SCRIPT)
	$(AM_V_GEN)$(PYTHON) $(GENERATE_DISPATCH_STUBS_SCRIPT) > $@
BUILT_SOURCES = g_glx_dispatch_stub_list.h
CLEANFILES = $(BUILT_SOURCES)
endif

libGLX_la_SOURCES = \
	libglx.c \
	libglxmapping.c \
	libglxproto.c \
	glvnd_genentry.c \
	g_glx_dispatch_stub_list.h

pkgconfigdir = $(libdir)/pkgconfig
pkgconfig_DATA = glx.pc

AM_TESTS_ENVIRONMENT = \
	TOP_SRCDIR='$(top_srcdir)' \
	TOP_BUILDDIR='$(top_builddir)' \
	NM='$(NM)' \
	PYTHON='$(PYTHON)'
TESTS = glx-symbol-check.sh

EXTRA_DIST += \
	glx-symbol-check.sh \
	glx.symbols \
	meson.build
