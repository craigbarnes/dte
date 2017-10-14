FINDLINKS = sed -n 's|^.*\(https\?://[A-Za-z0-9_/.-]*\).*|\1|gp'
CHECKURL = curl -sSI -w '%{http_code}  @1  %{redirect_url}\n' -o /dev/null @1
TTMAN = build/ttman$(EXEC_SUFFIX)

XARGS_P_FLAG = $(shell \
    printf "1\n2" | xargs -P2 -I@ echo '@' >/dev/null 2>&1 && \
    echo '-P$(NPROC)' \
)

man1 := dte.1
man5 := dte-syntax.5
man  := $(man1) $(man5)

html := $(addprefix public/, $(addsuffix .html, $(notdir $(man))))
img  := public/screenshot.png
web  := $(html) $(img) $(patsubst %, %.gz, $(html))

docs: man web
man: $(man)
web: $(web)

$(man1): Documentation/dte.txt $(TTMAN)
	$(E) TTMAN $@
	$(Q) $(TTMAN) < $< > $@

$(man5): Documentation/dte-syntax.txt $(TTMAN)
	$(E) TTMAN $@
	$(Q) $(TTMAN) < $< > $@

$(html): public/%.html: % | public/
	$(E) GROFF $@
	$(Q) groff -mandoc -Thtml $< > $@

$(img): public/%.png: Documentation/%.png | public/
	$(E) CP $@
	$(Q) cp $< $@

public/%.gz: public/%
	$(E) GZIP $@
	$(Q) gzip -9 < $< > $@

$(TTMAN): build/ttman.o
	$(E) HOSTLD $@
	$(Q) $(HOST_CC) $(HOST_LDFLAGS) $(BASIC_HOST_LDFLAGS) -o $@ $^

build/ttman.o: Documentation/ttman.c | build/
	$(E) HOSTCC $@
	$(Q) $(HOST_CC) $(HOST_CFLAGS) $(BASIC_HOST_CFLAGS) -c -o $@ $<

public/:
	@mkdir -p $@

check-docs: README.md CONTRIBUTING.md
	@$(FINDLINKS) $^ | xargs -I@1 $(XARGS_P_FLAG) $(CHECKURL)


CLEANFILES += $(man)
CLEANDIRS += public/
.PHONY: docs man web check-docs
