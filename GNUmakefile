include mk/compat.mk
include mk/util.mk
-include Config.mk
include mk/build.mk
include mk/check.mk
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

tags:
	ctags src/*.[ch]

clean:
	$(RM) $(CLEANFILES)
	$(if $(CLEANDIRS),$(RM) -r $(CLEANDIRS))

distclean: clean
	$(RM) tags


.DEFAULT_GOAL = all
.PHONY: all install uninstall tags clean distclean
.DELETE_ON_ERROR:
