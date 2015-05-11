# Linphone on iPhone

[![Build Status](https://travis-ci.org/BelledonneCommunications/linphone-iphone.svg?branch=master)](https://travis-ci.org/BelledonneCommunications/linphone-iphone)

## Build prerequisite

Linphone for iPhone depends on liblinphone SDK. This SDK is generated from makefiles and shell scripts.

* Xcode (download from apple or using appstore application)
* [Java SE](http://www.oracle.com/technetwork/java/javase/downloads/index.html) or openJDK
 This is required to generate a C sourcefile from SIP grammar using [antlr3](http://www.antlr3.org/) generator.
* [HomeBrew](http://brew.sh) or [Macports](http://www.macports.org/).


### Install dependencies

* Using HomeBrew:

        brew install autoconf automake pkg-config doxygen nasm gettext wget yasm optipng imagemagick coreutils intltool
        brew link gettext --force
        # antlr3.2 is faster than default homebrew version 3.4 - you can install official antlr3 though
        brew tap Gui13/linphone
        brew install antlr3.2

* Using MacPorts:

        sudo port install autoconf automake pkgconfig doxygen antlr3 nasm gettext wget yasm optipng ImageMagick coreutils intltool

### System linking

* For this part, we assume that `LOCAL_BIN_DIR` is set as following depending on which tool you use:

 For MacPorts: `LOCAL_BIN_DIR=/opt/local/bin`

 For HomeBrew: `LOCAL_BIN_DIR=/usr/local/bin`

* Modify your `PATH` so that the tools are taken in place of the versions brought by Apple in `/usr/bin`. Otherwise the build will fail with obscure errors:

        export PATH=$LOCAL_BIN_DIR:$PATH

* Install [gas-preprocessor.pl](http://github.com/yuvi/gas-preprocessor/) (version above July 2013) into your PATH. Suppose you use `LOCAL_BIN_DIR` directory:

        wget --no-check-certificate https://raw.github.com/yuvi/gas-preprocessor/master/gas-preprocessor.pl
        chmod +x gas-preprocessor.pl
        sudo mv gas-preprocessor.pl $LOCAL_BIN_DIR

* (HomeBrew only) Link `libtoolize` to `glibtoolize`

        sudo ln -s $LOCAL_BIN_DIR/glibtoolize $LOCAL_BIN_DIR/libtoolize

* Link host's `strings` to simulator SDK

        sudo ln -s /usr/bin/strings /Applications/Xcode.app/Contents/Developer/Platforms/iPhoneSimulator.platform/Developer/usr/bin/strings


## BUILDING THE SDK

* GPL third parties versus non GPL third parties

 This SDK can be generated in 2 flavors. First is with GPL third parties, it means liblinphone includes GPL third parties like FFMPEG or X264.
 If you choose this flavor, your final application must comply with GPL in any case. This is the default mode.

 Note: We are not compiling for the 32 bits i386 simulator by default, for speed reasons. If you want to activate it, you should call make with "enable_i386=yes".

 To generate the liblinphone multi arch sdk in GPL mode, do:

        cd submodules/build && make all

 ALTERNATIVELY, you can force liblinphone to use only non GPL code except for liblinphone, mediastreamer2, oRTP, belle-sip.
 If you choose this flavor, your final application  is still subject to GPL except if you have a commercial license for liblinphone, mediastreamer2, oRTP, belle-sip.

 To generate the liblinphone multi arch sdk in non GPL mode, do:

        cd submodules/build && make all enable_gpl_third_parties=no

* For Xcode prior to 4.5, use:

        make -f Makefile.xcode4.4

* ZRTP support

 You can disable ZRTP support with:

        make all enable_zrtp=no

* In case you upgrade your IOS SDK, you may force rebuilding everything, by doing

        make veryclean && make all

**The resulting sdk is in `liblinphone-sdk/` root directory.**

## BUILDING THE APPLICATION

After the SDK is built, just open the Linphone Xcode project with Xcode, and press `Run`.

* Note regarding third party components subject to license:

 The liblinphone-sdk is compiled with third parties code that are subject to patent license, specially: AMR, SILK G729 and H264 codecs.
 Linphone controls the embedding of these codecs thanks to the preprocessor macros HAVE_SILK, HAVE_AMR, HAVE_G729 HAVE_OPENH264 positioned in Xcode project.
 Before embedding these 4 codecs in the final application, make sure to have the right to do so.

## TESTING THE APPLICATION

We are using the KIF framework to test the UI of Linphone. It is used as a submodule (instead of CocoaPods) for ease.

Simply press `Command + U` and the default simulator / device will launch and try to pass all the tests.


## LIMITATIONS, KNOWN BUGS

* Video capture does not work in simulator (not implemented by simulator?).

* Link errors with x86_64: this happens when you try to run linphone on the iPhone 5S/6/6+ simulators.
  This is due to the fact that we're not building the SDK for the x86_64 architecture, due to it being redundant with i386 (and increasing dramatically the compile time and build size).
  The solution (temporary) is to force the acceptable architectures to be 'armv7' only (remove 'arm64') and disable the "build active architecture" flag in the linphone, XMLRPC and NinePatch projects.
  Don't forget to re-enable them when archiving your project.

## DEBUGING THE SDK

Sometime it can be useful to step into liblinphone SDK functions. To allow Xcode to enable breakpoint within liblinphone, SDK must be built with debug symbols.
To add debug symbol to liblinphone SDK, add make option `enable_debug=yes`:

        make all enable_gpl_third_parties=no enable_debug=yes

## DEBUGING MEDIASTREAMER2

For iOS specific media development like audio video capture/playback it may be interesting to use `mediastream` test tool.
The project `submodule/liblinphone.xcodeproj` can be used for this purpose.
