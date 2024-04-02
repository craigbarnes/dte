# This makefile contains various targets that are only useful for
# development purposes and is NOT included in release tarballs

RELEASE_VERSIONS = \
    1.11.1 1.11 \
    1.10 \
    1.9.1 1.9 \
    1.8.2 1.8.1 1.8 \
    1.7 1.6 1.5 1.4 1.3 1.2 1.1 1.0

RELEASE_DIST = $(addprefix dte-, $(addsuffix .tar.gz, $(RELEASE_VERSIONS)))
DISTVER = $(VERSION)
GIT_HOOKS = $(addprefix .git/hooks/, commit-msg pre-commit)
WSCHECK = $(AWK) -f tools/wscheck.awk
HCHECK = $(AWK) -f tools/hdrcheck.awk
SHELLCHECK ?= shellcheck
CODESPELL ?= codespell
TYPOS ?= typos
CLANGTIDY ?= clang-tidy
MUSLGCC ?= ccache musl-gcc
SPATCH ?= spatch
SPATCHFLAGS ?= --very-quiet
SPATCHFILTER = 2>&1 | sed '/egrep is obsolescent/d'
CTIDYFLAGS ?= -quiet
CTIDYCFLAGS ?= -std=gnu11 -Isrc -DDEBUG=3
CTIDYFILTER = 2>&1 | sed -E '/^[0-9]+ warnings? generated\.$$/d' >&2
FINDLINKS = sed -n 's|^.*\(https\?://[A-Za-z0-9_/.-]*\).*|\1|gp'
CHECKURL = curl -sSI -w '%{http_code}  @1  %{redirect_url}\n' -o /dev/null @1
GITATTRS = $(shell git ls-files --error-unmatch $(foreach A, $(1), ':(attr:$A)'))
DOCFILES = $(call GITATTRS, xml markdown)
SPATCHNAMES = arraylen minmax tailcall wrap perf pitfalls staticbuf
SPATCHFILES = $(foreach f, $(SPATCHNAMES), tools/coccinelle/$f.cocci)

all_headers = $(shell git ls-files -- 'src/**.h' 'test/**.h')
clang_tidy_headers = $(filter-out src/util/unidata.h, $(all_headers))
clang_tidy_targets := $(addprefix clang-tidy-, $(all_sources))

dist: dte-$(DISTVER).tar.gz
dist-latest-release: $(firstword $(RELEASE_DIST))
dist-all-releases: $(RELEASE_DIST)
git-hooks: $(GIT_HOOKS)
check-clang-tidy: check-clang-tidy-headers $(clang_tidy_targets)
check-source: check-whitespace check-headers check-codespell check-shell-scripts
check-aux: check-desktop-file check-appstream
check-all: check-source check-aux check distcheck check-clang-tidy

# Excluding analyzer checks makes this target complete in ~12s instead of ~58s
check-clang-tidy-fast: CTIDYFLAGS = -quiet --checks='-clang-analyzer-*'
check-clang-tidy-fast: check-clang-tidy

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

check-headers:
	$(Q) $(HCHECK) $$(git ls-files -- '**.[ch]') >&2

check-codespell:
	$(E) CODESPL 'src/ mk/ *.md *.xml'
	$(Q) $(CODESPELL) -Literm,clen,ede src/ mk/ $(DOCFILES)

check-typos:
	$(E) TYPOS 'src/ mk/ *.md *.xml'
	$(Q) $(TYPOS) --format brief src/ mk/ $(DOCFILES)

check-desktop-file:
	$(E) LINT share/dte.desktop
	$(Q) desktop-file-validate share/dte.desktop >&2

check-appstream:
	$(E) LINT share/dte.appdata.xml
	$(Q) appstream-util --nonet validate share/dte.appdata.xml | sed '/OK$$/d' >&2

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
	@echo 'ERROR: tarballs should be named "dte-*", not "dte-v*"' >&2
	@false

dte-%.tar.gz:
	$(E) ARCHIVE $@
	$(Q) git archive --prefix='dte-$*/' -o '$@' '$*'

portable: private TARBALL = dte-$(VERSION)-$(shell uname -sm | tr 'A-Z ' a-z-).tar.gz
portable: private MAKEFLAGS += --no-print-directory
portable: $(man)
	$(Q) $(MAKE) CC='$(MUSLGCC)' CFLAGS='-O2 -pipe -flto' LDFLAGS='-static -s' NO_CONFIG_MK=1
	$(E) ARCHIVE '$(TARBALL)'
	$(Q) tar -czf '$(TARBALL)' $(dte) $^

$(GIT_HOOKS): .git/hooks/%: tools/git-hooks/%
	$(E) CP $@
	$(Q) cp $< $@

show-sizes: MAKEFLAGS += \
    -j$(NPROC) --no-print-directory \
    CFLAGS=-O2\ -pipe \
    DEBUG=0 USE_SANITIZER=

show-sizes:
	$(MAKE) dte=build/dte-dynamic NO_CONFIG_MK=1
	$(MAKE) dte=build/dte-static LDFLAGS=-static NO_CONFIG_MK=1
	$(MAKE) dte=build/dte-dynamic-tiny CFLAGS='-Os -pipe' LDFLAGS=-fwhole-program BUILTIN_SYNTAX_FILES= NO_CONFIG_MK=1
	-$(MAKE) dte=build/dte-musl-static CC='$(MUSLGCC)' LDFLAGS=-static NO_CONFIG_MK=1
	-$(MAKE) dte=build/dte-musl-static-tiny CC='$(MUSLGCC)' CFLAGS='-Os -pipe' LDFLAGS=-static ICONV_DISABLE=1 BUILTIN_SYNTAX_FILES= NO_CONFIG_MK=1
	@strip build/dte-*
	@du -h build/dte-*

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
    dist distcheck dist-latest-release dist-all-releases portable \
    check-all check-source check-docs check-shell-scripts check-headers \
    check-whitespace check-codespell check-typos check-coccinelle \
    check-aux check-desktop-file check-appstream check-clang-tidy \
    check-clang-tidy-headers check-clang-tidy-fast $(clang_tidy_targets) \
    git-hooks show-sizes
