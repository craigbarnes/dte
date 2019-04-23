CC ?= gcc
CFLAGS ?= -O2
LDFLAGS ?=
AWK = awk
VERSION = 1.8.2

WARNINGS = \
    -Wall -Wextra -Wformat -Wformat-security \
    -Wmissing-prototypes -Wstrict-prototypes \
    -Wold-style-definition -Wwrite-strings -Wundef -Wshadow \
    -Werror=div-by-zero -Werror=implicit-function-declaration \
    -Wno-sign-compare -Wno-pointer-sign

WARNINGS_EXTRA = \
    -Wformat-signedness -Wformat-truncation -Wformat-overflow \
    -Wstringop-truncation -Wstringop-overflow -Wshift-overflow=2 \
    -Wframe-larger-than=32768 -Wvla

BUILTIN_SYNTAX_FILES := \
    awk c config css d diff docker dte gitcommit gitrebase go html \
    ini java javascript lua mail make markdown meson nginx ninja php \
    python robotstxt roff ruby sed sh sql tex texmfcnf vala xml

BUILTIN_CONFIGS := $(addprefix config/, \
    rc compiler/gcc compiler/go \
    binding/default binding/shift-select \
    $(addprefix color/, reset reset-basic default light light256 darkgray) \
    $(addprefix syntax/, $(BUILTIN_SYNTAX_FILES)) )

TEST_CONFIGS := $(addprefix test/data/, $(addsuffix .dterc, \
    env thai crlf fuzz1 ))

build_subdirs := $(addprefix build/, $(addsuffix /, \
    editorconfig encoding syntax terminal util test ))

util_objects := $(call prefix-obj, build/util/, \
    ascii exec path ptr-array regexp string strtonum unicode utf8 \
    wbuf xmalloc xreadwrite xsnprintf )

editorconfig_objects := $(call prefix-obj, build/editorconfig/, \
    editorconfig ini match )

encoding_objects := $(call prefix-obj, build/encoding/, \
    bom convert decoder encoder encoding )

syntax_objects := $(call prefix-obj, build/syntax/, \
    bitset color hashset highlight state syntax )

terminal_objects := $(call prefix-obj, build/terminal/, \
    color ecma48 input key output terminal terminfo xterm xterm-keys )

editor_objects := $(call prefix-obj, build/, \
    alias bind block block-iter buffer buffer-iter change cmdline \
    commands common compiler completion config ctags debug edit editor \
    env error file-history file-option filetype frame history \
    indent load-save lock main mode-command mode-git-open mode-normal \
    mode-search move msg options parse-args parse-command run \
    screen screen-cmdline screen-status screen-tabbar screen-view \
    search selection spawn tag view window ) \
    $(editorconfig_objects) \
    $(encoding_objects) \
    $(syntax_objects) \
    $(terminal_objects) \
    $(util_objects)

test_objects := $(call prefix-obj, build/test/, \
    cmdline config editorconfig encoding filetype main terminal test util )

all_objects := $(editor_objects) $(test_objects)

editor_sources := $(patsubst build/%.o, src/%.c, $(editor_objects))
test_sources := $(patsubst build/test/%.o, test/%.c, $(test_objects))

ifdef WERROR
  WARNINGS += -Werror
endif

ifdef TERMINFO_DISABLE
  build/terminal/terminfo.o: BASIC_CFLAGS += -DTERMINFO_DISABLE=1
else
  LDLIBS += $(or $(call pkg-libs, tinfo), $(call pkg-libs, ncurses), -lcurses)
endif

CWARNS = $(WARNINGS) $(foreach W,$(WARNINGS_EXTRA),$(call cc-option,$(W)))
CSTD = $(call cc-option,-std=gnu11,-std=gnu99)
$(call make-lazy,CWARNS)
$(call make-lazy,CSTD)

ifeq "$(KERNEL)" "Darwin"
  LDLIBS += -liconv
else ifeq "$(OS)" "Cygwin"
  LDLIBS += -liconv
  EXEC_SUFFIX = .exe
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

dte = dte$(EXEC_SUFFIX)
test = build/test/test$(EXEC_SUFFIX)

