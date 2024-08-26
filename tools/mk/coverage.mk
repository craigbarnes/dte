# This makefile contains various targets that are only useful for
# development purposes and is NOT included in release tarballs

GCOVR ?= gcovr
GCOVRFLAGS ?= -j$(NPROC) --config gcovr.cfg --sort uncovered-percent --sort-reverse

COVERAGE_MAKEFLAGS = \
    NO_CONFIG_MK=1 \
    DEBUG=3 \
    TESTFLAGS=-ct \
    CC='ccache gcc' \
    CFLAGS='-Og -g -pipe --coverage -fno-inline'

gcovr-html: public/coverage/index.html
gcovr-xml: build/coverage.xml

# Convenience target, for opening HTML coverage report in $BROWSER
coverage: public/coverage/index.html
	$(if $(BROWSER), $(BROWSER) $<, $(error $$BROWSER not set))

public/coverage/index.html: FORCE | public/coverage/
	$(RM) public/coverage/*.html public/coverage/*.html.gz
	$(MAKE) -j$(NPROC) check $(COVERAGE_MAKEFLAGS)
	$(GCOVR) $(GCOVRFLAGS) -s --html-nested '$@'
	find $| -name '*.css' -o -name '*.html' | $(XARGS_P) -- gzip -9kf

build/coverage.xml: FORCE | build/
	$(RM) $@ build/coverage.txt
	$(MAKE) -j$(NPROC) check $(COVERAGE_MAKEFLAGS)
	$(GCOVR) $(GCOVRFLAGS) --xml-pretty --xml '$@' --txt build/coverage.txt

public/coverage/: public/
	$(Q) mkdir -p $@


NON_PARALLEL_TARGETS += gcovr-html gcovr-xml coverage
NON_PARALLEL_TARGETS += public/coverage/index.html build/coverage.xml

.PHONY: gcovr-html gcovr-xml coverage
