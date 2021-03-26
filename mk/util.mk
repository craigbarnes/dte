PKGCONFIG = $(shell command -v pkg-config || command -v pkgconf || echo ':')
$(call make-lazy,PKGCONFIG)

CFILE := mk/feature-test/basic.c
streq = $(and $(findstring $(1),$(2)),$(findstring $(2),$(1)))
try-run = $(if $(shell $(1) >/dev/null 2>&1 && echo 1),$(2),$(3))
cc-option = $(call try-run,$(CC) $(1) -Werror -c -o /dev/null $(CFILE),$(1),$(2))
prefix-obj = $(addprefix $(1), $(addsuffix .o, $(2)))
pkg-var = $(shell $(PKGCONFIG) --variable='$(strip $(2))' $(1))
echo-if-set = $(foreach var, $(1), $(if $($(var)), $(var)))

MAKEFLAGS += -r
KERNEL := $(shell sh -c 'uname -s 2>/dev/null || echo not')
OS := $(shell sh -c 'uname -o 2>/dev/null || echo not')
DISTRO = $(shell . /etc/os-release && echo "$$NAME $$VERSION_ID")
ARCH = $(shell uname -m 2>/dev/null)
NPROC = $(or $(shell sh mk/nproc.sh), 1)
_POSIX_VERSION = $(shell getconf _POSIX_VERSION 2>/dev/null)
_XOPEN_VERSION = $(shell getconf _XOPEN_VERSION 2>/dev/null)
CC_VERSION = $(or \
    $(shell $(CC) --version 2>/dev/null | head -n1), \
    $(shell $(CC) -v 2>&1 | grep version) )
CC_TARGET = $(shell $(CC) -dumpmachine 2>/dev/null)
MAKE_S = $(findstring s,$(firstword -$(MAKEFLAGS)))$(filter -s,$(MAKEFLAGS))
PRINTVAR = printf '\033[1m%15s\033[0m = %s$(2)\n' '$(1)' '$(strip $($(1)))' $(3)
PRINTVARX = $(call PRINTVAR,$(1), \033[32m(%s)\033[0m, '$(origin $(1))')
USERVARS = CC CFLAGS CPPFLAGS LDFLAGS LDLIBS DEBUG

AUTOVARS = \
    VERSION KERNEL \
    $(if $(call streq,$(KERNEL),Linux), DISTRO) \
    ARCH NPROC _POSIX_VERSION _XOPEN_VERSION \
    TERM SHELL LANG $(call echo-if-set, LC_CTYPE LC_ALL) \
    PKGCONFIG MAKE_VERSION MAKEFLAGS CC_VERSION CC_TARGET

vars:
	@echo
	@$(foreach VAR, $(AUTOVARS), $(call PRINTVAR,$(VAR));)
	@$(foreach VAR, $(USERVARS), $(call PRINTVARX,$(VAR));)
	@echo


.PHONY: vars


ifneq "$(MAKE_S)" ""
  # Make "-s" flag was used (silent build)
  Q = @
  E = @:
else ifeq "$(V)" "1"
  # "V=1" variable was set (verbose build)
  Q =
  E = @:
else
  # Normal build
  Q = @
  E = @printf ' %7s  %s\n'
endif
