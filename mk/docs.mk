PANDOC = pandoc
PANDOC_FLAGS = -f markdown_github+definition_lists+auto_identifiers+yaml_metadata_block-hard_line_breaks
PDMAN = $(PANDOC) $(PANDOC_FLAGS) -t docs/pdman.lua
PDHTML = $(PANDOC) $(PANDOC_FLAGS) -t html5 --toc --template=docs/template.html -Voutput_basename=$(@F)
FINDLINKS = sed -n 's|^.*\(https\?://[A-Za-z0-9_/.-]*\).*|\1|gp'
CHECKURL = curl -sSI -w '%{http_code}  @1  %{redirect_url}\n' -o /dev/null @1
XARGS_P_FLAG = $(call try-run, printf "1\n2" | xargs -P2 -I@ echo '@', -P$(NPROC))

html-man = public/dterc.html public/dte-syntax.html
html = public/index.html public/releases.html $(html-man)

docs: man html gz
man: docs/dterc.5 docs/dte-syntax.5
html: $(html)
pdf: public/dte.pdf
gz: $(patsubst %, %.gz, $(html) public/style.css)

$(html): docs/template.html | public/style.css

docs/%.5: docs/%.md docs/pdman.lua
	$(E) PANDOC $@
	$(Q) $(PDMAN) -o $@ $<

public/dte.pdf: docs/dte.1 docs/dterc.5 docs/dte-syntax.5 | public/
	$(E) GROFF $@
	$(Q) groff -mandoc -Tpdf $^ > $@

public/index.html: build/docs/index.md | public/screenshot.png
	$(E) PANDOC $@
	$(Q) $(PDHTML) -Mtitle=_ -o $@ $<

build/docs/index.md: README.md docs/keys.md | build/docs/
	$(E) GEN $@
	$(Q) sed '/^Online documentation is/,/^Public License/d' README.md > $@
	$(Q) sed '/^`/s|`\([^`]\+\)`|<kbd>\1</kbd>|g' docs/keys.md >> $@

public/releases.html: CHANGELOG.md | public/
	$(E) PANDOC $@
	$(Q) $(PDHTML) -Mtitle=_ -o $@ $<

$(html-man): public/%.html: docs/%.md
	$(E) PANDOC $@
	$(Q) $(PDHTML) -o $@ $<

public/style.css: docs/layout.css docs/style.css | public/
	$(E) CSSCAT $@
	$(Q) cat $^ > $@

public/screenshot.png: docs/screenshot.png | public/
	$(E) CP $@
	$(Q) cp $< $@

public/%.gz: public/%
	$(E) GZIP $@
	$(Q) gzip -9 < $< > $@

public/:
	$(Q) mkdir -p $@

build/docs/: build/
	$(Q) mkdir -p $@

check-docs: README.md CHANGELOG.md docs/packaging.md docs/contributing.md docs/dterc.md docs/dte-syntax.md
	@$(FINDLINKS) $^ | xargs -I@1 $(XARGS_P_FLAG) $(CHECKURL)


CLEANDIRS += public/
.PHONY: docs man html pdf gz check-docs
