CC ?= gcc
CFLAGS ?= -O2
LDFLAGS ?=
AWK = awk
PKGCONFIG = pkg-config
PKGLIBS = $(shell $(PKGCONFIG) --libs $(1) 2>/dev/null)
VERSION = 1.7

WARNINGS = \
    -Wall -Wextra -Wformat -Wformat-security -Wvla \
    -Wmissing-prototypes -Wstrict-prototypes \
    -Wold-style-definition -Wwrite-strings -Wundef -Wshadow \
    -Werror=div-by-zero -Werror=implicit-function-declaration \
    -Wno-sign-compare -Wno-pointer-sign

WARNINGS_EXTRA = \
    -Wformat-signedness -Wframe-larger-than=32768

BUILTIN_SYNTAX_FILES := \
    awk c config css d diff docker dte gitcommit gitrebase go html \
    ini java javascript lua mail make markdown meson nginx perl \
    php python robotstxt roff ruby sh sql tex texmfcnf vala xml

BUILTIN_CONFIGS := $(addprefix config/, \
    rc filetype \
    binding/default binding/shift-select \
    color/dark color/light color/light256 color/darkgray \
    compiler/gcc compiler/go \
    $(addprefix syntax/, $(BUILTIN_SYNTAX_FILES)) )

editor_objects := $(addprefix build/, $(addsuffix .o, \
    alias ascii bind block block-iter buffer buffer-iter cconv change \
    cmdline color command-mode commands common compiler completion \
    config ctags decoder detect edit editor encoder encoding env error \
    file-history file-location file-option filetype fork format-status \
    frame git-open highlight history indent input-special key load-save \
    lock main move msg normal-mode obuf options parse-args parse-command \
    path ptr-array regexp run screen screen-tabbar screen-view \
    search-mode search selection spawn state str syntax tabbar tag \
    term-caps term uchar unicode view wbuf window xmalloc ))

ifdef WERROR
  WARNINGS += -Werror
endif

ifdef TERMINFO_DISABLE
  build/term-caps.o: BASIC_CFLAGS += -DTERMINFO_DISABLE=1
else
  LDLIBS += $(or $(call PKGLIBS, tinfo), $(call PKGLIBS, ncurses), -lcurses)
endif

ifdef USE_SANITIZER
  export ASAN_OPTIONS=detect_leaks=1:detect_stack_use_after_return=1
  SANITIZER_FLAGS = -fsanitize=address,undefined -fsanitize-address-use-after-scope
  BASIC_CFLAGS += $(SANITIZER_FLAGS) -fno-omit-frame-pointer -fno-common
  BASIC_LDFLAGS += $(SANITIZER_FLAGS)
  DEBUG = 3
else
  # 0: Disable debugging
  # 1: Enable BUG_ON() and light-weight sanity checks
  # 2: Enable logging to $(DTE_HOME)/debug.log
  # 3: Enable expensive sanity checks
  DEBUG = 0
endif

try-run = $(if $(shell $(1) >/dev/null 2>&1 && echo 1),$(2),$(3))
cc-option = $(call try-run, $(CC) $(1) -Werror -c -x c /dev/null -o /dev/null,$(1),$(2))

CWARNS = \
    $(call cc-option,$(WARNINGS)) \
    $(foreach W, $(WARNINGS_EXTRA),$(call cc-option,$(W)))

CSTD = $(call cc-option,-std=gnu11,-std=gnu99)

$(call make-lazy,CWARNS)
$(call make-lazy,CSTD)

BASIC_CFLAGS += $(CSTD) -DDEBUG=$(DEBUG) $(CWARNS)

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

# If "make install" with no other named targets
ifeq "" "$(filter-out install,$(or $(MAKECMDGOALS),all))"
  OPTCHECK = :
else
  OPTCHECK = SILENT_BUILD='$(MAKE_S)' mk/optcheck.sh
endif

ifndef NO_DEPS
ifeq '$(call try-run,$(CC) -MMD -MP -MF /dev/null -c -x c /dev/null -o /dev/null,y,n)' 'y'
  $(editor_objects) $(test_objects): DEPFLAGS = -MF $(patsubst %.o, %.mk, $@) -MMD -MP
  -include $(patsubst %.o, %.mk, $(editor_objects) $(test_objects))
endif
endif

dte = dte$(EXEC_SUFFIX)

$(dte): $(editor_objects)
build/builtin-config.h: build/builtin-config.list
build/config.o: build/builtin-config.h
build/term-caps.o: build/term-caps.cflags
build/editor.o: build/editor.cflags
build/editor.o: BASIC_CFLAGS += -DVERSION=\"$(VERSION)\"

$(dte):
	$(E) LINK $@
	$(Q) $(CC) $(LDFLAGS) $(BASIC_LDFLAGS) -o $@ $^ $(LDLIBS)

$(editor_objects): build/%.o: src/%.c build/all.cflags | build/
	$(E) CC $@
	$(Q) $(CC) $(CPPFLAGS) $(CFLAGS) $(BASIC_CFLAGS) $(DEPFLAGS) -c -o $@ $<

build/%.cflags: FORCE | build/
	@$(OPTCHECK) '$(CC) $(CPPFLAGS) $(CFLAGS) $(BASIC_CFLAGS)' $@

build/builtin-config.list: FORCE | build/
	@$(OPTCHECK) '$(BUILTIN_CONFIGS)' $@

build/builtin-config.h: $(BUILTIN_CONFIGS) mk/config2c.awk | build/
	$(E) GEN $@
	$(Q) $(AWK) -f mk/config2c.awk $(BUILTIN_CONFIGS) > $@

build/:
	$(Q) mkdir -p $@


CLEANFILES += $(dte)
CLEANDIRS += build/
.PHONY: FORCE
.SECONDARY: build/
