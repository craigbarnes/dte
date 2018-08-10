define LPERF_GEN
  $(E) GEN $(1).c
  $(Q) $(LUA) mk/lperf.lua $(1).lua > $(1).c
endef

gen: $(if $(call streq,$(USE_LUA),static), $(LUA))
	$(call LPERF_GEN, src/lookup/extensions)
	$(call LPERF_GEN, src/lookup/basenames)
	$(call LPERF_GEN, src/lookup/pathnames)
	$(call LPERF_GEN, src/lookup/interpreters)
	$(call LPERF_GEN, src/lookup/ignored-exts)
	$(call LPERF_GEN, src/lookup/attributes)
	$(call LPERF_GEN, src/lookup/colors)
	$(call LPERF_GEN, src/lookup/config-cmds)
	$(E) GEN src/lookup/xterm-keys.c
	$(Q) $(LUA) src/lookup/xterm-keys.lua > src/lookup/xterm-keys.c


.PHONY: gen
