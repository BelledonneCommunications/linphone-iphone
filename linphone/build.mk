#!/usr/bin/make -f
# This script "automatically" builds some binary distribution of linphone :
# debian packages (sid) and familiar (arm-linux/ipaq) packages.
# generation of rpms can be added in the future.
# it can be invoked in the following way:
# ./build.mk debs LINPHONE=<path to linphone's tar.gz>
# ./build.mk all LINPHONE=<path to linphone's tar.gz> GLIB=<path to glib>2.2's tar.gz>
#				LIBOSIP=<path to libosip's tar.gz>
#


ARMROOT=/armbuild
ARMTOOLCHAIN=/usr/local/arm/3.2.3/bin

all:	check debs ipks 


clean:
	rm -f *.stamp
	rm -rf build-debs
	rm -rf build-ipks
	
check:
	@if test -n "${LD_PRELOAD}" ; then echo "ERROR: LD_PRELOAD is set !!"; exit 1;fi

ipks:	ipkg-tools.stamp libosip.ipk linphone.ipk clean

ipkg-tools.stamp:
	if which ipkg-build ; then echo "Found ipkg-build." ;\
	else echo "** ERROR ** Cannot find ipkg-build"; exit 1; fi
	if test -e $(ARMTOOLCHAIN)/arm-linux-gcc ; then \
		echo "arm toolchain seems ok.";\
	else echo "** ERROR ** Cannot find arm toolchain."; exit 1; fi
	touch ipkg-tools.stamp

glib.ipk	:  glib.stamp linphone-sources-ipk.stamp
	rm -rf build-ipks/glib*
	cp $(GLIB) build-ipks/.
	export CONFIG_SITE=`pwd`/build-ipks/ipaq-config.site && \
	cd build-ipks && tar -xvzf glib*.tar.gz && rm -f glib*.tar.gz && cd glib* && \
	./configure --prefix=$(ARMROOT)/usr --host=arm-linux --with-gnu-ld --disable-static  \
	&& make AM_CFLAGS=-DSSIZE_MAX=0xffffffff && make install && \
	make install prefix=`pwd`$(ARMROOT)/usr
	echo "Building .ipk file..."
	cd build-ipks/glib*$(ARMROOT) && mkdir CONTROL && cd CONTROL && \
	cp ../../../linphone*/ipkg/glib.control control && \
	cd ../.. && rm -rf armbuild/usr/include
	cd build-ipks/glib* && ipkg-build -o root -g root armbuild/
	cp build-ipks/glib*/*.ipk .
	
libosip.ipk	: libosip.stamp linphone-sources-ipk.stamp
	rm -rf build-ipks/libosip*
	cp $(LIBOSIP) build-ipks/
	export CONFIG_SITE=`pwd`/build-ipks/ipaq-config.site && \
	cd build-ipks && tar -xvzf libosip*.tar.gz && rm -f libosip*.tar.gz && cd libosip* && \
	./configure --prefix=$(ARMROOT)/usr --host=arm-linux --with-gnu-ld --disable-static \
	&& make && make install && make install prefix=`pwd`$(ARMROOT)/usr
	echo "Building .ipk file..."
	cd build-ipks/libosip*$(ARMROOT) && mkdir CONTROL && cd CONTROL && \
	cp ../../../linphone*/ipkg/libosip.control control &&\
	cd ../.. && rm -rf armbuild/usr/include
	cd build-ipks/libosip* && ipkg-build -o root -g root armbuild/
	cp build-ipks/libosip*/*.ipk .
	
	
linphone.ipk : linphone.stamp linphone-sources-ipk.stamp
	export CONFIG_SITE=`pwd`/build-ipks/ipaq-config.site && \
	cd build-ipks/linphone* && export PKG_CONFIG_PATH=$(ARMROOT)/usr/lib/pkgconfig &&\
	./configure --prefix=$(ARMROOT)/usr --with-realprefix=/usr --host=arm-linux \
	--with-gnu-ld --disable-static --disable-gnome_ui --disable-glib --with-osip=$(ARMROOT)/usr \
	--disable-ogg --disable-rtcp && make && make install prefix=`pwd`$(ARMROOT)/usr
	echo "Building .ipk file..."
	cd build-ipks/linphone*$(ARMROOT) && mkdir -p CONTROL && cd CONTROL && \
	cp -f ../../ipkg/linphone.control control &&\
	cd ../.. && rm -rf armbuild/usr/include armbuild/usr/share/gtk-doc \
	&& cd armbuild/usr/share/sounds/linphone/rings && \
	rm -f rock.wav sweet.wav bigben.wav toy.wav tapping.wav synth.wav && cd - \
	cd build-ipks/linphone* && ipkg-build -o root -g root armbuild/
	cp -f build-ipks/linphone*/*.ipk .

build-ipks.stamp:
	-@touch $(ARMROOT)/dummy
	@if test -e $(ARMROOT)/dummy ; \
		then echo "armroot is fine: $(ARMROOT)"; \
	else \
		echo "** ERROR: you need to create a $(ARMROOT) directory readable and writeable by the user running this build script.";\
		exit 1;\
	fi	
	rm -f $(ARMROOT)/dummy
	touch build-ipks.stamp
	mkdir build-ipks


linphone-sources-ipk.stamp: linphone.stamp build-ipks.stamp
	cp $(LINPHONE) build-ipks/.
	cd build-ipks && tar -xvzf linphone*.tar.gz && cp linphone*/ipkg/ipaq-config.site . \
	&& rm -f linphone*.tar.gz
	touch linphone-sources-ipk.stamp

debs:	linphone.stamp clean
	rm -f linphone.stamp
	rm -rf build-debs
	mkdir build-debs
	cp $(LINPHONE) build-debs/.
	cd build-debs && tar -xvzf *.tar.gz && cd linphone* && dpkg-buildpackage -rfakeroot
	cp build-debs/*.deb .

linphone.stamp:
	@if test -n "$(LINPHONE)" ; \
		then echo "linphone source is $(LINPHONE)" ; \
		touch linphone.stamp; \
	else \
		echo "No linphone source found." ; \
		exit 1; \
	fi

libosip.stamp:
	@if test -n "$(LIBOSIP)" ; \
		then echo "libosip source is $(LIBOSIP)" ; \
		touch libosip.stamp; \
	else \
		echo "No libosip source found." ; \
		exit 1; \
	fi
	
glib.stamp:
	@if test -n "$(GLIB)" ; \
		then echo "glib source is $(GLIB)" ; \
	else \
		echo "No glib source found." ; \
		exit -1; \
	fi
	@if which glib-genmarshal ; \
	then \
			echo "native glib-2.2 found on build host." ; \
			touch glib.stamp ;\
	else \
			echo "** ERROR ** You need a working glib>2.2 on the build machine to be able to crosscompile it for arm." ;\
			echo "** ERROR ** Please install a glib on this machine."; exit 1; \
	fi
