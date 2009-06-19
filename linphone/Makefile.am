## Process this file with automake to produce Makefile.in

# let make re-run automake upon need
ACLOCAL_AMFLAGS = -I m4

if EXTERNAL_ORTP
ORTP_DIR =
else
ORTP_DIR = oRTP
endif

SUBDIRS = m4 pixmaps po ipkg $(ORTP_DIR) mediastreamer2\
		coreapi console gtk-glade share


EXTRA_DIST = config.rpath  BUGS linphone.kdevprj \
				intltool-extract.in	\
				intltool-merge.in	\
				intltool-update.in \
				README.arm \
				README.win32 \
				autogen.sh \
				linphone.spec.in linphone.spec
ACLOCAL_FLAGS=-I$(top_srcdir)/m4

DISTCLEANFILES= intltool-extract intltool-merge intltool-update po/stamp-it po/.intltool-merge-cache

INSTALLDIR=$(shell cd $(top_builddir) && pwd)/linphone-install
ZIPFILE=$(shell cd $(top_builddir) && pwd)/$(PACKAGE)-win32-$(VERSION).zip
ZIP_EXCLUDED=include lib

# `make rpm'

all-local: linphone.spec
linphone.spec: linphone.spec.in

.phony: rpm
rpm:
	$(MAKE) dist
# Create "Specfile" at the same level as the tarball content
	-rm -f $(PACKAGE)-$(VERSION).tar
	gunzip $(PACKAGE)-$(VERSION).tar.gz
	cp $(PACKAGE).spec Specfile
	tar --append --file=$(PACKAGE)-$(VERSION).tar Specfile
	gzip $(PACKAGE)-$(VERSION).tar
# <https://bugzilla.redhat.com/bugzilla/show_bug.cgi?id=2068410>
	TAR_OPTIONS=--wildcards rpmbuild -ta --clean --rmsource --rmspec $(PACKAGE)-$(VERSION).tar.gz

#a zip containing win32 binaries, suitable to generate an installer



zip:
	rm -f $(ZIPFILE)
	rm -rf $(INSTALLDIR)
	mkdir -p $(INSTALLDIR)
	make install DESTDIR=$(INSTALLDIR)
	cd $(INSTALLDIR)/$(prefix) && zip -r $(ZIPFILE) *

	