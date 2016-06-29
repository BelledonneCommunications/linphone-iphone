# Linphone on MacOS X

## Build prerequisite

* Xcode (download from apple or using appstore application)
* [Java SE](http://www.oracle.com/technetwork/java/javase/downloads/index.html) or openJDK
 This is required to generate a C sourcefile from SIP grammar using [antlr3](http://www.antlr3.org/) generator.
* [HomeBrew](http://brew.sh) or [Macports](http://www.macports.org/).

### Dependencies

#### Using MacPorts

##### Multiple MacOS version support

In order to enable generation of bundle for older MacOS version, it is recommended to:

 Edit `/opt/local/etc/macports/macports.conf` to add the following line:

 > macosx_deployment_target 10.7
 > buildfromsource always


##### Linphone library (liblinphone)

        sudo port install automake autoconf libtool pkgconfig intltool wget bcunit \
        antlr3 speex readline sqlite3 openldap libupnp \
        ffmpeg-devel -gpl2

##### Linphone UI (GTK version)

Install `GTK`. It is recommended to use the `quartz` backend for better integration.

        sudo port install gtk2 +quartz +no_x11
        sudo port install gtk-osx-application-gtk2 +no_python
        sudo port install hicolor-icon-theme

#### Using HomeBrew

##### Linphone library (liblinphone)

        brew install intltool libtool wget pkg-config automake libantlr3.4c \
                homebrew/versions/antlr3 gettext speex ffmpeg readline libvpx opus
        ln -s /usr/local/bin/glibtoolize /usr/local/bin/libtoolize
        brew link --force gettext
        #readline is required from linphonec.c otherwise compilation will fail
        brew link readline --force

##### Linphone UI (GTK version)

        brew install cairo --without-x11
        brew install gtk+ --without-x11
        brew install gtk-mac-integration hicolor-icon-theme

### Building Linphone

The next pieces need to be compiled manually.

* To ensure compatibility with multiple MacOS versions it is recommended to do:

        export MACOSX_DEPLOYMENT_TARGET=10.7
        export LDFLAGS="-Wl,-headerpad_max_install_names"

* (MacPorts only) Install libantlr3c (library used by belle-sip for parsing)

        git clone -b linphone git://git.linphone.org/antlr3.git
        cd antlr3/runtime/C
        ./autogen.sh
        ./configure --disable-static --prefix=/opt/local && make
        sudo make install

* Install polarssl (encryption library used by belle-sip)

        git clone git://git.linphone.org/polarssl.git
        cd polarssl
        ./autogen.sh && ./configure --prefix=/opt/local && make
        sudo make install

* Install libvpx (Must be manualy build because the macport recipe does not support 'macosx_deployment_target')

        git clone https://chromium.googlesource.com/webm/libvpx -b v1.4.0
        cd libvpx
        ./configure --prefix=/opt/local \
                --target=x86_64-darwin10-gcc \
                --enable-error-concealment \
                --enable-multithread \
                --enable-realtime-only \
                --enable-spatial-resampling \
                --enable-vp8 \
                --disable-vp9 \
                --enable-libs \
                --disable-install-docs \
                --disable-debug-libs \
                --disable-examples \
                --disable-unit-tests \
                --as=yasm
        make
        sudo make install

* Install belle-sip (sip stack)

        git clone git://git.linphone.org/belle-sip.git
        cd belle-sip
        ./autogen.sh && ./configure --prefix=/opt/local && make
        sudo make install

* (Optional) Install srtp for call encryption

        git clone git://git.linphone.org/srtp.git
        cd srtp && autoconf && ./configure --prefix=/opt/local && make libsrtp.a
        sudo make install

* (Optional) Install zrtp, for unbreakable call encryption

        git clone git://git.linphone.org/bzrtp.git
        cd bzrtp && ./autogen.sh && ./configure --prefix=/opt/local && make
        sudo make install

* (Optional) Install gsm codec

        git clone git://git.linphone.org/gsm.git
        cd gsm
        make CCFLAGS="$CFLAGS -c -O2 -DNeedFunctionPrototypes=1"
        sudo make install INSTALL_ROOT=/opt/local GSM_INSTALL_INC=/opt/local/include

* (Optional, proprietary extension only) Compile and install the tunnel library
 If you got the source code from git, run `./autogen.sh` first.
 Then or otherwise, do:

        ./configure --prefix=/opt/local && make && sudo make install

* Compile Linphone
 If you got the source code from git, run `./autogen.sh` first.
 Then or otherwise, :

        PKG_CONFIG_PATH=/opt/local/lib/pkgconfig ./configure --prefix=/opt/local --with-srtp=/opt/local --with-gsm=/opt/local --enable-zrtp --disable-strict && make

* Install on the system

        sudo make install

 You are done.

### Generate portable bundle

If you want to generate a portable bundle, then install `gtk-mac-bundler` linphone fork:

	git clone git://git.linphone.org/gtk-mac-bundler.git
	cd gtk-mac-bundler
	make install
    export PATH=$PATH:~/.local/bin
	# set writing right for owner on the libssl and libcrypto libraries in order gtk-mac-bundler
	# be able to rewrite their rpath
	sudo chmod u+w /opt/local/lib/libssl.1.0.0.dylib /opt/local/lib/libcrypto.1.0.0.dylib

The bundler file in `build/MacOS/linphone.bundle` expects some plugins to be installed in `/opt/local/lib/mediastreamer/plugins`.
If you don't need plugins, remove or comment out this line from the bundler file:

        <binary>
        ${prefix:ms2plugins}/lib/mediastreamer/plugins/*.*.so
        </binary>

If using HomeBrew, this is not working yet. However you will at least need to:

        brew install shared-mime-info glib-networking hicolor-icon-theme
        update-mime-database /usr/local/share/mime

 And modify also:

        <prefix name="default">/usr/local</prefix>

Then run, inside Linphone source tree configure as told before but with `--enable-relativeprefix` appended.

        make && make bundle

The resulting bundle is located in Linphone build directory, together with a zipped version.

* For a better appearance, you can install `gtk-quartz-engine` (a GTK theme) that makes GTK application more similar to other Mac applications (but not perfect).
	sudo port install gnome-common
	git clone https://github.com/jralls/gtk-quartz-engine.git
	cd gtk-quartz-engine
	./autogen.sh
	./configure --prefix=/opt/local CFLAGS="$CFLAGS -Wno-error" && make
	sudo make install

Generate a new bundle to have it included.





