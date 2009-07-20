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



ACLOCAL_FLAGS=-I$(top_srcdir)/m4


INSTALLDIR=$(shell cd $(top_builddir) && pwd)/linphone-install
INSTALLDIR_WITH_PREFIX=$(INSTALLDIR)/$(prefix)
ZIPFILE=$(shell cd $(top_builddir) && pwd)/$(PACKAGE)-win32-$(VERSION).zip
ZIP_EXCLUDED=include lib \
		share/sounds/linphone/rings/synth.wav \
		share/sounds/linphone/rings/tapping.wav \
		share/sounds/linphone/rings/orig.wav 

GTK_PREFIX=/usr/local
GTK_FILELIST=gtk+-2.16.2.filelist
GTK_FILELIST_PATH=$(shell cd $(top_srcdir) && pwd)/$(GTK_FILELIST)
LINPHONEDEPS_FILELIST=linphone-deps.filelist
WINBINDIST_FILES=$(shell cat $(top_srcdir)/$(LINPHONEDEPS_FILELIST))
ISS_SCRIPT=linphone.iss
ISS_SCRIPT_PATH=$(shell cd $(top_srcdir) && pwd)/$(ISS_SCRIPT)
#path to Inno Setup 5 compiler
ISCC=/c/Program\ Files/Inno\ Setup\ 5/ISCC.exe
PACKAGE_WIN32_FILELIST=$(PACKAGE)-win32.filelist

EXTRA_DIST = config.rpath  BUGS linphone.kdevelop  linphone.kdevelop.filelist\
				intltool-extract.in	\
				intltool-merge.in	\
				intltool-update.in \
				README.arm \
				README.mingw \
				autogen.sh \
				linphone.spec.in \
				$(GTK_FILELIST) \
				gen-gtkfilelist.sh \
				$(LINPHONEDEPS_FILELIST) \
				$(ISS_SCRIPT).in

DISTCLEANFILES= intltool-extract intltool-merge intltool-update po/stamp-it po/.intltool-merge-cache $(ISS_SCRIPT) $(PACKAGE_WIN32_FILELIST)

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

other-cherrypick:
	cd $(GTK_PREFIX) && \
	for file in $(WINBINDIST_FILES) ; do \
		if test -d $$file; then \
			mkdir -p $(INSTALLDIR_WITH_PREFIX)/$$file ;\
		else \
			cp $$file $(INSTALLDIR_WITH_PREFIX)/$$file ;\
		fi \
	done 

gtk-cherrypick:
	cd $(GTK_PREFIX) && \
	for file in `cat $(GTK_FILELIST_PATH)` ; do \
		if test -d $$file; then \
			mkdir -p $(INSTALLDIR_WITH_PREFIX)/$$file ;\
		else \
			cp $$file $(INSTALLDIR_WITH_PREFIX)/$$file ;\
		fi \
	done && \
	cp -rf share/themes $(INSTALLDIR_WITH_PREFIX)/share/.

zip:
	rm -f $(ZIPFILE)
	rm -rf $(INSTALLDIR)
	mkdir -p $(INSTALLDIR)
	make install DESTDIR=$(INSTALLDIR)
#remove unwanted linphone stuff
	cd $(INSTALLDIR_WITH_PREFIX) && rm -rf $(ZIP_EXCLUDED)
#add gtk dlls and files
	make gtk-cherrypick
	make other-cherrypick
	cp -f $(top_srcdir)/gtk-glade/gtkrc $(INSTALLDIR_WITH_PREFIX)/.
	cp -f $(top_srcdir)/README $(INSTALLDIR_WITH_PREFIX)/.
	cp -f $(top_srcdir)/COPYING $(INSTALLDIR_WITH_PREFIX)/.
	cd $(INSTALLDIR_WITH_PREFIX) && zip -r $(ZIPFILE) *

setup.exe: zip
	cd $(INSTALLDIR_WITH_PREFIX) && \
	rm -f $(PACKAGE_WIN32_FILELIST) && \
	for file in `find` ; do \
		if ! test -d $$file ; then \
			echo "Source: $$file; Destdir: {app}\\`dirname $$file`; Flags: ignoreversion" \
			>> $(PACKAGE_WIN32_FILELIST) ;\
		fi \
	done
	cp $(ISS_SCRIPT) $(INSTALLDIR_WITH_PREFIX)/.
	cd $(INSTALLDIR_WITH_PREFIX) && \
	$(ISCC) $(ISS_SCRIPT) 
	mv $(INSTALLDIR_WITH_PREFIX)/Output/setup.exe $(PACKAGE)-$(VERSION)-setup.exe
	rm -rf $(INSTALLDIR_WITH_PREFIX)/Output
	rm -f $(INSTALLDIR_WITH_PREFIX)/$(PACKAGE_WIN32_FILELIST)
	rm -f $(INSTALLDIR_WITH_PREFIX)/$(ISS_SCRIPT)


