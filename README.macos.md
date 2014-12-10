# Linphone on MacOS X

## Build prerequisite

* Xcode (download from apple or using appstore application)
* Java SE
* [HomeBrew](http://brew.sh) or [Macports](http://www.macports.org/).

### Dependencies

#### Using MacPorts

##### Multiple MacOS version support

In order to enable generation of bundle for multiple MacOS version and 32 bit processors, it is recommended to:

1. Edit `/opt/local/etc/macports/macports.conf` to add the following line:

 > macosx_deployment_target 10.6

2. Edit `/opt/local/etc/macports/variants.conf` to add the following line:

 > +universal


##### Linphone library (liblinphone)

        sudo port install automake autoconf libtool pkgconfig intltool wget cunit \
        antlr3 speex libvpx readline sqlite3 libsoup openldap libupnp \
        ffmpeg-devel -gpl2

##### Linphone UI (GTK version)

Install `GTK`. It is recommended to use the `quartz` backend for better integration.

        sudo port install gtk2 +quartz +no_x11
        sudo port install gtk-osx-application -python27
        sudo port install hicolor-icon-theme

#### Using HomeBrew

##### Linphone library (liblinphone)

        brew install automake intltool libtool pkg-config coreutils \
        yasm nasm wget imagemagick speex ffmpeg

        #then you have to install some dependencies from a tap.
        brew tap Gui13/linphone
        brew install antlr3.2 libantlr3.4c mattintosh4/gtk-mac-integration/gtk-mac-integration

##### Linphone UI (GTK version)

        brew install gettext pygtk gtk+
        brew link gettext --force
        #readline is required from linphonec.c otherwise compilation will fail
        brew link readline --force


### Building Linphone

The next pieces need to be compiled manually.

* To ensure compatibility with multiple MacOS versions it is recommended to do:

        export MACOSX_DEPLOYMENT_TARGET=10.6
        export CFLAGS="-arch i386 -arch x86_64 -mmacosx-version-min=10.5"
        export OBJCFLAGS="-arch i386 -arch x86_64 -mmacosx-version-min=10.5"
        export CXXFLAGS="-arch i386 -arch x86_64 -mmacosx-version-min=10.5"
        export LDFLAGS="-arch i386 -arch x86_64 -mmacosx-version-min=10.5 -Wl,-headerpad_max_install_names -Wl,-read_only_relocs -Wl,suppress"

* (MacPorts only) Install libantlr3c (library used by belle-sip for parsing)

        git clone -b linphone git://git.linphone.org/antlr3.git
        cd antlr3/runtime/C
        ./autogen.sh
        ./configure --disable-static --prefix=/opt/local && make
        sudo make install

* Install polarssl (encryption library used by belle-sip)

        git clone git://git.linphone.org/polarssl.git -b linphone
        cd polarssl
        ./autogen.sh && ./configure --prefix=/opt/local && make
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

        git clone git://git.linphone.org:bzrtp
        cd bzrtp && ./autogen.sh && ./configure --prefix=/opt/local && make
        sudo make install

* (Optional) Install gsm codec

        git clone git://git.linphone.org/gsm.git
        cd gsm
        make CCFLAGS="$CFLAGS -c -O2 -DNeedFunctionPrototypes=1"
        sudo make install INSTALL_ROOT=/opt/local GSM_INSTALL_INC=/opt/local/include

* (Optional) libvpx-1.2 has a bug on MacOS resulting in ugly video. It is recommended to upgrade it manually to 1.3 from source.
The libvpx build isn't able to produce dual architecture files. To workaround this, configure libvpx twice and use lipo to create a dual architecture `libvpx.a`.

* (Optional, proprietary extension only) Compile and install the tunnel library
 If you got the source code from git, run `./autogen.sh` first.
 Then or otherwise, do:

        ./configure --prefix=/opt/local && make && sudo make install

* Compile Linphone
 If you got the source code from git, run `./autogen.sh` first.
 Then or otherwise, :

        PKG_CONFIG_PATH=/usr/local/lib/pkgconfig ./configure --prefix=/opt/local --disable-x11 --with-srtp=/opt/local --with-gsm=/opt/local --enable-zrtp --disable-strict && make

* Install on the system

        sudo make install

 You are done.

### Generate portable bundle

If you want to generate a portable bundle, then install `gtk-mac-bundler`:

        git clone https://github.com/jralls/gtk-mac-bundler.git
        cd gtk-mac-bundler && make install
        export PATH=$PATH:~/.local/bin
        # make this dummy charset.alias file for the bundler to be happy:
        sudo touch /opt/local/lib/charset.alias

The bundler file in `build/MacOS/linphone.bundle` expects some plugins to be installed in `/opt/local/lib/mediastreamer/plugins`.
If you don't need plugins, remove or comment out this line from the bundler file:

        <binary>
        ${prefix:ms2plugins}/lib/mediastreamer/plugins/*.*.so
        </binary>

Then run, inside Linphone source tree configure as told before but with `--enable-relativeprefix` appended.

        make && make bundle

The resulting bundle is located in Linphone build directory, together with a zipped version.

* For a better appearance, you can install `gtk-quartz-engine` (a GTK theme) that makes GTK application more similar to other Mac applications (but not perfect).

        git clone https://github.com/jralls/gtk-quartz-engine.git
        cd gtk-quartz-engine
        autoreconf -i
        ./configure --prefix=/opt/local CFLAGS="$CFLAGS -Wno-error" && make
        sudo make install

Generate a new bundle to have it included.

### libiconv hack

The `Makefile.am` rules used to generate the bundle fetch a `libiconv.2.dylib` from a Linphone download page.
This library adds some additional symbols so that dependencies requiring the `iconv` from `/usr/lib` and the ones requiring from the bundle are both satisfied.
In case this library needs to generated, here are the commands:

        wget http://ftp.gnu.org/pub/gnu/libiconv/libiconv-1.14.tar.gz
        cd libiconv-1.14
        patch -p1 < ../linphone/build/MacOS/libiconv-MacOS.patch
        ./configure --prefix=/opt/local --disable-static 'CFLAGS=-arch i386 -arch x86_64 -mmacosx-version-min=10.5' 'LDFLAGS=-arch i386 -arch x86_64 -mmacosx-version-min=10.5'  CXXFLAGS="-arch i386 -arch x86_64 -mmacosx-version-min=10.5" && make
        make install DESTDIR=/tmp

The resulted library can be found in `/tmp/opt/local/lib`.




