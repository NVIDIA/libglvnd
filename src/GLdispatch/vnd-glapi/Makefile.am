# Used by OpenGL vendor-neutral dispatch library

EXTRA_DIST = mapi

TOP = $(top_srcdir)
GLAPI = $(top_srcdir)/$(MAPI_PREFIX)/glapi

noinst_LTLIBRARIES = libglapi.la

MAPI_UTIL_FILES = \
	$(TOP)/$(MAPI_PREFIX)/u_current.c \
	$(TOP)/$(MAPI_PREFIX)/u_execmem.c

MAPI_GLAPI_FILES = \
	$(TOP)/$(MAPI_PREFIX)/entry.c \
	$(TOP)/$(MAPI_PREFIX)/mapi_glapi.c \
	$(TOP)/$(MAPI_PREFIX)/stub.c \
	$(TOP)/$(MAPI_PREFIX)/table.c \
	$(TOP)/$(MAPI_PREFIX)/entryhelpers.c \
	$(MAPI_UTIL_FILES) \
	$(TOP)/src/util/utils_misc.c

libglapi_la_SOURCES = \
	$(MAPI_GLAPI_FILES) \
	g_glapi_inittable.c

libglapi_la_LIBADD = $(SELINUX_LIBS)
libglapi_la_LDFLAGS = -no-undefined

include $(GLAPI)/gen/glapi_gen.mk
glapi_mapi_tmp.h : $(GLAPI)/gen/gl_and_es_API.xml $(glapi_gen_mapi_deps)
	$(call glapi_gen_mapi,$<,vnd-glapi)

g_glapi_inittable.c: $(GLAPI)/gen/gl_and_es_API.xml $(glapi_gen_initdispatch_deps)
	$(call glapi_gen_initdispatch,$<)

glapitable.h: $(GLAPI)/gen/gl_and_es_API.xml $(glapi_gen_dispatch_deps)
	$(call glapi_gen_glapitable_header, $<)

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
