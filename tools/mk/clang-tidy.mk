CLANGTIDY ?= clang-tidy
CTIDYFLAGS ?= -quiet
CTIDYFILTER = 2>&1 | sed -E '/^[0-9]+ warnings? generated\.$$/d' >&2

CTIDYCFLAGS = \
    -std=gnu11 -O2 -Wall -Wextra -Wundef -Wcomma \
    -DDEBUG=3 -D_FILE_OFFSET_BITS=64 \
    -Isrc -Itools/mock-headers

clang_tidy_targets := $(addprefix clang-tidy-, $(all_sources))

tidy: check-clang-tidy
tidy-full: check-clang-tidy-full
check-clang-tidy-full: $(clang_tidy_targets)

# Excluding analyzer checks makes this target complete in ~12s instead of ~58s
check-clang-tidy: CTIDYFLAGS = -quiet --checks='-clang-analyzer-*'
check-clang-tidy: check-clang-tidy-full

$(clang_tidy_targets): clang-tidy-%:
	$(E) TIDY $*
	$(Q) $(CLANGTIDY) $(CTIDYFLAGS) $* -- $(CTIDYCFLAGS) $(CTIDYFILTER)


.PHONY: \
    tidy tidy-full check-clang-tidy check-clang-tidy-full \
    $(clang_tidy_targets)
