HASH := \#
streq = $(and $(findstring $(1),$(2)),$(findstring $(2),$(1)))
toupper = $(shell echo '$(1)' | tr '[:lower:]' '[:upper:]')
prefix-obj = $(addprefix $(1), $(addsuffix .o, $(2)))
echo-if-set = $(foreach var, $(1), $(if $($(var)), $(var)))

MAKEFLAGS += -r
MAKE_S = $(findstring s,$(firstword -$(MAKEFLAGS)))
OPTCHECK = mk/optcheck.sh

ifneq "$(MAKE_S)" ""
  # Make "-s" flag was used (silent build)
  LOG = :
  Q = @
  OPTCHECK += -s
else ifeq "$(V)" "1"
  # "V=1" variable was set (verbose build)
  LOG = :
  Q =
else
  # Normal build
  LOG = printf ' %7s  %s\n'
  Q = @
endif

E = @$(LOG)

.PHONY: FORCE
