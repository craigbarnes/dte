# This makefile contains various targets that are only useful for
# development purposes and is NOT included in release tarballs

include tools/mk/clang-tidy.mk
include tools/mk/coverage.mk
include tools/mk/dist.mk
include tools/mk/lint.mk
include tools/mk/portable.mk
include tools/mk/show-sizes.mk

CTAGS ?= ctags
GIT_HOOKS = $(addprefix .git/hooks/, commit-msg pre-commit)

git-hooks: $(GIT_HOOKS)

$(GIT_HOOKS): .git/hooks/%: tools/git-hooks/%
	$(E) CP $@
	$(Q) cp $< $@

tags:
	$(CTAGS) src/*.[ch] src/*/*.[ch] test/*.[ch]


DEVMK := loaded
.PHONY: git-hooks tags
