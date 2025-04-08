# This file can be moved to (or included from) Config.mk, as described
# in docs/packaging.md. It adds some useful debug options to CFLAGS
# and provides a few conveniences for working on the editor.

# https://ccache.dev/
CC = ccache gcc

# Create a debug build by default (with ASan/UBSan enabled, if available),
# unless `make RELEASE=1`, `make RELEASE_LTO=1`, `make NOSAN=1` or
# `make NO_CONFIG_MK=1` is used.
# See also:
# • https://clang.llvm.org/docs/AddressSanitizer.html
# • https://clang.llvm.org/docs/UndefinedBehaviorSanitizer.html
# • mk/compiler.sh (CC_SANITIZER_FLAGS)
# • mk/build.mk (DEBUG, USE_SANITIZER)
# • docs/packaging.md, mk/prelude.mk (NO_CONFIG_MK)
ifeq "$(RELEASE_LTO)" "1"
 CFLAGS = -O3 -march=native -flto
else ifeq "$(RELEASE)" "1"
 CFLAGS = -O3 -march=native
else
 ifneq "$(NOSAN)" "1"
  USE_SANITIZER = 1
 endif
 CFLAGS = -g -Og
 DEBUG = 3
endif

CFLAGS += -pipe

ifeq "$(shell uname)" "Linux"
 # Override the default build/ target defined in mk/build.mk and instead
 # create a (tmpfs) directory under /tmp/ and make `build` a symlink to it.
 # This may or may not be "secure" (depending on how your system is used)
 # and is provided only as an example of how to use CUSTOM_DTE_BUILD_DIR.
 # See the comment at the bottom of mk/build.mk for more details.
 CUSTOM_DTE_BUILD_DIR = 1
 BUILD_DIR_SYMLINK_TARGET = /tmp/dte-build
 CLEANDIRS += build $(BUILD_DIR_SYMLINK_TARGET)

 $(BUILD_DIR_SYMLINK_TARGET):
	$(Q) mkdir -m755 -p '$@'

 build/: $(BUILD_DIR_SYMLINK_TARGET)
	$(Q) ln -sf '$(BUILD_DIR_SYMLINK_TARGET)' build
endif
