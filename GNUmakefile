include mk/compat.mk
include mk/util.mk
-include Config.mk
include mk/build.mk
include mk/docs.mk
-include mk/dev.mk

prefix ?= /usr/local
bindir ?= $(prefix)/bin
datadir ?= $(prefix)/share
mandir ?= $(datadir)/man
man1dir ?= $(mandir)/man1
man5dir ?= $(mandir)/man5

INSTALL = install
RM = rm -f

all: $(dte) man

install: all
	$(INSTALL) -d -m755 $(DESTDIR)$(bindir)
	$(INSTALL) -d -m755 $(DESTDIR)$(man1dir)
	$(INSTALL) -d -m755 $(DESTDIR)$(man5dir)
	$(INSTALL) -m755 $(dte) $(DESTDIR)$(bindir)
	$(INSTALL) -m644 $(man1) $(DESTDIR)$(man1dir)
	$(INSTALL) -m644 $(man5) $(DESTDIR)$(man5dir)

uninstall:
	$(RM) $(DESTDIR)$(bindir)/$(dte)
	$(RM) $(addprefix $(DESTDIR)$(man1dir)/, $(man1))
	$(RM) $(addprefix $(DESTDIR)$(man5dir)/, $(man5))

check: $(test) all
	$(E) TEST $<
	$(Q) $<

check-commands: $(dte) | build/test/
	@$(dte) -R -c "$$(cat test/thai.dterc | sed '/^\#/d;/^$$/d' | tr '\n' ';')"
	@diff -q build/test/thai-utf8.txt test/thai-utf8.txt
	@diff -q build/test/thai-tis620.txt test/thai-tis620.txt
	@$(RM)  build/test/thai-utf8.txt build/test/thai-tis620.txt

tags:
	ctags src/*.[ch]

clean:
	$(RM) $(CLEANFILES)
	$(if $(CLEANDIRS),$(RM) -r $(CLEANDIRS))

distclean: clean
	$(RM) tags


.DEFAULT_GOAL = all
.PHONY: all install uninstall check check-commands tags clean distclean
.DELETE_ON_ERROR:
