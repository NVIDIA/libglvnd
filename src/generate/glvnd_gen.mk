
glapi_gen_glx_xml := \
	$(top_srcdir)/src/generate/xml/glx.xml \
	$(top_srcdir)/src/generate/xml/glx_other.xml
glapi_gen_glx_deps := \
	$(top_srcdir)/src/generate/genCommon.py \
	$(glapi_gen_glx_xml)

glapi_gen_libglglxstubs_script := $(top_srcdir)/src/generate/gen_libgl_glxstubs.py
glapi_gen_libglglxstubs_deps := \
	$(glapi_gen_libglglxstubs_script) \
	$(glapi_gen_glx_deps)

define glapi_gen_libglglxstubs
$(AM_V_at)$(MKDIR_P) $(@D)
$(AM_V_GEN)$(PYTHON2) $(PYTHON_FLAGS) $(glapi_gen_libglglxstubs_script) \
	$(glapi_gen_glx_xml) > $@
endef

