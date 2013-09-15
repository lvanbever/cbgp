DOC_FILES= @HELP_EXTRA_DIST@

EXTRA_DIST = $(DOC_FILES)

install-data-local:
	$(INSTALL) -d $(docdir)/txt
	@(for f in $(DOC_FILES); do \
		$(INSTALL) -m 444 $$f $(docdir)/txt/; \
	done)
