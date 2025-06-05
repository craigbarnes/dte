streq = $(and $(findstring $(1),$(2)),$(findstring $(2),$(1)))
prefix-obj = $(addprefix $(1), $(addsuffix .o, $(2)))
echo-if-set = $(foreach var, $(1), $(if $($(var)), $(var)))

# Generate a log filename for $@, by removing its extension (if any) and
# appending ".log" (so e.g. `build/xyz.log` would be generated, if the
# target is `build/xyz.o`)
logfile = $(@:$(suffix $@)=.log)

# If a target's stderr is redirected to $(logfile), for the purpose of
# collecting harmless/expected error messages, this can be used to dump
# its contents to Make's own stderr (e.g. with `|| $(dumplog_and_exit1)`)
# in case an *unexpected* error occurs
dumplog_and_exit1 = (cat $(logfile) >&2; exit 1)

# https://www.gnu.org/software/make/manual/html_node/Makefile-Contents.html#:~:text=e.g.%2C%20%5C%23
# https://lists.gnu.org/archive/html/make-w32/2020-01/msg00001.html#:~:text=H%20%3A%3D%20%5C%23
HASH := \#

MAKEFLAGS += -r
MAKE_S = $(findstring s,$(firstword -$(MAKEFLAGS)))
OPTCHECK = mk/optcheck.sh

USE_COLOR = $(if $(MAKE_TERMOUT),$(if $(NO_COLOR),,1))
GREEN = $(if $(USE_COLOR),\033[32m)
BOLD = $(if $(USE_COLOR),\033[1m)
SGR0 = $(if $(USE_COLOR),\033[0m)

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
