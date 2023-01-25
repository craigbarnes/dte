# This makefile contains various targets that are only useful for
# development purposes and is NOT included in release tarballs

GCOVR ?= gcovr
LCOV ?= lcov
LCOVFLAGS ?= --config-file mk/lcovrc
LCOV_REMOVE = $(foreach PAT, $(2), $(LCOV) -r $(1) -o $(1) '*/$(PAT)';)
GENHTML ?= genhtml
GENHTMLFLAGS ?= --config-file mk/lcovrc --title dte
GZIP_ALL_HTML_CSS = find $(1) -name '*.css' -o -name '*.html' | $(XARGS_P) -- gzip -9kf

EXCLUDE_FROM_COVERAGE = \
    src/lock.c \
    src/screen*.c \
    src/signals.c \
    src/terminal/input.c \
    src/terminal/ioctl.c \
    src/terminal/mode.c \
    src/util/debug.c \
    test/test.c

coverage-report: lcov
gcovr: gcovr-html
gcovr-html: public/coverage/gcovr.html
gcovr-xml: build/coverage.xml

lcov: build/docs/lcov.css | public/coverage/
	$(MAKE) -j$(NPROC) check CFLAGS='-Og -g -pipe --coverage -fno-inline' DEBUG=3 USE_SANITIZER=
	$(LCOV) $(LCOVFLAGS) -c -b . -d build/ -o build/coverage.info
	$(call LCOV_REMOVE, build/coverage.info, $(EXCLUDE_FROM_COVERAGE))
	$(GENHTML) $(GENHTMLFLAGS) -o public/coverage/ build/coverage.info
	$(call GZIP_ALL_HTML_CSS, '$|')

build/docs/lcov.css: docs/lcov-orig.css docs/lcov-extra.css | build/docs/
	$(E) CSSCAT $@
	$(Q) cat $^ > $@

public/coverage/gcovr.html: gcovr.cfg FORCE | public/coverage/
	$(RM) public/coverage/gcovr*
	$(MAKE) -j$(NPROC) check CFLAGS='-Og -g -pipe --coverage -fno-inline' DEBUG=3 USE_SANITIZER=
	$(GCOVR) -j$(NPROC) -sp --config '$<' --html-details '$@'
	$(call GZIP_ALL_HTML_CSS, '$|')

build/coverage.xml: gcovr.cfg FORCE | build/
	$(RM) $@ build/coverage.txt
	$(MAKE) -j$(NPROC) check CFLAGS='-Og -g -pipe --coverage -fno-inline' DEBUG=3 USE_SANITIZER=
	$(GCOVR) -j$(NPROC) -p --config '$<' --xml-pretty --xml '$@' --txt build/coverage.txt

public/coverage/: public/
	$(Q) mkdir -p $@


NON_PARALLEL_TARGETS += coverage-report lcov gcovr gcovr-html gcovr-xml
NON_PARALLEL_TARGETS += public/coverage/gcovr.html build/coverage.xml

.PHONY: coverage-report lcov gcovr gcovr-html gcovr-xml
