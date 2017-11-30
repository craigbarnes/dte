FINDLINKS = sed -n 's|^.*\(https\?://[A-Za-z0-9_/.-]*\).*|\1|gp'
CHECKURL = curl -sSI -w '%{http_code}  @1  %{redirect_url}\n' -o /dev/null @1
PDMAN = pandoc -f markdown-superscript-smart-tex_math_dollars -t docs/pdman.lua
SED = sed

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

docs/%.5: docs/%.md docs/pdman.lua | build/
	$(E) PDMAN $@
	$(Q) $(PDMAN) -o build/$(@F) $<
	$(Q) $(SED) 's/^$$/./' build/$(@F) > $@

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

public/:
	@mkdir -p $@

check-docs: README.md docs/contributing.md docs/dterc.md docs/dte-syntax.md
	@$(FINDLINKS) $^ | xargs -I@1 $(XARGS_P_FLAG) $(CHECKURL)


CLEANDIRS += public/
.PHONY: docs man web check-docs
