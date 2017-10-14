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

INSTALL = install
RM = rm -f

syntax_files := \
    awk c config css d diff docker dte gitcommit gitrebase go html html+smarty \
    ini java javascript lua mail make markdown meson php python robotstxt \
    ruby sh smarty sql vala xml

binding := $(addprefix share/binding/, default classic builtin)
color := $(addprefix share/color/, darkgray light light256)
compiler:= $(addprefix share/compiler/, gcc go)
config := $(addprefix share/, filetype option rc)
syntax := $(addprefix share/syntax/, $(syntax_files))

all: $(dte) man

install: all
	$(INSTALL) -d -m755 $(DESTDIR)$(bindir)
	$(INSTALL) -d -m755 $(DESTDIR)$(PKGDATADIR)/binding
	$(INSTALL) -d -m755 $(DESTDIR)$(PKGDATADIR)/color
	$(INSTALL) -d -m755 $(DESTDIR)$(PKGDATADIR)/compiler
	$(INSTALL) -d -m755 $(DESTDIR)$(PKGDATADIR)/syntax
	$(INSTALL) -d -m755 $(DESTDIR)$(mandir)/man1
	$(INSTALL) -d -m755 $(DESTDIR)$(mandir)/man5
	$(INSTALL) -m755 $(dte)      $(DESTDIR)$(bindir)
	$(INSTALL) -m644 $(config)   $(DESTDIR)$(PKGDATADIR)
	$(INSTALL) -m644 $(binding)  $(DESTDIR)$(PKGDATADIR)/binding
	$(INSTALL) -m644 $(color)    $(DESTDIR)$(PKGDATADIR)/color
	$(INSTALL) -m644 $(compiler) $(DESTDIR)$(PKGDATADIR)/compiler
	$(INSTALL) -m644 $(syntax)   $(DESTDIR)$(PKGDATADIR)/syntax
	$(INSTALL) -m644 $(man1)     $(DESTDIR)$(mandir)/man1
	$(INSTALL) -m644 $(man5)     $(DESTDIR)$(mandir)/man5

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
.PHONY: all install check check-commands tags clean distclean
.DELETE_ON_ERROR:
