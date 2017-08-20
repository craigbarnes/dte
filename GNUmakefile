CC ?= gcc
LD = $(CC)
CFLAGS ?= -g -O2
LDFLAGS ?=
HOST_CC ?= $(CC)
HOST_LD ?= $(HOST_CC)
HOST_CFLAGS ?= $(CFLAGS)
HOST_LDFLAGS ?=
INSTALL = install
SED = sed
RM = rm -f
prefix ?= /usr/local
bindir ?= $(prefix)/bin
datadir ?= $(prefix)/share
mandir ?= $(datadir)/man

# Enabled if supported by CC
WARNINGS = \
    -Wall -Wformat-security -Wmissing-prototypes -Wold-style-definition \
    -Wwrite-strings -Wundef -Wshadow \
    -Wextra -Wno-unused-parameter -Wno-sign-compare

# More verbose build
# V = 1

# Disable automatic dependency calculation
# NO_DEPS = 1

# 0: Disable debugging.
# 1: Enable BUG_ON() and light-weight sanity checks.
# 2: Enable logging to $(DTE_HOME)/debug.log.
# 3: Enable expensive sanity checks.
DEBUG = 1

# End of configuration =================================================

editor_objects := $(addprefix src/, $(addsuffix .o, \
    alias bind block buffer-iter buffer cconv change cmdline color \
    command-mode commands common compiler completion config ctags ctype \
    cursed decoder detect edit editor encoder encoding env error \
    file-history file-location file-option filetype fork format-status \
    frame git-open history hl indent input-special iter key \
    load-save lock main modes move msg normal-mode obuf options \
    parse-args parse-command path ptr-array regexp run screen \
    screen-tabbar screen-view search-mode search selection spawn state \
    strbuf syntax tabbar tag term-caps term uchar unicode vars view \
    wbuf window xmalloc ))

test_objects := src/test-main.o

syntax_files := \
    awk c config css diff docker dte gitcommit gitrebase go html html+smarty \
    ini java javascript lua mail make markdown meson php python robotstxt \
    ruby sh smarty sql xml

binding := $(addprefix share/binding/, default classic builtin)
color := $(addprefix share/color/, darkgray light light256)
compiler:= $(addprefix share/compiler/, gcc go)
config := $(addprefix share/, filetype option rc)
syntax := $(addprefix share/syntax/, $(syntax_files))
dte := dte$(X)

OBJECTS := $(editor_objects) $(test_objects)

PKGDATADIR = $(datadir)/dte
VERSION = 1.2

include mk/compat.mk
-include Config.mk
include mk/build.mk
include mk/docs.mk
-include mk/dev.mk

all: $(dte) test man

$(dte): $(editor_objects)
test: $(filter-out src/main.o, $(editor_objects)) $(test_objects)

$(dte) test:
	$(E) LINK $@
	$(Q) $(LD) $(LDFLAGS) $(BASIC_LDFLAGS) -o $@ $^ $(LIBS)

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

tags:
	ctags src/*.[ch]

clean:
	$(RM) .CFLAGS .VARS src/*.o src/bindings.inc $(dte) test $(CLEANFILES)
	$(RM) -r $(dep_dirs)

distclean: clean
	$(RM) tags


.DEFAULT_GOAL = all
.PHONY: all install tags clean distclean
.DELETE_ON_ERROR:
