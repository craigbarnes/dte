GPERF = gperf
GPERF_FILTER = sed -f mk/gperf-filter.sed
GPERF_GEN = $(GPERF) -m50 $(2) $(1).gperf | $(GPERF_FILTER) > $(1).c

gperf-gen:
	$(call GPERF_GEN, src/lookup/extensions, -D)
	$(call GPERF_GEN, src/lookup/basenames, -n)
	$(call GPERF_GEN, src/lookup/interpreters)
	$(call GPERF_GEN, src/lookup/ignored-exts, -n)
	$(call GPERF_GEN, src/lookup/attributes, -n)
	$(call GPERF_GEN, src/lookup/colors, -n)

xterm-keys-gen: $(if $(call streq,$(USE_LUA),static), $(LUA))
	$(LUA) mk/triegen.lua > src/lookup/xterm-keys.c


.PHONY: gperf-gen xterm-keys-gen
