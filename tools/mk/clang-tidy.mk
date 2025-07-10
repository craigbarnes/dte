CTIDYFLAGS ?= -quiet
clang_tidy_targets := $(addprefix clang-tidy-, $(all_sources))

tidy: check-clang-tidy
tidy-full: check-clang-tidy-full
check-clang-tidy-full: $(clang_tidy_targets)

# Excluding analyzer checks makes this target complete in ~12s instead of ~58s
check-clang-tidy: CTIDYFLAGS += --checks='-clang-analyzer-*'
check-clang-tidy: check-clang-tidy-full

$(clang_tidy_targets): clang-tidy-%:
	$(E) TIDY $*
	$(Q) tools/clang-tidy.sh $(CTIDYFLAGS) $*


.PHONY: \
    tidy tidy-full check-clang-tidy check-clang-tidy-full \
    $(clang_tidy_targets)
