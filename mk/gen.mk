FETCH = curl -LSs -o $@
LUA = lua

UCD_FILES = $(addprefix .cache/, \
    UnicodeData.txt EastAsianWidth.txt DerivedCoreProperties.txt )

gen-wcwidth: $(UCD_FILES)
	$(E) GEN src/util/wcwidth.c
	$(Q) $(LUA) src/util/wcwidth.lua $(UCD_FILES) > src/util/wcwidth.c

$(UCD_FILES): | .cache/
	$(E) FETCH $@
	$(Q) $(FETCH) https://unicode.org/Public/12.1.0/ucd/$(@F)

.cache/:
	@mkdir -p $@


.PHONY: gen-wcwidth
