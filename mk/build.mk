CC ?= gcc
CFLAGS ?= -g -O2
LDFLAGS ?=
HOST_CC ?= $(CC)
HOST_CFLAGS ?= $(CFLAGS)
HOST_LDFLAGS ?=
LDLIBS = -lcurses
SED = sed
PKGDATADIR = $(datadir)/dte

_VERSION := 1.3
VERSION = $(or \
    $(shell git describe --match='v$(_VERSION)' 2>/dev/null | sed 's/^v//'), \
    $(shell awk 'NR==1 && /^[a-f0-9]{40}$$/ {print "$(_VERSION)-" substr($$0,0,12) "-dist"}' .distinfo), \
    $(_VERSION)-unknown \
)

try-run = $(if $(shell $(1) >/dev/null 2>&1 && echo 1),$(2),$(3))
cc-option = $(call try-run, $(CC) $(1) -c -x c /dev/null -o /dev/null,$(1),$(2))

WARNINGS = \
    -Wall -Wno-pointer-sign -Wformat-security -Wmissing-prototypes \
    -Wold-style-definition -Wwrite-strings -Wundef -Wshadow \
    -Wextra -Wno-unused-parameter -Wno-sign-compare

# Use "initially recursive variable" trick so that cc-option is only
# called once per build instead of once per object file (or not at
# all for targets that don't use it).
CWARNS = $(eval CWARNS := $(call cc-option,$(WARNINGS)))$(CWARNS)

# 0: Disable debugging.
# 1: Enable BUG_ON() and light-weight sanity checks.
# 2: Enable logging to $(DTE_HOME)/debug.log.
# 3: Enable expensive sanity checks.
DEBUG = 1

BASIC_CFLAGS += -std=gnu99 -Ibuild -DDEBUG=$(DEBUG) $(CWARNS)
BASIC_HOST_CFLAGS += -std=gnu99 $(CWARNS)

editor_objects := $(addprefix build/, $(addsuffix .o, \
    alias bind block buffer-iter buffer cconv change cmdline color \
    command-mode commands common compiler completion config ctags ctype \
    cursed decoder detect edit editor encoder encoding env error \
    file-history file-location file-option filetype fork format-status \
    frame git-open history hl indent input-special iter key \
    load-save lock main move msg normal-mode obuf options \
    parse-args parse-command path ptr-array regexp run screen \
    screen-tabbar screen-view search-mode search selection spawn state \
    strbuf syntax tabbar tag term-caps term uchar unicode view \
    wbuf window xmalloc ))

test_objects := build/test-main.o
OBJECTS := $(editor_objects) $(test_objects)

ifeq "$(KERNEL)" "Darwin"
  LDLIBS += -liconv
else ifeq "$(OS)" "Cygwin"
  LDLIBS += -liconv
  EXEC_SUFFIX = .exe
else ifeq "$(KERNEL)" "FreeBSD"
  # libc of FreeBSD 10.0 includes iconv
  ifeq ($(shell expr "`uname -r`" : '[0-9]\.'),2)
    LDLIBS += -liconv
    BASIC_CFLAGS += -I/usr/local/include
    BASIC_LDFLAGS += -L/usr/local/lib
  endif
else ifeq "$(KERNEL)" "OpenBSD"
  LDLIBS += -liconv
  BASIC_CFLAGS += -I/usr/local/include
  BASIC_LDFLAGS += -L/usr/local/lib
else ifeq "$(KERNEL)" "NetBSD"
  ifeq ($(shell expr "`uname -r`" : '[01]\.'),2)
    LDLIBS += -liconv
  endif
  BASIC_CFLAGS += -I/usr/pkg/include
  BASIC_LDFLAGS += -L/usr/pkg/lib
endif

ifndef NO_DEPS
ifeq '$(call try-run,$(CC) -MMD -MP -MF /dev/null -c -x c /dev/null -o /dev/null,y,n)' 'y'
  $(OBJECTS): BASIC_CFLAGS += -MF build/$*.mk -MMD -MP
  -include $(patsubst %.o, %.mk, $(OBJECTS))
endif
endif

dte = dte$(EXEC_SUFFIX)

$(dte): $(editor_objects)
build/test: $(filter-out build/main.o, $(editor_objects)) $(test_objects)
build/main.o: build/bindings.inc
build/editor.o: build/VARS.txt
build/editor.o: BASIC_CFLAGS += -DVERSION=\"$(VERSION)\" -DPKGDATADIR=\"$(PKGDATADIR)\"

$(dte) build/test:
	$(E) LINK $@
	$(Q) $(CC) $(LDFLAGS) $(BASIC_LDFLAGS) -o $@ $^ $(LDLIBS)

$(OBJECTS): build/%.o: src/%.c build/CFLAGS.txt | build/
	$(E) CC $@
	$(Q) $(CC) $(CPPFLAGS) $(CFLAGS) $(BASIC_CFLAGS) -c -o $@ $<

build/CFLAGS.txt: FORCE | build/
	@mk/optcheck.sh "$(CC) $(CFLAGS) $(BASIC_CFLAGS)" $@

build/VARS.txt: FORCE | build/
	@mk/optcheck.sh "VERSION=$(VERSION) PKGDATADIR=$(PKGDATADIR)" $@

build/bindings.inc: share/binding/builtin mk/rc2c.sed | build/
	$(E) GEN $@
	$(Q) echo 'static const char *builtin_bindings =' > $@
	$(Q) $(SED) -f mk/rc2c.sed $< >> $@

build/:
	@mkdir -p $@


CLEANFILES += $(dte)
CLEANDIRS += build/
.PHONY: FORCE
.SECONDARY: build/
