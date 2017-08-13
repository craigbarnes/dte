FINDLINKS = sed -n 's|^.*\(https\?://[A-Za-z0-9_/.-]*\).*|\1|gp'
CHECKURL = curl -sSI -w '%{http_code}  @1  %{redirect_url}\n' -o /dev/null @1
NPROC = $(shell sh mk/nproc.sh)
TTMAN = Documentation/ttman$(X)

XARGS_P_FLAG = $(shell \
    printf "1\n2" | xargs -P2 -I@ echo '@' >/dev/null 2>&1 && \
    echo '-P$(NPROC)' \
)

man1 := Documentation/$(PROGRAM).1
man5 := Documentation/$(PROGRAM)-syntax.5
man  := $(man1) $(man5)
html := $(addprefix public/, $(addsuffix .html, $(notdir $(man))))
img  := public/screenshot.png

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

$(man5): Documentation/$(PROGRAM)-syntax.txt $(TTMAN)
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

check-docs: README.md CONTRIBUTING.md
	@$(FINDLINKS) $^ | xargs -I@1 $(XARGS_P_FLAG) $(CHECKURL)


CLEANFILES += $(man) $(html) $(img) $(TTMAN) $(TTMAN).o
.PHONY: docs man html img check-docs
