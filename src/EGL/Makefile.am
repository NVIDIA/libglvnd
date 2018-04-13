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

# TODO:
# - Add a configure option for whether to build EGL. Probably add one for GLX,
#   too.

noinst_HEADERS = egldispatchstubs.h \
	libeglabipriv.h \
	libeglcurrent.h \
	libeglmapping.h \
	libeglvendor.h \
	libeglerror.h \
	g_egldispatchstubs.h

lib_LTLIBRARIES = libEGL.la

UTIL_DIR = ../util
UTHASH_DIR = ../util/uthash/src
GL_DISPATCH_DIR = ../GLdispatch

# Warning settings
# Include paths
libEGL_la_CFLAGS =  -I$(srcdir)/$(UTHASH_DIR)
libEGL_la_CFLAGS += -I$(srcdir)/$(UTIL_DIR)
libEGL_la_CFLAGS += -I$(srcdir)/$(GL_DISPATCH_DIR)
libEGL_la_CFLAGS += -I$(top_srcdir)/include

libEGL_la_CFLAGS += \
	-DDEFAULT_EGL_VENDOR_CONFIG_DIRS=\"@sysconfdir@/glvnd/egl_vendor.d:@datadir@/glvnd/egl_vendor.d\"

# Required library flags
libEGL_la_CFLAGS += $(PTHREAD_CFLAGS)

# Required libraries
libEGL_la_LIBADD = @LIB_DL@
libEGL_la_LIBADD += -lm
libEGL_la_LIBADD += $(GL_DISPATCH_DIR)/libGLdispatch.la
libEGL_la_LIBADD += $(UTIL_DIR)/libtrace.la
libEGL_la_LIBADD += $(UTIL_DIR)/libglvnd_pthread.la
libEGL_la_LIBADD += $(UTIL_DIR)/libutils_misc.la
libEGL_la_LIBADD += $(UTIL_DIR)/libcJSON.la
libEGL_la_LIBADD += $(UTIL_DIR)/libwinsys_dispatch.la
libEGL_la_LIBADD += libEGL_dispatch_stubs.la

libEGL_la_LDFLAGS = -shared -Wl,-Bsymbolic -version-info 2:0:1 $(LINKER_FLAG_NO_UNDEFINED)

libEGL_la_SOURCES = \
	libegl.c \
	libeglcurrent.c \
	libeglmapping.c \
	libeglvendor.c \
	libeglerror.c

# The generated EGL dispatch stubs are build independantly of the rest of the
# EGL library. Use a convenience library here to make it easier to move them
# to another directory.

noinst_LTLIBRARIES = libEGL_dispatch_stubs.la
libEGL_dispatch_stubs_la_CFLAGS = -I$(top_srcdir)/include
libEGL_dispatch_stubs_la_SOURCES = \
	egldispatchstubs.c \
	g_egldispatchstubs.c

BUILT_SOURCES = g_egldispatchstubs.c g_egldispatchstubs.h
CLEANFILES = $(BUILT_SOURCES)

GENERATE_DISPATCH_SCRIPT = $(top_srcdir)/src/generate/gen_egl_dispatch.py
GENERATE_LIST_FILES = \
	$(top_srcdir)/src/generate/eglFunctionList.py \
	$(top_srcdir)/src/generate/xml/egl.xml

GENERATE_DEPS = \
	$(top_srcdir)/src/generate/genCommon.py \
	$(GENERATE_DISPATCH_SCRIPT) \
	$(GENERATE_LIST_FILES)

g_egldispatchstubs.c : $(GENERATE_DEPS)
	$(VM_V_GEN)$(PYTHON2) $(GENERATE_DISPATCH_SCRIPT) source $(GENERATE_LIST_FILES) > $@

g_egldispatchstubs.h : $(GENERATE_DEPS)
	$(VM_V_GEN)$(PYTHON2) $(GENERATE_DISPATCH_SCRIPT) header $(GENERATE_LIST_FILES) > $@
