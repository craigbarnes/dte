WSCHECK = $(AWK) -f tools/wscheck.awk
HCHECK = $(AWK) -f tools/hdrcheck.awk
SHELLCHECK ?= shellcheck
CODESPELL ?= codespell
TYPOS ?= typos
GAWK ?= gawk
AWKLINT = >/dev/null $(GAWK) --lint=fatal --posix
SPATCH ?= spatch
SPATCHFLAGS ?= --very-quiet
FINDLINKS = sed -n 's|^.*\(https\?://[A-Za-z0-9_/.-]*\).*|\1|gp'
CHECKURL = curl -sSI -w '%{http_code}  @1  %{redirect_url}\n' -o /dev/null @1
GITATTRS = $(shell git ls-files --error-unmatch $(foreach A, $(1), ':(attr:$A)'))
DOCFILES = $(call GITATTRS, xml markdown)
SHELLSCRIPTS = $(call GITATTRS, shell)
SPACE_INDENTED_FILES = $(call GITATTRS, space-indent)
# TODO: Re-enable `wrap`, after diagnosing/fixing the extreme slowness
SPATCHNAMES = arraylen minmax tailcall perf pitfalls staticbuf
SPATCHTARGETS = $(addprefix check-coccinelle-, $(SPATCHNAMES))

check-coccinelle: $(SPATCHTARGETS)
check-source: check-whitespace check-headers check-codespell check-shell check-awk
check-aux: check-desktop-file check-appstream
check-all: check-source check-aux check distcheck check-clang-tidy

$(SPATCHTARGETS): check-coccinelle-%:
	$(E) SPATCH 'tools/coccinelle/$*.cocci'
	$(Q) $(SPATCH) $(SPATCHFLAGS) --sp-file 'tools/coccinelle/$*.cocci' \
	     $(all_sources) 2>&1 | sed '/egrep is obsolescent/d'

check-shell:
	$(E) SHCHECK '*.sh *.bash $(filter-out %.sh %.bash, $(SHELLSCRIPTS))'
	$(Q) $(SHELLCHECK) -fgcc $(SHELLSCRIPTS) >&2

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
	$(Q) $(WSCHECK) $(SPACE_INDENTED_FILES) >&2

check-headers:
	$(Q) $(HCHECK) $$(git ls-files -- '**.[ch]') >&2

check-codespell:
	$(E) CODESPL 'src/ mk/ *.md *.xml'
	$(Q) $(CODESPELL) -Linpu,iterm,clen,ede src/ mk/ $(DOCFILES)

# The options and redirections here are to ensure that `make check-typos`
# is useable in dte command mode
check-typos:
	$(E) TYPOS 'src/ mk/ *.md *.xml'
	$(Q) $(TYPOS) --format brief --color never src/ mk/ $(DOCFILES) >&2

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


.PHONY: \
    check-all check-source check-aux check-coccinelle $(SPATCHTARGETS) \
    check-shell check-awk check-whitespace check-headers check-codespell \
    check-typos check-desktop-file check-appstream check-docs
