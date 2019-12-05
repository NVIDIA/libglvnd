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

libGLESv2_la_SOURCES =
libGLESv2_la_LDFLAGS = -shared \
	$(LINKER_FLAG_NO_UNDEFINED) \
	-version-info 3:0:1

libGLESv2_la_LIBADD = \
	../OpenGL/libopengl_main.la \
	../GLdispatch/libGLdispatch.la \
	../GLdispatch/vnd-glapi/libglapi_glesv2.la \
	../util/libutils_misc.la

pkgconfigdir = $(libdir)/pkgconfig
pkgconfig_DATA = glesv2.pc

AM_TESTS_ENVIRONMENT = \
	TOP_SRCDIR='$(top_srcdir)' \
	TOP_BUILDDIR='$(top_builddir)' \
	NM='$(NM)' \
	PYTHON='$(PYTHON)'
TESTS = glesv2-symbol-check.sh

EXTRA_DIST = \
	glesv2-symbol-check.sh \
	glesv2.symbols \
	meson.build
