PANDOC = pandoc
PANDOC_FLAGS = -f gfm+definition_lists+yaml_metadata_block-hard_line_breaks
PDMAN = $(PANDOC) $(PANDOC_FLAGS) -t docs/pdman.lua
PDHTML = $(PANDOC) $(PANDOC_FLAGS) -t html5 --toc --template=docs/template.html -Voutput_basename=$(@F)

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

$(html-man): public/%.html: docs/%.md
public/index.html: docs/index.yml build/docs/index.md docs/gitlab.md
public/releases.html: docs/releases.yml CHANGELOG.md
public/dterc.html public/dte-syntax.html: docs/fix-anchors.lua
public/dterc.html public/dte-syntax.html: PANDOC_FLAGS += -L docs/fix-anchors.lua
public/dte.html: PANDOC_FLAGS += --indented-code-classes=sh

docs/%.1: docs/%.md docs/pdman.lua
	$(E) PANDOC $@
	$(Q) $(PDMAN) -o $@ $<

docs/%.5: docs/%.md docs/pdman.lua
	$(E) PANDOC $@
	$(Q) $(PDMAN) -o $@ $<

public/dte.pdf: $(man) | public/
	$(E) GROFF $@
	$(Q) groff -mandoc -Tpdf $^ > $@

build/docs/index.md: docs/index.sed README.md | build/docs/
	$(E) GEN $@
	$(Q) sed -f $^ > $@

$(html): docs/template.html | public/style.css
	$(E) PANDOC $@
	$(Q) $(PDHTML) -o $@ $(filter-out %.html %.lua, $^)

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
