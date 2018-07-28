GPERF = gperf
GPERF_FILTER = sed -f mk/gperf-filter.sed

define GPERF_GEN
  $(E) GPERF $(1).c
  $(Q) $(GPERF) -m50 $(2) $(1).gperf | $(GPERF_FILTER) > $(1).c
endef

gen: gperf-gen xterm-keys-gen

gperf-gen:
	$(call GPERF_GEN, src/lookup/extensions, -D)
	$(call GPERF_GEN, src/lookup/basenames)
	$(call GPERF_GEN, src/lookup/pathnames, -n)
	$(call GPERF_GEN, src/lookup/interpreters)
	$(call GPERF_GEN, src/lookup/ignored-exts, -n)
	$(call GPERF_GEN, src/lookup/attributes, -n)
	$(call GPERF_GEN, src/lookup/colors, -n)
	$(call GPERF_GEN, src/lookup/config-cmds, -n)

xterm-keys-gen: $(if $(call streq,$(USE_LUA),static), $(LUA))
	$(E) GEN src/lookup/xterm-keys.c
	$(Q) $(LUA) src/lookup/xterm-keys.lua > src/lookup/xterm-keys.c


.PHONY: gen gperf-gen xterm-keys-gen
