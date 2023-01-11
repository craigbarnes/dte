include mk/compat.mk
include mk/util.mk
-include Config.mk
include mk/build.mk
include mk/docs.mk
include mk/gen.mk
-include mk/dev.mk
include mk/help.mk

# https://www.gnu.org/prep/standards/html_node/Directory-Variables.html
prefix ?= /usr/local
datarootdir ?= $(prefix)/share
exec_prefix ?= $(prefix)
bindir ?= $(exec_prefix)/bin
mandir ?= $(datarootdir)/man
man1dir ?= $(mandir)/man1
man5dir ?= $(mandir)/man5
appdir ?= $(datarootdir)/applications
metainfodir ?= $(datarootdir)/metainfo
bashcompletiondir ?= $(datarootdir)/bash-completion/completions
appid = dte

INSTALL = install
INSTALL_PROGRAM = $(INSTALL)
INSTALL_DATA = $(INSTALL) -m 644
RM = rm -f

# The following command is run after the *install-desktop-file targets,
# unless the DESTDIR variable is set. The presence of DESTDIR usually
# indicates a distro packaging environment, in which case the equivalent,
# distro-provided macros/hooks should be used instead.
define POSTINSTALL
 (update-desktop-database -q '$(appdir)' 2>/dev/null && $(LOG) UPDATE '$(appdir)/mimeinfo.cache') || :
endef

all: $(dte)
check: check-tests check-opts check-errors
install: install-bin install-man install-bash-completion
uninstall: uninstall-bin uninstall-man uninstall-bash-completion

ifneq "$(KERNEL)" "Darwin"
 install: install-desktop-file install-appstream
 uninstall: uninstall-desktop-file uninstall-appstream
endif

install-bin: all
	$(Q) $(INSTALL) -d -m755 '$(DESTDIR)$(bindir)'
	$(E) INSTALL '$(DESTDIR)$(bindir)/$(dte)'
	$(Q) $(INSTALL_PROGRAM) '$(dte)' '$(DESTDIR)$(bindir)'

install-man:
	$(Q) $(INSTALL) -d -m755 '$(DESTDIR)$(man1dir)'
	$(Q) $(INSTALL) -d -m755 '$(DESTDIR)$(man5dir)'
	$(E) INSTALL '$(DESTDIR)$(man1dir)/dte.1'
	$(Q) $(INSTALL_DATA) docs/dte.1 '$(DESTDIR)$(man1dir)'
	$(E) INSTALL '$(DESTDIR)$(man5dir)/dterc.5'
	$(Q) $(INSTALL_DATA) docs/dterc.5 '$(DESTDIR)$(man5dir)'
	$(E) INSTALL '$(DESTDIR)$(man5dir)/dte-syntax.5'
	$(Q) $(INSTALL_DATA) docs/dte-syntax.5 '$(DESTDIR)$(man5dir)'

uninstall-bin:
	$(RM) '$(DESTDIR)$(bindir)/$(dte)'

uninstall-man:
	$(RM) '$(DESTDIR)$(man1dir)/dte.1'
	$(RM) '$(DESTDIR)$(man5dir)/dterc.5'
	$(RM) '$(DESTDIR)$(man5dir)/dte-syntax.5'

install-bash-completion:
	$(Q) $(INSTALL) -d -m755 '$(DESTDIR)$(bashcompletiondir)'
	$(E) INSTALL '$(DESTDIR)$(bashcompletiondir)/$(dte)'
	$(Q) $(INSTALL_DATA) completion.bash '$(DESTDIR)$(bashcompletiondir)/$(dte)'

uninstall-bash-completion:
	$(RM) '$(DESTDIR)$(bashcompletiondir)/$(dte)'

install-desktop-file:
	$(Q) $(INSTALL) -d -m755 '$(DESTDIR)$(appdir)'
	$(E) INSTALL '$(DESTDIR)$(appdir)/$(appid).desktop'
	$(Q) mk/dtfilter.sh '$(bindir)/$(dte)' <'$(appid).desktop' >'$(DESTDIR)$(appdir)/$(appid).desktop'
	$(Q) $(if $(DESTDIR),, $(POSTINSTALL))

uninstall-desktop-file:
	$(RM) '$(DESTDIR)$(appdir)/$(appid).desktop'
	$(if $(DESTDIR),, $(POSTINSTALL))

install-appstream:
	$(Q) $(INSTALL) -d -m755 '$(DESTDIR)$(metainfodir)'
	$(E) INSTALL '$(DESTDIR)$(metainfodir)/$(appid).appdata.xml'
	$(Q) $(INSTALL_DATA) '$(appid).appdata.xml' '$(DESTDIR)$(metainfodir)/$(appid).appdata.xml'

uninstall-appstream:
	$(RM) '$(DESTDIR)$(metainfodir)/$(appid).appdata.xml'

check-tests: $(test) all
	$(E) EXEC '$(test)'
	$(Q) ./$(test)

check-opts: $(dte)
	$(E) EXEC 'test/check-opts.sh'
	$(Q) test/check-opts.sh './$<' '$(VERSION)'

check-errors: $(dte)
	$(E) EXEC 'test/check-errors.sh'
	$(Q) test/check-errors.sh './$<'

installcheck: install
	$(E) EXEC '$(DESTDIR)$(bindir)/$(dte) -V'
	$(Q) '$(DESTDIR)$(bindir)/$(dte)' -V >/dev/null

bench: $(bench)
	$(E) EXEC '$(bench)'
	$(Q) ./$(bench)

tags:
	ctags src/*.[ch] src/*/*.[ch] test/*.[ch]

clean:
	$(RM) $(CLEANFILES)
	$(if $(CLEANDIRS),$(RM) -r $(CLEANDIRS))


INSTALL_SUBTARGETS = bin man bash-completion desktop-file appstream
.DEFAULT_GOAL = all
.PHONY: all clean tags install uninstall
.PHONY: check check-tests check-opts check-errors installcheck bench
.PHONY: $(foreach T, $(INSTALL_SUBTARGETS), install-$(T) uninstall-$(T))
.DELETE_ON_ERROR:

NON_PARALLEL_TARGETS += clean install% uninstall%

ifeq "" "$(filter $(NON_PARALLEL_TARGETS), $(or $(MAKECMDGOALS),all))"
  ifeq "" "$(filter -j%, $(MAKEFLAGS))"
    MAKEFLAGS += -j$(NPROC)
  endif
endif
