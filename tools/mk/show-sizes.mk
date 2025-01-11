SSFLAGS = --no-print-directory CFLAGS=-O2 NO_CONFIG_MK=1 USE_SANITIZER= DEBUG=0
SSBIN = $(addprefix build/dte-, dynamic static dynamic-tiny musl-static musl-static-tiny)

show-sizes:
	$(MAKE) $(SSFLAGS) dte=build/dte-dynamic
	$(MAKE) $(SSFLAGS) dte=build/dte-static LDFLAGS=-static
	$(MAKE) $(SSFLAGS) dte=build/dte-dynamic-tiny CFLAGS='-Os -flto' BUILTIN_SYNTAX_FILES=
	-$(MAKE) $(SSFLAGS) dte=build/dte-musl-static CC='$(MUSLGCC)' LDFLAGS=-static
	-$(MAKE) $(SSFLAGS) dte=build/dte-musl-static-tiny CC='$(MUSLGCC)' CFLAGS='-Os -flto' LDFLAGS=-static ICONV_DISABLE=1 BUILTIN_SYNTAX_FILES=
	@strip $(SSBIN)
	@du -h $(SSBIN)


NON_PARALLEL_TARGETS += show-sizes
.PHONY: show-sizes
