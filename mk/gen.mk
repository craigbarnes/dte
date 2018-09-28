FETCH = curl -LSs -o $@
LUA_DEP = $(filter build/lua/lua, $(LUA))
UCD_FILES = .cache/UnicodeData.txt .cache/EastAsianWidth.txt

gen: gen-xterm-keys

gen-xterm-keys: $(LUA_DEP)
	$(E) GEN src/terminal/xterm-keys.c
	$(Q) $(LUA) src/terminal/xterm-keys.lua > src/terminal/xterm-keys.c

gen-wcwidth: $(UCD_FILES) $(LUA_DEP)
	$(E) GEN src/lookup/wcwidth.c
	$(Q) $(LUA) src/lookup/wcwidth.lua $(UCD_FILES) > src/lookup/wcwidth.c

$(UCD_FILES): | .cache/
	$(E) FETCH $@
	$(Q) $(FETCH) https://unicode.org/Public/11.0.0/ucd/$(@F)

.cache/:
	@mkdir -p $@


.PHONY: gen gen-xterm-keys gen-wcwidth
