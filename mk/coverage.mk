# This makefile contains various targets that are only useful for
# development purposes and is NOT included in release tarballs

GCOVR ?= gcovr
GCOVRFLAGS ?= -j$(NPROC) --config gcovr.cfg --sort-percentage

coverage-report: gcovr
gcovr: gcovr-html
gcovr-html: public/coverage/index.html
gcovr-xml: build/coverage.xml

# TODO: Use `--html-nested` instead of `--html-details` when Docker
# image used for GitLab CI `pages` job has `gcovr >= 6.0`
public/coverage/index.html: FORCE | public/coverage/
	$(RM) public/coverage/*.html public/coverage/*.html.gz
	$(MAKE) -j$(NPROC) check CFLAGS='-Og -g -pipe --coverage -fno-inline' DEBUG=3 USE_SANITIZER=
	$(GCOVR) $(GCOVRFLAGS) -s --html-details '$@'
	find $| -name '*.css' -o -name '*.html' | $(XARGS_P) -- gzip -9kf

build/coverage.xml: FORCE | build/
	$(RM) $@ build/coverage.txt
	$(MAKE) -j$(NPROC) check CFLAGS='-Og -g -pipe --coverage -fno-inline' DEBUG=3 USE_SANITIZER=
	$(GCOVR) $(GCOVRFLAGS) --xml-pretty --xml '$@' --txt build/coverage.txt

public/coverage/: public/
	$(Q) mkdir -p $@


NON_PARALLEL_TARGETS += coverage-report gcovr gcovr-html gcovr-xml
NON_PARALLEL_TARGETS += public/coverage/index.html build/coverage.xml

.PHONY: coverage-report gcovr gcovr-html gcovr-xml
