WSCHECK = $(AWK) -f tools/wscheck.awk
HCHECK = $(AWK) -f tools/hdrcheck.awk
MKCHECK = $(AWK) -f tools/mkcheck.awk
SHELLCHECK ?= shellcheck
CODESPELL ?= codespell
TYPOS ?= typos
GAWK ?= gawk
AWKLINT = >/dev/null $(GAWK) --lint=fatal --posix
SPATCH ?= spatch
SPATCHFLAGS ?= --very-quiet
GITATTRS = $(shell git ls-files --error-unmatch $(foreach A, $(1), ':(attr:$A)'))
DOCFILES = $(call GITATTRS, xml markdown)
SHELLSCRIPTS = $(call GITATTRS, shell)
SPACE_INDENTED_FILES = $(call GITATTRS, space-indent)
MK_FILES = $(call GITATTRS, make)
AWK_FILES = $(call GITATTRS, awk)
# TODO: Re-enable `wrap`, after diagnosing/fixing the extreme slowness
SPATCH_NAMES = arraylen minmax tailcall perf pitfalls stat-mtime staticbuf

check_coccinelle_targets = $(addprefix check-coccinelle-, $(SPATCH_NAMES))
check_awk_targets = $(addprefix check-awk-, $(AWK_FILES))

check-awk-config/script/longest-line.awk: src/msg.c
check-awk-mk/config2c.awk: config/rc
check-awk-tools/cat.awk: src/msg.c src/msg.h
check-awk-tools/gcovr-txt.awk: tools/test/gcovr-report.txt
check-awk-tools/git-hooks/commit-msg: tools/test/COMMIT_EDITMSG
check-awk-tools/hdrcheck.awk: src/msg.c src/msg.h
check-awk-tools/mkcheck.awk: mk/build.mk
check-awk-tools/wscheck.awk: src/msg.c src/msg.h

check-all: check-source check-aux check distcheck check-clang-tidy
check-source: $(addprefix check-, whitespace headers makefiles codespell shell awk)
check-aux: check-desktop-file check-appstream
check-awk: $(check_awk_targets)
check-coccinelle: $(check_coccinelle_targets)

$(check_awk_targets): check-awk-%:
	$(E) AWKLINT '$*'
	$(Q) $(AWKLINT) -f '$*' $^

$(check_coccinelle_targets): check-coccinelle-%:
	$(E) SPATCH 'tools/coccinelle/$*.cocci'
	$(Q) $(SPATCH) $(SPATCHFLAGS) --sp-file 'tools/coccinelle/$*.cocci' \
	     $(all_sources) 2>&1 | sed '/egrep is obsolescent/d'

check-shell:
	$(E) SHCHECK '*.sh *.bash $(filter-out %.sh %.bash, $(SHELLSCRIPTS))'
	$(Q) $(SHELLCHECK) -fgcc $(SHELLSCRIPTS) >&2

check-whitespace:
	$(Q) $(WSCHECK) $(SPACE_INDENTED_FILES) >&2

check-headers:
	$(Q) $(HCHECK) $$(git ls-files -- '**.[ch]') >&2

check-makefiles:
	$(Q) $(MKCHECK) $(MK_FILES) >&2

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


.PHONY: \
    $(check_awk_targets) $(check_coccinelle_targets) \
    $(addprefix check-, all source aux coccinelle shell awk whitespace) \
    $(addprefix check-, headers makefiles codespell typos desktop-file) \
    $(addprefix check-, appstream)
