CC ?= gcc
LD = $(CC)
CFLAGS ?= -g -O2
LDFLAGS ?=
HOST_CC ?= $(CC)
HOST_LD ?= $(HOST_CC)
HOST_CFLAGS ?= $(CFLAGS)
HOST_LDFLAGS ?=
LIBS = -lcurses
SED = sed

# Enabled if supported by CC
WARNINGS = \
    -Wall -Wformat-security -Wmissing-prototypes -Wold-style-definition \
    -Wwrite-strings -Wundef -Wshadow \
    -Wextra -Wno-unused-parameter -Wno-sign-compare

VERSION = 1.3
PKGDATADIR = $(datadir)/dte
EXE =
dte = dte$(EXE)

try-run = $(shell if ($(1)) >/dev/null 2>&1; then echo "$(2)"; else echo "$(3)"; fi)
cc-option = $(call try-run, $(CC) $(1) -c -x c /dev/null -o /dev/null,$(1),$(2))
uname_S := $(shell sh -c 'uname -s 2>/dev/null || echo not')
uname_O := $(shell sh -c 'uname -o 2>/dev/null || echo not')
uname_R := $(shell sh -c 'uname -r 2>/dev/null || echo not')

ifneq "$(findstring s,$(firstword -$(MAKEFLAGS)))$(filter -s,$(MAKEFLAGS))" ""
  # Make "-s" flag was used (silent build)
  Q = @
  E = @:
else ifeq "$(V)" "1"
  # "V=1" variable was set (verbose build)
  Q =
  E = @:
else
  # Normal build
  Q = @
  E = @printf ' %7s  %s\n'
endif

# Disable automatic dependency calculation
# NO_DEPS = 1

# 0: Disable debugging.
# 1: Enable BUG_ON() and light-weight sanity checks.
# 2: Enable logging to $(DTE_HOME)/debug.log.
# 3: Enable expensive sanity checks.
DEBUG = 1

editor_objects := $(addprefix src/, $(addsuffix .o, \
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

test_objects := src/test-main.o
OBJECTS := $(editor_objects) $(test_objects)

ifeq "$(uname_S)" "Darwin"
  LIBS += -liconv
endif
ifeq "$(uname_O)" "Cygwin"
  LIBS += -liconv
  EXE = .exe
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
ifneq "$(uname_S)" "Darwin"
  WARNINGS += -Wcast-align
endif
endif

BASIC_CFLAGS += -std=gnu99
BASIC_CFLAGS += $(call cc-option,$(WARNINGS))
BASIC_CFLAGS += $(call cc-option,-Wno-pointer-sign) # char vs unsigned char madness

ifdef WERROR
  BASIC_CFLAGS += $(call cc-option,-Werror -Wno-error=shadow -Wno-error=unused-variable)
endif

BASIC_CFLAGS += -DDEBUG=$(DEBUG)

# Dependency generation macros copied from GIT
ifndef NO_DEPS
  CC_CAN_GENERATE_DEPS := $(call try-run,$(CC) -MMD -MP -MF /dev/null -c -x c /dev/null -o /dev/null,y,n)
  ifeq "$(CC_CAN_GENERATE_DEPS)" "n"
    NO_DEPS := y
  endif
endif

ifndef NO_DEPS
dep_files := $(foreach f,$(OBJECTS),$(dir $f).depend/$(notdir $f).d)
dep_dirs := $(addsuffix .depend,$(sort $(dir $(OBJECTS))))
missing_dep_dirs := $(filter-out $(wildcard $(dep_dirs)),$(dep_dirs))
$(OBJECTS): BASIC_CFLAGS += -MF $(dir $@).depend/$(notdir $@).d -MMD -MP
dep_files_present := $(wildcard $(dep_files))

ifneq "$(dep_files_present)" ""
include $(dep_files_present)
endif

$(dep_dirs):
	@mkdir -p $@
endif

$(dte): $(editor_objects)
test: $(filter-out src/main.o, $(editor_objects)) $(test_objects)
src/main.o: src/bindings.inc
src/editor.o: .VARS
src/editor.o: BASIC_CFLAGS += -DVERSION=\"$(VERSION)\" -DPKGDATADIR=\"$(PKGDATADIR)\"

$(dte) test:
	$(E) LINK $@
	$(Q) $(LD) $(LDFLAGS) $(BASIC_LDFLAGS) -o $@ $^ $(LIBS)

$(OBJECTS): src/%.o: src/%.c .CFLAGS | $(missing_dep_dirs)
	$(E) CC $@
	$(Q) $(CC) $(CFLAGS) $(BASIC_CFLAGS) -c -o $@ $<

.CFLAGS: FORCE
	@mk/optcheck.sh "$(CC) $(CFLAGS) $(BASIC_CFLAGS)" $@

.VARS: FORCE
	@mk/optcheck.sh "VERSION=$(VERSION) PKGDATADIR=$(PKGDATADIR)" $@

src/bindings.inc: share/binding/builtin mk/rc2c.sed
	$(E) GEN $@
	$(Q) echo 'static const char *builtin_bindings =' > $@
	$(Q) $(SED) -f mk/rc2c.sed $< >> $@


CLEANFILES += $(dte) test $(OBJECTS) src/bindings.inc .VARS .CFLAGS
CLEANDIRS += $(dep_dirs)
.PHONY: FORCE
.SECONDARY: $(dep_dirs)
