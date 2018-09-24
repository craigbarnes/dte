FETCH = curl -LSs -o $@
UCD_TOOL = tools/gen-unicode-tables.lua
UCD_FILES = .cache/UnicodeData.txt .cache/EastAsianWidth.txt
LUA_DEP = $(filter build/lua/lua, $(LUA))

GEN_LOOKUP_NAMES = basenames extensions ignored-exts interpreters pathnames
GEN_LOOKUP_TARGETS = $(addprefix gen-lookup-, $(GEN_LOOKUP_NAMES))

gen: $(GEN_LOOKUP_TARGETS) gen-xterm-keys

$(GEN_LOOKUP_TARGETS): gen-lookup-%: $(LUA_DEP)
	$(E) GEN src/lookup/$*.c
	$(Q) $(LUA) mk/lperf.lua src/lookup/$*.lua src/lookup/$*.c

gen-xterm-keys: $(LUA_DEP)
	$(E) GEN src/terminal/xterm-keys.c
	$(Q) $(LUA) src/terminal/xterm-keys.lua > src/terminal/xterm-keys.c

gen-unicode-tables: build/unicode-tables.c

build/unicode-tables.c: $(UCD_TOOL) $(UCD_FILES) | $(LUA_DEP) build/
	$(E) GEN $@
	$(Q) $(LUA) $(UCD_TOOL) $(UCD_FILES) > $@

$(UCD_FILES): | .cache/
	$(E) FETCH $@
	$(Q) $(FETCH) https://unicode.org/Public/11.0.0/ucd/$(@F)

.cache/:
	@mkdir -p $@


.PHONY: gen gen-xterm-keys gen-unicode-tables $(GEN_LOOKUP_TARGETS)
