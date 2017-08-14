FINDLINKS = sed -n 's|^.*\(https\?://[A-Za-z0-9_/.-]*\).*|\1|gp'
CHECKURL = curl -sSI -w '%{http_code}  @1  %{redirect_url}\n' -o /dev/null @1
NPROC = $(shell sh mk/nproc.sh)
TTMAN = Documentation/ttman$(X)

XARGS_P_FLAG = $(shell \
    printf "1\n2" | xargs -P2 -I@ echo '@' >/dev/null 2>&1 && \
    echo '-P$(NPROC)' \
)

man1 := Documentation/dte.1
man5 := Documentation/dte-syntax.5
man  := $(man1) $(man5)
html := $(addprefix public/, $(addsuffix .html, $(notdir $(man))))
img  := public/screenshot.png

docs: man html img
man: $(man)
html: $(html)
img: $(img)

$(man1): Documentation/dte.txt $(TTMAN)
	$(E) TTMAN $@
	$(Q) $(SED) 's|%PKGDATADIR%|$(PKGDATADIR)|g' '$<' | $(TTMAN) > $@

$(man5): Documentation/dte-syntax.txt $(TTMAN)
	$(E) TTMAN $@
	$(Q) $(SED) 's|%PKGDATADIR%|$(PKGDATADIR)|g' '$<' | $(TTMAN) > $@

$(html): public/%.html: Documentation/% | public/
	$(E) GROFF $@
	$(Q) groff -mandoc -Thtml $< > $@

$(img): public/%.png: Documentation/%.png | public/
	$(E) CP $@
	$(Q) cp $< $@

$(TTMAN): Documentation/ttman.o
	$(E) HOSTLD $@
	$(Q) $(HOST_LD) $(HOST_LDFLAGS) $(BASIC_HOST_LDFLAGS) -o $@ $^

Documentation/ttman.o: Documentation/ttman.c
	$(E) HOSTCC $@
	$(Q) $(HOST_CC) $(HOST_CFLAGS) $(BASIC_HOST_CFLAGS) -c -o $@ $<

public/:
	@mkdir -p $@

check-docs: README.md CONTRIBUTING.md
	@$(FINDLINKS) $^ | xargs -I@1 $(XARGS_P_FLAG) $(CHECKURL)


CLEANFILES += $(man) $(html) $(img) $(TTMAN) $(TTMAN).o
.PHONY: docs man html img check-docs
