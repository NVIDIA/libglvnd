# Helpers for glapi header generation

glapi_gen_common_deps := \
	$(wildcard $(top_srcdir)/$(MAPI_PREFIX)/glapi/gen/*.xml) \
	$(wildcard $(top_srcdir)/$(MAPI_PREFIX)/glapi/gen/*.py)

glapi_gen_mapi_script := $(top_srcdir)/$(MAPI_PREFIX)/mapi_abi.py
glapi_gen_mapi_deps := \
	$(glapi_gen_mapi_script) \
	$(glapi_gen_common_deps)

# $(1): path to an XML file
# $(2): name of the printer
define glapi_gen_mapi
@mkdir -p $(dir $@)
$(AM_V_GEN)$(PYTHON2) $(PYTHON_FLAGS) $(glapi_gen_mapi_script) \
	--mode lib --printer $(2) $(1) > $@
endef

glapi_gen_dispatch_script := $(top_srcdir)/$(MAPI_PREFIX)/glapi/gen/gl_table.py
glapi_gen_dispatch_deps := $(glapi_gen_common_deps)

# $(1): path to an XML file
# $(2): empty, es1, or es2 for entry point filtering
define glapi_gen_dispatch
@mkdir -p $(dir $@)
$(AM_V_GEN)$(PYTHON2) $(PYTHON_FLAGS) $(glapi_gen_dispatch_script) \
	-f $(1) -m remap_table $(if $(2),-c $(2),) > $@
endef

# $(1): path to an XML file
define glapi_gen_glapitable_header
@mkdir -p $(dir $@)
$(AM_V_GEN)$(PYTHON2) $(PYTHON_FLAGS) $(glapi_gen_dispatch_script) \
	-f $(1) > $@
endef

glapi_gen_initdispatch_script := $(top_srcdir)/$(MAPI_PREFIX)/glapi/gen/gl_inittable.py
glapi_gen_initdispatch_deps := $(glapi_gen_common_deps)

# $(1): path to an XML file
define glapi_gen_initdispatch
@mkdir -p $(dir $@)
$(AM_V_GEN)$(PYTHON2) $(PYTHON_FLAGS) $(glapi_gen_initdispatch_script) \
	-f $(1) > $@
endef

glapi_gen_remap_script := $(top_srcdir)/$(MAPI_PREFIX)/glapi/gen/remap_helper.py
glapi_gen_remap_deps := $(glapi_gen_common_deps)

# $(1): path to an XML file
# $(2): empty, es1, or es2 for entry point filtering
define glapi_gen_remap
@mkdir -p $(dir $@)
$(AM_V_GEN)$(PYTHON2) $(PYTHON_FLAGS) $(glapi_gen_remap_script) \
	-f $(1) $(if $(2),-c $(2),) > $@
endef
