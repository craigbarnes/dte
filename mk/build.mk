CC ?= gcc
CFLAGS ?= -g -O2
LDFLAGS ?=
HOST_CC ?= $(CC)
HOST_CFLAGS ?= $(CFLAGS)
HOST_LDFLAGS ?=
SED = sed
PKGDATADIR = $(datadir)/dte

ifdef TERMINFO_DISABLE
  build/term-caps.o: BASIC_CFLAGS += -DTERMINFO_DISABLE=1
else
  LDLIBS = -lcurses
endif

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

CWARNS = $(call cc-option,$(WARNINGS))
CSTD = $(call cc-option,-std=gnu11,-std=gnu99)

ifneq "$(MAKE_VERSION)" "3.81"
  # Use "initially recursive" trick so that these variables are only
  # expanded (at most) once per build instead of once per object file.
  # Disabled for Make 3.81 due to an intermittent crash bug.
  make-lazy = $(eval $1 = $$(eval $1 := $(value $(1)))$$($1))
  $(call make-lazy,CWARNS)
  $(call make-lazy,CSTD)
endif

ifdef USE_SANITIZER
  export ASAN_OPTIONS=detect_leaks=0
  SANITIZER_FLAGS = -fsanitize=address,undefined
  BASIC_CFLAGS += $(SANITIZER_FLAGS)
  BASIC_LDFLAGS += $(SANITIZER_FLAGS)
  DEBUG = 3
else
  # 0: Disable debugging
  # 1: Enable BUG_ON() and light-weight sanity checks
  # 2: Enable logging to $(DTE_HOME)/debug.log
  # 3: Enable expensive sanity checks
  DEBUG = 1
endif

BASIC_CFLAGS += $(CSTD) -Ibuild -DDEBUG=$(DEBUG) $(CWARNS)
BASIC_HOST_CFLAGS += $(CSTD) $(CWARNS)

editor_objects := $(addprefix build/, $(addsuffix .o, \
    alias ascii bind block buffer-iter buffer cconv change cmdline \
    color command-mode commands common compiler completion config ctags \
    decoder detect edit editor encoder encoding env error \
    file-history file-location file-option filetype fork format-status \
    frame git-open history hl indent input-special iter key \
    load-save lock main move msg normal-mode obuf options \
    parse-args parse-command path ptr-array regexp run screen \
    screen-tabbar screen-view search-mode search selection spawn state \
    strbuf syntax tabbar tag term-caps term uchar unicode view \
    wbuf window xmalloc ))

test_objects := build/test/test_main.o

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
  $(editor_objects) $(test_objects): DEPFLAGS = -MF $(patsubst %.o, %.mk, $@) -MMD -MP
  -include $(patsubst %.o, %.mk, $(editor_objects) $(test_objects))
endif
endif

dte = dte$(EXEC_SUFFIX)
test = build/test/test$(EXEC_SUFFIX)

$(dte): $(editor_objects)
$(test): $(filter-out build/main.o, $(editor_objects)) $(test_objects)
build/main.o: build/BINDINGS.h
build/editor.o: build/VERSION.h build/PKGDATADIR.h

$(dte) $(test):
	$(E) LINK $@
	$(Q) $(CC) $(LDFLAGS) $(BASIC_LDFLAGS) -o $@ $^ $(LDLIBS)

$(editor_objects): build/%.o: src/%.c build/CFLAGS.txt | build/
	$(E) CC $@
	$(Q) $(CC) $(CPPFLAGS) $(CFLAGS) $(BASIC_CFLAGS) $(DEPFLAGS) -c -o $@ $<

$(test_objects): build/test/%.o: test/%.c build/CFLAGS.txt | build/test/
	$(E) CC $@
	$(Q) $(CC) $(CPPFLAGS) $(CFLAGS) $(BASIC_CFLAGS) $(DEPFLAGS) -Isrc -c -o $@ $<

build/CFLAGS.txt: FORCE | build/
	@mk/optcheck.sh "$(CC) $(CPPFLAGS) $(CFLAGS) $(BASIC_CFLAGS)" $@

build/PKGDATADIR.h: FORCE | build/
	@mk/optcheck.sh '#define PKGDATADIR "$(PKGDATADIR)"' $@

build/VERSION.h: FORCE | build/
	@mk/optcheck.sh '#define VERSION "$(VERSION)"' $@

build/BINDINGS.h: share/binding/builtin mk/rc2c.sed | build/
	$(E) GEN $@
	$(Q) echo 'static const char *builtin_bindings =' > $@
	$(Q) $(SED) -f mk/rc2c.sed $< >> $@

build/ build/test/:
	@mkdir -p $@


CLEANFILES += $(dte)
CLEANDIRS += build/
.PHONY: FORCE
.SECONDARY: build/
