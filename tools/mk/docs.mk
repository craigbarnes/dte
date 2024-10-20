manpage-names = $(basename $(notdir $(man)))
manpage-date-targets = $(addprefix manpage-date-, $(manpage-names))
last-commit-date = git log -1 --format='%ad' --date='format:%B %Y'
html-aux = public/contributing.html public/TODO.html public/releasing.html

html-aux: $(html-aux)
htmlgz-aux: $(patsubst %, %.gz, $(html-aux))
bump-manpage-dates: $(manpage-date-targets)

$(html-aux): public/%.html: docs/%.md docs/template.html build/docs/pdhtml.flags | $(css) $(img_tpl)
	$(E) PANDOC $@
	$(Q) $(PDHTML) --metadata title='dte - $(basename $(notdir $@))' -o $@ $<

# Note that sed -i'' isn't specified by POSIX, but it's supported by
# at least GNU sed, busybox sed and *BSD sed
$(manpage-date-targets): manpage-date-%: docs/%.md
	$(E) BUMP $<
	$(Q) sed -i'' "/^---/,/^---/s|^date: .*|date: `$(last-commit-date) -- $<`|" $<


NON_PARALLEL_TARGETS += bump-manpage-dates $(manpage-date-targets)
.PHONY: html-aux htmlgz-aux bump-manpage-dates $(manpage-date-targets)
