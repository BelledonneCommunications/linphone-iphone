# Change Log
All notable changes to this project will be documented in this file.

Group changes to describe their impact on the project, as follows:

	Added for new features.
	Changed for changes in existing functionality.
	Deprecated for once-stable features removed in upcoming releases.
	Removed for deprecated features removed in this release.
	Fixed for any bug fixes.
	Security to invite users to upgrade in case of vulnerabilities.
	
	
## [6.1.0] - 2025-11-13

### Added
- LDAP and CardDAV settings
- Advanced settings in third-party SIP account login view
- Phone number calls in contact details

### Changed
- Launch Screen (Splash Screen)
- Updated translations from Weblate
- Updated SPM dependencies
- Disabled meetings view when audio/video conference factory address is missing
- Moved disable_chat_feature to UI section
- Updated configuration files

### Fixed
- International prefix reset in settings
- Prevent editing of read-only (LDAP) contacts
- Crash when editing a contact (safe unwrapping of friend/photo)
- EditContactFragment view and “+” allowed in dialer
- Dial plan selector and default dial plan
- Encryption update when call state changes


## [6.0.2] - 2025-09-26

### Added
- Advanced settings to third-party SIP account login view
- Burger button to open the side menu

### Changed
- Layout icon in conference call
- Translations from Weblate
- Disable meetings view when audio/video conference factory address is missing

### Fixed
- EditContactFragment view and allow '+' in number dialer
- Dial plan selector and dial plan default
- Crash when editing a contact by safely unwrapping friend/photo
- Meeting scheduler


## [6.0.1] - 2025-09-12

### Added
- Done button toolbar to number pads
- Help view to login page

### Changed
- textToImage updated to generate image on the core queue
- Send DTMF execution moved to the core queue
- Use saveImage on core queue
- Use point_to_point string for encrypted calls in conference
- Hide VFS setting

### Fixed
- Avatar photo refresh
- onEphemeralMessageTimerStarted callback
- Crash in updateEncryption by safely handling optional currentCall
- Sorted list in MagicSearch when friend is nil
- Friend list refresh triggered by onPresenceReceived
- Crash when adding or removing SIP addresses and phone numbers in EditContactFragment
- awaitDataWrite execution on main queue
- Crash by copying Friend addresses and phone numbers before removal
- Ensure core is On before stopping it on background entry
- textToImage crash


## [6.0.0] - 2025-09-01

6.0.0 release is a complete rework of Linphone, with a fully redesigned UI, so it is impossible to list everything here.

### Changed
- Separated threads: Contrary to previous versions, our SDK is now running in it's own thread, meaning it won't freeze the UI anymore in case of heavy work.
- Asymmetrical video : you no longer need to send your own camera feed to receive the one from the remote end of the call, and vice versa.
- Improved multi account: you'll only see history, conversations, meetings etc... related to currently selected account, and you can switch the default account in two clicks.
- User can only send up to 12 files in a single chat message.
- IMDNs are now only sent to the message sender, preventing huge traffic in large groups, and thus the delivery status icon for received messages is now hidden in groups (as it was in 1-1 conversations).
- Settings: a lot of them are gone, the one that are still there have been reworked to increase user friendliness.
- Default screen (between contacts, call history, conversations & meetings list) will change depending on where you were when the app was paused or killed, and you will return to that last visited screen on the next startup.
- Account creation no longer allows you to use your phone number as username, but it is still required to provide it to receive activation code by SMS.
- Minimum supported iOS version is now 15.
- Some settings have changed name and/or section in linphonerc file.

### Added
- You can now react to a chat message using any emoji.
- Chat while in call: a shortcut to a conversation screen with the remote.
- Chat while in a conference: if the conference has a text stream enabled, you can chat with the other participants of the conference while it lasts. At the end, you'll find the messages history in the call history (and not in the list of conversations).
- Security focus: security & trust is more visible than ever, and unsecure conversations & calls are even more visible than before.
- OpenID: when used with a SSO compliant SIP server (such as Flexisip), we support single-sign-on login.
- MWI support: display and allow to call your voicemail when you have new messages (if supported by your VoIP provider and properly configured in your account params).
- CCMP support: if you configure a CCMP server URL in your accounts params, it will be used when scheduling meetings & to fetch list of meetings you've organized/been invited to.
- Devices list: check on which device your sip.linphone.org account is connected and the last connection date & time (like on subscribe.linphone.org).
- Dialer & in-call numpad show letters under the digit.

### Removed
- Dialer: the previous home screen (dialer) has been removed, you'll find it as an input option in the new start call screen.
- Peer-to-peer: a SIP account (sip.linphone.org or other) is now required.


## [5.2.0] - 2023-28-12
### Added
- Added extra Czech and Japanese translations

### Changed
- Update linphone SDK to 5.3.4

### Fixed
- Re-enabled the G729 audio codec
	
## [5.2.0] - 2023-21-12
### Added
- Chat messages emoji "reactions"
- Hardware video codecs (H264, H265) are now used in priority when possible (SDK)

### Changed
- Minimum iOS version is now 13
- Update linphone SDK to 5.3.1

### Fixed
- Several crashes in chat conversation when receiving files
- Various UI fixes in conference views
- Fix crash upon refreshing register

