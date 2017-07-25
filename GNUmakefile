# To change build options use either command line or put the variables
# to Config.mk file (optional).
#
# Define V=1 for more verbose build.
#
# Define WERROR if most warnings should be treated as errors.
#
# Define NO_DEPS to disable automatic dependency calculation.
# Dependency calculation is enabled by default if CC supports
# the -MMD -MP -MF options.

CC = gcc
LD = $(CC)
CFLAGS = -g -O2 -Wall
LDFLAGS =
HOST_CC = gcc
HOST_LD = $(HOST_CC)
HOST_CFLAGS = -g -O2 -Wall
HOST_LDFLAGS =
INSTALL = install
SED = sed
RM = rm -f
prefix = /usr/local
bindir = $(prefix)/bin
datadir = $(prefix)/share
mandir = $(datadir)/man

# 0: Disable debugging.
# 1: Enable BUG_ON() and light-weight sanity checks.
# 2: Enable logging to ~/.$(PROGRAM)/debug.log.
# 3: Enable expensive sanity checks.
DEBUG = 1

# Enabled if CC supports them
WARNINGS = \
    -Wdeclaration-after-statement \
    -Wformat-security \
    -Wmissing-prototypes \
    -Wold-style-definition \
    -Wwrite-strings \
    -Wundef \
    -Wshadow

# End of configuration

PROGRAM = dte
VERSION = 1.0
TARNAME = $(PROGRAM)-$(VERSION)
PKGDATADIR = $(datadir)/$(PROGRAM)
LIBS =
X =
uname_S := $(shell sh -c 'uname -s 2>/dev/null || echo not')
uname_O := $(shell sh -c 'uname -o 2>/dev/null || echo not')
uname_R := $(shell sh -c 'uname -r 2>/dev/null || echo not')

editor_objects := $(addsuffix .o, \
    alias bind block buffer-iter buffer cconv change cmdline color \
    command-mode commands common compiler completion config ctags ctype \
    cursed decoder detect edit editor encoder encoding env error \
    file-history file-location file-option filetype fork format-status \
    frame gbuf git-open history hl indent input-special iter key \
    load-save lock main modes move msg normal-mode obuf options \
    parse-args parse-command path ptr-array regexp run screen \
    screen-tabbar screen-view search-mode search selection spawn state \
    syntax tabbar tag term-caps term uchar unicode vars view wbuf window \
    xmalloc )

test_objects := test-main.o

syntax_files := \
    awk c config css diff docker gitcommit gitrebase go html html+smarty \
    ini java javascript lua mail make php python robotstxt sh smarty sql xml \
    $(PROGRAM)

binding := $(addprefix share/binding/, default classic)
color := $(addprefix share/color/, darkgray light light256)
compiler:= $(addprefix share/compiler/, gcc go)
config := $(addprefix share/, filetype rc)
syntax := $(addprefix share/syntax/, $(syntax_files))

OBJECTS := $(editor_objects) $(test_objects)

all: $(PROGRAM)$(X) test man

-include Config.mk
include mk/compat.mk
include mk/build.mk
include mk/docs.mk

LIBS += -lcurses

ifeq "$(uname_S)" "Darwin"
  LIBS += -liconv
endif
ifeq "$(uname_O)" "Cygwin"
  LIBS += -liconv
  X = .exe
endif
ifeq "$(uname_S)" "FreeBSD"
  # libc of FreeBSD 10.0 includes iconv
  ifeq ($(shell expr "$(uname_R)" : '[0-9]\.'),2)
    LIBS += -liconv
    BASIC_CFLAGS += -I/usr/local/include
    BASIC_LDFLAGS += -L/usr/local/lib
  endif
endif
ifeq "$(uname_S)" "OpenBSD"
  LIBS += -liconv
  BASIC_CFLAGS += -I/usr/local/include
  BASIC_LDFLAGS += -L/usr/local/lib
