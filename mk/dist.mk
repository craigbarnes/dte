DIST_VERSIONS = 1.0
DIST = $(addprefix public/dist/dte-, $(addsuffix .tar.gz, $(DIST_VERSIONS)))

dist: $(DIST)

check-dist: dist
	sha1sum -c mk/dist-sha1sums.txt

$(DIST): public/dist/dte-%.tar.gz: | public/dist/
	git archive --prefix=dte-$*/ -o $@ v$*

public/dist/:
	mkdir -p $@


.PHONY: dist check-dist
