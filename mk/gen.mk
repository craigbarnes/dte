GEN_MK = $(lastword $(MAKEFILE_LIST))
FETCH = curl -LSs -o $@
LUA = lua

UCD_FILES = $(addprefix .cache/, \
    UnicodeData.txt EastAsianWidth.txt DerivedCoreProperties.txt )

gen-unidata: $(UCD_FILES)
	$(E) GEN src/util/unidata.h
	$(Q) $(LUA) src/util/unidata.lua $(UCD_FILES) > src/util/unidata.h

$(UCD_FILES): $(GEN_MK) | .cache/
	$(E) FETCH $@
	$(Q) $(FETCH) https://unicode.org/Public/14.0.0/ucd/$(@F)

.cache/:
	@mkdir -p $@


.PHONY: gen-unidata
