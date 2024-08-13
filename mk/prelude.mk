# This file simply includes some other utility/generated makefiles.
# It's kept separate from GNUmakefile so that packagers can glance
# at that file and find the parts relevant to installation, without
# this obscure mess being the first thing they see.

include build/gen/platform.mk

filter-cmdgoals = $(filter $(1), $(or $(MAKECMDGOALS),all))

ifeq "" "$(call filter-cmdgoals, clean help git-hooks dist docs man html%)"
  include build/gen/compiler.mk
endif

include mk/util.mk

ifneq "$(NO_CONFIG_MK)" "1"
  -include Config.mk
endif
