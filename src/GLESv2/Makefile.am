# Copyright (c) 2015, NVIDIA CORPORATION.
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

lib_LTLIBRARIES = libGLESv2.la

MAPI = $(top_srcdir)/$(MAPI_PREFIX)
GLAPI = $(MAPI)/glapi

include $(top_srcdir)/src/GLdispatch/vnd-glapi/entry_files.mk

libGLESv2_la_SOURCES = \
	$(top_srcdir)/src/OpenGL/libopengl.c \
	$(MAPI_GLDISPATCH_ENTRY_FILES) \
	$(MAPI)/stub.c     \
	$(top_srcdir)/src/util/utils_misc.c

BUILT_SOURCES = g_glapi_mapi_glesv2_tmp.h g_glesv2_exports.sym
CLEANFILES = $(BUILT_SOURCES)

include $(top_srcdir)/src/generate/glvnd_gen.mk
g_glapi_mapi_glesv2_tmp.h : $(glapi_gen_mapi_deps)
	$(call glapi_gen_mapi, glesv2)

g_glesv2_exports.sym : $(glapi_gen_libopengl_exports_deps)
	$(call glapi_gen_libopengl_exports, glesv2)

libGLESv2_la_CFLAGS = \
	-I$(top_srcdir)/include

if USE_DT_AUXILIARY
DT_AUX_FLAGS = -Xlinker "--auxiliary=libGLdispatch.so.0"
else
DT_AUX_FLAGS =
endif

EXTRA_libGLESv2_la_DEPENDENCIES = $(builddir)/g_glesv2_exports.sym

libGLESv2_la_LDFLAGS = -shared \
	-version-info 2 \
	$(LINKER_FLAG_NO_UNDEFINED) \
	-export-symbols $(builddir)/g_glesv2_exports.sym \
	$(DT_AUX_FLAGS)

AM_CPPFLAGS = \
	-I$(top_srcdir)/src/GLdispatch/vnd-glapi/mapi \
	-I$(top_srcdir)/src/GLdispatch/ \
	-I$(top_srcdir)/src/util \
	-I$(top_srcdir)/src/util/glvnd_pthread \
	-DMAPI_ABI_HEADER=\"$(builddir)/g_glapi_mapi_glesv2_tmp.h\" \
	-DSTATIC_DISPATCH_ONLY

libGLESv2_la_LIBADD = ../GLdispatch/libGLdispatch.la
