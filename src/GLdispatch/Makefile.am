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
	GLdispatch.h \
	GLdispatchPrivate.h

lib_LTLIBRARIES = libGLdispatch.la

SUBDIRS = vnd-glapi

libGLdispatch_la_CFLAGS = -I$(top_srcdir)/src/util
libGLdispatch_la_CFLAGS += -I$(srcdir)/vnd-glapi
libGLdispatch_la_CFLAGS += -I$(top_srcdir)/include

libGLdispatch_la_LDFLAGS = -shared -version-info 0 $(LINKER_FLAG_NO_UNDEFINED)

if GLDISPATCH_USE_TLS
VERSION_SCRIPT=$(top_srcdir)/src/GLdispatch/export_list_tls.ver
else
VERSION_SCRIPT=$(top_srcdir)/src/GLdispatch/export_list_tsd.ver
endif
EXTRA_libGLdispatch_la_DEPENDENCIES = $(VERSION_SCRIPT)
libGLdispatch_la_LDFLAGS += -Xlinker --version-script=$(VERSION_SCRIPT)

libGLdispatch_la_SOURCES = \
	GLdispatch.c

libGLdispatch_la_LIBADD = vnd-glapi/libglapi.la
libGLdispatch_la_LIBADD += ../util/libtrace.la
libGLdispatch_la_LIBADD += ../util/libglvnd_pthread.la
libGLdispatch_la_LIBADD += ../util/libapp_error_check.la
libGLdispatch_la_LIBADD += @LIB_DL@

EXTRA_DIST = export_list_tls.ver export_list_tsd.ver
