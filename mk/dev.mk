DIST_VERSIONS = 1.0 1.1 1.2
DIST = $(addprefix public/dist/dte-, $(addsuffix .tar.gz, $(DIST_VERSIONS)))
GIT_HOOKS = $(addprefix .git/hooks/, commit-msg pre-commit)

dist: $(DIST)
git-hooks: $(GIT_HOOKS)

check-dist: dist
	@sha1sum -c mk/dist-sha1sums.txt

$(DIST): public/dist/dte-%.tar.gz: | public/dist/
	$(E) ARCHIVE $@
	$(Q) git archive --prefix='dte-$*/' -o '$@' 'v$*'

$(GIT_HOOKS): .git/hooks/%: mk/git-hooks/%
	$(E) CP $@
	$(Q) cp $< $@

public/dist/:
	@mkdir -p $@


CLEANFILES += $(DIST)
.PHONY: dist check-dist git-hooks
