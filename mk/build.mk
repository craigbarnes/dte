CC ?= gcc
CFLAGS ?= -g -O2
LDFLAGS ?=
AWK = awk
VERSION = $(shell mk/version.sh 1.11.1)

# These options are used unconditionally, since they've been supported
# by GCC since at least the minimum required version (GCC 4.8) and are
# also supported by Clang.
# - https://gcc.gnu.org/onlinedocs/gcc-4.8.5/gcc/Warning-Options.html
# - https://clang.llvm.org/docs/DiagnosticsReference.html
WARNINGS = \
    -Wall -Wextra -Wformat -Wformat-security -Wformat-nonliteral \
    -Wmissing-prototypes -Wstrict-prototypes -Wwrite-strings \
    -Wundef -Wshadow -Wcast-align -Wredundant-decls -Wswitch-enum \
    -Wvla -Wold-style-definition -Wframe-larger-than=32768 \
    -Wpointer-arith -Wnested-externs -Winit-self -Wbad-function-cast \
    -Werror=div-by-zero -Werror=implicit-function-declaration \
    -Wno-sign-compare -Wno-pointer-sign

BUILTIN_SYNTAX_FILES ?= \
    awk c coccinelle config css ctags d diff docker dte gcode gitblame \
    gitcommit gitignore gitrebase go gomod html haskell ini java \
    javascript json lisp lua mail make markdown meson nftables nginx \
    ninja php python robotstxt roff ruby scheme sed sh sql tex texmfcnf \
    tmux vala weechatlog xml xresources zig inc/c-comment inc/c-uchar \
    inc/diff

BUILTIN_CONFIGS = $(addprefix config/, \
    rc binding/default compiler/gcc compiler/go \
    $(addprefix color/, reset default darkgray) \
    $(addprefix syntax/, $(BUILTIN_SYNTAX_FILES)) )

TEST_CONFIGS := $(addprefix test/data/, $(addsuffix .dterc, \
    env thai crlf insert join pipe redo replace shift repeat fuzz1 fuzz2 ))

util_objects := $(call prefix-obj, build/util/, \
    array ascii base64 debug fd fork-exec hashmap hashset intern intmap \
    log numtostr path ptr-array readfile string strtonum time-util unicode \
    utf8 xmalloc xmemmem xreadwrite xsnprintf xstdio )

command_objects := $(call prefix-obj, build/command/, \
    alias args cache macro parse run serialize )

editorconfig_objects := $(call prefix-obj, build/editorconfig/, \
    editorconfig ini match )

syntax_objects := $(call prefix-obj, build/syntax/, \
    color highlight merge state syntax )

terminal_objects := $(call prefix-obj, build/terminal/, \
    color cursor input ioctl key linux mode osc52 output parse paste \
    query rxvt style terminal )

editor_objects := $(call prefix-obj, build/, \
    bind block block-iter bookmark buffer change cmdline commands \
    compat compiler completion config convert copy ctags edit editor \
    encoding error exec file-history file-option filetype frame history \
    indent load-save lock main misc mode move msg options regexp replace \
    search selection shift show signals spawn status tag vars view window \
    $(addprefix ui-, cmdline prompt status tabbar view window) ui ) \
    $(command_objects) \
    $(editorconfig_objects) \
    $(syntax_objects) \
    $(terminal_objects) \
    $(util_objects)

test_objects := $(call prefix-obj, build/test/, \
    bind bookmark buffer cmdline command config ctags dump editorconfig \
    encoding error filetype frame history indent main options shift spawn \
    status syntax terminal test util )

bench_objects := $(call prefix-obj, build/test/, benchmark)

feature_tests := $(addprefix build/feature/, $(addsuffix .h, \
    dup3 pipe2 fsync memmem mkostemp sigisemptyset TIOCGWINSZ TIOCNOTTY \
    tcgetwinsize posix_madvise qsort_r ))

all_objects := $(editor_objects) $(test_objects) $(bench_objects)
build_subdirs := $(filter-out build/, $(sort $(dir $(all_objects)))) build/feature/ build/gen/

editor_sources := $(patsubst build/%.o, src/%.c, $(editor_objects))
test_sources := $(patsubst build/test/%.o, test/%.c, $(test_objects))
bench_sources := $(patsubst build/test/%.o, test/%.c, $(bench_objects))
all_sources := $(editor_sources) $(test_sources) $(bench_sources)

ifeq "$(WERROR)" "1"
  WARNINGS += -Werror
endif

ifneq "$(ICONV_DISABLE)" "1"
  LDLIBS += $(LDLIBS_ICONV)
endif

ifeq "$(SANE_WCTYPE)" "1"
  BASIC_CPPFLAGS += -DSANE_WCTYPE=1
endif

dte = dte$(EXEC_SUFFIX)
test = build/test/test$(EXEC_SUFFIX)
bench = build/test/bench$(EXEC_SUFFIX)

