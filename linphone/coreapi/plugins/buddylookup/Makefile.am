SUBDIRS=src



INSTALLDIR=$(shell cd $(top_builddir) && pwd)/buddylookup-install
ZIPFILE=$(shell cd $(top_builddir) && pwd)/$(PACKAGE)-win32-$(VERSION).zip
ZIP_EXCLUDED=include 



zip:
	rm -f $(ZIPFILE)
	rm -rf $(INSTALLDIR)
	mkdir -p $(INSTALLDIR)
	make install DESTDIR=$(INSTALLDIR)
	cd $(INSTALLDIR)/$(prefix) && rm -rf $(ZIP_EXCLUDED) && \
	zip -r $(ZIPFILE) *

clean-local:
	rm -rf buddylookup-install
	rm $(PACKAGE)-win32-$(VERSION).zip
