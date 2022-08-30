include mk/compat.mk
include mk/util.mk
-include Config.mk
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
metainfodir ?= $(datarootdir)/metainfo
bashcompletiondir ?= $(call pkg-var, bash-completion, completionsdir)
appid = dte

INSTALL = install
INSTALL_PROGRAM = $(INSTALL)
INSTALL_DATA = $(INSTALL) -m 644
INSTALL_DESKTOP_FILE = desktop-file-install
RM = rm -f

all: $(dte)
check: check-tests check-opts
install: install-bin install-man
uninstall: uninstall-bin uninstall-man
install-full: install install-bash-completion install-desktop-file install-appstream
uninstall-full: uninstall uninstall-bash-completion uninstall-desktop-file uninstall-appstream

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
	@$(if $(bashcompletiondir),, $(error $${bashcompletiondir} is unset))
	$(Q) $(INSTALL) -d -m755 '$(DESTDIR)$(bashcompletiondir)'
	$(E) INSTALL '$(DESTDIR)$(bashcompletiondir)/$(dte)'
	$(Q) $(INSTALL_DATA) completion.bash '$(DESTDIR)$(bashcompletiondir)/$(dte)'

uninstall-bash-completion:
	@$(if $(bashcompletiondir),, $(error $${bashcompletiondir} is unset))
	$(RM) '$(DESTDIR)$(bashcompletiondir)/$(dte)'

install-desktop-file:
	$(E) INSTALL '$(DESTDIR)$(appdir)/$(appid).desktop'
	$(Q) $(INSTALL_DESKTOP_FILE) \
	  --dir='$(DESTDIR)$(appdir)' \
	  --set-key=TryExec --set-value='$(bindir)/$(dte)' \
	  --set-key=Exec --set-value='$(bindir)/$(dte) %F' \
	  $(if $(DESTDIR),, --rebuild-mime-info-cache) \
	  '$(appid).desktop'

uninstall-desktop-file:
	$(RM) '$(DESTDIR)$(appdir)/$(appid).desktop'
	$(if $(DESTDIR),, update-desktop-database -q '$(appdir)' || :)

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


.DEFAULT_GOAL = all
.PHONY: all install install-bin install-man
.PHONY: uninstall uninstall-bin uninstall-man
.PHONY: install-full uninstall-full
.PHONY: install-bash-completion uninstall-bash-completion
.PHONY: install-desktop-file uninstall-desktop-file
.PHONY: install-appstream uninstall-appstream
.PHONY: check check-tests check-opts installcheck bench tags clean
.DELETE_ON_ERROR:

NON_PARALLEL_TARGETS += clean install% uninstall%

ifeq "" "$(filter $(NON_PARALLEL_TARGETS), $(or $(MAKECMDGOALS),all))"
  ifeq "" "$(filter -j%, $(MAKEFLAGS))"
    MAKEFLAGS += -j$(NPROC)
  endif
endif
