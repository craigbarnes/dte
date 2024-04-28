CLANGTIDY ?= clang-tidy
CTIDYFLAGS ?= -quiet
CTIDYCFLAGS ?= -std=gnu11 -Isrc -Ibuild/gen -DDEBUG=3
CTIDYFILTER = 2>&1 | sed -E '/^[0-9]+ warnings? generated\.$$/d' >&2

all_headers = $(shell git ls-files -- 'src/**.h' 'test/**.h')
clang_tidy_headers = $(filter-out src/util/unidata.h, $(all_headers))
clang_tidy_targets := $(addprefix clang-tidy-, $(all_sources))

check-clang-tidy: check-clang-tidy-headers $(clang_tidy_targets)

# Excluding analyzer checks makes this target complete in ~12s instead of ~58s
check-clang-tidy-fast: CTIDYFLAGS = -quiet --checks='-clang-analyzer-*'
check-clang-tidy-fast: check-clang-tidy

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

clang-tidy-src/config.c: build/gen/builtin-config.h
clang-tidy-src/editor.c: build/gen/version.h src/compat.h
clang-tidy-src/main.c: build/gen/version.h
clang-tidy-src/compat.c: src/compat.h
clang-tidy-src/load-save.c: src/compat.h
clang-tidy-src/signals.c: src/compat.h
clang-tidy-src/tag.c: src/compat.h
clang-tidy-src/util/fd.c: src/compat.h
clang-tidy-src/util/xmemmem.c: src/compat.h
clang-tidy-src/terminal/ioctl.c: src/compat.h
clang-tidy-test/config.c: build/gen/test-data.h


.PHONY: \
    check-clang-tidy check-clang-tidy-headers check-clang-tidy-fast \
    $(clang_tidy_targets)
