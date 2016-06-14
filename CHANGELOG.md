# Change Log
All notable changes to this project will be documented in this file.

Group changes to describe their impact on the project, as follows:

    Added for new features.
    Changed for changes in existing functionality.
    Deprecated for once-stable features removed in upcoming releases.
    Removed for deprecated features removed in this release.
    Fixed for any bug fixes.
    Security to invite users to upgrade in case of vulnerabilities.

## [Unreleased]

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
- Removed deprecated polarssl submodule, using mbedtls instead

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

[Unreleased]: https://github.com/BelledonneCommunications/linphone-iphone/compare/3.12.1...HEAD
[3.12.1]: http://www.linphone.org/releases/ios/liblinphone-iphone-sdk-3.12.1.zip
[plugins registration]: https://github.com/BelledonneCommunications/linphone-iphone/blob/3.12.1/Classes/LinphoneManager.m#L1461-L1472
[openh264 issue 2434]: https://github.com/cisco/openh264/issues/2434
[Full IPv6 support to comply Apple requirements]: https://developer.apple.com/news/?id=05042016a
