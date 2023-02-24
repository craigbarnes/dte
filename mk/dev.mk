# This makefile contains various targets that are only useful for
# development purposes and is NOT included in release tarballs

RELEASE_VERSIONS = 1.11 1.10 1.9.1 1.9 1.8.2 1.8.1 1.8 1.7 1.6 1.5 1.4 1.3 1.2 1.1 1.0
RELEASE_DIST = $(addprefix dte-, $(addsuffix .tar.gz, $(RELEASE_VERSIONS)))
DISTVER = $(VERSION)
GIT_HOOKS = $(addprefix .git/hooks/, commit-msg pre-commit)
WSCHECK = $(AWK) -f tools/wscheck.awk
SHELLCHECK ?= shellcheck
CODESPELL ?= codespell
CLANGTIDY ?= clang-tidy
SPATCH ?= spatch
SPATCHFLAGS ?= --very-quiet
SPATCHFILTER = 2>&1 | sed '/egrep is obsolescent/d'
FINDLINKS = sed -n 's|^.*\(https\?://[A-Za-z0-9_/.-]*\).*|\1|gp'
CHECKURL = curl -sSI -w '%{http_code}  @1  %{redirect_url}\n' -o /dev/null @1
GITATTRS = $(shell git ls-files --error-unmatch $(foreach A, $(1), ':(attr:$A)'))
DOCFILES = $(call GITATTRS, xml markdown)
SPATCHFILES = $(foreach f, arraylen assertions minmax wrap, tools/coccinelle/$f.cocci)

clang_tidy_targets = $(addprefix clang-tidy-, $(all_sources))

dist: dte-$(DISTVER).tar.gz
dist-latest-release: $(firstword $(RELEASE_DIST))
dist-all-releases: $(RELEASE_DIST)
git-hooks: $(GIT_HOOKS)
check-clang-tidy: $(clang_tidy_targets)
check-source: check-whitespace check-codespell check-shell-scripts
check-aux: check-desktop-file check-appstream
check-all: check-source check-aux check distcheck check-clang-tidy

check-coccinelle:
	$(Q) $(foreach sp, $(SPATCHFILES), \
	  $(LOG) SPATCH $(sp); \
	  $(SPATCH) $(SPATCHFLAGS) --sp-file $(sp) $(all_sources) $(SPATCHFILTER); \
	)

check-shell-scripts:
	$(E) SHCHECK '*.sh *.bash $(filter-out %.sh %.bash, $(call GITATTRS, shell))'
	$(Q) $(SHELLCHECK) -fgcc $(call GITATTRS, shell) >&2

check-whitespace:
	$(Q) $(WSCHECK) $(call GITATTRS, space-indent) >&2

check-codespell:
	$(E) CODESPL '*.md $(filter-out %.md, $(DOCFILES))'
	$(Q) $(CODESPELL) -Literm,clen,ede src/ mk/ $(DOCFILES) >&2

check-desktop-file:
	$(E) LINT dte.desktop
	$(Q) desktop-file-validate dte.desktop >&2

check-appstream:
	$(E) LINT dte.appdata.xml
	$(Q) appstream-util --nonet validate dte.appdata.xml | sed '/OK$$/d' >&2

check-docs:
	@printf '\nChecking links from:\n\n'
	@echo '$(DOCFILES) ' | tr ' ' '\n'
	$(Q) $(FINDLINKS) $(DOCFILES) | sort | uniq | $(XARGS_P) -I@1 $(CHECKURL)

distcheck: private TARDIR = build/dte-$(DISTVER)/
distcheck: private DIST = dte-$(DISTVER).tar.gz
distcheck: dist | build/
	cp -f $(DIST) build/
	cd build/ && tar -xzf $(DIST)
	$(MAKE) -C$(TARDIR) -B check
	$(MAKE) -C$(TARDIR) installcheck prefix=/usr DESTDIR=pkg
	$(RM) -r '$(TARDIR)'
	$(RM) 'build/$(DIST)'

$(RELEASE_DIST): dte-%.tar.gz:
	$(E) ARCHIVE $@
	$(Q) git archive --prefix='dte-$*/' -o '$@' 'v$*'

dte-v%.tar.gz:
	@echo 'ERROR: tarballs should be named "dte-*", not "dte-v*"'
	@false

dte-%.tar.gz:
	$(E) ARCHIVE $@
	$(Q) git archive --prefix='dte-$*/' -o '$@' '$*'

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

$(clang_tidy_targets): clang-tidy-%:
	$(E) TIDY $*
	$(Q) $(CLANGTIDY) -quiet $* -- $(CSTD) $(CWARNS) -Isrc -DDEBUG=3 2>&1 | \
	  sed '/^[0-9]\+ warnings generated\.$$/d' >&2

clang-tidy-src/config.c: build/builtin-config.h
clang-tidy-src/editor.c: build/version.h src/compat.h
clang-tidy-src/main.c: build/version.h
clang-tidy-src/compat.c: src/compat.h
clang-tidy-src/load-save.c: src/compat.h
clang-tidy-src/signals.c: src/compat.h
clang-tidy-src/util/fd.c: src/compat.h
clang-tidy-src/util/xmemmem.c: src/compat.h
clang-tidy-src/terminal/ioctl.c: src/compat.h
clang-tidy-test/config.c: build/test/data.h


CLEANFILES += dte-*.tar.gz
NON_PARALLEL_TARGETS += distcheck show-sizes
DEVMK := loaded

.PHONY: \
    dist distcheck dist-latest-release dist-all-releases \
    check-all check-source check-docs check-shell-scripts \
    check-whitespace check-codespell check-coccinelle \
    check-aux check-desktop-file check-appstream \
    check-clang-tidy $(clang_tidy_targets) \
    git-hooks show-sizes
