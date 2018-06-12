SYNTAX_LINT = $(AWK) -f test/syntax-lint.awk

check-commands: $(dte)
	$(E) CMDTEST test/data/thai.dterc
	$(Q) ./$(dte) -R -c 'eval cat test/data/thai.dterc'
	$(Q) diff -q build/test/thai-utf8.txt test/data/thai-utf8.txt
	$(Q) diff -q build/test/thai-tis620.txt test/data/thai-tis620.txt
	$(Q) $(RM) build/test/thai-*.txt
	$(E) CMDTEST test/data/fuzz1.dterc
	$(Q) ./$(dte) -R -c 'eval cat test/data/fuzz1.dterc'
	$(E) RCTEST test/data/env.dterc
	$(Q) ./$(dte) -r test/data/env.dterc -c 'quit -f'

check-syntax-files:
	$(E) LINT 'config/syntax/*'
	$(Q) $(SYNTAX_LINT) $(addprefix config/syntax/, $(BUILTIN_SYNTAX_FILES))
	$(Q) ! $(SYNTAX_LINT) test/data/syntax-lint.dterc 2>/dev/null


.PHONY: check-commands check-syntax-files
