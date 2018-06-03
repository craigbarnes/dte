SYNTAX_LINT = $(AWK) -f test/syntax-lint.awk

check-commands: $(dte)
	$(E) CMDTEST test/thai.dterc
	$(Q) ./$(dte) -R -c 'eval cat test/thai.dterc'
	$(Q) diff -q build/test/thai-utf8.txt test/thai-utf8.txt
	$(Q) diff -q build/test/thai-tis620.txt test/thai-tis620.txt
	$(Q) $(RM) build/test/thai-*.txt
	$(E) CMDTEST test/fuzz1.dterc
	$(Q) ./$(dte) -R -c 'eval cat test/fuzz1.dterc'

check-syntax-files:
	$(E) LINT 'config/syntax/*'
	$(Q) $(SYNTAX_LINT) $(addprefix config/syntax/, $(BUILTIN_SYNTAX_FILES))
	$(Q) ! $(SYNTAX_LINT) test/syntax-lint.dterc 2>/dev/null


.PHONY: check-commands check-syntax-files
