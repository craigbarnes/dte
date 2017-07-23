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
DESTDIR =
prefix = /usr/local
bindir = $(prefix)/bin
datadir = $(prefix)/share
mandir = $(datadir)/man

# 0: Disable debugging.
# 1: Enable BUG_ON() and light-weight sanity checks.
# 2: Enable logging to ~/.$(PROGRAM)/debug.log.
# 3: Enable expensive sanity checks.
DEBUG = 1

# enabled if CC supports them
WARNINGS = \
	-Wdeclaration-after-statement \
	-Wformat-security \
	-Wmissing-prototypes \
	-Wold-style-definition \
	-Wredundant-decls \
	-Wwrite-strings \
	-Wundef \
	-Wshadow

# End of configuration

LIBS =
X =

uname_S := $(shell sh -c 'uname -s 2>/dev/null || echo not')
uname_O := $(shell sh -c 'uname -o 2>/dev/null || echo not')
uname_R := $(shell sh -c 'uname -r 2>/dev/null || echo not')

PROGRAM = dte
VERSION = 1.0

all: $(PROGRAM)$(X) test man

editor_objects :=	 		\
	alias.o			\
	bind.o			\
	block.o			\
	buffer-iter.o		\
	buffer.o		\
	cconv.o			\
	change.o		\
	cmdline.o		\
	color.o			\
	command-mode.o		\
	commands.o		\
	common.o		\
	compiler.o		\
	completion.o		\
	config.o		\
	ctags.o			\
	ctype.o			\
	cursed.o		\
	decoder.o		\
	detect.o		\
	edit.o			\
	editor.o		\
	encoder.o		\
	encoding.o		\
	env.o			\
	error.o			\
	file-history.o		\
	file-location.o		\
	file-option.o		\
	filetype.o		\
	fork.o			\
	format-status.o		\
	frame.o			\
	gbuf.o			\
	git-open.o		\
	history.o		\
	hl.o			\
	indent.o		\
	input-special.o		\
	iter.o			\
	key.o			\
	load-save.o		\
	lock.o			\
	main.o			\
	modes.o			\
	move.o			\
	msg.o			\
	normal-mode.o		\
	obuf.o			\
	options.o		\
	parse-args.o		\
	parse-command.o		\
	path.o			\
	ptr-array.o		\
	regexp.o		\
	run.o			\
	screen-tabbar.o		\
	screen-view.o		\
	screen.o		\
	search-mode.o		\
	search.o		\
	selection.o		\
	spawn.o			\
	state.o			\
	syntax.o		\
	tabbar.o		\
	tag.o			\
	term.o			\
	term-caps.o		\
	uchar.o			\
	unicode.o		\
	vars.o			\
	view.o			\
	wbuf.o			\
	window.o		\
	xmalloc.o		\
	# end

test_objects :=			\
	test-main.o		\
	# end

binding	:=	 		\
	binding/default		\
	binding/classic		\
	# end

color	:=			\
	color/darkgray		\
	color/light		\
	color/light256		\
	# end

compiler :=			\
	compiler/gcc		\
	compiler/go		\
	# end

config	:=			\
	filetype		\
	rc			\
	# end

syntax	:=			\
	syntax/awk		\
	syntax/c		\
	syntax/config		\
	syntax/css		\
	syntax/diff		\
	syntax/$(PROGRAM)	\
	syntax/gitcommit	\
	syntax/gitrebase	\
	syntax/go		\
	syntax/html		\
	syntax/html+smarty	\
	syntax/java		\
	syntax/javascript	\
	syntax/lua		\
	syntax/mail		\
	syntax/make		\
	syntax/php		\
	syntax/python		\
	syntax/sh		\
	syntax/smarty		\
	syntax/sql		\
	syntax/xml		\
	# end

binding	:= $(addprefix share/,$(binding))
color	:= $(addprefix share/,$(color))
compiler:= $(addprefix share/,$(compiler))
config	:= $(addprefix share/,$(config))
syntax	:= $(addprefix share/,$(syntax))

