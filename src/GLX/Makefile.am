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
	libglxabipriv.h \
	libglxcurrent.h \
	libglxmapping.h \
	libglxnoop.h \
	libglxgl.h \
	libglxstring.h \
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
libGLX_la_CFLAGS += $(GLPROTO_CFLAGS)

# Required library flags
libGLX_la_CFLAGS += $(PTHREAD_CFLAGS)

# Required libraries
libGLX_la_LIBADD = -ldl
libGLX_la_LIBADD += $(X11_LIBS)
libGLX_la_LIBADD += $(XEXT_LIBS)
libGLX_la_LIBADD += $(GL_DISPATCH_DIR)/libGLdispatch.la
libGLX_la_LIBADD += $(UTIL_DIR)/libtrace.la
libGLX_la_LIBADD += $(UTIL_DIR)/libglvnd_pthread.la
libGLX_la_LIBADD += $(UTIL_DIR)/libglvnd_genentry.la
libGLX_la_LIBADD += $(UTIL_DIR)/libutils_misc.la
libGLX_la_LIBADD += $(UTIL_DIR)/libapp_error_check.la

libGLX_la_LDFLAGS = -shared -Wl,-Bsymbolic -version-info 0 $(LINKER_FLAG_NO_UNDEFINED)

BUILT_SOURCES = g_libglxnoop.c

CLEANFILES = $(BUILT_SOURCES)

EXTRA_DIST = gen_stubs.pl glx_funcs.spec

g_libglxnoop.c : gen_stubs.pl glx_funcs.spec
	$(AM_V_GEN)$(PERL) $(srcdir)/gen_stubs.pl "noop" $(srcdir)/glx_funcs.spec > $@

libGLX_la_SOURCES = \
	libglx.c \
	libglxmapping.c \
	libglxstring.c \
	libglxproto.c \
	g_libglxnoop.c
