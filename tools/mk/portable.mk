MUSLGCC ?= ccache musl-gcc

portable: private TARBALL = dte-$(VERSION)-$(shell uname -sm | tr 'A-Z ' a-z-).tar.gz
portable: private MAKEFLAGS += --no-print-directory
portable: $(man)
	$(Q) $(MAKE) CC='$(MUSLGCC)' CFLAGS='-O2 -pipe -flto' LDFLAGS='-static -s' NO_CONFIG_MK=1
	$(E) ARCHIVE '$(TARBALL)'
	$(Q) tar -czf '$(TARBALL)' $(dte) $^


.PHONY: portable
