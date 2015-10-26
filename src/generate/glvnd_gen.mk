
glapi_gen_gl_xml := $(top_srcdir)/src/generate/xml/gl.xml
glapi_gen_gl_deps := \
	$(top_srcdir)/src/generate/genCommon.py \
	$(glapi_gen_gl_xml)

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

