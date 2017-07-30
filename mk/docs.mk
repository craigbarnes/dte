man1 := Documentation/$(PROGRAM).1
man7 := Documentation/$(PROGRAM)-syntax.7
man  := $(man1) $(man7)
html := $(addprefix public/, $(addsuffix .html, $(notdir $(man))))
img  := public/screenshot.png
TTMAN = Documentation/ttman$(X)

quiet_cmd_ttman = TTMAN  $@
cmd_ttman = mk/manvars.sh '$<' '$(PROGRAM)' | $(TTMAN) > $@

quiet_cmd_man2html = GROFF  $@
cmd_man2html = groff -mandoc -Thtml $< > $@

quiet_cmd_cp = CP     $@
cmd_cp = cp $< $@

docs: man html img
man: $(man)
html: $(html)
img: $(img)

$(man1): Documentation/$(PROGRAM).txt $(TTMAN)
	$(call cmd,ttman)

$(man7): Documentation/$(PROGRAM)-syntax.txt $(TTMAN)
	$(call cmd,ttman)

$(html): public/%.html: Documentation/% | public/
	$(call cmd,man2html)

$(img): public/%.png: Documentation/%.png | public/
	$(call cmd,cp)

$(TTMAN): %: %.o
	$(call cmd,host_ld,)

Documentation/%.o: Documentation/%.c
	$(call cmd,host_cc)

public/:
	@mkdir -p $@


CLEANFILES += $(man) $(html) $(img) $(TTMAN) $(TTMAN).o
.PHONY: docs man html img
