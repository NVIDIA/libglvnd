# Used by OpenGL vendor-neutral dispatch library

EXTRA_DIST = mapi

TOP = $(top_srcdir)
GLAPI = $(top_srcdir)/$(MAPI_PREFIX)/glapi
include $(srcdir)/mapi/Makefile.sources

noinst_LTLIBRARIES = libglapi.la

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
	-I$(top_srcdir)/$(MAPI_PREFIX) \
	-I$(top_builddir)/$(MAPI_PREFIX) \
	-I$(top_srcdir)/$(MAPI_PREFIX)/glapi \
	-I$(top_srcdir)/$(MAPI_MESA_PREFIX) \
	-DMAPI_MODE_GLAPI \
	-DMAPI_ABI_HEADER=\"$(builddir)/glapi_mapi_tmp.h\"
