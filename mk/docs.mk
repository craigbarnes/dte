man1 := Documentation/$(PROGRAM).1
man7 := Documentation/$(PROGRAM)-syntax.7
man  := $(man1) $(man7)
html := $(addprefix public/, $(addsuffix .html, $(notdir $(man))))
TTMAN = Documentation/ttman$(X)

quiet_cmd_ttman = TTMAN  $@
cmd_ttman = \
    $(SED) -e s/%MAN%/$(shell echo $(basename $(@F)) | tr a-z A-Z)/g \
    -e s/%PROGRAM%/$(PROGRAM)/g \
    < $< | $(TTMAN) > $@

quiet_cmd_man2html = GROFF  $@
cmd_man2html = groff -mandoc -Thtml $< > $@

man: $(man)
html: $(html)

$(man1): Documentation/$(PROGRAM).txt $(TTMAN)
	$(call cmd,ttman)

$(man7): Documentation/$(PROGRAM)-syntax.txt $(TTMAN)
	$(call cmd,ttman)

$(html): public/%.html: Documentation/% | public/
	$(call cmd,man2html)

$(TTMAN): %: %.o
	$(call cmd,host_ld,)

Documentation/%.o: Documentation/%.c
	$(call cmd,host_cc)

public/:
	mkdir -p $@


CLEANFILES += $(man) $(html) $(TTMAN) $(TTMAN).o
.PHONY: man html
