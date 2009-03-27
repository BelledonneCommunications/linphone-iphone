# linphone/oRTP/Makefile.am -- 

EXTRA_DIST = oRTP.prj ortp-config.h.in TODO pkg.list autogen.sh ortp.pc.in ortp.spec.in ortp.spec ortp.doxygen

SUBDIRS=src build include

pkgconfigdir = $(libdir)/pkgconfig
pkgconfig_DATA = ortp.pc

SOURCES=$(top_srcdir)/include/ortp/*.h $(top_srcdir)/src/*.c $(top_srcdir)/src/*.h

#html doc
if HAVE_DOXYGEN

# doxdir & pkgdocdir are not always defined by automake
docdir=$(datadir)/doc
pkgdocdir=$(docdir)/$(PACKAGE)-$(VERSION)
doc_htmldir=$(pkgdocdir)/html

doc_html_DATA = $(top_builddir)/doc/html/html.tar

$(doc_html_DATA): $(top_builddir)/doc/html/index.html
	cd $(<D) && tar cf html.tar *

$(top_builddir)/doc/html/index.html: $(SOURCES) ortp.doxygen Makefile.am
	rm -rf doc
	$(DOXYGEN) ortp.doxygen

install-data-hook:
	cd $(DESTDIR)$(doc_htmldir) && tar xf html.tar && rm -f html.tar

uninstall-hook:
	cd $(DESTDIR)$(doc_htmldir) && rm -f *

endif

## oRTP packaging methods:

# `make rpm'

.phony: rpm

rpm:
	$(MAKE) dist
# <https://bugzilla.redhat.com/bugzilla/show_bug.cgi?id=206841>
	TAR_OPTIONS=--wildcards rpmbuild -ta --clean --rmsource --rmspec $(PACKAGE)-$(VERSION).tar.gz

# `make package'

if WITH_EPM

.PHONY: package

PKG_NAME = $(PACKAGE)-$(VERSION)-$(RELEASE)
BUILDROOT = $(shell pwd)/epm-install

package: $(srcdir)/pkg.list $(srcdir)/configure
	-rm -rf pkg $(BUILDROOT) $(PKG_NAME).*
	$(MAKE) install DESTDIR=$(BUILDROOT)
	$(MKEPMLIST) -u $(SYS_USER) -g $(SYS_GROUP) --prefix $(prefix) \
		$(BUILDROOT)/$(prefix) > files.list
	$(EPM) -vv -f native -g -n -a $(ARCH) --keep-files --output-dir pkg \
		srcdir=$(srcdir) \
		top_srcdir=$(top_srcdir) \
		top_builddir=$(top_builddir) \
		PACKAGE=$(PACKAGE) \
		SUMMARY="$(SUMMARY)" \
		VERSION=$(ORTP_PKGCONFIG_VERSION) \
		RELEASE=$(RELEASE) \
		LICENSE="$(LICENSE)" \
		VENDOR="$(VENDOR)" \
		PACKAGER="$(PACKAGER)" \
		$(PACKAGE) $(srcdir)/pkg.list
	mv -f pkg/$(PACKAGE)-$(ORTP_PKGCONFIG_VERSION)-$(RELEASE).$(EPM_PKG_EXT) $(PKG_NAME).$(ARCH).$(EPM_PKG_EXT)

clean-local:
	rm -rf pkg $(BUILDROOT)
	rm -f files.list
	rm -rf doc

endif WITH_EPM

distclean-local:
	-rm -f ortp.defs

all-local: ortp.spec

ortp.spec: ortp.spec.in

