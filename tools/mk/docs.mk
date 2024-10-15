manpage-names = $(basename $(notdir $(man)))
manpage-date-targets = $(addprefix manpage-date-, $(manpage-names))
last-commit-date = git log -1 --format='%ad' --date='format:%B %Y'

bump-manpage-dates: $(manpage-date-targets)

# Note that sed -i'' isn't specified by POSIX, but it's supported by
# at least GNU sed, busybox sed and *BSD sed
$(manpage-date-targets): manpage-date-%: docs/%.md
	$(E) BUMP $<
	$(Q) sed -i'' "/^---/,/^---/s|^date: .*|date: `$(last-commit-date) -- $<`|" $<


.PHONY: bump-manpage-dates $(manpage-date-targets)
