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

# This is the common makefile for libOpenGL.so and libGLES*.so.
# All of thoose are identical except for the set of entrypoints that they
# define.

MAPI = $(top_srcdir)/$(MAPI_PREFIX)
GLAPI = $(MAPI)/glapi

include $(top_srcdir)/src/GLdispatch/vnd-glapi/entry_files.mk
include $(top_srcdir)/src/generate/glvnd_gen.mk

ENTRYPOINT_COMMON_FILES = \
	$(top_srcdir)/src/OpenGL/libopengl.c \
	$(MAPI_GLDISPATCH_ENTRY_FILES) \
	$(MAPI)/stub.c     \
	$(top_srcdir)/src/util/utils_misc.c

noinst_HEADERS = \
	g_glapi_mapi_$(ENTRYPOINT_TARGET)_tmp.h

# TODO: Ideally, we should be able to just include the symbol file in make dist
# so that it doesn't have to be rebuilt. But if we do that, then we'd have to
# use a different path file in LDFLAGS depending on whether or not the file
# was generated.
#EXTRA_DIST = \
#	g_$(ENTRYPOINT_TARGET)_exports.sym

BUILT_SOURCES = \
	g_glapi_mapi_$(ENTRYPOINT_TARGET)_tmp.h \
	g_$(ENTRYPOINT_TARGET)_exports.sym

CLEANFILES = $(BUILT_SOURCES)

g_glapi_mapi_$(ENTRYPOINT_TARGET)_tmp.h : $(glapi_gen_mapi_deps)
	$(call glapi_gen_mapi, $(ENTRYPOINT_TARGET))

g_$(ENTRYPOINT_TARGET)_exports.sym : $(glapi_gen_libopengl_exports_deps)
	$(call glapi_gen_libopengl_exports, $(ENTRYPOINT_TARGET))

if USE_DT_AUXILIARY
DT_AUX_FLAGS = -Xlinker "--auxiliary=libGLdispatch.so.0"
else
DT_AUX_FLAGS =
endif

ENTRYPOINT_COMMON_CFLAGS = \
	-I$(top_srcdir)/include \
	-I$(top_srcdir)/src/GLdispatch/vnd-glapi/mapi \
	-I$(top_srcdir)/src/GLdispatch/ \
	-I$(top_srcdir)/src/util \
	-I$(top_srcdir)/src/util/glvnd_pthread \
	-DMAPI_ABI_HEADER=\"$(builddir)/g_glapi_mapi_$(ENTRYPOINT_TARGET)_tmp.h\" \
	-DSTATIC_DISPATCH_ONLY

ENTRYPOINT_COMMON_DEPENDENCIES = $(builddir)/g_$(ENTRYPOINT_TARGET)_exports.sym

ENTRYPOINT_COMMON_LDFLAGS = -shared \
	$(LINKER_FLAG_NO_UNDEFINED) \
	-export-symbols $(builddir)/g_$(ENTRYPOINT_TARGET)_exports.sym \
	$(DT_AUX_FLAGS)

ENTRYPOINT_COMMON_LIBADD = ../GLdispatch/libGLdispatch.la

