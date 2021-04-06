# Change Log
All notable changes to this project will be documented in this file.

Group changes to describe their impact on the project, as follows:

    Added for new features.
    Changed for changes in existing functionality.
    Deprecated for once-stable features removed in upcoming releases.
    Removed for deprecated features removed in this release.
    Fixed for any bug fixes.
    Security to invite users to upgrade in case of vulnerabilities.


## [4.4.0] - 2021-03-30

### Added
- Option to store chat files in Gallery.

### Changed
- Updating SDK to 4.5 version
- Asking user to read and accept privacy policy and general terms
- Updated translations
- Store chat files in App Groups instead of Gallery.

### Removed
- Location permission request.

### Fixed
- Various crashs and issues.


## [4.3.0] - 2020-06-23

### Added
- "push notification application extension" to manage message reception.
- Dark Mode.
- CI to test the build and archive the application.

### Changed
- Presenting the callkit view upon receipt of the push notification.
- Using PushKit only for calls.
- Moving db files from app containers to App Groups.
- Updating SDK to 4.4 version
- Some files are written in Swift.

### Fixed
- Various crashs and issues.

## [4.2.0] - 2019-12-09

### Added
- Possiblity to enable Media Encryption Mandatory  in Settings.
- Possiblity to not show app's calls in iphone's history
- Using new AAudio & Camera2 frameworks for better performances (if available)
- Asking the user to agree to access location information from ios 13.

### Changed
- Improved performances to reduce startup time.
- Added our own devices in LIME encrypted chatrooms' security view.
- License changed from GPLv2 to GPLv3.
- Switched from MD5 to SHA-256 as password protection algorithm.
- Updated translations, mainly French and English.
- Disable bitcode by Xcode.
- Modify some views for iphone, XR and Xs.

### Fixed
- Automatically downloaded images are copied when shared in a chat room.
- Some UI errors from ios 13.
    
## [4.1.0] - 2019-05-06

### Added
- End-to-end encryption for instant messaging, for both one-to-one and group conversations.
- Video H.265 codec support, based on iOS VideoToolbox framework.
- Enhanced call and IM notifications, so that it is possible to answer, decline, reply or mark as read directly from them.
- Setting to request attachments to be automatically downloaded, unconditionnally or based on their size.
- Possibility to send multiple attachments (images, documents) in a same message.
- Possibility to open all kinds of documents received in a conversation.
- Possibility to share an image through Linphone from an external application (ex: photo app)
- Button to invite contacts to use Linphone by sending them a SMS.
- Possibility to record calls (audio only), and replay them from the "Recordings" menu.
- Remote provisioning from a QR code providing the http(s) url of a provisioning server.
- Optional Crashlythics support.

### Changed
- Compilation procedure is simplified: a binary SDK containing dependencies (liblinphone) is retrieved automatically from a CocoaPods repository.
  Full compilation remains absolutely supported. Please check local README.md for more details.
- Updated translations, mainly French and English.
- Use of Photokit instead of Asset Library for image handling.
- Auto-layout of images in chat messages.
- Use Xcode test navigator for tests.
- Move important files from `Documents` folder to `Application Library`. 

### Fixed
- Issues around Bluetooth devices management.
- Loss of audio after accepting a second call while already in a call.
- Crashes when during calls.
- Nowebcam when leaving conference.

### Removed
- Static build of iOS linphone SDK.
- All git submodules previously containing dependencies.
- Some resource files now provided by linphone-sdk.

## [4.0.2] - 2018-10-15

### Fixed
- fix IOS12 crash
- fix bluethooth issue with some cars
- fix nat helper (sdk)

## [4.0.1] - 2018-06-26

### Fixed
- Fix TURN
- Start video stream on first call
- Fix audio unit management in case of call time out
- Fix registration issue with some SIP services (ie: Asterix)

## [4.0] - 2018-06-11

### Added
- Supports of group chat
- New address search algorithm

### Fixed
- Minor bugs fixes

## [3.16.5] - 2017-11-28

### Added
- Support of IOS 11
- new algorithm to adapt audio and video codec bitrates to the available bandwidth (https://wiki.linphone.org/xwiki/wiki/public/view/FAQ/How%20does%20adaptive%20bitrate%20algorithm%20work%20%3F/)

### Changed
- Contact, CNContact implmentation.
- Contacts loading optimization.
- Sound management updated

### Fixed
- Chat file resend fixed
- Minor bugs fixes
- Audio fixed on conference call

## [3.16.3] - 2017-05-03

### Added
- Imdm, chat message reception/lecture notification.

### Changed
- Optimization of Chat list

### Fixed
- Minor bugs fixes
- Crashes on Call cancel too soon

## [3.16.2] - 2017-03-01

### Added
- Link to GPLv2 licence and Linphone privacy policy in About View.

### Changed
- Optimization of Contact Lists

### Fixed
- CallKit bugs when invalid SIP address
- CallKit error screens no longer displayed but ours
- Crashes in Contact Lists
- Presence supports network changes
- Uses of linked address instead of phone number in chat rooms
- Uses of display name instead of sip addresses in chat rooms and history lists

## [3.16.1] - 2017-09-01

### Added
- Support of CallKit
- Support of background task to finish sending messages and files when app is in background

### Fixed
- Freeze of UI when cancelling a swipe to delete too quickly

## [3.15] - 2016-11-09

### Added
- Support of iOS 10
- Support of PushKit (VoIP push notifications)
- Added long term presence for linphone.org accounts: any user can now see his/her friends with a linphone.org account
- Added TURN support
- Change your password in your account settings

### Changed
- Updated push notification sound
- Updated assistant to allow creating and authenticating account with a phone number
- Updated translations
- Improve VideoToolbox H264 decoder

### Removed
- None VoIP push notifications

### Fixed
- Correctly display name in Settings when using exotic characters
- Correctly handle video policy when answering from push notification
- Hide keyboard on dialer when address is empty
- Better handling of multi accounts in side menu
- Handle of notification actions
- Rotation of camera view

## [3.13.9] - 2016-06-15

### Added
- Added "Forgot your password?" link in Linphone account assistant
- [Full IPv6 support to comply Apple requirements]
- Hardware accelerated H264 codec
- Full video HD support for newest devices (iPhone 6, iPhone SE, etc.)

### Changed
- Enable Neon intrinsics optimizations for speex resampler (ENABLE_ARM_NEON_INTRINSICS)
- Push notifications are now configurable per account
- Update to latest OpenH264 version to fix issue with Xcode 7.3 and arm64 devices [openh264 issue 2434]
- Default transport reset to "UDP" for external accounts in assistant since most providers only support that
- Remove deprecated polarssl submodule, using mbedtls instead

### Fixed
- Fix invalid photo rotation when using Camera for avatars
- Fix self avatar save when using camera
- Parse user input as SIP address or phone number depending on default account settings: if "substitute + by country code" is set,
consider inputs to be phone numbers, otherwise SIP addresses.
- Automatically start call when answering from within notification in iOS9+
- Contact details view is now scrollable to fix issue on small screens
- Unregister accounts in case of application shutdown when remote push notifications are not enabled
- Reregister accounts in case of WiFi change

## [3.12.1] - 2016-02-19

### Changed
- New About view
- [plugins registration] procedure has been updated
- iLBC has been removed - we are now using webrtc implementation instead, which is built by default. Removed libilbc.a from XCode project

### Fixed
- reload chat view on iPad on changes
- remove “invalid length” error in assistant
- remove comma from user-agent
- properly display numpad in call
- update application badge count when answering within notification

## 3.0 - 2016-01-06

### Added
- Multi account support

### Changed
- New flat design rebranding

## 0.7.6 - 2013-03-04

### Added
- Initial version

[Unreleased]: https://github.com/BelledonneCommunications/linphone-iphone/compare/3.16.2...HEAD
[3.16.2]: http://www.linphone.org/releases/ios/liblinphone-iphone-sdk-3.16.2.zip
[3.16.1]: http://www.linphone.org/releases/ios/liblinphone-iphone-sdk-3.16.1.zip
[plugins registration]: https://github.com/BelledonneCommunications/linphone-iphone/blob/3.12.1/Classes/LinphoneManager.m#L1461-L1472
[openh264 issue 2434]: https://github.com/cisco/openh264/issues/2434
[Full IPv6 support to comply Apple requirements]: https://developer.apple.com/news/?id=05042016a
