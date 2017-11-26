FINDLINKS = sed -n 's|^.*\(https\?://[A-Za-z0-9_/.-]*\).*|\1|gp'
CHECKURL = curl -sSI -w '%{http_code}  @1  %{redirect_url}\n' -o /dev/null @1
TTMAN = build/ttman$(EXEC_SUFFIX)

XARGS_P_FLAG = $(shell \
    printf "1\n2" | xargs -P2 -I@ echo '@' >/dev/null 2>&1 && \
    echo '-P$(NPROC)' \
)

man  := docs/dte.1 docs/dterc.5 docs/dte-syntax.5
pdf  := public/dte.pdf
img  := public/screenshot.png
html := $(addprefix public/, $(addsuffix .html, $(notdir $(man))))
web  := $(html) $(pdf) $(img) $(patsubst %, %.gz, $(html) $(pdf))

docs: man web
man: docs/dterc.5 docs/dte-syntax.5
web: $(web)

docs/%.5: docs/%.txt $(TTMAN)
	$(E) TTMAN $@
	$(Q) $(TTMAN) < $< > $@

$(pdf): $(man) | public/
	$(E) GROFF $@
	$(Q) groff -mandoc -Tpdf $^ > $@

$(html): public/%.html: docs/% | public/
	$(E) GROFF $@
	$(Q) groff -mandoc -Thtml $< > $@

$(img): public/%.png: docs/%.png | public/
	$(E) CP $@
	$(Q) cp $< $@

public/%.gz: public/%
	$(E) GZIP $@
	$(Q) gzip -9 < $< > $@

$(TTMAN): build/ttman.o
	$(E) HOSTLD $@
	$(Q) $(HOST_CC) $(HOST_LDFLAGS) $(BASIC_HOST_LDFLAGS) -o $@ $^

build/ttman.o: docs/ttman.c | build/
	$(E) HOSTCC $@
	$(Q) $(HOST_CC) $(HOST_CFLAGS) $(BASIC_HOST_CFLAGS) -c -o $@ $<

public/:
	@mkdir -p $@

check-docs: README.md docs/CONTRIBUTING.md
	@$(FINDLINKS) $^ | xargs -I@1 $(XARGS_P_FLAG) $(CHECKURL)


CLEANDIRS += public/
.PHONY: docs man web check-docs
