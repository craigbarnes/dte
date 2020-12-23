PANDOC = pandoc
PANDOC_FLAGS = -f markdown_github+definition_lists+auto_identifiers+yaml_metadata_block-hard_line_breaks
PDMAN = $(PANDOC) $(PANDOC_FLAGS) -t docs/pdman.lua
PDHTML = $(PANDOC) $(PANDOC_FLAGS) -t html5 --toc --template=docs/template.html -Voutput_basename=$(@F)

public/dte.html: PANDOC_FLAGS += --indented-code-classes=sh

man = docs/dte.1 docs/dterc.5 docs/dte-syntax.5
html-man = public/dte.html public/dterc.html public/dte-syntax.html
html = public/index.html public/releases.html $(html-man)
css = public/style.css
img = public/screenshot.png public/favicon.ico

docs: man html htmlgz
man: $(man)
html: $(html) $(css) $(img)
htmlgz: $(patsubst %, %.gz, $(html) $(css) public/favicon.ico)
pdf: public/dte.pdf

$(html): docs/template.html | public/style.css

docs/%.1 docs/%.5: docs/%.md docs/pdman.lua
	$(E) PANDOC $@
	$(Q) $(PDMAN) -o $@ $<

public/dte.pdf: $(man) | public/
	$(E) GROFF $@
	$(Q) groff -mandoc -Tpdf $^ > $@

public/index.html: docs/index.yml build/docs/index.md docs/gitlab.md | public/
	$(E) PANDOC $@
	$(Q) $(PDHTML) -o $@ $(filter-out docs/template.html, $^)

build/docs/index.md: docs/index.sed README.md | build/docs/
	$(E) GEN $@
	$(Q) sed -f $^ > $@

public/releases.html: docs/releases.yml CHANGELOG.md | public/
	$(E) PANDOC $@
	$(Q) $(PDHTML) -o $@ $(filter-out docs/template.html, $^)

$(html-man): public/%.html: docs/%.md docs/fix-anchors.lua
	$(E) PANDOC $@
	$(Q) $(PDHTML) --lua-filter=docs/fix-anchors.lua -o $@ $<

public/style.css: docs/layout.css docs/style.css | public/
	$(E) CSSCAT $@
	$(Q) cat $^ > $@

$(img): public/%: docs/% | public/
	$(E) CP $@
	$(Q) cp $< $@

public/%.gz: public/%
	$(E) GZIP $@
	$(Q) gzip -9 < $< > $@

public/:
	$(Q) mkdir -p $@

build/docs/: build/
	$(Q) mkdir -p $@


CLEANDIRS += public/
.PHONY: docs man html htmlgz pdf
