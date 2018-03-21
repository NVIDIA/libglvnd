
glapi_gen_gl_xml := \
	$(top_srcdir)/src/generate/xml/gl.xml \
	$(top_srcdir)/src/generate/xml/gl_other.xml
glapi_gen_gl_deps := \
	$(top_srcdir)/src/generate/genCommon.py \
	$(glapi_gen_gl_xml)

glapi_gen_glx_xml := \
	$(top_srcdir)/src/generate/xml/glx.xml \
	$(top_srcdir)/src/generate/xml/glx_other.xml
glapi_gen_glx_deps := \
	$(top_srcdir)/src/generate/genCommon.py \
	$(glapi_gen_glx_xml)

glapi_gen_mapi_script := $(top_srcdir)/src/generate/gen_gldispatch_mapi.py
glapi_gen_mapi_deps := \
	$(glapi_gen_mapi_script) \
	$(glapi_gen_gl_deps)

# glapi_gen_mapi:
# Generates the header file that's used to define all of the public entrypoint
# functions in libGLdispatch.so, libOpenGL.so, and libGL.so.
# $(1) specifies which library we're building, which defines the set of
# functions to include. It should be one of:
# "opengl" for libOpenGL.so
# "gl" for libGL.so
# "gldispatch" for libGLdispatch.so
# "glesv1" for libGLESv1_CM.so
# "glesv2" for libGLESv2.so
define glapi_gen_mapi
$(AM_V_at)$(MKDIR_P) $(@D)
$(AM_V_GEN)$(PYTHON2) $(PYTHON_FLAGS) $(glapi_gen_mapi_script) \
	$(1) $(glapi_gen_gl_xml) > $@
endef

glapi_gen_libglglxstubs_script := $(top_srcdir)/src/generate/gen_libgl_glxstubs.py
glapi_gen_libglglxstubs_deps := \
	$(glapi_gen_libglglxstubs_script) \
	$(glapi_gen_glx_deps)

define glapi_gen_libglglxstubs
$(AM_V_at)$(MKDIR_P) $(@D)
$(AM_V_GEN)$(PYTHON2) $(PYTHON_FLAGS) $(glapi_gen_libglglxstubs_script) \
	$(glapi_gen_glx_xml) > $@
endef

