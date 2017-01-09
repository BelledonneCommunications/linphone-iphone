# Change Log
All notable changes to this project will be documented in this file.

Group changes to describe their impact on the project, as follows:

    Added for new features.
    Changed for changes in existing functionality.
    Deprecated for once-stable features removed in upcoming releases.é
    Removed for deprecated features removed in this release.
    Fixed for any bug fixes.
    Security to invite users to upgrade in case of vulnerabilities.

## [Unreleased]

## [3.16] - 2017-09-01

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

[Unreleased]: https://github.com/BelledonneCommunications/linphone-iphone/compare/3.15...HEAD
[3.15]: http://www.linphone.org/releases/ios/liblinphone-iphone-sdk-3.14.12.zip
[3.13.9]: http://www.linphone.org/releases/ios/liblinphone-iphone-sdk-3.13.9.zip
[plugins registration]: https://github.com/BelledonneCommunications/linphone-iphone/blob/3.12.1/Classes/LinphoneManager.m#L1461-L1472
[openh264 issue 2434]: https://github.com/cisco/openh264/issues/2434
[Full IPv6 support to comply Apple requirements]: https://developer.apple.com/news/?id=05042016a
