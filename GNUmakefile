include mk/compat.mk
include mk/util.mk
ifneq "$(NO_CONFIG_MK)" "1"
-include Config.mk
endif
include mk/build.mk
include mk/docs.mk
include mk/gen.mk
-include mk/dev.mk
-include mk/coverage.mk
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

CONTRIB_SCRIPTS = \
    fzf.sh git-changes.sh lf-wrapper.sh longest-line.awk \
    open-c-header.sh ranger-wrapper.sh xtag.sh

INSTALL_TARGETS_BASIC := bin man bash-completion
INSTALL_TARGETS_FULL := $(INSTALL_TARGETS_BASIC) icons desktop-file appstream

ifeq "$(KERNEL)" "Darwin"
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
	$(Q) $(INSTALL_DATA) completion.bash '$(DESTDIR)$(bashcompletiondir)/$(dte)'

uninstall-bash-completion:
	$(RM) '$(DESTDIR)$(bashcompletiondir)/$(dte)'

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

install-contrib:
	$(Q) $(INSTALL) -d -m755 '$(DESTDIR)$(datadir)/dte'
	$(Q) $(foreach f, $(CONTRIB_SCRIPTS), \
	  $(LOG) INSTALL '$(DESTDIR)$(datadir)/dte/$(f)'; \
	  $(INSTALL_PROGRAM) 'contrib/$(f)' '$(DESTDIR)$(datadir)/dte'; \
	)
	$(E) INSTALL '$(DESTDIR)$(datadir)/dte/README.md'
	$(Q) $(INSTALL_DATA) contrib/README.md '$(DESTDIR)$(datadir)/dte'

uninstall-contrib:
	$(RM) -r '$(DESTDIR)$(datadir)/dte'

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


INSTALL_TARGETS_ALL := $(INSTALL_TARGETS_FULL) full basic contrib
.DEFAULT_GOAL = all
.PHONY: all clean tags install uninstall
.PHONY: check check-tests check-opts installcheck bench
.PHONY: $(foreach T, $(INSTALL_TARGETS_ALL), install-$(T) uninstall-$(T))
.DELETE_ON_ERROR:

NON_PARALLEL_TARGETS += clean install% uninstall%

ifeq "" "$(filter $(NON_PARALLEL_TARGETS), $(or $(MAKECMDGOALS),all))"
  ifeq "" "$(filter -j%, $(MAKEFLAGS))"
    MAKEFLAGS += -j$(NPROC)
  endif
endif
