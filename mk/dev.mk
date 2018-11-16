DIST_VERSIONS = 1.7 1.6 1.5 1.4 1.3 1.2 1.1 1.0
DIST_ALL = $(addprefix dte-, $(addsuffix .tar.gz, $(DIST_VERSIONS)))
GIT_HOOKS = $(addprefix .git/hooks/, commit-msg pre-commit)
SYNTAX_LINT = $(AWK) -f tools/syntax-lint.awk
LCOV ?= lcov
LCOVFLAGS ?= --no-external --rc lcov_excl_line='(BUG|fatal_error) *\('
GENHTML ?= genhtml
GENHTMLFLAGS ?= --no-function-coverage --title dte

dist: $(firstword $(DIST_ALL))
dist-all: $(DIST_ALL)
git-hooks: $(GIT_HOOKS)

check-syntax-files:
	$(E) LINT 'config/syntax/*'
	$(Q) $(SYNTAX_LINT) $(addprefix config/syntax/, $(BUILTIN_SYNTAX_FILES))
	$(Q) ! $(SYNTAX_LINT) test/data/syntax-lint.dterc 2>/dev/null

check-dist: dist-all
	@sha256sum -c mk/sha256sums.txt

$(DIST_ALL): dte-%.tar.gz:
	$(E) ARCHIVE $@
	$(Q) git archive --prefix='dte-$*/' -o '$@' 'v$*'

$(GIT_HOOKS): .git/hooks/%: tools/git-hooks/%
	$(E) CP $@
	$(Q) cp $< $@

show-sizes: MAKEFLAGS += \
    -j$(NPROC) --no-print-directory \
    CFLAGS=-O2\ -pipe \
    TERMINFO_DISABLE=1 \
    DEBUG=0 USE_SANITIZER=

show-sizes:
	$(MAKE) dte=build/dte-dynamic
	$(MAKE) dte=build/dte-static LDFLAGS=-static
	$(MAKE) dte=build/dte-dynamic-tiny CFLAGS='-Os -pipe' LDFLAGS=-fwhole-program BUILTIN_SYNTAX_FILES=
	-$(MAKE) dte=build/dte-musl-static CC=musl-gcc LDFLAGS=-static
	@strip build/dte-*
	@du -h build/dte-*

coverage-report:
	$(MAKE) -j$(NPROC) check CFLAGS='-Og -g -pipe --coverage -fno-inline' DEBUG=3 USE_SANITIZER=
	$(LCOV) $(LCOVFLAGS) -c -b . -d build/ -o build/coverage.info
	$(LCOV) -r build/coverage.info '*/test/test.c' -o build/coverage.info
	$(GENHTML) $(GENHTMLFLAGS) -o public/coverage/ build/coverage.info
	find public/coverage/ -type f -regex '.*\.\(css\|html\)$$' | \
	  xargs $(XARGS_P_FLAG) -- gzip -9 -k -f


CLEANFILES += dte-*.tar.gz

.PHONY: \
    check-syntax-files check-dist dist dist-all git-hooks \
    show-sizes coverage-report
