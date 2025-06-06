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
VEQ1 = $(call streq,$(V),1)
MAKE_S = $(findstring s,$(firstword -$(MAKEFLAGS)))
OPTCHECK = mk/optcheck.sh $(if $(MAKE_S), -s)

USE_COLOR = $(if $(MAKE_TERMOUT),$(if $(NO_COLOR),,1))
GREEN = $(if $(USE_COLOR),\033[32m)
BOLD = $(if $(USE_COLOR),\033[1m)
SGR0 = $(if $(USE_COLOR),\033[0m)

# Recipe prefix macros for custom build output, including special
# handling of `make V=1` (verbose) and `make -s` (silent)
LOG = $(if $(MAKE_S)$(VEQ1), :, printf ' %7s  %s\n')
Q = $(if $(MAKE_S), @, $(if $(VEQ1),, @))
E = @$(LOG)

.PHONY: FORCE
