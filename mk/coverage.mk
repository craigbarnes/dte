# This makefile contains various targets that are only useful for
# development purposes and is NOT included in release tarballs

GCOVR ?= gcovr
LCOV ?= lcov
LCOVFLAGS ?= --config-file mk/lcovrc
LCOV_REMOVE = $(foreach PAT, $(2), $(LCOV) -r $(1) -o $(1) '*/$(PAT)';)
GENHTML ?= genhtml
GENHTMLFLAGS ?= --config-file mk/lcovrc --title dte

EXCLUDE_FROM_COVERAGE = \
    src/lock.c \
    src/screen*.c \
    src/signals.c \
    src/terminal/input.c \
    src/terminal/ioctl.c \
    src/terminal/mode.c \
    src/util/debug.c \
    test/test.c

coverage-report: build/docs/lcov.css
	$(MAKE) -j$(NPROC) check CFLAGS='-Og -g -pipe --coverage -fno-inline' DEBUG=3 USE_SANITIZER=
	$(LCOV) $(LCOVFLAGS) -c -b . -d build/ -o build/coverage.info
	$(call LCOV_REMOVE, build/coverage.info, $(EXCLUDE_FROM_COVERAGE))
	$(GENHTML) $(GENHTMLFLAGS) -o public/coverage/ build/coverage.info
	find public/coverage/ -type f -regex '.*\.\(css\|html\)$$' | \
	  $(XARGS_P) -- gzip -9kf

build/docs/lcov.css: docs/lcov-orig.css docs/lcov-extra.css | build/docs/
	$(E) CSSCAT $@
	$(Q) cat $^ > $@

build/coverage.xml: gcovr-xml.cfg | build/
	$(MAKE) -j$(NPROC) check CFLAGS='-Og -g -pipe --coverage -fno-inline' DEBUG=3 USE_SANITIZER=
	$(GCOVR) -j$(NPROC) -s --config $< --xml-pretty --xml $@


NON_PARALLEL_TARGETS += coverage-report
.PHONY: coverage-report
