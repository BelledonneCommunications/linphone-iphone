/*
 * Copyright (c) 2010-2020 Belledonne Communications SARL.
 *
 * This file is part of linphone-iphone 
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 */

#import <Foundation/Foundation.h>
#import <SystemConfiguration/SCNetworkReachability.h>
#import <AudioToolbox/AudioToolbox.h>
#import <Photos/Photos.h>
#import <CoreTelephony/CTCallCenter.h>

#import <sqlite3.h>

#import "IASKSettingsReader.h"
#import "IASKSettingsStore.h"
#import "IASKAppSettingsViewController.h"
#import "FastAddressBook.h"
#import "InAppProductsManager.h"

#include "linphone/linphonecore.h"
#include "bctoolbox/list.h"
#import "OrderedDictionary.h"

#import "linphoneapp-Swift.h"

extern NSString *const LINPHONERC_APPLICATION_KEY;

extern NSString *const kLinphoneCoreUpdate;
extern NSString *const kLinphoneDisplayStatusUpdate;
extern NSString *const kLinphoneMessageReceived;
extern NSString *const kLinphoneTextComposeEvent;
extern NSString *const kLinphoneCallUpdate;
extern NSString *const kLinphoneRegistrationUpdate;
extern NSString *const kLinphoneMainViewChange;
extern NSString *const kLinphoneAddressBookUpdate;
extern NSString *const kLinphoneLogsUpdate;
extern NSString *const kLinphoneSettingsUpdate;
extern NSString *const kLinphoneBluetoothAvailabilityUpdate;
extern NSString *const kLinphoneConfiguringStateUpdate;
extern NSString *const kLinphoneGlobalStateUpdate;
extern NSString *const kLinphoneNotifyReceived;
extern NSString *const kLinphoneNotifyPresenceReceivedForUriOrTel;
extern NSString *const kLinphoneCallEncryptionChanged;
extern NSString *const kLinphoneFileTransferSendUpdate;
extern NSString *const kLinphoneFileTransferRecvUpdate;
extern NSString *const kLinphoneQRCodeFound;
extern NSString *const kLinphoneChatCreateViewChange;

extern NSString *const kLinphoneMsgNotificationAppGroupId;

typedef enum _NetworkType {
    network_none = 0,
    network_2g,
    network_3g,
    network_4g,
    network_lte,
    network_wifi
} NetworkType;

extern const int kLinphoneAudioVbrCodecDefaultBitrate;

/* Application specific call context */
typedef struct _CallContext {
    LinphoneCall* call;
    bool_t cameraIsEnabled;
} CallContext;

typedef struct _LinphoneManagerSounds {
    SystemSoundID vibrate;
} LinphoneManagerSounds;

@interface LinphoneManager : NSObject {
@protected
	SCNetworkReachabilityRef proxyReachability;

@private
	NSTimer* mIterateTimer;
        NSMutableArray*  pushCallIDs;

	UIBackgroundTaskIdentifier pausedCallBgTask;
	UIBackgroundTaskIdentifier incallBgTask;
	UIBackgroundTaskIdentifier pushBgTaskRefer;
	UIBackgroundTaskIdentifier pushBgTaskCall;
	UIBackgroundTaskIdentifier pushBgTaskMsg;
	CTCallCenter* mCallCenter;
    NSDate *mLastKeepAliveDate;
@public
    CallContext currentCallContextBeforeGoingBackground;
}
+ (LinphoneManager*)instance;
#ifdef DEBUG
+ (void)instanceRelease;
#endif
+ (LinphoneCore*) getLc;
+ (BOOL)isLcInitialized;
+ (BOOL)runningOnIpad;
+ (BOOL)isNotIphone3G;
+ (NSString *)getPreferenceForCodec: (const char*) name withRate: (int) rate;
+ (BOOL)isCodecSupported: (const char*)codecName;
+ (NSSet *)unsupportedCodecs;
+ (NSString *)getUserAgent;
+ (int)unreadMessageCount;

- (void)playMessageSound;
- (void)resetLinphoneCore;
- (void)launchLinphoneCore;
- (void)destroyLinphoneCore;
- (void)startLinphoneCore;
- (BOOL)resignActive;
- (void)becomeActive;
- (BOOL)enterBackgroundMode;
- (void)addPushCallId:(NSString*) callid;
- (void)configurePushTokenForProxyConfigs;
- (void)configurePushTokenForProxyConfig: (LinphoneProxyConfig*)cfg;
- (BOOL)popPushCallID:(NSString*) callId;
- (void)acceptCallForCallId:(NSString*)callid;
+ (BOOL)langageDirectionIsRTL;

- (void)refreshRegisters;

- (void)configureVbrCodecs;

