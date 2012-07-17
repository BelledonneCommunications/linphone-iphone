## Process this file with automake to produce Makefile.in

# let make re-run automake upon need
ACLOCAL_AMFLAGS = -I m4 $(ACLOCAL_MACOS_FLAGS)

SUBDIRS = build m4 pixmaps po @ORTP_DIR@ @MS2_DIR@ \
		coreapi console gtk share scripts



ACLOCAL_FLAGS=-I$(top_srcdir)/m4

OPTIONAL_SOUNDS=\
		share/sounds/linphone/rings/synth.wav \
		share/sounds/linphone/rings/tapping.wav \
		share/sounds/linphone/rings/orig.wav \
		share/sounds/linphone/rings/sweet.wav \
		share/sounds/linphone/rings/rock.wav


INSTALLDIR=$(shell cd $(top_builddir) && pwd)/linphone-install
INSTALLDIR_WITH_PREFIX=$(INSTALLDIR)/$(prefix)
ZIPFILE=$(shell cd $(top_builddir) && pwd)/$(PACKAGE)-win32-$(VERSION).zip
ZIP_EXCLUDED=include lib \
		$(OPTIONAL_SOUNDS)

SDK_ZIPFILE=$(shell cd $(top_builddir) && pwd)/lib$(PACKAGE)-win32-$(VERSION).zip
SDK_EXCLUDED= \
	bin/linphone.exe \
	lib/*.la \
	share/linphone \
	share/pixmaps \
	share/locale \
	share/gnome \
	$(OPTIONAL_SOUNDS)


GTK_PREFIX=/
GTK_THEME=Outcrop
GTK_FILELIST=gtk+-2.22.1.filelist
GTK_FILELIST_PATH=$(shell cd $(top_srcdir) && pwd)/$(GTK_FILELIST)
LINPHONEDEPS_FILELIST=linphone-deps.filelist
WINBINDIST_FILES=$(shell cat $(top_srcdir)/$(LINPHONEDEPS_FILELIST))
ISS_SCRIPT=linphone.iss
ISS_SCRIPT_PATH=$(shell cd $(top_srcdir) && pwd)/$(ISS_SCRIPT)
#path to Inno Setup 5 compiler
ISCC=ISCC.exe
PACKAGE_WIN32_FILELIST=$(PACKAGE)-win32.filelist
PACKAGE_BUNDLE_FILE=$(top_srcdir)/build/macos/$(PACKAGE).bundle

EXTRA_DIST = config.rpath  BUGS linphone.kdevelop  \
				README.arm \
				README.mingw \
				README.macos \
				autogen.sh \
				linphone.spec \
				linphone.spec.in \
				$(GTK_FILELIST) \
				gen-gtkfilelist.sh \
				$(LINPHONEDEPS_FILELIST) \
				$(ISS_SCRIPT).in

DISTCLEANFILES= $(ISS_SCRIPT) $(PACKAGE_WIN32_FILELIST)

CLEANFILES=Portfile Portfile-devel

# `make rpm'

all-local: linphone.spec linphone.iss

linphone.spec: linphone.spec.in

.phony: rpm rpm-novideo rpm-base
rpm-base:
	$(MAKE) dist
	-rm -f $(PACKAGE)-$(VERSION).tar
	gunzip $(PACKAGE)-$(VERSION).tar.gz
#remove ms2 and ortp spec file to make sure linphone spec file is used bu rpmbuild
	tar --delete --file=$(PACKAGE)-$(VERSION).tar $(PACKAGE)-$(VERSION)/mediastreamer2/mediastreamer2.spec
	tar --delete --file=$(PACKAGE)-$(VERSION).tar $(PACKAGE)-$(VERSION)/oRTP/ortp.spec
	gzip $(PACKAGE)-$(VERSION).tar

rpm: rpm-base
# <https://bugzilla.redhat.com/bugzilla/show_bug.cgi?id=2068410>
	TAR_OPTIONS=--wildcards rpmbuild -ta --clean --rmsource --rmspec $(PACKAGE)-$(VERSION).tar.gz

rpm-novideo: rpm-base
# <https://bugzilla.redhat.com/bugzilla/show_bug.cgi?id=2068410>
	TAR_OPTIONS=--wildcards rpmbuild -ta --clean --rmsource --rmspec $(PACKAGE)-$(VERSION).tar.gz --without video

#a zip containing win32 binaries, suitable to generate an installer

if BUILD_ZRTP
WINBINDIST_FILES+=./bin/libzrtpcpp.dll ./bin/msys-1.0.dll ./bin/msys-crypto-1.0.0.dll
endif

if BUILD_TUNNEL
WINBINDIST_FILES+=./bin/libtunnel-0.dll
endif

other-cherrypick:
	cd $(GTK_PREFIX) && \
	for file in $(WINBINDIST_FILES) ; do \
		if test -d $$file; then \
			mkdir -p $(INSTALLDIR_WITH_PREFIX)/$$file ;\
		else \
			cp $$file $(INSTALLDIR_WITH_PREFIX)/$$file ;\
		fi \
	done
	cp /mingw/bin/libgcc_s*.dll \
		/mingw/bin/libstdc++-6.dll \
		/mingw/bin/libintl-8.dll \
		/mingw/bin/libiconv-2.dll \
		/mingw/bin/pthreadGC2.dll \
		$(INSTALLDIR_WITH_PREFIX)/bin/.


gtk-cherrypick:
	cd $(GTK_PREFIX) && \
	for file in `cat $(GTK_FILELIST_PATH)` ; do \
		if test -d $$file; then \
			mkdir -p $(INSTALLDIR_WITH_PREFIX)/$$file ;\
		else \
			cp $$file $(INSTALLDIR_WITH_PREFIX)/$$file ;\
		fi \
	done && \
	mkdir -p  $(INSTALLDIR_WITH_PREFIX)/share/themes && \
	cp -rf share/themes/$(GTK_THEME) $(INSTALLDIR_WITH_PREFIX)/share/themes/.

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
	cp -f $(top_srcdir)/gtk/gtkrc $(INSTALLDIR_WITH_PREFIX)/.
	cp -f $(top_srcdir)/README $(INSTALLDIR_WITH_PREFIX)/.
	cp -f $(top_srcdir)/COPYING $(INSTALLDIR_WITH_PREFIX)/.
	cd $(INSTALLDIR_WITH_PREFIX) && zip -r $(ZIPFILE) *

sdk:
	rm -f $(SDK_ZIPFILE)
	rm -rf $(INSTALLDIR)
	mkdir -p $(INSTALLDIR)
	make install DESTDIR=$(INSTALLDIR)
# remove unwanted stuff (gtk interface)
	cd $(INSTALLDIR_WITH_PREFIX) && rm -rf $(SDK_EXCLUDED)
	make other-cherrypick
	cp -f $(top_srcdir)/README $(INSTALLDIR_WITH_PREFIX)/.
	cp -f $(top_srcdir)/COPYING $(INSTALLDIR_WITH_PREFIX)/.
	cd $(INSTALLDIR_WITH_PREFIX) && zip -r $(SDK_ZIPFILE) *

filelist: zip
	cd $(INSTALLDIR_WITH_PREFIX) && \
	rm -f $(PACKAGE_WIN32_FILELIST) && \
	for file in `find` ; do \
		if ! test -d $$file ; then \
			echo "Source: $$file; Destdir: {app}\\`dirname $$file`; Flags: ignoreversion" \
			>> $(PACKAGE_WIN32_FILELIST) ;\
		fi \
	done


### WINDOWS

setup.exe: filelist
	cp $(ISS_SCRIPT) $(INSTALLDIR_WITH_PREFIX)/.
	cd $(INSTALLDIR_WITH_PREFIX) && \
	$(ISCC) $(ISS_SCRIPT) 
	mv $(INSTALLDIR_WITH_PREFIX)/Output/setup.exe $(PACKAGE)-$(VERSION)-setup.exe
	rm -rf $(INSTALLDIR_WITH_PREFIX)/Output
	rm -f $(INSTALLDIR_WITH_PREFIX)/$(PACKAGE_WIN32_FILELIST)
	rm -f $(INSTALLDIR_WITH_PREFIX)/$(ISS_SCRIPT)

###

newdate:
	cd gtk && $(MAKE) newdate

if HAVE_MD5SUM
GEN_MD5=$(shell $(MD5SUM) linphone-$(VERSION).tar.gz | awk {'print $$4'})
else
GEN_MD5=$(shell $(MD5SUM) linphone-$(VERSION).tar.gz | awk {'print $$1'})
endif

Portfile:	$(top_srcdir)/scripts/Portfile.tmpl dist
	sed -e 's/\@VERSION\@/$(LINPHONE_VERSION)/g' \
	  -e 's/\@LINPHONE_MD5\@/$(GEN_MD5)/' < $< > $@

Portfile-devel:	$(top_srcdir)/scripts/Portfile-devel.tmpl dist
	sed -e 's/\@VERSION\@/$(LINPHONE_VERSION)/g' \
	  -e 's/\@LINPHONE_MD5\@/$(GEN_MD5)/' < $< > $@


### MAC

MACAPPNAME=Linphone.app
MACAPPZIP=$(PACKAGE)-$(VERSION).app.zip
BUNDLEPREFIX=./
BUNDLEDIR=$(BUNDLEPREFIX)$(MACAPPNAME)
LIBICONV_HACK=$(top_builddir)/build/macos/libiconv.2.dylib

$(LIBICONV_HACK):
	cd $(top_builddir)/build/macos && \
	wget http://download-mirror.savannah.gnu.org/releases/linphone/misc/libiconv.2.dylib

bundle: $(LIBICONV_HACK)
	rm -rf $(INSTALLDIR)
	mkdir -p $(INSTALLDIR)
	make install DESTDIR=$(INSTALLDIR)
	BUNDLE_PREFIX=$(BUNDLEPREFIX) \
	LINPHONE_INSTALL_PREFIX=$(INSTALLDIR_WITH_PREFIX) \
	LIBLINPHONE_INSTALL_PREFIX=$(INSTALLDIR_WITH_PREFIX) \
	MS2_PLUGINS_INSTALL_PREFIX=$(prefix) \
	gtk-mac-bundler $(PACKAGE_BUNDLE_FILE)
	printf "[Pango]\nModuleFiles=./etc/pango/pango.modules\n" \
	> $(BUNDLEDIR)/Contents/Resources/etc/pango/pangorc 
	cp -f $(LIBICONV_HACK) $(BUNDLEDIR)/Contents/Resources/lib/.
	cd $(BUNDLEDIR)/.. && rm -f $(MACAPPZIP) && zip -r $(MACAPPZIP) $(MACAPPNAME) && cd -

###
### CLEAN

clean-local:
	rm -rf $(BUNDLEDIR)
discovery:
	touch specs.cpp
	$(CC) --include $(top_builddir)/config.h \
          --include $(top_builddir)/mediastreamer2/mediastreamer-config.h  \
          --include $(top_builddir)/oRTP/ortp-config.h \
          $(TUNNEL_CFLAGS) $(CFLAGS) $(MEDIASTREAMER2_CFLAGS) $(ORTP_CFLAGS) -E -P -v -dD specs.cpp
