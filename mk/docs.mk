PANDOC = pandoc
PANDOC_FLAGS = -f gfm+definition_lists
PDMAN = $(PANDOC) $(PANDOC_FLAGS) -t docs/pdman.lua
PDHTML = $(PANDOC) $(PANDOC_FLAGS) -t html5 --toc --template=docs/template.html -Voutput_basename=$(@F)

man = docs/dte.1 docs/dterc.5 docs/dte-syntax.5
html-man = public/dte.html public/dterc.html public/dte-syntax.html
html = public/index.html public/releases.html $(html-man)
css = public/style.css
img_tpl = public/logo.svg public/favicon.ico # Images used in template.html
img_ss = public/screenshot.png
img = $(img_tpl) $(img_ss)

docs: man html htmlgz
man: $(man)
html: $(html) $(css) $(img)
htmlgz: $(patsubst %, %.gz, $(html) $(css) public/favicon.ico)
pdf: public/dte.pdf

$(html-man): public/%.html: docs/%.md
public/index.html: docs/index.yml README.md docs/gitlab.md | $(img_ss)
public/releases.html: docs/releases.yml CHANGELOG.md docs/releases.lua
public/dterc.html public/dte-syntax.html: docs/fix-anchors.lua
public/logo.svg: share/dte.svg
public/screenshot.png: docs/screenshot.png
public/favicon.ico: docs/favicon.ico
public/releases.html: private PDHTML += -L docs/releases.lua
public/dterc.html public/dte-syntax.html: private PDHTML += -L docs/fix-anchors.lua
public/dte.html: private PDHTML += --indented-code-classes=sh

docs/%.1: docs/%.md docs/pdman.lua build/docs/pdman.flags
	$(E) PANDOC $@
	$(Q) $(PDMAN) -o $@ $<

docs/%.5: docs/%.md docs/pdman.lua build/docs/pdman.flags
	$(E) PANDOC $@
	$(Q) $(PDMAN) -o $@ $<

$(html): docs/template.html build/docs/pdhtml.flags | $(css) $(img_tpl)
	$(E) PANDOC $@
	$(Q) $(PDHTML) -o $@ $(filter %.md %.yml, $^)

public/style.css: docs/layout.css docs/style.css | public/
	$(E) CSSCAT $@
	$(Q) cat $^ > $@

$(img): | public/
	$(E) CP $@
	$(Q) cp '$<' '$@'

public/%.gz: public/%
	$(E) GZIP $@
	$(Q) gzip -9 < $< > $@

public/dte.pdf: $(man) | public/
	$(E) GROFF $@
	$(Q) groff -mandoc -Tpdf $^ > $@

build/docs/pdman.flags: FORCE | build/docs/
	@$(OPTCHECK) '$(PDMAN)' $@

build/docs/pdhtml.flags: FORCE | build/docs/
	@$(OPTCHECK) '$(PDHTML)' $@

public/:
	$(Q) mkdir -p $@

build/docs/: build/
	$(Q) mkdir -p $@


CLEANDIRS += public/
.PHONY: docs man html htmlgz pdf
