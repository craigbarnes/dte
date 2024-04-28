show-sizes: MAKEFLAGS += \
    -j$(NPROC) --no-print-directory \
    CFLAGS=-O2\ -pipe \
    DEBUG=0 USE_SANITIZER=

show-sizes:
	$(MAKE) dte=build/dte-dynamic NO_CONFIG_MK=1
	$(MAKE) dte=build/dte-static LDFLAGS=-static NO_CONFIG_MK=1
	$(MAKE) dte=build/dte-dynamic-tiny CFLAGS='-Os -pipe' LDFLAGS=-fwhole-program BUILTIN_SYNTAX_FILES= NO_CONFIG_MK=1
	-$(MAKE) dte=build/dte-musl-static CC='$(MUSLGCC)' LDFLAGS=-static NO_CONFIG_MK=1
	-$(MAKE) dte=build/dte-musl-static-tiny CC='$(MUSLGCC)' CFLAGS='-Os -pipe' LDFLAGS=-static ICONV_DISABLE=1 BUILTIN_SYNTAX_FILES= NO_CONFIG_MK=1
	@strip build/dte-*
	@du -h build/dte-*


NON_PARALLEL_TARGETS += show-sizes
.PHONY: show-sizes
