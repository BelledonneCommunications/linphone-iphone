# LINPHONE ON IPHONE

## BUILD PREQUISITES

Linphone for iPhone depends on liblinphone SDK. This SDK is generated from makefiles and shell scripts.

You must first install both Xcode with iPhone OS SDK and [HomeBrew](brew.sh) or [MacPorts](www.macports.org) for these scripts to work.

### Install dependencies

* Using HomeBrew:

```sh
brew install automake intltool libtool pkg-config coreutils yasm nasm wget imagemagick
# then you have to install antlr3 from a tap.
wget https://gist.githubusercontent.com/Gui13/f5cf103f50d34c28c7be/raw/f50242f5e0c3a6d25ed7fca1462bce3a7b738971/antlr3.rb
mv antlr3.rb /usr/local/Library/Formula/
brew install antlr3
```

* Using MacPorts:

```sh
sudo port install coreutils automake autoconf libtool intltool wget pkgconfig cmake gmake yasm nasm grep doxygen ImageMagick optipng antlr3
```

### System linking

* For this part, we assume that `LOCAL_BIN_DIR` is set as following depending on which tool you use:

 For MacPorts: `LOCAL_BIN_DIR=/opt/local/bin`

 For HomeBrew: `LOCAL_BIN_DIR=/usr/local/bin`

* Modify your `PATH` so that the tools are taken in place of the versions brought by Apple in `/usr/bin`. Otherwise the build will fail with obscure errors:

 `export PATH=$LOCAL_BIN_DIR:$PATH`

* Install [gas-preprosessor.pl](http://github.com/yuvi/gas-preprocessor/) (version above July 2013) into your `LOCAL_BIN_DIR` directory

 ```sh
 wget --no-check-certificate https://raw.github.com/yuvi/gas-preprocessor/master/gas-preprocessor.pl
 chmod +x gas-preprocessor.pl
 sudo mv gas-preprocessor.pl $LOCAL_BIN_DIR
 ```

* Link `libtoolize` to `glibtoolize`

 `sudo ln -s $LOCAL_BIN_DIR/glibtoolize $LOCAL_BIN_DIR/libtoolize`

* Link host's `strings` to simulator SDK

 `sudo ln -s /usr/bin/strings /Applications/Xcode.app/Contents/Developer/Platforms/iPhoneSimulator.platform/Developer/usr/bin/strings`


## BUILDING THE SDK

* GPL third parties versus non GPL third parties

 This SDK can be generated in 2 flavors. First is with GPL third parties, it means liblinphone includes GPL third parties like FFMPEG or X264.
 If you choose this flavor, your final application must comply with GPL in any case. This is the default mode.

 To generate the liblinphone multi arch sdk in GPL mode, do:

 `cd submodules/build && make all`

 ALTERNATIVELY, you can force liblinphone to use only non GPL code except for liblinphone, mediastreamer2, oRTP, belle-sip.
 If you choose this flavor, your final application  is still subject to GPL except if you have a commercial license for liblinphone, mediastreamer2, oRTP, belle-sip.

 To generate the liblinphone multi arch sdk in non GPL mode, do:

 `cd submodules/build && make all enable_gpl_third_parties=no`

* For Xcode prior to 4.5, use:

 `make -f Makefile.xcode4.4`

* ZRTP support

 You can disable ZRTP support with:

 `make all enable_zrtp=no`

* In case you upgrade your IOS SDK, you may force rebuilding everything, by doing

 `make veryclean && make all`

**The resulting sdk is in `liblinphone-sdk/` root directory.**

## BUILDING THE APPLICATION

After the SDK is built, just open the Linphone Xcode project with Xcode, and press `Run`.

* Note regarding third party components subject to license:

 The liblinphone-sdk is compiled with third parties code that are subject to patent license, specially: AMR, SILK G729 and H264 codecs.
 Linphone controls the embedding of these codecs thanks to the preprocessor macros HAVE_SILK, HAVE_AMR, HAVE_G729 HAVE_OPENH264 positioned in Xcode project.
 Before embedding these 4 codecs in the final application, make sure to have the right to do so.

## LIMITATIONS, KNOWN BUGS

* Video capture does not work in simulator (not implemented by simulator?).

## DEBUGING THE SDK

Sometime it can be useful to step into liblinphone SDK functions. To allow Xcode to enable breakpoint within liblinphone, SDK must be built with debug symbols.
To add debug symbol to liblinphone SDK, add make option `enable_debug=yes`:

`make all enable_gpl_third_parties=no enable_debug=yes`

## DEBUGING MEDIASTREAMER2

For iOS specific media development like audio video capture/playback it may be interesting to use `mediastream` test tool.
The project `submodule/liblinphone.xcodeproj` can be used for this purpose.
