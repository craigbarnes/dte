include mk/compat.mk
-include Config.mk
include mk/util.mk
include mk/build.mk
include mk/docs.mk
include mk/gen.mk
-include mk/dev.mk

prefix ?= /usr/local
bindir ?= $(prefix)/bin
datadir ?= $(prefix)/share
mandir ?= $(datadir)/man
man1dir ?= $(mandir)/man1
man5dir ?= $(mandir)/man5

INSTALL = install
INSTALL_PROGRAM = $(INSTALL)
INSTALL_DATA = $(INSTALL) -m 644
RM = rm -f

all: $(dte)

check: $(test) all
	$(E) TEST $<
	$(Q) ln -sf ../../README.md build/test/test-symlink
	$(Q) ./$<
	$(Q) diff -u build/test/env.txt test/data/env.txt
	$(Q) diff -u build/test/thai-utf8.txt test/data/thai-utf8.txt
# TODO: $(Q) diff -u build/test/thai-tis620.txt test/data/thai-tis620.txt
	$(Q) diff -u build/test/crlf.txt test/data/crlf.txt
	$(Q) $(RM) build/test/thai-*.txt

install: all
	$(Q) $(INSTALL) -d -m755 '$(DESTDIR)$(bindir)'
	$(Q) $(INSTALL) -d -m755 '$(DESTDIR)$(man1dir)'
	$(Q) $(INSTALL) -d -m755 '$(DESTDIR)$(man5dir)'
	$(E) INSTALL '$(DESTDIR)$(bindir)/$(dte)'
	$(Q) $(INSTALL_PROGRAM) '$(dte)' '$(DESTDIR)$(bindir)'
	$(E) INSTALL '$(DESTDIR)$(man1dir)/dte.1'
	$(Q) $(INSTALL_DATA) docs/dte.1 '$(DESTDIR)$(man1dir)'
	$(E) INSTALL '$(DESTDIR)$(man5dir)/dterc.5'
	$(Q) $(INSTALL_DATA) docs/dterc.5 '$(DESTDIR)$(man5dir)'
	$(E) INSTALL '$(DESTDIR)$(man5dir)/dte-syntax.5'
	$(Q) $(INSTALL_DATA) docs/dte-syntax.5 '$(DESTDIR)$(man5dir)'

uninstall:
	$(RM) '$(DESTDIR)$(bindir)/$(dte)'
	$(RM) '$(DESTDIR)$(man1dir)/dte.1'
	$(RM) '$(DESTDIR)$(man5dir)/dterc.5'
	$(RM) '$(DESTDIR)$(man5dir)/dte-syntax.5'

tags:
	ctags $$(find src/ test/ -type f -name '*.[ch]')

clean:
	$(RM) $(CLEANFILES)
	$(if $(CLEANDIRS),$(RM) -r $(CLEANDIRS))


.DEFAULT_GOAL = all
.PHONY: all check install uninstall tags clean
.DELETE_ON_ERROR:
