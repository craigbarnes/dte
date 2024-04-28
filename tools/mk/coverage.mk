# This makefile contains various targets that are only useful for
# development purposes and is NOT included in release tarballs

GCOVR ?= gcovr
GCOVRFLAGS ?= -j$(NPROC) --config gcovr.cfg --sort-percentage

coverage-report: gcovr-html
gcovr-html: public/coverage/index.html
gcovr-xml: build/coverage.xml

public/coverage/index.html: FORCE | public/coverage/
	$(RM) public/coverage/*.html public/coverage/*.html.gz
	$(MAKE) -j$(NPROC) check CC='ccache gcc' CFLAGS='-Og -g -pipe --coverage -fno-inline' DEBUG=3 NO_CONFIG_MK=1
	$(GCOVR) $(GCOVRFLAGS) -s --html-nested '$@'
	find $| -name '*.css' -o -name '*.html' | $(XARGS_P) -- gzip -9kf

build/coverage.xml: FORCE | build/
	$(RM) $@ build/coverage.txt
	$(MAKE) -j$(NPROC) check CC='ccache gcc' CFLAGS='-Og -g -pipe --coverage -fno-inline' DEBUG=3 NO_CONFIG_MK=1
	$(GCOVR) $(GCOVRFLAGS) --xml-pretty --xml '$@' --txt build/coverage.txt

public/coverage/: public/
	$(Q) mkdir -p $@


NON_PARALLEL_TARGETS += coverage-report gcovr-html gcovr-xml
NON_PARALLEL_TARGETS += public/coverage/index.html build/coverage.xml

.PHONY: coverage-report gcovr-html gcovr-xml
