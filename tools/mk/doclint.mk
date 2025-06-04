FINDLINKS = sed -n 's|^.*\(https\?://[A-Za-z0-9_/.-]*\).*|\1|gp'
DOCURLS = $(shell $(FINDLINKS) $(DOCFILES) | sort | uniq)

# stackoverflow.com returns all kinds of nonsensical HTTP error codes
# for perfectly valid URLs, so we simply filter them out
DOCURLS_FILTERED = $(filter-out https://stackoverflow.com/%, $(DOCURLS))

# These curl(1) flags are "global" and apply to the whole operation
CHECKURL = $(CURL) --silent --show-error --parallel --fail-early

# ...whereas these must be specified before every URL argument
CHECKURL_LOCAL_FLAGS = -I -o /dev/null -w '%{http_code}  %{url}  %{redirect_url}\n'

# ...as done here
CHECKURL_ARGS = $(foreach u, $(DOCURLS_FILTERED), $(CHECKURL_LOCAL_FLAGS) $(u))

check-docs:
	@printf '\nChecking links from:\n\n'
	@echo '$(DOCFILES) ' | tr ' ' '\n'
	$(Q) $(CHECKURL) $(CHECKURL_ARGS)


.PHONY: check-docs