+ (BOOL)copyFile:(NSString*)src destination:(NSString*)dst override:(BOOL)override ignore:(BOOL)ignore;
+ (PHFetchResult *)getPHAssets:(NSString *)key;
+ (NSString*)bundleFile:(NSString*)file;
+ (NSString *)preferenceFile:(NSString *)file;
+ (NSString *)documentFile:(NSString *)file;
+ (NSString*)dataFile:(NSString*)file;
+ (NSString*)cacheDirectory;
// migration
+ (NSString *)oldPreferenceFile:(NSString *)file;
+ (NSString *)oldDataFile:(NSString *)file;

- (void)send:(NSString *)replyText toChatRoom:(LinphoneChatRoom *)room;
- (void)call:(const LinphoneAddress *)address;

+(id)getMessageAppDataForKey:(NSString*)key inMessage:(LinphoneChatMessage*)msg;
+(void)setValueInMessageAppData:(id)value forKey:(NSString*)key inMessage:(LinphoneChatMessage*)msg;

- (void)lpConfigSetString:(NSString*)value forKey:(NSString*)key;
- (void)lpConfigSetString:(NSString *)value forKey:(NSString *)key inSection:(NSString *)section;
- (NSString *)lpConfigStringForKey:(NSString *)key;
- (NSString *)lpConfigStringForKey:(NSString *)key inSection:(NSString *)section;
- (NSString *)lpConfigStringForKey:(NSString *)key withDefault:(NSString *)value;
- (NSString *)lpConfigStringForKey:(NSString *)key inSection:(NSString *)section withDefault:(NSString *)value;

- (void)lpConfigSetInt:(int)value forKey:(NSString *)key;
- (void)lpConfigSetInt:(int)value forKey:(NSString *)key inSection:(NSString *)section;
- (int)lpConfigIntForKey:(NSString *)key;
- (int)lpConfigIntForKey:(NSString *)key inSection:(NSString *)section;
- (int)lpConfigIntForKey:(NSString *)key withDefault:(int)value;
- (int)lpConfigIntForKey:(NSString *)key inSection:(NSString *)section withDefault:(int)value;

- (void)lpConfigSetBool:(BOOL)value forKey:(NSString*)key;
- (void)lpConfigSetBool:(BOOL)value forKey:(NSString *)key inSection:(NSString *)section;
- (BOOL)lpConfigBoolForKey:(NSString *)key;
- (BOOL)lpConfigBoolForKey:(NSString *)key inSection:(NSString *)section;
- (BOOL)lpConfigBoolForKey:(NSString *)key withDefault:(BOOL)value;
- (BOOL)lpConfigBoolForKey:(NSString *)key inSection:(NSString *)section withDefault:(BOOL)value;

- (void)silentPushFailed:(NSTimer*)timer;

- (void)removeAllAccounts;

+ (BOOL)isMyself:(const LinphoneAddress *)addr;

- (void)shouldPresentLinkPopup;

- (void) setLinphoneManagerAddressBookMap:(OrderedDictionary*) addressBook;
- (OrderedDictionary*) getLinphoneManagerAddressBookMap;

- (void) setContactsUpdated:(BOOL) updated;
- (BOOL) getContactsUpdated;

- (void)checkNewVersion;

- (void)loadAvatar;
- (void)migrationPerAccount;

- (void)setupGSMInteraction;
- (void)setBluetoothEnabled:(BOOL)enable;
- (BOOL)isCTCallCenterExist;

@property (readonly) BOOL isTesting;
@property(readonly, strong) FastAddressBook *fastAddressBook;
@property (readonly) NetworkType network;
@property (readonly) const char*  frontCamId;
@property (readonly) const char*  backCamId;
@property(strong, nonatomic) NSString *SSID;
@property (readonly) sqlite3* database;
@property(nonatomic, strong) NSData *pushKitToken;
@property(nonatomic, strong) NSData *remoteNotificationToken;
@property (readonly) LinphoneManagerSounds sounds;
@property (readonly) NSMutableArray *logs;
@property (nonatomic, assign) BOOL bluetoothAvailable;
@property (readonly) NSString* contactSipField;
@property (readonly,copy) NSString* contactFilter;
@property (copy) void (^silentPushCompletion)(UIBackgroundFetchResult);
@property (readonly) BOOL wasRemoteProvisioned;
@property (readonly) LpConfig *configDb;
@property(readonly) InAppProductsManager *iapManager;
@property(strong, nonatomic) NSMutableArray *fileTransferDelegates;
@property BOOL conf;
@property NSDictionary *pushDict;
@property(strong, nonatomic) OrderedDictionary *linphoneManagerAddressBookMap;
@property (nonatomic, assign) BOOL contactsUpdated;
@property (nonatomic, assign) BOOL canConfigurePushTokenForProxyConfigs; // used to register at the right time when receiving push notif tokens
@property UIImage *avatar;

@end
