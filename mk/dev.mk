RELEASE_VERSIONS = 1.9.1 1.9 1.8.2 1.8.1 1.8 1.7 1.6 1.5 1.4 1.3 1.2 1.1 1.0
RELEASE_DIST = $(addprefix dte-, $(addsuffix .tar.gz, $(RELEASE_VERSIONS)))
DISTVER = $(VERSION)
GIT_HOOKS = $(addprefix .git/hooks/, commit-msg pre-commit)
SYNTAX_LINT = $(AWK) -f tools/syntax-lint.awk
LCOV ?= lcov
LCOVFLAGS ?= --config-file mk/lcovrc
LCOV_REMOVE = $(foreach PAT, $(2), $(LCOV) -r $(1) -o $(1) '$(PAT)';)
GENHTML ?= genhtml
GENHTMLFLAGS ?= --config-file mk/lcovrc --title dte
FINDLINKS = sed -n 's|^.*\(https\?://[A-Za-z0-9_/.-]*\).*|\1|gp'
CHECKURL = curl -sSI -w '%{http_code}  @1  %{redirect_url}\n' -o /dev/null @1
XARGS_P_FLAG = $(call try-run, printf "1\n2" | xargs -P2 -I@ echo '@', -P$(NPROC))

clang_tidy_targets = $(addprefix clang-tidy-, $(editor_sources) $(test_sources))

dist: dte-$(DISTVER).tar.gz
dist-latest-release: $(firstword $(RELEASE_DIST))
dist-all-releases: $(RELEASE_DIST)
git-hooks: $(GIT_HOOKS)
clang-tidy: $(clang_tidy_targets)

check-docs:
	@printf '\nChecking links from:\n\n%s\n\n' "`git ls-files '*.md'`"
	@$(FINDLINKS) `git ls-files '*.md'` | xargs -I@1 $(XARGS_P_FLAG) $(CHECKURL)

check-syntax-files:
	$(E) LINT 'config/syntax/*'
	$(Q) $(SYNTAX_LINT) $(addprefix config/syntax/, $(BUILTIN_SYNTAX_FILES))
	$(Q) ! $(SYNTAX_LINT) test/data/syntax-lint.dterc 2>/dev/null

distcheck: private TARDIR = build/dte-$(DISTVER)/
distcheck: build/dte-$(DISTVER).tar.gz | build/
	$(E) EXTRACT $(TARDIR)
	$(Q) cd $(<D) && tar -xzf $(<F)
	$(E) MAKE $(TARDIR)
	$(Q) $(MAKE) -B -j$(NPROC) -C$(TARDIR) check install prefix=/usr DESTDIR=pkg
	$(E) TEST $(TARDIR)pkg/usr/bin/dte
	$(Q) $(TARDIR)pkg/usr/bin/dte -V | grep '^dte [1-9]' >/dev/null
	$(E) RM $(TARDIR)
	$(Q) $(RM) -r '$(TARDIR)'
	$(E) RM $<
	$(Q) $(RM) '$<'

check-release-digests: dist-all-releases
	@sha256sum -c mk/sha256sums.txt

$(RELEASE_DIST): dte-%.tar.gz:
	$(E) ARCHIVE $@
	$(Q) git archive --prefix='dte-$*/' -o '$@' 'v$*'

dte-v%.tar.gz:
	@echo 'ERROR: tarballs should be named "dte-*", not "dte-v*"'
	@false

dte-%.tar.gz:
	$(E) ARCHIVE $@
	$(Q) git archive --prefix='dte-$*/' -o '$@' '$*'

build/dte-%.tar.gz: dte-%.tar.gz | build/
	$(E) CP $@
	$(Q) cp $< $@

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
	-$(MAKE) dte=build/dte-musl-static-tiny CC=musl-gcc CFLAGS='-Os -pipe' LDFLAGS=-static ICONV_DISABLE=1 BUILTIN_SYNTAX_FILES=
	@strip build/dte-*
	@du -h build/dte-*

coverage-report: build/docs/lcov.css
	$(MAKE) -j$(NPROC) check CFLAGS='-Og -g -pipe --coverage -fno-inline' DEBUG=3 USE_SANITIZER=
	$(LCOV) $(LCOVFLAGS) -c -b . -d build/ -o build/coverage.info
	$(call LCOV_REMOVE, build/coverage.info, */src/debug.c */test/test.c)
	$(GENHTML) $(GENHTMLFLAGS) -o public/coverage/ build/coverage.info
	find public/coverage/ -type f -regex '.*\.\(css\|html\)$$' | \
	  xargs $(XARGS_P_FLAG) -- gzip -9 -k -f

build/docs/lcov.css: docs/lcov-orig.css docs/lcov-extra.css | build/docs/
	$(E) CSSCAT $@
	$(Q) cat $^ > $@

$(clang_tidy_targets): clang-tidy-%:
	$(E) TIDY $*
	$(Q) clang-tidy -quiet $* -- $(CSTD) $(CWARNS) -DDEBUG=3 1>&2

clang-tidy-src/config.c: build/builtin-config.h
clang-tidy-src/editor.c: build/version.h
clang-tidy-test/config.c: build/test/data.h


CLEANFILES += dte-*.tar.gz

.PHONY: \
    dist distcheck dist-latest-release dist-all-releases \
    check-docs check-release-digests check-syntax-files git-hooks \
    show-sizes coverage-report clang-tidy $(clang_tidy_targets)
