CLANGTIDY ?= clang-tidy
CTIDYFLAGS ?= -quiet
CTIDYFILTER = 2>&1 | sed -E '/^[0-9]+ warnings? generated\.$$/d' >&2

CTIDYCFLAGS = \
    -std=gnu11 -O2 -Wall -Wextra -DDEBUG=3 -D_FILE_OFFSET_BITS=64 \
    -Isrc -Itools/mock-headers -Ibuild/gen

all_headers = $(shell git ls-files -- 'src/**.h' 'test/**.h')
clang_tidy_headers = $(filter-out src/util/unidata.h, $(all_headers))
clang_tidy_targets := $(addprefix clang-tidy-, $(all_sources))

check-clang-tidy-full: check-clang-tidy-headers $(clang_tidy_targets)
tidy: check-clang-tidy
tidy-full: check-clang-tidy-full
clang-tidy-src/config.c: build/gen/builtin-config.h
clang-tidy-test/config.c: build/gen/test-data.h

# Excluding analyzer checks makes this target complete in ~12s instead of ~58s
check-clang-tidy: CTIDYFLAGS = -quiet --checks='-clang-analyzer-*'
check-clang-tidy: check-clang-tidy-full

$(clang_tidy_targets): clang-tidy-%:
	$(E) TIDY $*
	$(Q) $(CLANGTIDY) $(CTIDYFLAGS) $* -- $(CTIDYCFLAGS) $(CTIDYFILTER)

# This is done separately from $(clang_tidy_targets), because the
# $(clang_tidy_headers) macro shells out to `git ls-files` and we don't
# want that happening on every make invocation. The motivating reason
# for this was to avoid git's "detected dubious ownership" warning when
# running `sudo make install`, but it's good practice regardless.
check-clang-tidy-headers:
	$(Q) $(foreach f, $(clang_tidy_headers), \
	  $(LOG) TIDY $(f); \
	  $(CLANGTIDY) $(CTIDYFLAGS) $(f) -- $(CTIDYCFLAGS) $(CTIDYFILTER); \
	)


.PHONY: \
    tidy tidy-full check-clang-tidy check-clang-tidy-full \
    check-clang-tidy-headers $(clang_tidy_targets)
