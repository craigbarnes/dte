DISTRO = $(shell (. /etc/os-release 2>/dev/null && echo "$$NAME $$VERSION_ID") || uname -o)
ARCH = $(shell uname -m 2>/dev/null)
_POSIX_VERSION = $(shell getconf _POSIX_VERSION 2>/dev/null)
_XOPEN_VERSION = $(shell getconf _XOPEN_VERSION 2>/dev/null)
CC_TARGET = $(shell $(CC) -dumpmachine 2>/dev/null)
PRINTVAR = printf '\033[1m%15s\033[0m = %s$(2)\n' '$(1)' '$(strip $($(1)))' $(3)
PRINTVARX = $(call PRINTVAR,$(1), \033[32m(%s)\033[0m, '$(origin $(1))')
USERVARS = CC CFLAGS CPPFLAGS LDFLAGS LDLIBS DEBUG

AUTOVARS = \
    VERSION KERNEL \
    $(if $(call streq,$(KERNEL),Linux), DISTRO) \
    ARCH NPROC _POSIX_VERSION _XOPEN_VERSION \
    TERM SHELL LANG $(call echo-if-set, LC_CTYPE LC_ALL) \
    MAKE_VERSION MAKEFLAGS CC_VERSION CC_TARGET

vars:
	@echo
	@$(foreach VAR, $(AUTOVARS), $(call PRINTVAR,$(VAR));)
	@$(foreach VAR, $(USERVARS), $(call PRINTVARX,$(VAR));)
	@echo

help: private P = @printf '   %-24s %s\n'
help:
	@printf '\n Targets:\n\n'
	$P all 'Build $(dte) (default target)'
	$P vars 'Print system/build information'
	$P tags 'Create tags(5) file using ctags(1)'
	$P clean 'Remove generated files'
	$P install 'Equivalent to the 7 (4 on macOS) install-* targets below'
	$P install-bin 'Install $(dte) binary'
	$P install-man 'Install man pages'
	$P install-bash-completion 'Install bash auto-completion script'
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
	$P install-full 'Same as "make install", but without macOS-specific exclusions'
	@echo
ifeq "$(DEVMK)" "loaded"
	@printf ' Dev targets:\n\n'
	$P git-hooks 'Enable git hooks from tools/git-hooks/*'
	$P docs 'Equivalent to "make man html htmlgz"'
	$P man 'Generate man pages'
	$P html 'Generate website'
	$P htmlgz 'Generate statically gzipped website (for GitLab Pages)'
	$P pdf 'Generate PDF user manual from man pages'
	$P coverage-report 'Generate HTML coverage report with gcovr(1)'
	$P bench 'Run benchmarks for some utility functions'
	$P gen-unidata 'Generate Unicode data tables'
	$P dist 'Generate tarball for latest git commit'
	$P dist-latest-release 'Generate tarball for latest release'
	$P dist-all-releases 'Generate tarballs for all releases'
	$P check-coccinelle 'Apply Coccinelle semantic patches'
	$P check-codespell 'Check spelling errors with codespell(1)'
	$P check-shell-scripts 'Check shell scripts with shellcheck(1)'
	$P check-whitespace 'Check source files for indent/newline errors'
	$P check-headers 'Check C headers and includes for various mistakes'
	$P check-docs 'Check HTTP status of URLs found in docs'
	$P check-clang-tidy 'Run clang-tidy(1) checks from .clang-tidy'
	$P check-desktop-file 'Run desktop-file-validate(1) checks'
	$P check-appstream 'Run appstream-util(1) checks'
	$P check-source 'Equivalent to "make check-{whitespace,headers,codespell,shell-scripts}"'
	$P check-aux 'Equivalent to "make check-{desktop-file,appstream}"'
	$P distcheck 'Run "make check" on the unpacked "make dist" tarball'
	$P 'check TESTFLAGS=-t' 'Same as "make check", but also showing timing info'
	@echo
endif

.PHONY: vars help
