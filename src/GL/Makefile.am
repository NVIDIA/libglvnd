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

lib_LTLIBRARIES = libGL.la

TOP = $(top_srcdir)
MAPI = $(top_srcdir)/$(MAPI_PREFIX)
GLAPI = $(MAPI)/glapi

noinst_HEADERS =        \
	libgl.h             \
	glapi_mapi_tmp.h

libGL_la_SOURCES =      \
	libgl.c             \
	$(MAPI)/entry.c     \
	$(MAPI)/stub.c      \
	g_libglglxwrapper.c \
	$(top_srcdir)/src/util/utils_misc.c

BUILT_SOURCES = glapi_mapi_tmp.h g_libglglxwrapper.c
CLEANFILES = $(BUILT_SOURCES)

include $(GLAPI)/gen/glapi_gen.mk

glapi_mapi_tmp.h : $(GLAPI)/gen/gl_and_es_API.xml $(glapi_gen_mapi_deps)
	$(call glapi_gen_mapi,$<,vnd-glapi)

GLX_GEN_STUBS = $(top_srcdir)/src/GLX/gen_stubs.pl
GLX_FUNCS_SPEC = $(top_srcdir)/src/GLX/glx_funcs.spec

g_libglglxwrapper.c : $(GLX_GEN_STUBS) $(GLX_FUNCS_SPEC)
	$(AM_V_GEN)$(PERL) $(GLX_GEN_STUBS) "glwrap" $(GLX_FUNCS_SPEC) > $@

libGL_la_CFLAGS = \
	-I$(top_srcdir)/include

if USE_DT_AUXILIARY
# XXX: libtool has trouble processing "-Wl,*" options, so we use -Xlinker here
# instead.
DT_AUX_FLAGS = -Xlinker "--auxiliary=libGLX.so.0" -Xlinker "--auxiliary=libGLdispatch.so.0"
else
DT_AUX_FLAGS =
endif

libGL_la_LDFLAGS = -shared -version-info 1 $(DT_AUX_FLAGS)

AM_CPPFLAGS = \
	-I$(TOP)/src/GLdispatch/vnd-glapi/mapi \
	-I$(TOP)/src/GLX \
	-I$(TOP)/src/GLdispatch \
	-I$(TOP)/src/util \
	-I$(TOP)/src/util/glvnd_pthread \
	-DMAPI_ABI_HEADER=\"$(builddir)/glapi_mapi_tmp.h\" \
	-DSTATIC_DISPATCH_ONLY

libGL_la_LIBADD  = ../GLX/libGLX.la
libGL_la_LIBADD += ../GLdispatch/libGLdispatch.la
libGL_la_LIBADD += -ldl
