# Used by OpenGL vendor-neutral dispatch library

EXTRA_DIST = mapi

TOP = $(top_srcdir)
GLAPI = $(top_srcdir)/$(MAPI_PREFIX)/glapi

include $(top_srcdir)/src/GLdispatch/vnd-glapi/entry_files.mk

noinst_LTLIBRARIES = libglapi.la

MAPI_UTIL_FILES = \
	$(TOP)/$(MAPI_PREFIX)/u_execmem.c

MAPI_GLAPI_FILES = \
	$(MAPI_GLDISPATCH_ENTRY_FILES) \
	$(TOP)/$(MAPI_PREFIX)/mapi_glapi.c \
	$(TOP)/$(MAPI_PREFIX)/stub.c \
	$(TOP)/$(MAPI_PREFIX)/table.c \
	$(MAPI_UTIL_FILES)

libglapi_la_SOURCES = \
	$(MAPI_GLAPI_FILES) \
	g_glapi_inittable.c

# Select the appropriate file for looking up the current dispatch table.
if GLDISPATCH_USE_TLS
libglapi_la_SOURCES += $(top_srcdir)/$(MAPI_PREFIX)/u_current_tls.c
else
libglapi_la_SOURCES += $(top_srcdir)/$(MAPI_PREFIX)/u_current_tsd.c
endif

libglapi_la_LDFLAGS = -no-undefined
libglapi_la_LIBADD = $(top_builddir)/src/util/libutils_misc.la

include $(top_srcdir)/src/generate/glvnd_gen.mk
glapi_mapi_tmp.h : $(glapi_gen_mapi_deps)
	$(call glapi_gen_mapi, gldispatch)

g_glapi_inittable.c: $(glapi_gen_initdispatch_deps)
	$(call glapi_gen_initdispatch)

glapitable.h: $(glapi_gen_glapitable_deps)
	$(call glapi_gen_glapitable_header)

BUILT_SOURCES = glapi_mapi_tmp.h g_glapi_inittable.c glapitable.h
CLEANFILES = $(BUILT_SOURCES)

AM_CPPFLAGS = \
	$(DEFINES) \
	-I$(top_srcdir)/include \
	-I$(top_srcdir)/src/util \
	-I$(top_srcdir)/$(MAPI_PREFIX) \
	-I$(top_builddir)/$(MAPI_PREFIX) \
	-I$(top_srcdir)/$(MAPI_PREFIX)/glapi \
	-I$(top_srcdir)/$(MAPI_MESA_PREFIX) \
	-DMAPI_ABI_HEADER=\"$(builddir)/glapi_mapi_tmp.h\"
