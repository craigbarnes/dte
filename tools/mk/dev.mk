# This makefile contains various targets that are only useful for
# development purposes and is NOT included in release tarballs

include tools/mk/docs.mk
include tools/mk/clang-tidy.mk
include tools/mk/coverage.mk
include tools/mk/dist.mk
include tools/mk/lint.mk
include tools/mk/doclint.mk
include tools/mk/portable.mk
include tools/mk/show-sizes.mk

# • Treat *.[ch] files as C (not C++ or ObjectiveC)
# • Prevent DESIGNATED_INIT macro being confused for a struct tag
# • Disable generation of pointless __anon* tags (e.g. for anonymous enums)
CTAGSFLAGS = \
    --langmap=C:.c.h \
    -I DESIGNATED_INIT \
    --extras='-{anonymous}'

CTAGS ?= ctags
GIT_HOOKS = $(addprefix .git/hooks/, commit-msg pre-commit)
GEN_MKFILES = $(foreach f, platform compiler, build/gen/$(f).mk build/gen/$(f).log)

git-hooks: $(GIT_HOOKS)

$(GIT_HOOKS): .git/hooks/%: tools/git-hooks/%
	$(E) CP $@
	$(Q) cp $< $@

print-struct-sizes: $(test)
	$(E) EXEC '$(test) -s'
	$(Q) $(test) -s

tags:
	$(CTAGS) $(CTAGSFLAGS) src/*.[ch] src/*/*.[ch] test/*.[ch]

# For conveniently inspecting generated makefiles and their associated logs,
# if something goes wrong
dump-generated-makefiles: $(GEN_MKFILES)
	$(Q) $(AWK) -f mk/cat.awk $^


DEVMK := loaded
.PHONY: print-struct-sizes git-hooks tags dump-generated-makefiles
