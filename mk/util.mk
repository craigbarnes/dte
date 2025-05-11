streq = $(and $(findstring $(1),$(2)),$(findstring $(2),$(1)))
prefix-obj = $(addprefix $(1), $(addsuffix .o, $(2)))
echo-if-set = $(foreach var, $(1), $(if $($(var)), $(var)))

# https://www.gnu.org/software/make/manual/html_node/Makefile-Contents.html#:~:text=e.g.%2C%20%5C%23
# https://lists.gnu.org/archive/html/make-w32/2020-01/msg00001.html#:~:text=H%20%3A%3D%20%5C%23
HASH := \#

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