## [5.1.0] - 2023-21-08
### Added
- In contacts and chat conversations view, show short term presence for contacts whom publish it + added setting to disable it (enabled by default for sip.linphone.org accounts)
- Advanced settings - option to prevent the taking of screenshot
- Emoji picker in chat conversations
- Add Organization label to contacts, and the possibility to filter through it
- Possibility to make an attended transfer from one call to another
- Contact names, phone numbers and sip addresses are now copyable through with long press action

### Changed
- Switched Account Creator backend from XMLRPC to FlexiAPI, it now requires to be able to receive a push notification
- Chat conversation view (one-to-one and group) completely remade with Swift, with various quality of life improvements.
- Minimum iOS version is now 11.2
- Update linphone SDK to 5.2.95

### Fixed
- Several crashes in the chat conversation view and background mode
- Url scheme handler : can now properly do a remote configuration when opening a linphone-config:URL from another app
- Bug that could cause push notification to stop working after killing the app manually

## [5.0.2] - 2023-16-03

### Changed
- Update linphone SDK to 5.2.32

### Fixed
- Performance issue causing a global slowing of the app, especially at launch
- Fix several memory leaks and crashes

## [5.0.1] - 2023-10-01

### Changed
- Update linphone SDK to 5.2.11

### Fixed
- Makes sure sip.linphone.org accounts have a LIME X3DH server URL for E2E chat messages encryption
- Fix potential crash when displaying images received in a chatroom
- Fix bug that would cause the previous call to be terminated when resuming another call that was paused
- Fix participant video display in conferences when a second participant joined with video enabled

## [5.0.0] - 2022-12-06

### Added
- Post Quantum encryption when using ZRTP
- Conference creation with scheduling, video, different layouts, showing who is speaking and who is muted, etc...
- Group calls directly from group chat rooms
- Chat rooms can be individually muted (no notification when receiving a chat message)
- Outgoing call video in early-media if requested by callee
- Call recordings can be exported
- Setting to prevent international prefix from account to be applied to call & chat
- Add a "Never ask again" option to the "Link my account" pop-up when starting the app

### Changed
- In-call views have been re-designed
- Improved how contact avatars are generated
- 3-dots menu even for basic chat rooms with more options
- Update linphone SDK to 5.2.0

### Fixed
- Chatroom appearing as empty when being logged on multiple accounts
- Chatroom appearing as empty after playing a video file inside it
- Fix potential crash when entering a chatroom
- Fix potential crash when accessing to the delivery infos of a message in a group chat.
- IMDN logo not properly displayed when transfering or replying to a message with media (voice message, photo...)
- Clarified view when sending an image from the galery
- Various audio route fixes for CallKit and IOS 16
		
## [4.6.4] - 2022-08-06
### Changed
- Update linphone SDK to 5.1.42

### Fixed
- Prevent possible application freeze and crash when creating a new chatroom, depending on the phone's contacts.
	
## [4.6.3] - 2022-02-06

### Added
- New "Contacts" menu in the settings, which allows the use of LDAP configurations
- Using new MagicSearch API to improve contacts list performances, and search contacts using LDAP if appropriate

### Changed
- Update linphone SDK to 5.1.41

### Fixed
- Prevent read-only 1-1 chat room
- Small quality of life fixes for voice recording messages
- Display bug when changing audio device

	
## [4.6.2] - 2022-07-03

### Fixed
- Bug preventing the activation of the phone speaker during calls
- Bug with "reply" feature in chatrooms
- Bug causing IMDNs to be missing in some chatrooms
- Update linphone SDK to 5.1.7
	
## [4.6.1] - 2022-04-03

### Fixed
- Crash in chatroom info view after entering background and re-entering foreground
- Crash in local call conferences when pausing/resuming
- Hard to see text (written in black) on dark mode
- Removed duplicate push authorization request pop up on install

## [4.6.0] - 2022-31-02

### Added
- Reply to chat message feature (with original message preview)
- Transfert chat message feature
- Swipe action on chat messages to reply / delete
- Voice recordings in chat feature
- SIP URIs in chat messages are clickable to easily initiate a call
- New fragment explaining generic SIP account limitations contrary to sip.linphone.org SIP accounts
- Link to Weblate added in about page
- New 'scroll to bottom' button in chat conversations, which a "unread message count" badge

### Changed
- Removed beta feature of ephemeral messages in the settings, now always available.
- SDK updated to 5.1.0 release

### Fixed
- Potential crash when editing a contact avatar image.
- App extension logs missing when exporting logs

## [4.5.0] - 2021-07-08

### Added
- Add option to enable VFS
- Ephemeral messages (beta)

### Changed
- Updating SDK to 5.0 version
- Using linphone SDK 5.0 API to better handle audio route 
- Replaced all notions of "Proxy configs" with "Accounts" from the 5.0 SDK
- Removed most of the code related to remote and VOIP Push Notification receptions, now handled in the SDK
- No longer pause all calls when receiving a new call.
- No longer switch to speaker during video call if another output device (bluetooth headset) is already connected
- When answering a video call while the phone is locked, send the "No camera available" image until the video is enabled through the CallKit button
- Chat messages containing both text and file are now displayed in the same chat bubble

### Fixed
- Fix several memory leaks
- Various crashs and issues.
- When the App is started through a Push Notification, properly redirect the view to the corresponding chat rather than going to the home page

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
