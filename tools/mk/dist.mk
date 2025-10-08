TAR = tar
TAR_EXTRACT = $(TAR) -xzf

RELEASE_VERSIONS = \
    1.11.1 1.11 \
    1.10 \
    1.9.1 1.9 \
    1.8.2 1.8.1 1.8 \
    1.7 1.6 1.5 1.4 1.3 1.2 1.1 1.0

RELEASE_DIST = $(addprefix dte-, $(addsuffix .tar.gz, $(RELEASE_VERSIONS)))
DISTVER = $(VERSION)

dist: dte-$(DISTVER).tar.gz
dist-latest-release: $(firstword $(RELEASE_DIST))
dist-all-releases: $(RELEASE_DIST)

distcheck: private TARDIR = build/dte-$(DISTVER)/
distcheck: private DIST = dte-$(DISTVER).tar.gz
distcheck: dist | build/
	cp -f $(DIST) build/
	cd build/ && $(TAR_EXTRACT) $(DIST)
	$(MAKE) -C$(TARDIR) -B check
	$(MAKE) -C$(TARDIR) installcheck prefix=/usr DESTDIR=pkg
	$(RM) -r '$(TARDIR)'
	$(RM) 'build/$(DIST)'

$(RELEASE_DIST): dte-%.tar.gz:
	$(E) ARCHIVE $@
	$(Q) git archive --prefix='dte-$*/' -o '$@' 'v$*'

dte-v%.tar.gz:
	@echo 'ERROR: tarballs should be named "dte-*", not "dte-v*"' >&2
	@false

dte-%.tar.gz:
	$(E) ARCHIVE $@
	$(Q) git archive --prefix='dte-$*/' -o '$@' '$*'


CLEANFILES += dte-*.tar.gz
NON_PARALLEL_TARGETS += distcheck

.PHONY: dist distcheck dist-latest-release dist-all-releases
