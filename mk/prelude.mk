# This file simply includes some other utility/generated makefiles.
# It's kept separate from GNUmakefile so that packagers can glance
# at that file and find the parts relevant to installation, without
# this obscure mess being the first thing they see.

filter-cmdgoals = $(filter $(1), $(or $(MAKECMDGOALS),all))

ifeq "" "$(call filter-cmdgoals, clean help git-hooks dist docs man html%)"
  # Only generate and load this makefile if $(MAKECMDGOALS) contains
  # a target other than those listed above
  include build/gen/compiler.mk
endif

include build/gen/platform.mk
include mk/util.mk

# See: "persistent configuration" in docs/packaging.md
ifneq "$(NO_CONFIG_MK)" "1"
  -include Config.mk
endif
