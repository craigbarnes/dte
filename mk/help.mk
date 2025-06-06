DISTRO = $(shell (. /etc/os-release 2>/dev/null && echo "$$NAME $$VERSION_ID") || uname -o)
ARCH = $(shell uname -m 2>/dev/null)
_POSIX_VERSION = $(shell getconf _POSIX_VERSION 2>/dev/null)
_XOPEN_VERSION = $(shell getconf _XOPEN_VERSION 2>/dev/null)
PRINTVAR = printf '$(BOLD)%15s$(SGR0) = %s$(2)\n' '$(1)' '$(strip $($(1)))' $(3)
PRINTVARX = $(call PRINTVAR,$(1), $(GREEN)(%s)$(SGR0), '$(origin $(1))')

USERVARS = \
    LANG \
    $(call echo-if-set, LC_CTYPE LC_ALL) \
    DEBUG AWK CC CFLAGS \
    $(call echo-if-set, CPPFLAGS LDFLAGS LDLIBS TESTFLAGS WERROR V) \
    $(call echo-if-set, ICONV_DISABLE NO_DEPS NO_COLOR NO_CONFIG_MK)

USERVARS_VERBOSE = \
    PANDOC LUA DESTDIR prefix bindir mandir \
    DTE_HOME HOME XDG_RUNTIME_DIR \
    $(call echo-if-set, DTE_LOG DTE_LOG_LEVEL DTE_LOG_TRACE) \
    $(call echo-if-set, SANE_WCTYPE USE_SANITIZER)

AUTOVARS = \
    VERSION KERNEL \
    $(if $(call xstreq,$(KERNEL),Linux), DISTRO) \
    ARCH NPROC _POSIX_VERSION _XOPEN_VERSION SHELL TERM \
    $(call echo-if-set, COLORTERM TERM_PROGRAM) \
    XARGS_P OPTCHECK DEVMK MAKEFLAGS MAKE_VERSION AWK_VERSION \
    CC_VERSION CC_TARGET CC_IS_CCACHE

AUTOVARS_VERBOSE = \
    .FEATURES MAKE_TERMOUT MAKE_TERMERR USE_COLOR CFLAGS_ALL LDFLAGS_ALL

vvars: USERVARS += $(USERVARS_VERBOSE)
vvars: AUTOVARS += $(AUTOVARS_VERBOSE)
vvars: vars

vars:
	@echo
	@$(foreach VAR, $(AUTOVARS), $(call PRINTVAR,$(VAR));)
	@$(foreach VAR, $(USERVARS), $(call PRINTVARX,$(VAR));)
	@echo

help: private P = @printf '   %-24s %s\n'
help:
	@printf '\n Targets:\n\n'
	$P all 'Build dte (default target)'
	$P vars 'Print system/build information'
	$P vvars 'Verbose version of "make vars"'
	$P tags 'Create tags(5) file using ctags(1)'
	$P clean 'Remove generated files'
	$P install 'Equivalent to the first 8 (5 on macOS/Android) install-* targets below'
	$P install-bin 'Install dte binary'
	$P install-man 'Install man pages'
	$P install-bash-completion 'Install bash auto-completion script'
	$P install-fish-completion 'Install fish auto-completion script'
	$P install-zsh-completion 'Install zsh auto-completion script'
	$P install-desktop-file 'Install desktop entry file'
	$P install-icons 'Install SVG/PNG icon files'
	$P install-appstream 'Install AppStream metadata'
	$P uninstall 'Uninstall files installed by "make install"'
	$P 'uninstall-*' 'Uninstall files installed by "make install-*"'
	$P installcheck 'Run "make install" and sanity test installed binary'
	$P check 'Equivalent to "make check-tests check-opts"'
	$P check-tests 'Compile and run unit tests'
	$P check-opts 'Test dte(1) command-line options and error handling'
	$P install-full 'Same as "make install", but without exclusions for macOS/Android'
	@echo
ifeq "$(DEVMK)" "loaded"
	@printf ' Dev targets:\n\n'
	$P git-hooks 'Enable git hooks from tools/git-hooks/*'
	$P docs 'Equivalent to "make man html htmlgz"'
	$P man 'Generate man pages'
	$P html 'Generate website'
	$P htmlgz 'Generate statically gzipped website (for GitLab Pages)'
	$P pdf 'Generate PDF user manual from man pages'
	$P coverage 'Generate HTML coverage report with gcovr(1) and open in $$BROWSER'
	$P bench 'Run benchmarks for some utility functions'
	$P gen-unidata 'Generate Unicode data tables'
	$P dist 'Generate tarball for latest git commit'
	$P dist-latest-release 'Generate tarball for latest release'
	$P dist-all-releases 'Generate tarballs for all releases'
	$P check-coccinelle 'Apply Coccinelle semantic patches'
	$P check-codespell 'Check spelling errors with codespell(1)'
	$P check-shell 'Check shell scripts with shellcheck(1)'
	$P check-awk 'Check AWK scripts with gawk --lint=fatal'
	$P check-whitespace 'Check source files for indent/newline errors'
	$P check-headers 'Check C headers and includes for various mistakes'
	$P check-makefiles 'Check makefiles for various mistakes'
	$P check-docs 'Check HTTP status of URLs found in docs'
	$P check-clang-tidy 'Run clang-tidy(1) checks from .clang-tidy'
	$P check-desktop-file 'Run desktop-file-validate(1) checks'
	$P check-appstream 'Run appstream-util(1) checks'
	$P check-source 'Equivalent to "make check-{whitespace,headers,makefiles,codespell,shell,awk}"'
	$P check-aux 'Equivalent to "make check-{desktop-file,appstream}"'
	$P distcheck 'Run "make check" on the unpacked "make dist" tarball'
	$P 'check TESTFLAGS=-t' 'Same as "make check", but also showing timing info'
	@echo
endif

# --version: gawk, nawk, goawk
# -version: FreeBSD
# -V: OpenBSD, gawk
# -Wversion: mawk, gawk
AWK_VERSION = $(or \
    $(shell $(AWK) --version 2>/dev/null | head -n1), \
    $(shell $(AWK) -V 2>/dev/null | head -n1), \
    $(shell $(AWK) -version 2>/dev/null | head -n1), \
    $(shell $(AWK) -Wversion 2>/dev/null | head -n1), \
)

.PHONY: vars vvars help
