# This makefile is used to generate the "Portable Builds" mentioned
# in CHANGELOG.md (which is also the "Releases" page on the website)

# These 2 compilers can be found in the `musl` and `musl-aarch64`
# packages on Arch Linux
MUSLGCC ?= ccache musl-gcc
MUSLGCC_AARCH64 ?= ccache aarch64-linux-musl-gcc

# Work around a bug in the Arch Linux packaging of gcc/musl-gcc
# See also: https://gitlab.archlinux.org/archlinux/packaging/packages/gcc/-/work_items/39#:~:text=interim%20workaround%20for%20the%20musl%2Dgcc%20problem
MUSLGCC_LDFLAGS = -fno-link-libatomic

TAR_CREATE = $(TAR) --owner=0 --group=0 --sort=name --numeric-owner -czf

PORTABLE_MAKEFLAGS = \
    --no-print-directory \
    CFLAGS='-O2 -pipe -flto' \
    LDFLAGS='-static -s $(MUSLGCC_LDFLAGS)' \
    NO_CONFIG_MK=1 \
    SANE_WCTYPE=1 \
    DEBUG=0

define PORTABLE_SYMLINK
  $(E) SYMLINK 'public/dte-master-linux-$(1).tar.gz'
  $(Q) ln -sf 'dte-$(VERSION)-linux-$(1).tar.gz' 'public/dte-master-linux-$(1).tar.gz'
endef

portable: portable-x86_64 portable-aarch64
	$(call PORTABLE_SYMLINK,x86_64)
	$(call PORTABLE_SYMLINK,aarch64)

portable-x86_64: private TARBALL = public/dte-$(VERSION)-linux-x86_64.tar.gz
portable-x86_64: $(man) | public/
	$(Q) test "`uname -sm`" = 'Linux x86_64'
	$(Q) $(MAKE) vvars all $(PORTABLE_MAKEFLAGS) CC='$(MUSLGCC)'
	$(E) TAR '$(TARBALL)'
	$(Q) $(TAR_CREATE) '$(TARBALL)' dte $^

portable-aarch64: private TARBALL = public/dte-$(VERSION)-linux-aarch64.tar.gz
portable-aarch64: $(man) | public/
	$(Q) $(MAKE) vvars all $(PORTABLE_MAKEFLAGS) CC='$(MUSLGCC_AARCH64)'
	$(E) TAR '$(TARBALL)'
	$(Q) $(TAR_CREATE) '$(TARBALL)' dte $^


NON_PARALLEL_TARGETS += portable portable-x86_64 portable-aarch64
.PHONY: portable portable-x86_64 portable-aarch64
