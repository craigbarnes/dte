include mk/compat.mk
include mk/prelude.mk
include mk/build.mk
include mk/docs.mk
include mk/gen.mk
-include tools/mk/dev.mk
include mk/help.mk

# https://www.gnu.org/prep/standards/html_node/Directory-Variables.html
prefix ?= /usr/local
datarootdir ?= $(prefix)/share
datadir ?= $(datarootdir)
exec_prefix ?= $(prefix)
bindir ?= $(exec_prefix)/bin
mandir ?= $(datarootdir)/man
man1dir ?= $(mandir)/man1
man5dir ?= $(mandir)/man5
appdir ?= $(datarootdir)/applications
icondir ?= $(datarootdir)/icons
hicolordir ?= $(icondir)/hicolor
metainfodir ?= $(datarootdir)/metainfo
bashcompletiondir ?= $(datarootdir)/bash-completion/completions
fishcompletiondir ?= $(datarootdir)/fish/vendor_completions.d
zshcompletiondir ?= $(datarootdir)/zsh/site-functions
appid = dte

INSTALL = install
INSTALL_PROGRAM = $(INSTALL)
INSTALL_DATA = $(INSTALL) -m 644
RM = rm -f

# The following command is run after the *install-desktop-file targets,
# unless the DESTDIR variable is set. The presence of DESTDIR usually
# indicates a distro packaging environment, in which case the equivalent,
# distro-provided macros/hooks should be used instead.
define POSTINSTALL_DESKTOP
( \
  update-desktop-database -q '$(appdir)' 2>/dev/null \
  && $(LOG) UPDATE '$(appdir)/mimeinfo.cache' \
) || :
endef

define POSTINSTALL_ICONS
( \
  touch -c '$(hicolordir)' \
  && gtk-update-icon-cache -qtf '$(hicolordir)' 2>/dev/null \
  && $(LOG) UPDATE '$(hicolordir)/icon-theme.cache' \
) || :
endef

INSTALL_TARGETS_BASIC := bin man bash-completion fish-completion zsh-completion
INSTALL_TARGETS_FULL := $(INSTALL_TARGETS_BASIC) icons desktop-file appstream

# See "installation targets" in docs/packaging.md
ifeq "$(NO_INSTALL_XDG_CLUTTER)" "1"
 INSTALL_TARGETS := $(INSTALL_TARGETS_BASIC)
else
 INSTALL_TARGETS := $(INSTALL_TARGETS_FULL)
endif

all: $(dte)
check: check-tests check-opts
install: $(addprefix install-, $(INSTALL_TARGETS))
uninstall: $(addprefix uninstall-, $(INSTALL_TARGETS))
install-full: $(addprefix install-, $(INSTALL_TARGETS_FULL))
install-basic: $(addprefix install-, $(INSTALL_TARGETS_BASIC))
uninstall-full: $(addprefix uninstall-, $(INSTALL_TARGETS_FULL))
uninstall-basic: $(addprefix uninstall-, $(INSTALL_TARGETS_BASIC))

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
	$(Q) $(INSTALL_DATA) share/completion.bash '$(DESTDIR)$(bashcompletiondir)/$(dte)'

uninstall-bash-completion:
	$(RM) '$(DESTDIR)$(bashcompletiondir)/$(dte)'

install-fish-completion:
	$(Q) $(INSTALL) -d -m755 '$(DESTDIR)$(fishcompletiondir)'
	$(E) INSTALL '$(DESTDIR)$(fishcompletiondir)/$(dte).fish'
	$(Q) $(INSTALL_DATA) share/completion.fish '$(DESTDIR)$(fishcompletiondir)/$(dte).fish'

uninstall-fish-completion:
	$(RM) '$(DESTDIR)$(fishcompletiondir)/$(dte).fish'

install-zsh-completion:
	$(Q) $(INSTALL) -d -m755 '$(DESTDIR)$(zshcompletiondir)'
	$(E) INSTALL '$(DESTDIR)$(zshcompletiondir)/_$(dte)'
	$(Q) $(INSTALL_DATA) share/completion.zsh '$(DESTDIR)$(zshcompletiondir)/_$(dte)'

uninstall-zsh-completion:
	$(RM) '$(DESTDIR)$(zshcompletiondir)/_$(dte)'