OBJECTS := $(editor_objects) $(test_objects)

-include Config.mk
include Makefile.lib

LIBS += -lcurses

ifeq ($(uname_S),Darwin)
	LIBS += -liconv
endif
ifeq ($(uname_O),Cygwin)
	LIBS += -liconv
	X = .exe
endif
ifeq ($(uname_S),FreeBSD)
	# libc of FreeBSD 10.0 includes iconv
	ifeq ($(shell expr "$(uname_R)" : '[0-9]\.'),2)
		LIBS += -liconv
		BASIC_CFLAGS += -I/usr/local/include
		BASIC_LDFLAGS += -L/usr/local/lib
	endif
endif
ifeq ($(uname_S),OpenBSD)
	LIBS += -liconv
	BASIC_CFLAGS += -I/usr/local/include
	BASIC_LDFLAGS += -L/usr/local/lib
endif
ifeq ($(uname_S),NetBSD)
	ifeq ($(shell expr "$(uname_R)" : '[01]\.'),2)
		LIBS += -liconv
	endif
	BASIC_CFLAGS += -I/usr/pkg/include
	BASIC_LDFLAGS += -L/usr/pkg/lib
endif

PKGDATADIR = $(datadir)/$(PROGRAM)

# clang does not like container_of()
ifneq ($(CC),clang)
WARNINGS += -Wcast-align
endif

BASIC_CFLAGS += $(call cc-option,$(WARNINGS))
BASIC_CFLAGS += $(call cc-option,-Wno-pointer-sign) # char vs unsigned char madness

ifdef WERROR
BASIC_CFLAGS += $(call cc-option,-Werror -Wno-error=shadow -Wno-error=unused-variable)
endif

BASIC_CFLAGS += -DDEBUG=$(DEBUG)

clean += .CFLAGS
$(OBJECTS): .CFLAGS

.CFLAGS: FORCE
	@./update-option "$(CC) $(CFLAGS) $(BASIC_CFLAGS)" $@

TARNAME = $(PROGRAM)-$(VERSION)

clean += .VARS
vars.o: .VARS
vars.o: BASIC_CFLAGS += -DPROGRAM=\"$(PROGRAM)\" -DVERSION=\"$(VERSION)\" -DPKGDATADIR=\"$(PKGDATADIR)\"

.VARS: FORCE
	@./update-option "PROGRAM=$(PROGRAM) VERSION=$(VERSION) PKGDATADIR=$(PKGDATADIR)" $@

clean += *.o $(PROGRAM)$(X)
$(PROGRAM)$(X): $(editor_objects)
	$(call cmd,ld,$(LIBS))

clean += test
test: $(filter-out main.o,$(editor_objects)) $(test_objects)
	$(call cmd,ld,$(LIBS))

man	:=					\
	Documentation/$(PROGRAM).1		\
	Documentation/$(PROGRAM)-syntax.7	\
	# end

clean += $(man) Documentation/*.o Documentation/ttman$(X)
man: $(man)
$(man): Documentation/ttman$(X)

%.1: %.txt
	$(call cmd,ttman)

%.7: %.txt
	$(call cmd,ttman)

Documentation/ttman.o: Documentation/ttman.c
	$(call cmd,host_cc)

Documentation/ttman$(X): Documentation/ttman.o
	$(call cmd,host_ld,)

quiet_cmd_ttman = MAN    $@
      cmd_ttman = sed -e s/%MAN%/$(shell echo $@ | sed 's:.*/\([^.]*\)\..*:\1:' | tr a-z A-Z)/g \
			-e s/%PROGRAM%/$(PROGRAM)/g \
			< $< | Documentation/ttman$(X) > $@

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

distclean += tags
tags:
	ctags *.[ch]

dist:
	git archive --prefix=$(TARNAME)/ -o $(TARNAME).tar.gz HEAD

.PHONY: all man install tags dist FORCE
