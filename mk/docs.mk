# TODO: Use "-f gfm+definition_lists" and remove $(QUIET_PANDOC) when
# pandoc 2.13 becomes widely available
PANDOC_FLAGS = -f markdown_github+definition_lists+auto_identifiers+yaml_metadata_block-hard_line_breaks
QUIET_PANDOC = 2>&1 | sed '/^\[WARNING] Deprecated: markdown_github/d' >&2

PANDOC = pandoc
PDMAN = $(PANDOC) $(PANDOC_FLAGS) -t docs/pdman.lua
PDHTML = $(PANDOC) $(PANDOC_FLAGS) -t html5 --toc --template=docs/template.html -Voutput_basename=$(@F)

man = docs/dte.1 docs/dterc.5 docs/dte-syntax.5
html-man = public/dte.html public/dterc.html public/dte-syntax.html
html = public/index.html public/releases.html $(html-man)
css = public/style.css
img = public/logo.svg public/screenshot.png public/favicon.ico

docs: man html htmlgz
man: $(man)
html: $(html) $(css) $(img)
htmlgz: $(patsubst %, %.gz, $(html) $(css) public/favicon.ico)
pdf: public/dte.pdf

$(html-man): public/%.html: docs/%.md
public/index.html: docs/index.yml build/docs/index.md docs/gitlab.md | public/screenshot.png
public/releases.html: docs/releases.yml CHANGELOG.md docs/releases.lua
public/dterc.html public/dte-syntax.html: docs/fix-anchors.lua
public/releases.html: private PDHTML += -L docs/releases.lua
public/dterc.html public/dte-syntax.html: private PDHTML += -L docs/fix-anchors.lua
public/dte.html: private PDHTML += --indented-code-classes=sh

docs/%.1: docs/%.md docs/pdman.lua
	$(E) PANDOC $@
	$(Q) $(PDMAN) -o $@ $< $(QUIET_PANDOC)

docs/%.5: docs/%.md docs/pdman.lua
	$(E) PANDOC $@
	$(Q) $(PDMAN) -o $@ $< $(QUIET_PANDOC)

public/dte.pdf: $(man) | public/
	$(E) GROFF $@
	$(Q) groff -mandoc -Tpdf $^ > $@

build/docs/index.md: docs/index.sed README.md | build/docs/
	$(E) GEN $@
	$(Q) sed -f $^ > $@

$(html): docs/template.html | public/style.css public/logo.svg
	$(E) PANDOC $@
	$(Q) $(PDHTML) -o $@ $(filter-out %.html %.lua, $^) $(QUIET_PANDOC)

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
