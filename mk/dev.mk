# This makefile contains various targets that are only useful for
# development purposes (e.g. linting, coverage, etc.) and is NOT
# included in release tarballs.

RELEASE_VERSIONS = 1.10 1.9.1 1.9 1.8.2 1.8.1 1.8 1.7 1.6 1.5 1.4 1.3 1.2 1.1 1.0
RELEASE_DIST = $(addprefix dte-, $(addsuffix .tar.gz, $(RELEASE_VERSIONS)))
DISTVER = $(VERSION)
GIT_HOOKS = $(addprefix .git/hooks/, commit-msg pre-commit)
WSCHECK = $(AWK) -f tools/wscheck.awk
SHELLCHECK ?= shellcheck
CODESPELL ?= codespell
GCOVR ?= gcovr
LCOV ?= lcov
LCOVFLAGS ?= --config-file mk/lcovrc
LCOV_REMOVE = $(foreach PAT, $(2), $(LCOV) -r $(1) -o $(1) '$(PAT)';)
GENHTML ?= genhtml
GENHTMLFLAGS ?= --config-file mk/lcovrc --title dte
FINDLINKS = sed -n 's|^.*\(https\?://[A-Za-z0-9_/.-]*\).*|\1|gp'
CHECKURL = curl -sSI -w '%{http_code}  @1  %{redirect_url}\n' -o /dev/null @1
XARGS_P_FLAG = $(call try-run, printf "1\n2" | xargs -P2 -I@ echo '@', -P$(NPROC))
DOCFILES = $(shell git ls-files -- '*.md' '*.xml')

clang_tidy_targets = $(addprefix clang-tidy-, $(all_sources))

dist: dte-$(DISTVER).tar.gz
dist-latest-release: $(firstword $(RELEASE_DIST))
dist-all-releases: $(RELEASE_DIST)
git-hooks: $(GIT_HOOKS)
clang-tidy: $(clang_tidy_targets)
check-aux: check-desktop-file check-appstream

check-shell-scripts:
	$(Q) $(SHELLCHECK) -fgcc -eSC1091 mk/*.sh test/*.sh tools/*.sh
	$(E) SHCHECK 'mk/*.sh test/*.sh tools/*.sh'

check-whitespace:
	$(Q) $(WSCHECK) `git ls-files --error-unmatch ':(attr:space-indent)'`

check-codespell:
	$(Q) $(CODESPELL) -Literm,clen,ede src/ mk/ $(DOCFILES) >&2

check-desktop-file:
	$(E) CHECK dte.desktop
	$(Q) desktop-file-validate dte.desktop

check-appstream:
	$(E) CHECK dte.appdata.xml
	$(Q) appstream-util --nonet validate dte.appdata.xml

check-docs:
	@printf '\nChecking links from:\n\n'
	@echo '$(DOCFILES) ' | tr ' ' '\n'
	@$(FINDLINKS) $(DOCFILES) | xargs -I@1 $(XARGS_P_FLAG) $(CHECKURL)

distcheck: TARDIR = build/dte-$(DISTVER)/
distcheck: build/dte-$(DISTVER).tar.gz | build/
	$(E) EXTRACT $(TARDIR)
	$(Q) cd $(<D) && tar -xzf $(<F)
	$(E) MAKE $(TARDIR)
	$(Q) $(MAKE) -B -C$(TARDIR) check installcheck prefix=/usr DESTDIR=pkg
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
	$(call LCOV_REMOVE, build/coverage.info, */src/util/debug.c */test/test.c)
	$(GENHTML) $(GENHTMLFLAGS) -o public/coverage/ build/coverage.info
	find public/coverage/ -type f -regex '.*\.\(css\|html\)$$' | \
	  xargs $(XARGS_P_FLAG) -- gzip -9 -k -f

build/docs/lcov.css: docs/lcov-orig.css docs/lcov-extra.css | build/docs/
	$(E) CSSCAT $@
	$(Q) cat $^ > $@

build/coverage.xml: gcovr-xml.cfg | build/
	$(MAKE) -j$(NPROC) check CFLAGS='-Og -g -pipe --coverage -fno-inline' DEBUG=3 USE_SANITIZER=
	$(GCOVR) -j$(NPROC) -s --config $< --xml-pretty --xml $@

$(clang_tidy_targets): clang-tidy-%:
	$(E) TIDY $*
	$(Q) clang-tidy -quiet $* -- $(CSTD) $(CWARNS) -Isrc -DDEBUG=3 1>&2

clang-tidy-src/config.c: build/builtin-config.h
clang-tidy-src/editor.c: build/version.h build/feature.h
clang-tidy-src/main.c: build/version.h
clang-tidy-src/load-save.c: build/feature.h
clang-tidy-src/util/fd.c: build/feature.h
clang-tidy-src/util/xmemmem.c: build/feature.h
clang-tidy-src/terminal/ioctl.c: build/feature.h
clang-tidy-test/config.c: build/test/data.h


CLEANFILES += dte-*.tar.gz
NON_PARALLEL_TARGETS += distcheck show-sizes coverage-report
DEVMK := loaded

.PHONY: \
    dist distcheck dist-latest-release dist-all-releases \
    check-docs check-shell-scripts check-whitespace check-codespell \
    check-release-digests check-aux check-desktop-file check-appstream \
    git-hooks show-sizes coverage-report clang-tidy $(clang_tidy_targets)
