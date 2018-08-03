CC ?= gcc
CFLAGS ?= -g -O2
LDFLAGS ?=
AWK = awk
PKGLIBS = $(shell $(PKGCONFIG) --libs $(1) 2>/dev/null)
VERSION = $(shell mk/version.sh 1.7)

WARNINGS = \
    -Wall -Wextra -pedantic -Wformat -Wformat-security -Wvla \
    -Wmissing-prototypes -Wstrict-prototypes \
    -Wold-style-definition -Wwrite-strings -Wundef -Wshadow \
    -Werror=div-by-zero -Werror=implicit-function-declaration \
    -Wno-sign-compare -Wno-pointer-sign

WARNINGS_EXTRA = \
    -Wformat-signedness -Wshift-overflow=2 -Wframe-larger-than=32768

BUILTIN_SYNTAX_FILES := \
    awk c config css d diff docker dte gitcommit gitrebase go html \
    ini java javascript lua mail make markdown meson nginx php \
    python robotstxt roff ruby sh sql tex texmfcnf vala xml

BUILTIN_CONFIGS := $(addprefix config/, \
    rc compiler/gcc compiler/go \
    binding/default binding/shift-select \
    $(addprefix color/, reset default light light256 darkgray) \
    $(addprefix syntax/, $(BUILTIN_SYNTAX_FILES)) )

TEST_CONFIGS := $(addprefix test/data/, $(addsuffix .dterc, \
    thai fuzz1 ))

util_objects := $(addprefix build/util/, $(addsuffix .o, \
    ascii path ptr-array string string-view strtonum uchar unicode xmalloc ))

editor_objects := $(addprefix build/, $(addsuffix .o, \
    alias bind block block-iter buffer buffer-iter cconv change \
    cmdline color commands common compiler completion config ctags \
    decoder detect edit editor encoder encoding env error \
    file-history file-option filetype format-status frame git-open \
    highlight history indent input-special key load-save lock main \
    mode-command mode-normal mode-search move msg options parse-args \
    parse-command regexp run screen screen-tabbar screen-view \
    script search selection spawn state syntax tag term-info \
    term-read term-write view wbuf window )) \
    $(util_objects)

test_objects := $(addprefix build/test/, $(addsuffix .o, \
    test-main test-config test-key test-util ))

all_objects := $(editor_objects) $(test_objects)

ifeq "$(USE_LUA)" "static"
  BUILTIN_CONFIGS += config/rc.lua
else ifeq "$(USE_LUA)" "dynamic"
  BUILTIN_CONFIGS += config/rc.lua
endif

ifdef WERROR
  WARNINGS += -Werror
endif

ifdef TERMINFO_DISABLE
  build/term-info.o: BASIC_CFLAGS += -DTERMINFO_DISABLE=1
else
  LDLIBS += $(or $(call PKGLIBS, tinfo), $(call PKGLIBS, ncurses), -lcurses)
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
  DEBUG = 1
endif

$(all_objects): BASIC_CFLAGS += $(CSTD) -DDEBUG=$(DEBUG) $(CWARNS)
$(dte) $(test): LDLIBS += $(LUA_LDLIBS)

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
build/builtin-config.h: build/builtin-config.mk
build/config.o: build/builtin-config.h
build/test/test-config.o: build/test/test-config.h
build/editor.o: build/version.h
build/term-info.o: build/term-info.cflags
build/script.o: build/script.cflags
build/script.o: BASIC_CFLAGS += $(LUA_CFLAGS)

$(dte) $(test): build/all.ldflags
	$(E) LINK $@
	$(Q) $(CC) $(CFLAGS) $(LDFLAGS) $(BASIC_LDFLAGS) -o $@ \
	     $(filter %.o, $^) $(LDLIBS)

$(editor_objects): build/%.o: src/%.c build/all.cflags | build/
	$(E) CC $@
	$(Q) $(CC) $(CPPFLAGS) $(CFLAGS) $(BASIC_CFLAGS) $(DEPFLAGS) -c -o $@ $<

$(test_objects): build/test/%.o: test/%.c build/all.cflags | build/test/
	$(E) CC $@
	$(Q) $(CC) $(CPPFLAGS) $(CFLAGS) $(BASIC_CFLAGS) $(DEPFLAGS) -c -o $@ $<

build/all.ldflags: FORCE | build/
	@$(OPTCHECK) '$(CC) $(CFLAGS) $(LDFLAGS) $(BASIC_LDFLAGS) $(LDLIBS)' $@

build/%.cflags: FORCE | build/
	@$(OPTCHECK) '$(CC) $(CPPFLAGS) $(CFLAGS) $(BASIC_CFLAGS)' $@

build/version.h: FORCE | build/
	@$(OPTCHECK) 'static const char version[] = "$(VERSION)";' $@

build/builtin-config.mk: FORCE | build/
	@$(OPTCHECK) '$(@:.mk=.h): $(BUILTIN_CONFIGS)' $@

build/builtin-config.h: $(BUILTIN_CONFIGS) mk/config2c.awk | build/
	$(E) GEN $@
	$(Q) $(AWK) -f mk/config2c.awk $(BUILTIN_CONFIGS) > $@

build/test/test-config.h: $(TEST_CONFIGS) mk/config2c.awk | build/test/
	$(E) GEN $@
	$(Q) $(AWK) -f mk/config2c.awk $(TEST_CONFIGS) > $@

build/util/ build/test/: | build/
	$(Q) mkdir -p $@

build/:
	$(Q) mkdir -p $@


CLEANFILES += $(dte)
CLEANDIRS += build/
.PHONY: FORCE
.SECONDARY: build/