ifeq "$(USE_SANITIZER)" "1"
  SANITIZER_FLAGS := \
    -fsanitize=address,undefined -fsanitize-address-use-after-scope \
    -fno-sanitize-recover=all -fno-omit-frame-pointer -fno-common
  CC_SANITIZER_FLAGS := $(or \
    $(call cc-option,$(SANITIZER_FLAGS)), \
    $(warning USE_SANITIZER set but compiler doesn't support ASan/UBSan) )
  $(all_objects): BASIC_CFLAGS += $(CC_SANITIZER_FLAGS)
  $(dte) $(test) $(bench): BASIC_LDFLAGS += $(CC_SANITIZER_FLAGS)
  DEBUG = 3
else
  # 0: Disable debugging
  # 1: Enable BUG_ON() and light-weight sanity checks
  # 2: Enable LOG_DEBUG()
  # 3: Enable LOG_TRACE() and expensive sanity checks
  DEBUG ?= 0
endif

ifeq "$(DEBUG)" "0"
  UNWIND = $(call cc-option,-fno-asynchronous-unwind-tables)
  $(call make-lazy,UNWIND)
  BASIC_CFLAGS += $(UNWIND)
else
  BASIC_CPPFLAGS += -DDEBUG=$(DEBUG)
endif

BASIC_CFLAGS += -std=gnu11 $(WARNINGS)
BASIC_CPPFLAGS += -D_FILE_OFFSET_BITS=64
$(all_objects): BASIC_CPPFLAGS += -Isrc -Ibuild/gen

OPTCHECK = mk/optcheck.sh $(if $(MAKE_S),-s)

# If "make install*" with no other named targets
ifeq "" "$(filter-out install%,$(or $(MAKECMDGOALS),all))"
  # Only generate files with $(OPTCHECK) if they don't already exist
  OPTCHECK += -R
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
$(test): $(filter-out build/main.o, $(editor_objects)) $(test_objects)
$(bench): $(filter-out build/main.o, $(editor_objects)) $(bench_objects)
$(util_objects): | build/util/
$(command_objects): | build/command/
$(editorconfig_objects): | build/editorconfig/
$(syntax_objects): | build/syntax/
$(terminal_objects): | build/terminal/
$(build_subdirs): | build/
$(feature_tests): mk/feature-test/defs.h build/all.cflags | build/feature/
build/convert.o: build/gen/buildvar-iconv.h
build/gen/builtin-config.h: build/gen/builtin-config.mk
build/gen/test-data.h: build/gen/test-data.mk
build/config.o: build/gen/builtin-config.h
build/test/config.o: build/gen/test-data.h
build/main.o: build/gen/version.h
build/editor.o: build/gen/version.h
build/compat.o: build/gen/feature.h build/gen/buildvar-iconv.h
build/load-save.o: build/gen/feature.h
build/signals.o: build/gen/feature.h
build/tag.o: build/gen/feature.h
build/terminal/ioctl.o: build/gen/feature.h
build/util/fd.o: build/gen/feature.h
build/util/xmemmem.o: build/gen/feature.h

CFLAGS_FILTERED = $(filter-out -fexceptions, $(CFLAGS))
CFLAGS_ALL = $(CPPFLAGS) $(CFLAGS_FILTERED) $(BASIC_CPPFLAGS) $(BASIC_CFLAGS)
LDFLAGS_ALL = $(CFLAGS_FILTERED) $(LDFLAGS) $(BASIC_LDFLAGS)
featuredef = $(HASH)define HAVE_$(call toupper,$(notdir $(basename $(1))))

$(dte) $(test) $(bench): build/all.ldflags
	$(E) LINK $@
	$(Q) $(CC) $(LDFLAGS_ALL) -o $@ $(filter %.o, $^) $(LDLIBS)

$(editor_objects): build/%.o: src/%.c build/all.cflags | build/
	$(E) CC $@
	$(Q) $(CC) $(CFLAGS_ALL) $(DEPFLAGS) -c -o $@ $<

$(test_objects) $(bench_objects): build/test/%.o: test/%.c build/all.cflags | build/test/
	$(E) CC $@
	$(Q) $(CC) $(CFLAGS_ALL) $(DEPFLAGS) -c -o $@ $<

build/all.ldflags: FORCE | build/
	@$(OPTCHECK) '$(CC) $(LDFLAGS_ALL) $(LDLIBS)' $@

build/%.cflags: FORCE | build/
	@$(OPTCHECK) '$(CC) $(CFLAGS_ALL)' $@

build/gen/version.h: FORCE | build/gen/
	@$(OPTCHECK) '$(HASH)define VERSION "$(VERSION)"' $@

build/gen/buildvar-iconv.h: FORCE | build/gen/
	@$(OPTCHECK) '$(HASH)define ICONV_DISABLE $(if $(call streq,$(ICONV_DISABLE),1),1,0)' $@

build/gen/builtin-config.mk: FORCE | build/gen/
	@$(OPTCHECK) '$(@:.mk=.h): $(BUILTIN_CONFIGS)' $@

build/gen/test-data.mk: FORCE | build/gen/
	@$(OPTCHECK) '$(@:.mk=.h): $(TEST_CONFIGS)' $@

build/gen/builtin-config.h: $(BUILTIN_CONFIGS) mk/config2c.awk | build/gen/
	$(E) GEN $@
	$(Q) $(AWK) -f mk/config2c.awk $(BUILTIN_CONFIGS) > $@

build/gen/test-data.h: $(TEST_CONFIGS) mk/config2c.awk | build/gen/
	$(E) GEN $@
	$(Q) $(AWK) -f mk/config2c.awk $(TEST_CONFIGS) > $@

build/gen/feature.h: mk/feature-test/defs.h $(feature_tests) | build/gen/
	$(E) GEN $@
	$(Q) cat $^ > $@

build/gen/platform.mk: mk/platform.sh | build/gen/
	$(E) GEN $@
	$(Q) mk/platform.sh > $@

$(feature_tests): build/feature/%.h: mk/feature-test/%.c
	$(E) DETECT $@
	$(Q) if $(CC) $(filter-out --coverage, $(CFLAGS_ALL)) -Werror -o $(@:.h=.o) $< 2>$(@:.h=.log); then \
	  echo '$(call featuredef,$@) 1' > $@ ; \
	else \
	  echo '$(call featuredef,$@) 0' > $@ ; \
	fi

build/ $(build_subdirs):
	$(Q) mkdir -p $@


CLEANFILES += $(dte)
CLEANDIRS += build/
.SECONDARY: build/
