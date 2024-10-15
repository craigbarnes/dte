# This makefile contains various targets that are only useful for
# development purposes and is NOT included in release tarballs

include tools/mk/docs.mk
include tools/mk/clang-tidy.mk
include tools/mk/coverage.mk
include tools/mk/dist.mk
include tools/mk/lint.mk
include tools/mk/portable.mk
include tools/mk/show-sizes.mk

CTAGS ?= ctags
GIT_HOOKS = $(addprefix .git/hooks/, commit-msg pre-commit)
html-aux = public/contributing.html public/TODO.html public/releasing.html
html += $(html-aux)

git-hooks: $(GIT_HOOKS)
html-aux: $(html-aux)

$(html-aux): public/%.html: docs/%.md
$(html-aux): private PDHTML += --metadata title='dte - $(basename $(notdir $@))'

$(GIT_HOOKS): .git/hooks/%: tools/git-hooks/%
	$(E) CP $@
	$(Q) cp $< $@

tags:
	$(CTAGS) src/*.[ch] src/*/*.[ch] test/*.[ch]


DEVMK := loaded
.PHONY: git-hooks html-aux tags
