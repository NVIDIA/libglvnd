# Copyright (c) 2016, NVIDIA CORPORATION.
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

noinst_HEADERS = glxdispatchstubs.h

EXTRA_DIST = example_glx_function_list.py gen_glx_dispatch.py

BUILT_SOURCES = g_glxdispatchstubs.h g_glxdispatchstubs.c

CLEANFILES = $(BUILT_SOURCES)

GEN_DISPATCH_SCRIPT = $(top_srcdir)/src/vendor/gen_glx_dispatch.py
DISPATCH_FUNC_LIST = $(top_srcdir)/src/GLX/libglx_function_list.py
DISPATCH_XML_FILES = $(top_srcdir)/src/generate/xml/glx.xml
GEN_DISPATCH_DEPS = \
	$(GEN_DISPATCH_SCRIPT) \
	$(DISPATCH_FUNC_LIST) \
	$(DISPATCH_XML_FILES) \
	$(top_srcdir)/src/generate/genCommon.py

g_glxdispatchstubs.h : $(GEN_DISPATCH_DEPS)
	$(AM_V_GEN) PYTHONPATH=$(top_srcdir)/src/generate $(PYTHON2) \
		$(GEN_DISPATCH_SCRIPT) header $(DISPATCH_FUNC_LIST) \
		$(DISPATCH_XML_FILES) > $@

g_glxdispatchstubs.c : $(GEN_DISPATCH_DEPS)
	$(AM_V_GEN) PYTHONPATH=$(top_srcdir)/src/generate $(PYTHON2) \
		$(GEN_DISPATCH_SCRIPT) source $(DISPATCH_FUNC_LIST) \
		$(DISPATCH_XML_FILES) > $@

noinst_LTLIBRARIES = libglxdispatchstubs.la
libglxdispatchstubs_la_CFLAGS = -I$(top_srcdir)/include
libglxdispatchstubs_la_SOURCES = \
	glxdispatchstubs.c \
	g_glxdispatchstubs.c

