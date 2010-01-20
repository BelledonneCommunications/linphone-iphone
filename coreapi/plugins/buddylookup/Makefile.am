SUBDIRS=src



INSTALLDIR=$(shell cd $(top_builddir) && pwd)/$(PACKAGE)-install
INSTALLDIR_WITH_PREFIX=$(INSTALLDIR)/$(prefix)
ZIPFILE=$(shell cd $(top_builddir) && pwd)/$(PACKAGE)-win32-$(VERSION).zip
ZIP_EXCLUDED=include 
PLUGIN_DEPS_PREFIX=/usr/bin
PLUGIN_DEPS=	libsoup-2.4-1.dll \
		libgcrypt-11.dll \
		libgnutls-extra-26.dll \
		libgnutlsxx-26.dll \
		libtasn1-3.dll \
		libgnutls-26.dll \
		libgnutls-openssl-26.dll \
		libgpg-error-0.dll


zip:
	rm -f $(ZIPFILE)
	rm -rf $(INSTALLDIR)
	mkdir -p $(INSTALLDIR)
	make install DESTDIR=$(INSTALLDIR)
	mkdir -p $(INSTALLDIR_WITH_PREFIX)/bin
	cd $(PLUGIN_DEPS_PREFIX) && \
		cp -f $(PLUGIN_DEPS) $(INSTALLDIR_WITH_PREFIX)/bin/.
	cd $(INSTALLDIR)/$(prefix) && rm -rf $(ZIP_EXCLUDED) && \
	zip -r $(ZIPFILE) *

clean-local:
	rm -rf $(PACKAGE)-install
	rm -f $(PACKAGE)-win32-$(VERSION).zip

