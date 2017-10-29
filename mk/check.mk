test_objects := $(addprefix build/test/, $(addsuffix .o, \
    test_main ))

test = build/test/test$(EXEC_SUFFIX)

$(test): $(filter-out build/main.o, $(editor_objects)) $(test_objects)

check: $(test) all
	$(E) TEST $<
	$(Q) $<

check-commands: $(dte) | build/test/
	@$(dte) -R -c "$$(cat test/thai.dterc | sed '/^\#/d;/^$$/d' | tr '\n' ';')"
	@diff -q build/test/thai-utf8.txt test/thai-utf8.txt
	@diff -q build/test/thai-tis620.txt test/thai-tis620.txt
	@$(RM)  build/test/thai-utf8.txt build/test/thai-tis620.txt

$(test):
	$(E) LINK $@
	$(Q) $(CC) $(LDFLAGS) $(BASIC_LDFLAGS) -o $@ $^ $(LDLIBS)

$(test_objects): build/test/%.o: test/%.c build/all.cflags | build/test/
	$(E) CC $@
	$(Q) $(CC) $(CPPFLAGS) $(CFLAGS) $(BASIC_CFLAGS) $(DEPFLAGS) -Isrc -c -o $@ $<

build/test/:
	@mkdir -p $@


.PHONY: check check-commands
