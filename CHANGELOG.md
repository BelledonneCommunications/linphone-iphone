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
