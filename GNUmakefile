include mk/compat.mk
-include Config.mk
include mk/util.mk
include mk/build.mk
include mk/docs.mk
include mk/gen.mk
-include mk/dev.mk

# https://www.gnu.org/prep/standards/html_node/Directory-Variables.html
prefix ?= /usr/local
datarootdir ?= $(prefix)/share
exec_prefix ?= $(prefix)
bindir ?= $(exec_prefix)/bin
mandir ?= $(datarootdir)/man
man1dir ?= $(mandir)/man1
man5dir ?= $(mandir)/man5
appdir ?= $(datarootdir)/applications

INSTALL = install
INSTALL_PROGRAM = $(INSTALL)
INSTALL_DATA = $(INSTALL) -m 644
INSTALL_DESKTOP_FILE = desktop-file-install
RM = rm -f

all: $(dte)

install: all installdirs
	$(E) INSTALL '$(DESTDIR)$(bindir)/$(dte)'
	$(Q) $(INSTALL_PROGRAM) '$(dte)' '$(DESTDIR)$(bindir)'
	$(E) INSTALL '$(DESTDIR)$(man1dir)/dte.1'
	$(Q) $(INSTALL_DATA) docs/dte.1 '$(DESTDIR)$(man1dir)'
	$(E) INSTALL '$(DESTDIR)$(man5dir)/dterc.5'
	$(Q) $(INSTALL_DATA) docs/dterc.5 '$(DESTDIR)$(man5dir)'
	$(E) INSTALL '$(DESTDIR)$(man5dir)/dte-syntax.5'
	$(Q) $(INSTALL_DATA) docs/dte-syntax.5 '$(DESTDIR)$(man5dir)'

installdirs:
	$(Q) $(INSTALL) -d -m755 '$(DESTDIR)$(bindir)'
	$(Q) $(INSTALL) -d -m755 '$(DESTDIR)$(man1dir)'
	$(Q) $(INSTALL) -d -m755 '$(DESTDIR)$(man5dir)'

uninstall:
	$(RM) '$(DESTDIR)$(bindir)/$(dte)'
	$(RM) '$(DESTDIR)$(man1dir)/dte.1'
	$(RM) '$(DESTDIR)$(man5dir)/dterc.5'
	$(RM) '$(DESTDIR)$(man5dir)/dte-syntax.5'

install-desktop-file:
	$(E) INSTALL '$(DESTDIR)$(appdir)/dte.desktop'
	$(Q) $(INSTALL_DESKTOP_FILE) \
	  --dir='$(DESTDIR)$(appdir)' \
	  --set-key=TryExec --set-value='$(bindir)/$(dte)' \
	  --set-key=Exec --set-value='$(bindir)/$(dte) %F' \
	  $(if $(DESTDIR),, --rebuild-mime-info-cache) \
	  dte.desktop

uninstall-desktop-file:
	$(RM) '$(DESTDIR)$(appdir)/dte.desktop'
	$(if $(DESTDIR),, update-desktop-database -q '$(appdir)' || :)

check: $(test) all
	$(E) TEST $<
	$(Q) ./$<

installcheck: install
	$(E) TEST '$(DESTDIR)$(bindir)/$(dte)'
	$(Q) '$(DESTDIR)$(bindir)/$(dte)' -V >/dev/null

tags:
	ctags src/*.[ch] src/*/*.[ch] test/*.[ch]

clean:
	$(RM) $(CLEANFILES)
	$(if $(CLEANDIRS),$(RM) -r $(CLEANDIRS))


.DEFAULT_GOAL = all
.PHONY: all check install installdirs uninstall installcheck tags clean
.PHONY: install-desktop-file uninstall-desktop-file
.DELETE_ON_ERROR:

NON_PARALLEL_TARGETS += clean

ifeq "" "$(filter $(NON_PARALLEL_TARGETS), $(or $(MAKECMDGOALS),all))"
  ifeq "" "$(filter -j%, $(MAKEFLAGS))"
    MAKEFLAGS += -j$(NPROC)
  endif
endif
