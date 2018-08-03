SYNTAX_LINT = $(AWK) -f test/syntax-lint.awk

check-syntax-files:
	$(E) LINT 'config/syntax/*'
	$(Q) $(SYNTAX_LINT) $(addprefix config/syntax/, $(BUILTIN_SYNTAX_FILES))
	$(Q) ! $(SYNTAX_LINT) test/data/syntax-lint.dterc 2>/dev/null


.PHONY: check-syntax-files
