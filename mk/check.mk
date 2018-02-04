CMDTEST = $(dte) -Rc "$$(sed '/^\#/d;/^$$/d' < '$(strip $(1))' | tr '\n' ';')"
SYNTAX_LINT = $(AWK) -f test/syntax-lint.awk

test_objects := $(addprefix build/test/, $(addsuffix .o, \
    test_main ))

test = build/test/test$(EXEC_SUFFIX)

$(test): $(filter-out build/main.o, $(editor_objects)) $(test_objects)

check: $(test) all
	$(E) TEST $<
	$(Q) $<

check-commands: $(dte) | build/test/
	$(E) CMDTEST test/thai.dterc
	$(Q) $(call CMDTEST, test/thai.dterc)
	$(Q) diff -q build/test/thai-utf8.txt test/thai-utf8.txt
	$(Q) diff -q build/test/thai-tis620.txt test/thai-tis620.txt
	$(Q) $(RM) build/test/thai-*.txt

check-syntax-files:
	$(E) LINT 'config/syntax/*'
	$(Q) $(SYNTAX_LINT) $(addprefix config/syntax/, $(BUILTIN_SYNTAX_FILES))
	$(Q) ! $(SYNTAX_LINT) test/syntax-lint.dterc >/dev/null

$(test):
	$(E) LINK $@
	$(Q) $(CC) $(LDFLAGS) $(BASIC_LDFLAGS) -o $@ $^ $(LDLIBS)

$(test_objects): build/test/%.o: test/%.c build/all.cflags | build/test/
	$(E) CC $@
	$(Q) $(CC) $(CPPFLAGS) $(CFLAGS) $(BASIC_CFLAGS) $(DEPFLAGS) -Isrc -c -o $@ $<

build/test/:
	@mkdir -p $@


.PHONY: check check-commands check-syntax-files
