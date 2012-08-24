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
#import <sqlite3.h>

#import "IASKSettingsReader.h"
#import "IASKSettingsStore.h"
#import "IASKAppSettingsViewController.h"
#import "FastAddressBook.h"
#import "Utils.h"

#include "linphonecore.h"

extern NSString *const kLinphoneDisplayStatusUpdate;
extern NSString *const kLinphoneTextReceived;
extern NSString *const kLinphoneTextReceivedSound;
extern NSString *const kLinphoneCallUpdate;
extern NSString *const kLinphoneRegistrationUpdate;
extern NSString *const kLinphoneMainViewChange;
extern NSString *const kLinphoneAddressBookUpdate;
extern NSString *const kLinphoneLogsUpdate;
extern NSString *const kLinphoneSettingsUpdate;

extern NSString *const kContactSipField;

typedef enum _Connectivity {
	wifi,
	wwan
	,none
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

typedef struct _LinphoneCallAppData {
    bool_t batteryWarningShown;
    UILocalNotification *notification;
} LinphoneCallAppData;

typedef struct _LinphoneManagerSounds {
    SystemSoundID call;
    SystemSoundID message;
} LinphoneManagerSounds;

@interface LinphoneManager : NSObject <AVAudioSessionDelegate> {
@protected
	SCNetworkReachabilityRef proxyReachability;
    
@private
	NSTimer* mIterateTimer;
    
	Connectivity connectivity;
    
    NSMutableArray *inhibitedEvent;
    
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

+ (void)kickOffNetworkConnection;
- (void)setupNetworkReachabilityCallback;

- (void)refreshRegisters;

- (void)enableSpeaker:(BOOL)enable;
- (BOOL)isSpeakerEnabled;

- (void)addInhibitedEvent:(NSString*)event;
- (BOOL)removeInhibitedEvent:(NSString*)event;

+ (BOOL)copyFile:(NSString*)src destination:(NSString*)dst override:(BOOL)override;
+ (NSString*)bundleFile:(NSString*)file;
+ (NSString*)documentFile:(NSString*)file;

- (void)call:(NSString *)address displayName:(NSString*)displayName transfer:(BOOL)transfer;

@property (nonatomic, retain) id<IASKSettingsStore> settingsStore;
@property (readonly) FastAddressBook* fastAddressBook;
@property Connectivity connectivity;
@property (nonatomic) int defaultExpires;
@property (readonly) const char*  frontCamId;
@property (readonly) const char*  backCamId;
@property (readonly) sqlite3* database;
@property (nonatomic, retain) NSData *pushNotificationToken;
@property (readonly) LinphoneManagerSounds sounds;
@property (readonly) NSMutableArray *logs;

@end

