KERNEL := $(shell sh -c 'uname -s 2>/dev/null || echo not')
OS := $(shell sh -c 'uname -o 2>/dev/null || echo not')
DISTRO = $(shell . /etc/os-release && echo "$$NAME")
STREQ = $(and $(findstring $(1),$(2)),$(findstring $(2),$(1)))
PRINTVAR = printf '\033[1m%-11s\033[0m= %s\n' '$(1)' '$(strip $($(1)))'

USERVARS = \
    VERSION CC CFLAGS LDFLAGS LDLIBS DEBUG PKGDATADIR KERNEL \
    $(if $(call STREQ, $(KERNEL), Linux), DISTRO)

vars:
	@$(foreach VAR, $(USERVARS), $(call PRINTVAR,$(VAR));)


.PHONY: vars

ifneq "$(findstring s,$(firstword -$(MAKEFLAGS)))$(filter -s,$(MAKEFLAGS))" ""
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