ifdef USE_SANITIZER
  SANITIZER_FLAGS := \
    -fsanitize=address,undefined -fsanitize-address-use-after-scope \
    -fno-sanitize-recover=all -fno-omit-frame-pointer -fno-common
  CC_SANITIZER_FLAGS := $(or \
    $(call cc-option, $(SANITIZER_FLAGS)), \
    $(warning USE_SANITIZER set but compiler doesn't support ASan/UBSan) )
  $(all_objects): BASIC_CFLAGS += $(CC_SANITIZER_FLAGS)
  $(dte) $(test): BASIC_LDFLAGS += $(CC_SANITIZER_FLAGS)
  export ASAN_OPTIONS=detect_leaks=1:detect_stack_use_after_return=1
  DEBUG = 3
else
  # 0: Disable debugging
  # 1: Enable BUG_ON() and light-weight sanity checks
  # 3: Enable expensive sanity checks
  DEBUG = 0
endif

ifeq "$(DEBUG)" "0"
  UNWIND = $(call cc-option,-fno-asynchronous-unwind-tables)
  $(call make-lazy,UNWIND)
endif

$(all_objects): BASIC_CFLAGS += $(CSTD) -DDEBUG=$(DEBUG) $(CWARNS) $(UNWIND)

# If "make install" with no other named targets
ifeq "" "$(filter-out install,$(or $(MAKECMDGOALS),all))"
  OPTCHECK = :
else
  OPTCHECK = SILENT_BUILD='$(MAKE_S)' mk/optcheck.sh
endif

ifndef NO_DEPS
  ifneq '' '$(call cc-option,-MMD -MP -MF /dev/null)'
    $(all_objects): DEPFLAGS = -MMD -MP -MF $(patsubst %.o, %.mk, $@)
  else ifneq '' '$(call cc-option,-MD -MF /dev/null)'
    $(all_objects): DEPFLAGS = -MD -MF $(patsubst %.o, %.mk, $@)
  endif
  -include $(patsubst %.o, %.mk, $(all_objects))
endif

$(dte): $(editor_objects)
$(test): $(filter-out build/main.o, $(all_objects))
$(util_objects): | build/util/
$(editorconfig_objects): | build/editorconfig/
$(encoding_objects): | build/encoding/
$(syntax_objects): | build/syntax/
$(terminal_objects): | build/terminal/
build/builtin-config.h: build/builtin-config.mk
build/config.o: build/builtin-config.h
build/test/config.o: build/test/data.h
build/editor.o: build/version.h
build/terminal/terminfo.o: build/terminal/terminfo.cflags
build/terminal/terminfo.cflags: | build/terminal/
build/filetype.o: BASIC_CFLAGS += -Wno-unused-label

CFLAGS_ALL = $(CPPFLAGS) $(CFLAGS) $(BASIC_CFLAGS)
LDFLAGS_ALL = $(CFLAGS) $(LDFLAGS) $(BASIC_LDFLAGS)

$(dte) $(test): build/all.ldflags
	$(E) LINK $@
	$(Q) $(CC) $(LDFLAGS_ALL) -o $@ $(filter %.o, $^) $(LDLIBS)

$(editor_objects): build/%.o: src/%.c build/all.cflags | build/
	$(E) CC $@
	$(Q) $(CC) $(CFLAGS_ALL) $(DEPFLAGS) -c -o $@ $<

$(test_objects): build/test/%.o: test/%.c build/all.cflags | build/test/
	$(E) CC $@
	$(Q) $(CC) $(CFLAGS_ALL) $(DEPFLAGS) -c -o $@ $<

build/all.ldflags: FORCE | build/
	@$(OPTCHECK) '$(CC) $(LDFLAGS_ALL) $(LDLIBS)' $@

build/%.cflags: FORCE | build/
	@$(OPTCHECK) '$(CC) $(CFLAGS_ALL)' $@

build/version.h: FORCE | build/
	@$(OPTCHECK) 'static const char version[] = "$(VERSION)";' $@

build/builtin-config.mk: FORCE | build/
	@$(OPTCHECK) '$(@:.mk=.h): $(BUILTIN_CONFIGS)' $@

build/builtin-config.h: $(BUILTIN_CONFIGS) mk/config2c.awk | build/
	$(E) GEN $@
	$(Q) $(AWK) -f mk/config2c.awk $(BUILTIN_CONFIGS) > $@

build/test/data.h: $(TEST_CONFIGS) mk/config2c.awk | build/test/
	$(E) GEN $@
	$(Q) $(AWK) -f mk/config2c.awk $(TEST_CONFIGS) > $@

$(build_subdirs): | build/
	$(Q) mkdir -p $@

build/:
	$(Q) mkdir -p $@


CLEANFILES += $(dte)
CLEANDIRS += build/
.PHONY: FORCE
.SECONDARY: build/
