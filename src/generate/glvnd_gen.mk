
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

define glapi_gen_mapi
@mkdir -p $(dir $@)
$(AM_V_GEN)$(PYTHON2) $(PYTHON_FLAGS) $(glapi_gen_mapi_script) \
	$(glapi_gen_gl_xml) > $@
endef

glapi_gen_glapitable_script := $(top_srcdir)/src/generate/gl_table.py
glapi_gen_glapitable_deps := \
	$(glapi_gen_glapitable_script) \
	$(glapi_gen_gl_deps)

define glapi_gen_glapitable_header
@mkdir -p $(dir $@)
$(AM_V_GEN)$(PYTHON2) $(PYTHON_FLAGS) $(glapi_gen_glapitable_script) \
	$(glapi_gen_gl_xml) > $@
endef

glapi_gen_initdispatch_script := $(top_srcdir)/src/generate/gl_inittable.py
glapi_gen_initdispatch_deps := \
	$(glapi_gen_initdispatch_script) \
	$(glapi_gen_gl_deps)

define glapi_gen_initdispatch
@mkdir -p $(dir $@)
$(AM_V_GEN)$(PYTHON2) $(PYTHON_FLAGS) $(glapi_gen_initdispatch_script) \
	$(glapi_gen_gl_xml) > $@
endef

glapi_gen_libopengl_exports_script := $(top_srcdir)/src/generate/gen_libOpenGL_exports.py
glapi_gen_libopengl_exports_deps := \
	$(glapi_gen_libopengl_exports_script) \
	$(glapi_gen_gl_deps)

define glapi_gen_libopengl_exports
@mkdir -p $(dir $@)
$(AM_V_GEN)$(PYTHON2) $(PYTHON_FLAGS) $(glapi_gen_libopengl_exports_script) \
	$(top_srcdir)/src/generate/xml/gl.xml > $@
endef

glapi_gen_libglglxstubs_script := $(top_srcdir)/src/generate/gen_libgl_glxstubs.py
glapi_gen_libglglxstubs_deps := \
	$(glapi_gen_libglglxstubs_script) \
	$(glapi_gen_glx_deps)

define glapi_gen_libglglxstubs
@mkdir -p $(dir $@)
$(AM_V_GEN)$(PYTHON2) $(PYTHON_FLAGS) $(glapi_gen_libglglxstubs_script) \
	$(glapi_gen_glx_xml) > $@
endef

