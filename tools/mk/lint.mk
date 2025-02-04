WSCHECK = $(AWK) -f tools/wscheck.awk
HCHECK = $(AWK) -f tools/hdrcheck.awk
SHELLCHECK ?= shellcheck
CODESPELL ?= codespell
TYPOS ?= typos
SPATCH ?= spatch
GAWK ?= gawk
AWKLINT = >/dev/null $(GAWK) --lint=fatal --posix
SPATCHFLAGS ?= --very-quiet
SPATCHFILTER = 2>&1 | sed '/egrep is obsolescent/d'
FINDLINKS = sed -n 's|^.*\(https\?://[A-Za-z0-9_/.-]*\).*|\1|gp'
CHECKURL = curl -sSI -w '%{http_code}  @1  %{redirect_url}\n' -o /dev/null @1
GITATTRS = $(shell git ls-files --error-unmatch $(foreach A, $(1), ':(attr:$A)'))
DOCFILES = $(call GITATTRS, xml markdown)
SPATCHNAMES = arraylen minmax tailcall wrap perf pitfalls staticbuf
SPATCHFILES = $(foreach f, $(SPATCHNAMES), tools/coccinelle/$f.cocci)

check-source: check-whitespace check-headers check-codespell check-shell check-awk
check-aux: check-desktop-file check-appstream
check-all: check-source check-aux check distcheck check-clang-tidy

check-shell:
	$(E) SHCHECK '*.sh *.bash $(filter-out %.sh %.bash, $(call GITATTRS, shell))'
	$(Q) $(SHELLCHECK) -fgcc $(call GITATTRS, shell) >&2

check-awk: src/msg.c src/msg.h
	$(E) AWKLINT mk/config2c.awk
	$(Q) $(AWKLINT) -f mk/config2c.awk $(BUILTIN_CONFIGS)
	$(E) AWKLINT tools/hdrcheck.awk
	$(Q) $(AWKLINT) -f tools/hdrcheck.awk $^
	$(E) AWKLINT tools/wscheck.awk
	$(Q) $(AWKLINT) -f tools/wscheck.awk $^
	$(E) AWKLINT contrib/longest-line.awk
	$(Q) $(AWKLINT) -f contrib/longest-line.awk $<
	$(E) AWKLINT tools/git-hooks/commit-msg
	$(Q) printf 'L1\n\nL3.\n' | $(AWKLINT) -f tools/git-hooks/commit-msg
	$(E) AWKLINT tools/gcovr-txt.awk
	$(Q) printf '.\n.\n.\n1 2 3 4 5\n' | $(AWKLINT) -f tools/gcovr-txt.awk

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

check-coccinelle:
	$(Q) $(foreach sp, $(SPATCHFILES), \
	  $(LOG) SPATCH $(sp); \
	  $(SPATCH) $(SPATCHFLAGS) --sp-file $(sp) $(all_sources) $(SPATCHFILTER); \
	)


.PHONY: \
    check-all check-source check-aux check-typos check-coccinelle check-docs \
    check-whitespace check-headers check-codespell check-shell check-awk \
    check-desktop-file check-appstream
