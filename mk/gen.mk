FETCH = curl -LSs -o $@
UCD_FILES = .cache/UnicodeData.txt .cache/EastAsianWidth.txt
LUA_DEP = $(if $(call streq,$(USE_LUA),static), $(LUA))

define LPERF_GEN
  $(E) GEN $(1).c
  $(Q) $(LUA) mk/lperf.lua $(1).lua $(1).c
endef

gen: $(LUA_DEP)
	$(call LPERF_GEN, src/lookup/extensions)
	$(call LPERF_GEN, src/lookup/basenames)
	$(call LPERF_GEN, src/lookup/pathnames)
	$(call LPERF_GEN, src/lookup/interpreters)
	$(call LPERF_GEN, src/lookup/ignored-exts)
	$(E) GEN src/terminal/xterm-keys.c
	$(Q) $(LUA) src/terminal/xterm-keys.lua > src/terminal/xterm-keys.c

gen-unicode-tables: build/unicode-tables.c

build/unicode-tables.c: tools/gen-unicode-tables.lua $(UCD_FILES) | $(LUA_DEP)
	$(E) GEN $@
	$(Q) $(LUA) $< $(UCD_FILES) > $@

$(UCD_FILES): | .cache/
	$(E) FETCH $@
	$(Q) $(FETCH) https://unicode.org/Public/11.0.0/ucd/$(@F)

.cache/:
	@mkdir -p $@


.PHONY: gen gen-unicode-tables