endif
ifeq "$(uname_S)" "NetBSD"
  ifeq ($(shell expr "$(uname_R)" : '[01]\.'),2)
    LIBS += -liconv
  endif
  BASIC_CFLAGS += -I/usr/pkg/include
  BASIC_LDFLAGS += -L/usr/pkg/lib
endif

# Clang does not like container_of()
ifneq "$(CC)" "clang"
  WARNINGS += -Wcast-align
endif

BASIC_CFLAGS += $(call cc-option,$(WARNINGS))
BASIC_CFLAGS += $(call cc-option,-Wno-pointer-sign) # char vs unsigned char madness

ifdef WERROR
  BASIC_CFLAGS += $(call cc-option,-Werror -Wno-error=shadow -Wno-error=unused-variable)
endif

BASIC_CFLAGS += -DDEBUG=$(DEBUG)

$(OBJECTS): .CFLAGS

.CFLAGS: FORCE
	@mk/update-option "$(CC) $(CFLAGS) $(BASIC_CFLAGS)" $@

vars.o: BASIC_CFLAGS += -DPROGRAM=\"$(PROGRAM)\" -DVERSION=\"$(VERSION)\" -DPKGDATADIR=\"$(PKGDATADIR)\"
vars.o: .VARS
main.o: bindings.inc

.VARS: FORCE
	@mk/update-option "PROGRAM=$(PROGRAM) VERSION=$(VERSION) PKGDATADIR=$(PKGDATADIR)" $@

bindings.inc: share/binding/builtin mk/rc2c.sed
	$(call cmd,rc2c,builtin_bindings)

$(PROGRAM)$(X): $(editor_objects)
	$(call cmd,ld,$(LIBS))

test: $(filter-out main.o, $(editor_objects)) $(test_objects)
	$(call cmd,ld,$(LIBS))

install: all
	$(INSTALL) -d -m755 $(DESTDIR)$(bindir)
	$(INSTALL) -d -m755 $(DESTDIR)$(PKGDATADIR)/binding
	$(INSTALL) -d -m755 $(DESTDIR)$(PKGDATADIR)/color
	$(INSTALL) -d -m755 $(DESTDIR)$(PKGDATADIR)/compiler
	$(INSTALL) -d -m755 $(DESTDIR)$(PKGDATADIR)/syntax
	$(INSTALL) -d -m755 $(DESTDIR)$(mandir)/man1
	$(INSTALL) -d -m755 $(DESTDIR)$(mandir)/man7
	$(INSTALL) -m755 $(PROGRAM)$(X)  $(DESTDIR)$(bindir)
	$(INSTALL) -m644 $(config)   $(DESTDIR)$(PKGDATADIR)
	$(INSTALL) -m644 $(binding)  $(DESTDIR)$(PKGDATADIR)/binding
	$(INSTALL) -m644 $(color)    $(DESTDIR)$(PKGDATADIR)/color
	$(INSTALL) -m644 $(compiler) $(DESTDIR)$(PKGDATADIR)/compiler
	$(INSTALL) -m644 $(syntax)   $(DESTDIR)$(PKGDATADIR)/syntax
	$(INSTALL) -m644 Documentation/$(PROGRAM).1 $(DESTDIR)$(mandir)/man1
	$(INSTALL) -m644 Documentation/$(PROGRAM)-syntax.7 $(DESTDIR)$(mandir)/man7

tags:
	ctags *.[ch]

dist:
	git archive --prefix=$(TARNAME)/ -o $(TARNAME).tar.gz HEAD

clean:
	$(RM) .CFLAGS .VARS *.o bindings.inc $(PROGRAM)$(X) test $(CLEANFILES)
	$(RM) -r $(dep_dirs)

distclean: clean
	$(RM) tags


.DEFAULT_GOAL = all
.PHONY: all install tags dist clean distclean FORCE

# FIXME: Using this without a deplist breaks bash completion for make targets
.SECONDARY:
