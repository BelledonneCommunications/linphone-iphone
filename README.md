[![Build Status](https://travis-ci.org/BelledonneCommunications/linphone-iphone.svg?branch=master)](https://travis-ci.org/BelledonneCommunications/linphone-iphone)

# Building the SDK

Linphone for iPhone depends on liblinphone SDK. This SDK is generated from makefiles and shell scripts.

 To generate the liblinphone multi-arch SDK in GPL mode, simply invoke:

        ./prepare.py [options] && make

**The resulting SDK is located in `liblinphone-sdk/` root directory.**

## Licensing: GPL third parties versus non GPL third parties

This SDK can be generated in 2 flavors.

* When choosing using GPL third parties, it means liblinphone includes GPL third parties like FFmpeg or X264. If you choose this flavor, your final application must comply with GPL in any case. This is the default mode.

* When choosing NOT using GPL third parties, Linphone will only use non GPL code except for `liblinphone`, `mediastreamer2`, `oRTP` and `belle-sip`.
 If you choose this flavor, your final application  is still subject to GPL except if you have a commercial license for liblinphone, mediastreamer2, oRTP, belle-sip.
 To generate the liblinphone multi arch SDK in non GPL mode, do:

        ./prepare.py -DENABLE_GPL_THIRD_PARTIES=NO [other options] && make

## Customizing features

You can choose to enable / disable features such as custom audio / video codecs, media encryption, etc. To get a list of all features, the simplest way is to invoke `prepare.py` with `--list-features`:

        ./prepare.py --list-features

You can for instance enable X264 by using:

        ./prepare.py -DENABLE_X264=ON [other options]

## Built architectures

4 architectures currently exists on iOS:

- 64 bits ARM64 for iPhone 5s, iPad Air, iPad mini 2, iPhone 6, iPhone 6 Plus, iPad Air 2, iPad mini 3.
- 32 bits ARMv7 for older devices.
- 64 bits x86_64 for simulator for all ARM64 devices.
- 32 bits i386 for simulator for all ARMv7 older devices.

 Note: We are not compiling for the 32 bits i386 simulator by default because xCode default device (iPhone 6) runs in 64 bits. If you want to enable it, you should invoke `prepare.py` with `i386` argument: `./prepare.py i386 [other options]`.

## Upgrading your iOS SDK

Simply re-invoking `make` should update your SDK. If compilation fails, you may need to rebuilding everything by invoking:

        ./prepare.py -c && ./prepare.py [options] && make

# Building the application

After the SDK is built, just open the Linphone Xcode project with Xcode, and press `Run`.

## Note regarding third party components subject to license

 The liblinphone-sdk is compiled with third parties code that are subject to patent license, specially: AMR, SILK G729 and H264 codecs.
 Linphone controls the embedding of these codecs thanks to the preprocessor macros HAVE_SILK, HAVE_AMR, HAVE_G729 HAVE_OPENH264 positioned in Xcode project.
 Before embedding these 4 codecs in the final application, make sure to have the right to do so.

# Testing the application

We are using the KIF framework to test the UI of Linphone. It is used as a submodule (instead of CocoaPods) for ease.

Simply press `Command + U` and the default simulator / device will launch and try to pass all the tests.


# Limitations and known bugs

* Video capture will not work in simulator (not implemented in simulator).

# Debugging the SDK

Sometime it can be useful to step into liblinphone SDK functions. To allow XCode to enable breakpoint within liblinphone, SDK must be built with debug symbols by using option `--debug`:

        ./prepare.py --debug [other options] && make

## Debugging mediastreamer2

For iOS specific media development like audio video capture/playback it may be interesting to use `mediastream` test tool.
The project `submodule/liblinphone.xcodeproj` can be used for this purpose.

# Quick UI reference

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


When the application is started, the phoneMainView gets asked to transition to the Dialer view or the Wizard view.
PhoneMainView exposes the -changeCurrentView: method, which will setup its
Any Linphone view is actually presented in the UICompositeViewController, with or without a stateBar and tabBar.

The UICompositeViewController consists of 3 areas laid out vertically. From top to bottom: StateBar, Content and TabBar.
The TabBar is usually the UIMainBar, which is used as a navigation controller: clicking on each of the buttons will trigger
a transition to another "view".