install-desktop-file:
	$(Q) $(INSTALL) -d -m755 '$(DESTDIR)$(appdir)'
	$(E) INSTALL '$(DESTDIR)$(appdir)/$(appid).desktop'
	$(Q) mk/dtfilter.sh '$(bindir)/$(dte)' <'share/$(appid).desktop' >'$(DESTDIR)$(appdir)/$(appid).desktop'
	$(Q) $(if $(DESTDIR),, $(POSTINSTALL_DESKTOP))

uninstall-desktop-file:
	$(RM) '$(DESTDIR)$(appdir)/$(appid).desktop'
	$(if $(DESTDIR),, $(POSTINSTALL_DESKTOP))

# https://specifications.freedesktop.org/icon-theme-spec/icon-theme-spec-latest.html#install_icons
install-icons:
	$(Q) $(INSTALL) -d -m755 '$(DESTDIR)$(hicolordir)/scalable/apps'
	$(Q) $(INSTALL) -d -m755 '$(DESTDIR)$(hicolordir)/48x48/apps'
	$(E) INSTALL '$(DESTDIR)$(hicolordir)/scalable/apps/dte.svg'
	$(Q) $(INSTALL_DATA) 'share/dte.svg' '$(DESTDIR)$(hicolordir)/scalable/apps/dte.svg'
	$(E) INSTALL '$(DESTDIR)$(hicolordir)/48x48/apps/dte.png'
	$(Q) $(INSTALL_DATA) 'share/dte-48x48.png' '$(DESTDIR)$(hicolordir)/48x48/apps/dte.png'
	$(Q) $(if $(DESTDIR),, $(POSTINSTALL_ICONS))

uninstall-icons:
	$(RM) '$(DESTDIR)$(hicolordir)/scalable/apps/dte.svg'
	$(RM) '$(DESTDIR)$(hicolordir)/48x48/apps/dte.png'
	$(if $(DESTDIR),, $(POSTINSTALL_ICONS))

install-appstream:
	$(Q) $(INSTALL) -d -m755 '$(DESTDIR)$(metainfodir)'
	$(E) INSTALL '$(DESTDIR)$(metainfodir)/$(appid).appdata.xml'
	$(Q) $(INSTALL_DATA) 'share/$(appid).appdata.xml' '$(DESTDIR)$(metainfodir)/$(appid).appdata.xml'

uninstall-appstream:
	$(RM) '$(DESTDIR)$(metainfodir)/$(appid).appdata.xml'

check-tests: $(test) all
	$(E) EXEC '$(test)'
	$(Q) ./$(test) $(TESTFLAGS)

check-opts: $(dte) | build/test/
	$(E) EXEC 'test/check-opts.sh'
	$(Q) test/check-opts.sh './$(dte)' '$(VERSION)' build/test/check-opts.log

installcheck: install
	$(E) EXEC '$(DESTDIR)$(bindir)/$(dte) -V'
	$(Q) '$(DESTDIR)$(bindir)/$(dte)' -V >/dev/null

bench: $(bench)
	$(E) EXEC '$(bench)'
	$(Q) ./$(bench)

clean:
	$(RM) $(CLEANFILES)
	$(if $(CLEANDIRS),$(RM) -r $(CLEANDIRS))


INSTALL_TARGETS_ALL := $(INSTALL_TARGETS_FULL) full basic
.DEFAULT_GOAL = all
.PHONY: all clean install uninstall
.PHONY: check check-tests check-opts installcheck bench
.PHONY: $(foreach T, $(INSTALL_TARGETS_ALL), install-$(T) uninstall-$(T))
.DELETE_ON_ERROR:

NON_PARALLEL_TARGETS += clean install% uninstall%

# Enable parallel builds automatically, if `make -jN` wasn't used explicitly
# and no non-parallel targets are being rebuilt (and MAKE_VERSION >= 4.3).
# https://lists.gnu.org/archive/html/info-gnu/2020-01/msg00004.html#:~:text=their-,MAKEFLAGS
ifeq "" "$(call filter-cmdgoals, $(NON_PARALLEL_TARGETS))"
  ifeq "" "$(filter -j%, $(MAKEFLAGS))"
    MAKEFLAGS += -j$(NPROC)
  endif
endif
