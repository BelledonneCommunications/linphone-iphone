					LINPHONE ON M68k-LINUX (by GIAN)
					********************************

The console version of linphone works on arm-linux, but also on m68k-linux? I’m trying to provide this:

* I used the same toolchain specified in the LTIB menu configuration, that is, on my system:
/opt/mtwk/usr/local/m68k-linux/gcc-3.4.0-glibc-2.3.2-v4e

* I have created within my home directory a ColdFire-install-environment/ directory, copied into it the fresh tarballs of libogg-1.1.3, libosip2-2.2.2, speex-1.1.12, linphone-1.4.0 readline-5.1 and ncurses-5.5 (readline needs ncurses) Uncompressed all these
tarballs. 

Very important things common to all packages being cross compiled:
******************************************************************
* Copy the ipaq-config.site in the ipkg/ directory of linphone into some safe place, for example:
cp /home/gianluca/ColdFire-install-environment/linphone-1.4.0-install/linphone-1.4.0/ipkg/ipag-config.site /home/gianluca/ColdFire-install-environment/linphone-1.4.0-install/
Edit the ipaq-config.site file and replace all the arm-linux strings with m68k-linux one. Add also the –mcfv4e flag to the CFLAGS, CXXFLAGS, and CPPFLAGS labels. 
* You need a directory that we call M68K_INSTALL_TREE that will own files in the same way they will be installed on the target computer.

mkdir /ColdFire-linphonec-1.4.0-mcfv4e
export M68K_INSTALL_TREE=/ColdFire-linphonec-1.4.0-mcfv4e
export CONFIG_SITE=/home/gianluca/ColdFire-install-environment/linphone-1.4.0-install/ipaq-config.site
export PATH=$PATH:/opt/mtwk/usr/local/m68k-linux/gcc-3.4.0-glibc-2.3.2-v4e/bin


Cross compiling ncurses for M68k:
********************************
./configure --prefix=/usr --host=m68k-linux --with-gnu-ld --with-shared
make
make install DESTDIR=$M68K_INSTALL_TREE


Cross compiling readline for M68k:
*********************************
./configure --prefix=/usr --host=m68k-linux --with-gnu-ld --disable-static
make
make install DESTDIR=$M68K_INSTALL_TREE



Cross compiling libosip for M68k:
********************************
./configure --prefix=/usr --host=m68k-linux --with-gnu-ld --disable-static
make
make install DESTDIR=$M68K_INSTALL_TREE


Cross compiling libogg for M68k:
********************************
./configure --prefix=/usr --host=m68k-linux --with-gnu-ld --disable-static –enable-fixed-point
make
make install DESTDIR=$M68K_INSTALL_TREE


Cross compiling speex for M68k:
********************************
./configure --prefix=/usr --host=m68k-linux --with-gnu-ld --disable-static --enable-fixed-point --enable-m68k-asm --with-ogg=/ColdFire-linphonec-1.4.0-mcfv4e/usr --with-ogg-includes=/ColdFire-linphonec-1.4.0-mcfv4e/usr/include –with-ogg-libraries=/ColdFire-linphonec-1.4.0-mcfv4e/usr/lib --disable-oggtest 
make
make install DESTDIR=$M68K_INSTALL_TREE


cp /home/gianluca/ColdFire-iinstall-environment/linphone-1.4.0-install/linphone-1.4.0/mediastreamer2/src/.libs/libquickstream.so.0.0.0 /opt/mtwk/usr/local/m68k-linux/gcc-3.4.0-glibc-2.3.2-v4e/m68k-linux/lib
cd /opt/mtwk/usr/local/m68k-linux/gcc-3.4.0-glibc-2.3.2-v4e/m68k-linux/lib
ln -s libquickstream.so.0.0.0 libquickstream.so.0
ln -s libquickstream.so.0.0.0 libquickstream.so

cp /home/gianluca/ColdFire-install-environment/linphone-1.4.0-install/linphone-1.4.0/mediastreamer2/src/.libs/libmediastreamer.so.0.0.0 /opt/mtwk/usr/local/m68k-linux/gcc-3.4.0-glibc-2.3.2-v4e/m68k-linux/lib
cd /opt/mtwk/usr/local/m68k-linux/gcc-3.4.0-glibc-2.3.2-v4e/m68k-linux/lib
ln -s libmediastreamer.so.0.0.0 libmediastreamer.so.0
ln -s libmediastreamer.so.0.0.0 libmediastreamer.so


Cross compiling linphone for M68k:
********************************
First you need to remove all .la files from the M68K_INSTALL_TREE because it confuses libtool and makes
the linker use your build machine binaries instead of the m68k-crosscompiled ones.
rm -f $M68K_INSTALL_TREE/usr/lib/*.la
#for some reason pkg-config doesn't like cross-compiling...
export PKG_CONFIG=/usr/bin/pkg-config
./configure --prefix=/usr --host=m68k-linux --with-gnu-ld --disable-static --disable-strict --enable-gnome_ui=no --enable-alsa --disable-glib --disable-video --with-osip=$ARM_INSTALL_TREE/usr --with-osipparser=$ARM_INSTALL_TREE/usr --with-readline=$ARM_INSTALL_TREE/usr SPEEX_CFLAGS="-I$ARM_INSTALL_TREE/usr/include" SPEEX_LIBS="-L$ARM_INSTALL_TREE/usr/lib -lspeex"
make 
make install DESTDIR=$M68K_INSTALL_TREE


Binaries can also be stripped with m68k-linux-strip to save more space.


Running linphone under the ColdFire board
********************************************

You just have to start linphone from a terminal by typing 'linphonec'.

Gianluca Salvador

