[![Build Status](https://travis-ci.org/BelledonneCommunications/linphone-iphone.svg?branch=master)](https://travis-ci.org/BelledonneCommunications/linphone-iphone)

Linphone is a free VoIP and video softphone based on the SIP protocol.

![Dialer screenshot](http://www.linphone.org/img/slideshow-phone.png)

# How can I contribute?

Thanks for asking! We love pull requests from everyone. Depending on what you want to do, you can help us improve Linphone in
various ways:

## Help on translations

<a target="_blank" style="text-decoration:none; color:black; font-size:66%" href="https://www.transifex.com/belledonne-communications/linphone-ios/"
title="See more information on Transifex.com">Top translations: linphone-ios</a><br/>
<img border="0" src="https://transifex.com/projects/p/linphone-ios/resource/localizablestrings/chart/image_png"/><br/><a target="_blank" href="/"><img border="0" src="https://ds0k0en9abmn1.cloudfront.net/static/charts/images/tx-logo-micro.646b0065fce6.png"/></a>

Interested in helping translate Linphone? Contribute [on Transifex](https://www.transifex.com/belledonne-communications/linphone-ios).

## Report bugs and submit patchs

If you want to dig through Linphone code or report a bug, please read `CONTRIBUTING.md` first. You should also read this `README` entirely ;-).

## How to be a beta tester ?

Enter the Beta :
- Download TestFlight from the App Store and log in it with your apple-id
-Tap the public link on your iOS device. The public link : https://testflight.apple.com/join/LUlmZWjH
-Touch View in TestFlight or Start Testing. You can also touch Accept, Install, or Update for Linphone app.
-And voilà ! You can update your beta version with the same public link when a new one is available

Send a crash report :
 - It is done automatically by TestFlight

Report a bug :
 - Open Linphone
 - Go to Settings —> Advanced —> Send logs
 - An email to linphone-iphone@belledonne-communications.com is created with your logs attached
 - Fill in the bug description with :
	* What you were doing
	* What happened
	* What you were expecting
	* Approximately when the bug happened
 - Change the object to [Beta test - Bug report]
 - Send the mail

# Building the application

## What's new

Now the default way of building linphone-iphone is to use CocoaPods. 
Compared to previous versions, this project no longer uses submodules developper has to build in order to get a working app.
However, if you wish to use a locally compiled SDK see below how to proceed.

## Building the app

If you don't have CocoaPods, you can download it using :
- sudo gem install cocoapods

Build the linphone-iphone :
- pod install
It will download the linphone library from our gitlab repository so you don't have to build anything yourelf.

Then open linphone.xcworkspace (instead of linphone.xcodeproj) to install the app.

# Testing the application

We are using the Xcode test navigator to test the UI of Linphone.

Change the Scheme to LinphoneTester. Press the test navigator button and all the tests will show.
See: https://developer.apple.com/library/archive/documentation/DeveloperTools/Conceptual/testing_with_xcode/chapters/05-running_tests.html

# Limitations and known bugs

* Video capture will not work in simulator (not implemented in it).

# Debugging the SDK

## Building a local SDK

-Clone the linphone-sdk repository from out gitlab:
  * git clone https://gitlab.linphone.org/BC/public/linphone-sdk.git --recursive

-Follow the instructions in the linphone-sdk/README file to build the SDK.

-Rebuild the project:
 * PODFILE_PATH=<path to linphone-sdk-ios> pod install
 
-Then open linphone.xcworkspace (instead of linphone.xcodeproj) to install the app.

# Use crashlythics

We've integrated the crashlythics into liphone-iphone, which can automatically send us a crash report. It is disabled by default.
To activate it :

-Download GoogleService-Info.plist from :
 https://console.firebase.google.com/project/linphone-iphone/settings/general/ios:org.linphone.phone
 You may not have access to this website because it is restricted to certain developers.

-Replace GoogleService-Info.plist for this project with the file you downloaded.

-Rebuild the project:
* USE_CRASHLYTHICS=true pod install

-Then open linphone.xcworkspace (instead of linphone.xcodeproj) to install the app.

# Quick UI reference

- The app is contained in a window, which resides in the MainStoryboard file.
- The delegate is set to LinphoneAppDelegate in main.m, in the UIApplicationMain() by passing its class
- Basic layout:

        MainStoryboard
                |
                | (rootViewController)
                |
            PhoneMainView ---> view |--> app background
                |                   |
                |                   |--> statusbar background
                |
                | (mainViewController)
                |
            UICompositeView : TPMultilayout
                        |
                        |---> view  |--> statusBar
                                    |
                                    |--> contentView
                                    |
                                    |--> tabBar


When the application is started, the phoneMainView gets asked to transition to the Dialer view or the Assistant view.
PhoneMainView exposes the -changeCurrentView: method, which will setup its
Any Linphone view is actually presented in the UICompositeView, with or without a statusBar and tabBar.

The UICompositeView consists of 3 areas laid out vertically. From top to bottom: StatusBar, Content and TabBar.
The TabBar is usually the UIMainBar, which is used as a navigation controller: clicking on each of the buttons will trigger
a transition to another "view".
