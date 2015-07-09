[![Build Status](https://travis-ci.org/BelledonneCommunications/linphone-iphone.svg?branch=master)](https://travis-ci.org/BelledonneCommunications/linphone-iphone)

# BUILDING THE SDK

Linphone for iPhone depends on liblinphone SDK. This SDK is generated from makefiles and shell scripts.

* GPL third parties versus non GPL third parties

 This SDK can be generated in 2 flavors. First is with GPL third parties, it means liblinphone includes GPL third parties like FFMPEG or X264.
 If you choose this flavor, your final application must comply with GPL in any case. This is the default mode.

 Note: We are not compiling for the 32 bits i386 simulator by default because since iPhone 6 / iOS 8 simulators run in 64 bits. If you want to activate it, you should call prepare.py "i386" architecture.

 To generate the liblinphone multi arch SDK in GPL mode, do:

        ./prepare.py && make

 ALTERNATIVELY, you can force liblinphone to use only non GPL code except for liblinphone, mediastreamer2, oRTP, belle-sip.
 If you choose this flavor, your final application  is still subject to GPL except if you have a commercial license for liblinphone, mediastreamer2, oRTP, belle-sip.

 To generate the liblinphone multi arch SDK in non GPL mode, do:

        ./prepare.py -DENABLE_GPL_THIRD_PARTIES=NO && make

* In case you upgrade your IOS SDK, you may force rebuilding everything, by doing

        ./prepare.py -c && ./prepare.py && make

**The resulting sdk is in `liblinphone-sdk/` root directory.**

# BUILDING THE APPLICATION

After the SDK is built, just open the Linphone Xcode project with Xcode, and press `Run`.

* Note regarding third party components subject to license:

 The liblinphone-sdk is compiled with third parties code that are subject to patent license, specially: AMR, SILK G729 and H264 codecs.
 Linphone controls the embedding of these codecs thanks to the preprocessor macros HAVE_SILK, HAVE_AMR, HAVE_G729 HAVE_OPENH264 positioned in Xcode project.
 Before embedding these 4 codecs in the final application, make sure to have the right to do so.

# TESTING THE APPLICATION

We are using the KIF framework to test the UI of Linphone. It is used as a submodule (instead of CocoaPods) for ease.

Simply press `Command + U` and the default simulator / device will launch and try to pass all the tests.


# LIMITATIONS, KNOWN BUGS

* Video capture does not work in simulator (not implemented by simulator?).

# DEBUGING THE SDK

Sometime it can be useful to step into liblinphone SDK functions. To allow Xcode to enable breakpoint within liblinphone, SDK must be built with debug symbols.
To add debug symbol to liblinphone SDK, use:

        ./prepare.py -d && make

# DEBUGING MEDIASTREAMER2

For iOS specific media development like audio video capture/playback it may be interesting to use `mediastream` test tool.
The project `submodule/liblinphone.xcodeproj` can be used for this purpose.

# Quick UI reference for Linphone iOS:

- The app is contained in a window, which resides in the MainStoryboard file.
- The delegate is set to LinphoneAppDelegate in main.m, in the UIApplicationMain() by passing its class
- Basic layout:

MainStoryboard
        |
        | (rootViewController)
        |
    PhoneMainView ---> view #--> app background
        |                   |
        |                   #--> statusbar background
        |
        | (mainViewController)
        |
    UICompositeViewController : TPMultilayout
                |
                #---> view  #--> stateBar
                            |
                            #--> contentView
                            |
                            #--> tabBar


When the app is started, the phoneMainView gets asked to transition to the Dialer view or the Wizard view.
PhoneMainView exposes the -changeCurrentView: method, which will setup its
Any Linphone view is actually presented in the UICompositeViewController, with or without a stateBar and tabBar.

The UICompositeViewController consists of 3 areas laid out vertically. From top to bottom: StateBar, Content and TabBar.
The TabBar is usually the UIMainBar, which is used as a navigation controller: clicking on each of the buttons will trigger
a transition to another "view".
