manpage-names = $(basename $(notdir $(man)))
manpage-date-targets = $(addprefix manpage-date-, $(manpage-names))
last-commit-date = git log -1 --format='%ad' --date='format:%B %Y'
html-aux-basenames = contributing packaging releasing TODO
html-aux = $(foreach f, $(html-aux-basenames), public/$(f).html)

# These are miscellaneous Markdown files dotted around the repo and
# aren't intended to be part of the public website. `make html-aux`
# can be used to render them as HTML, but only for proof-reading
# purposes.
public/contributing.html: docs/contributing.md
public/TODO.html: docs/TODO.md
public/releasing.html: docs/releasing.md
public/packaging.html: docs/packaging.md

html-aux: $(html-aux)
htmlgz-aux: $(patsubst %, %.gz, $(html-aux))
bump-manpage-dates: $(manpage-date-targets)

$(html-aux): public/%.html: docs/template.html build/docs/pdhtml.flags | $(css) $(img_tpl)
	$(E) PANDOC $@
	$(Q) $(PDHTML) --metadata title='dte - $(basename $(notdir $@))' -o $@ $(filter %.md, $^)

# Note that sed -i'' isn't specified by POSIX, but it's supported by
# at least GNU sed, busybox sed and *BSD sed
$(manpage-date-targets): manpage-date-%: docs/%.md
	$(E) BUMP $<
	$(Q) sed -i'' "/^---/,/^---/s|^date: .*|date: `$(last-commit-date) -- $<`|" $<


NON_PARALLEL_TARGETS += bump-manpage-dates $(manpage-date-targets)
.PHONY: html-aux htmlgz-aux bump-manpage-dates $(manpage-date-targets)
