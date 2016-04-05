# Used by OpenGL vendor-neutral dispatch library

AUTOMAKE_OPTIONS = subdir-objects
EXTRA_DIST = mapi

MAPI_PREFIX = src/GLdispatch/vnd-glapi/mapi

include $(top_srcdir)/src/GLdispatch/vnd-glapi/entry_files.mk

noinst_HEADERS =

noinst_LTLIBRARIES = libglapi.la

libglapi_la_SOURCES = \
	$(MAPI_GLDISPATCH_ENTRY_FILES) \
	mapi/mapi_glapi.c \
	mapi/stub.c \
	mapi/table.c \
	mapi/u_execmem.c \
	g_glapi_inittable.c

# Select the appropriate file for looking up the current dispatch table.
if GLDISPATCH_USE_TLS
libglapi_la_SOURCES += mapi/u_current_tls.c
else
libglapi_la_SOURCES += mapi/u_current_tsd.c
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

COMMON_CPPFLAGS = \
	$(DEFINES) \
	-I$(top_srcdir)/include \
	-I$(top_srcdir)/src/util \
	-I$(top_srcdir)/$(MAPI_PREFIX) \
	-I$(top_builddir)/$(MAPI_PREFIX) \
	-I$(top_srcdir)/$(MAPI_PREFIX)/glapi \
	-I$(top_srcdir)/$(MAPI_MESA_PREFIX)

libglapi_la_CPPFLAGS = \
	$(COMMON_CPPFLAGS) \
	-DMAPI_ABI_HEADER=\"$(builddir)/glapi_mapi_tmp.h\"


ENTRYPOINT_SOURCES = \
	$(MAPI_GLDISPATCH_ENTRY_FILES) \
	mapi/stub.c

ENTRYPOINT_CPPFLAGS = \
	$(COMMON_CPPFLAGS) \
	-DSTATIC_DISPATCH_ONLY

noinst_LTLIBRARIES += libglapi_gl.la
libglapi_gl_la_SOURCES = $(ENTRYPOINT_SOURCES)
libglapi_gl_la_CPPFLAGS = $(ENTRYPOINT_CPPFLAGS) \
	-DMAPI_ABI_HEADER=\"$(builddir)/g_glapi_mapi_gl_tmp.h\"
noinst_HEADERS += g_glapi_mapi_gl_tmp.h
BUILT_SOURCES += g_glapi_mapi_gl_tmp.h
g_glapi_mapi_gl_tmp.h : $(glapi_gen_mapi_deps)
	$(call glapi_gen_mapi, gl)

noinst_LTLIBRARIES += libglapi_opengl.la
libglapi_opengl_la_SOURCES = $(ENTRYPOINT_SOURCES)
libglapi_opengl_la_CPPFLAGS = $(ENTRYPOINT_CPPFLAGS) \
	-DMAPI_ABI_HEADER=\"$(builddir)/g_glapi_mapi_opengl_tmp.h\"
noinst_HEADERS += g_glapi_mapi_opengl_tmp.h
BUILT_SOURCES += g_glapi_mapi_opengl_tmp.h
g_glapi_mapi_opengl_tmp.h : $(glapi_gen_mapi_deps)
	$(call glapi_gen_mapi, opengl)

noinst_LTLIBRARIES += libglapi_glesv1.la
libglapi_glesv1_la_SOURCES = $(ENTRYPOINT_SOURCES)
libglapi_glesv1_la_CPPFLAGS = $(ENTRYPOINT_CPPFLAGS) \
	-DMAPI_ABI_HEADER=\"$(builddir)/g_glapi_mapi_glesv1_tmp.h\"
noinst_HEADERS += g_glapi_mapi_glesv1_tmp.h
BUILT_SOURCES += g_glapi_mapi_glesv1_tmp.h
g_glapi_mapi_glesv1_tmp.h : $(glapi_gen_mapi_deps)
	$(call glapi_gen_mapi, glesv1)

noinst_LTLIBRARIES += libglapi_glesv2.la
libglapi_glesv2_la_SOURCES = $(ENTRYPOINT_SOURCES)
libglapi_glesv2_la_CPPFLAGS = $(ENTRYPOINT_CPPFLAGS) \
	-DMAPI_ABI_HEADER=\"$(builddir)/g_glapi_mapi_glesv2_tmp.h\"
noinst_HEADERS += g_glapi_mapi_glesv2_tmp.h
BUILT_SOURCES += g_glapi_mapi_glesv2_tmp.h
g_glapi_mapi_glesv2_tmp.h : $(glapi_gen_mapi_deps)
	$(call glapi_gen_mapi, glesv2)

CLEANFILES = $(BUILT_SOURCES)
