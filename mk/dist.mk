DIST_VERSIONS = 1.0
DIST = $(addprefix public/dist/dte-, $(addsuffix .tar.gz, $(DIST_VERSIONS)))

quiet_cmd_git_archive = ARCHIVE $@
cmd_git_archive = git archive --prefix='dte-$*/' -o '$@' '$(1)'

dist: $(DIST)

check-dist: dist
	@sha1sum -c mk/dist-sha1sums.txt

$(DIST): public/dist/dte-%.tar.gz: | public/dist/
	$(call cmd,git_archive,v$*)

public/dist/:
	@mkdir -p $@


CLEANFILES += $(DIST)
.PHONY: dist check-dist
