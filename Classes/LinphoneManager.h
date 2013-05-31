/* LinphoneManager.h
 *
 * Copyright (C) 2011  Belledonne Comunications, Grenoble, France
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or   
 *  (at your option) any later version.                                 
 *                                                                      
 *  This program is distributed in the hope that it will be useful,     
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of      
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the       
 *  GNU General Public License for more details.                
 *                                                                      
 *  You should have received a copy of the GNU General Public License   
 *  along with this program; if not, write to the Free Software         
 *  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */              

#import <Foundation/Foundation.h>
#import <AVFoundation/AVAudioSession.h>
#import <SystemConfiguration/SCNetworkReachability.h>
#import <AudioToolbox/AudioToolbox.h>
#import <AssetsLibrary/ALAssetsLibrary.h>
#import <CoreTelephony/CTCallCenter.h>

#import <sqlite3.h>

#import "IASKSettingsReader.h"
#import "IASKSettingsStore.h"
#import "IASKAppSettingsViewController.h"
#import "FastAddressBook.h"
#import "Utils.h"

#include "linphonecore.h"

extern const char *const LINPHONERC_APPLICATION_KEY;

extern NSString *const kLinphoneCoreUpdate;
extern NSString *const kLinphoneDisplayStatusUpdate;
extern NSString *const kLinphoneTextReceived;
extern NSString *const kLinphoneCallUpdate;
extern NSString *const kLinphoneRegistrationUpdate;
extern NSString *const kLinphoneMainViewChange;
extern NSString *const kLinphoneAddressBookUpdate;
extern NSString *const kLinphoneLogsUpdate;
extern NSString *const kLinphoneSettingsUpdate;
extern NSString *const kLinphoneBluetoothAvailabilityUpdate;

typedef enum _NetworkType {
    network_none = 0,
    network_2g,
    network_3g,
    network_4g,
    network_lte,
    network_wifi
} NetworkType;

typedef enum _Connectivity {
	wifi,
	wwan,
    none
} Connectivity;

/* Application specific call context */
typedef struct _CallContext {
    LinphoneCall* call;
    bool_t cameraIsEnabled;
} CallContext;

struct NetworkReachabilityContext {
    bool_t testWifi, testWWan;
    void (*networkStateChanged) (Connectivity newConnectivity);
};

@interface LinphoneCallAppData :NSObject {
    @public
	bool_t batteryWarningShown;
    UILocalNotification *notification;
    NSMutableDictionary *userInfos;
	bool_t videoRequested; /*set when user has requested for video*/
};
@end

typedef struct _LinphoneManagerSounds {
    SystemSoundID call;
    SystemSoundID message;
} LinphoneManagerSounds;

@interface LinphoneManager : NSObject <AVAudioSessionDelegate> {
@protected
	SCNetworkReachabilityRef proxyReachability;
    
@private
	NSTimer* mIterateTimer;
    NSMutableArray*  pendindCallIdFromRemoteNotif;
	Connectivity connectivity;
    BOOL stopWaitingRegisters;
	UIBackgroundTaskIdentifier pausedCallBgTask;
	UIBackgroundTaskIdentifier incallBgTask;
	CTCallCenter* mCallCenter;
    
@public
    CallContext currentCallContextBeforeGoingBackground;
}
+ (LinphoneManager*)instance;
#ifdef DEBUG
+ (void)instanceRelease;
#endif
+ (LinphoneCore*) getLc;
+ (BOOL)isLcReady;
+ (BOOL)runningOnIpad;
+ (BOOL)isNotIphone3G;
+ (NSString *)getPreferenceForCodec: (const char*) name withRate: (int) rate;
+ (NSSet *)unsupportedCodecs;
+ (NSString *)getUserAgent;


- (void)startLibLinphone;
- (void)destroyLibLinphone;
- (BOOL)resignActive;
- (void)becomeActive;
- (BOOL)enterBackgroundMode;
- (void)enableAutoAnswerForCallId:(NSString*) callid;
- (void)addPushTokenToProxyConfig: (LinphoneProxyConfig*)cfg;
- (BOOL)shouldAutoAcceptCallForCallId:(NSString*) callId;
- (void)acceptCallForCallId:(NSString*)callid;
- (void)waitForRegisterToArrive;

+ (void)kickOffNetworkConnection;
- (void)setupNetworkReachabilityCallback;

- (void)refreshRegisters;

- (bool)allowSpeaker;

+ (BOOL)copyFile:(NSString*)src destination:(NSString*)dst override:(BOOL)override;
+ (NSString*)bundleFile:(NSString*)file;
+ (NSString*)documentFile:(NSString*)file;

- (void)acceptCall:(LinphoneCall *)call;
- (void)call:(NSString *)address displayName:(NSString*)displayName transfer:(BOOL)transfer;

- (void)lpConfigSetString:(NSString*)value forKey:(NSString*)key;
- (NSString*)lpConfigStringForKey:(NSString*)key;
- (NSString*)lpConfigStringForKey:(NSString*)key withDefault:(NSString*)value;
- (void)lpConfigSetString:(NSString*)value forKey:(NSString*)key forSection:(NSString*)section;
- (NSString*)lpConfigStringForKey:(NSString*)key forSection:(NSString*)section;
- (void)lpConfigSetInt:(NSInteger)value forKey:(NSString*)key;
- (NSInteger)lpConfigIntForKey:(NSString*)key;
- (void)lpConfigSetInt:(NSInteger)value forKey:(NSString*)key forSection:(NSString*)section;
- (NSInteger)lpConfigIntForKey:(NSString*)key forSection:(NSString*)section;
- (void)lpConfigSetBool:(BOOL)value forKey:(NSString*)key; 
- (BOOL)lpConfigBoolForKey:(NSString*)key;
- (void)lpConfigSetBool:(BOOL)value forKey:(NSString*)key forSection:(NSString*)section;
- (BOOL)lpConfigBoolForKey:(NSString*)key forSection:(NSString*)section;


@property (readonly) FastAddressBook* fastAddressBook;
@property Connectivity connectivity;
@property (readonly) NetworkType network;
@property (readonly) const char*  frontCamId;
@property (readonly) const char*  backCamId;
@property (readonly) sqlite3* database;
@property (nonatomic, retain) NSData *pushNotificationToken;
@property (readonly) LinphoneManagerSounds sounds;
@property (readonly) NSMutableArray *logs;
@property (nonatomic, assign) BOOL speakerEnabled;
@property (nonatomic, assign) BOOL bluetoothAvailable;
@property (nonatomic, assign) BOOL bluetoothEnabled;
@property (readonly) ALAssetsLibrary *photoLibrary;
@property (readonly) NSString* contactSipField;
@property (readonly,copy) NSString* contactFilter;

@end

