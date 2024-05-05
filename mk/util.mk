HASH := \#
CFILE := mk/feature-test/basic.c
streq = $(and $(findstring $(1),$(2)),$(findstring $(2),$(1)))
toupper = $(shell echo '$(1)' | tr '[:lower:]' '[:upper:]')
try-run = $(if $(shell $(1) >/dev/null 2>&1 && echo 1),$(2),$(3))
cc-option = $(call try-run,$(CC) $(1) -Werror -c -o /dev/null $(CFILE),$(1),$(2))
prefix-obj = $(addprefix $(1), $(addsuffix .o, $(2)))
echo-if-set = $(foreach var, $(1), $(if $($(var)), $(var)))

# Note: this doesn't work reliably in make 3.81, due to a bug, but
# we already check for GNU Make 4.0+ in mk/compat.mk.
# See also: https://blog.jgc.org/2016/07/lazy-gnu-make-variables.html
make-lazy = $(eval $1 = $$(eval $1 := $(value $(1)))$$($1))

NPROC = $(or $(shell sh mk/nproc.sh), 1)
$(call make-lazy,NPROC)
XARGS = xargs
XARGS_P_FLAG = $(call try-run, printf "1\n2" | $(XARGS) -P2 -I@ echo '@', -P$(NPROC))
$(call make-lazy,XARGS_P_FLAG)
XARGS_P = $(XARGS) $(XARGS_P_FLAG)

MAKEFLAGS += -r
KERNEL := $(shell sh -c 'uname -s 2>/dev/null || echo not')
OS := $(shell sh -c 'uname -o 2>/dev/null || echo not')
MAKE_S = $(findstring s,$(firstword -$(MAKEFLAGS)))

ifneq "$(MAKE_S)" ""
  # Make "-s" flag was used (silent build)
  LOG = :
  Q = @
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
