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

#import "LogView.h"
#import "IASKSettingsReader.h"
#import "IASKSettingsStore.h"
#import "IASKAppSettingsViewController.h"

#include "linphonecore.h"

typedef enum _PhoneView {
    PhoneView_FirstLoginView,
    PhoneView_Dialer,
    PhoneView_History,
    PhoneView_Settings,
    PhoneView_Chat,
    PhoneView_Contacts,
    PhoneView_InCall,
    PhoneView_IncomingCall,
    PhoneView_END
} PhoneView;

typedef enum _Connectivity {
	wifi,
	wwan
	,none
} Connectivity;

@class FastAddressBook;

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
    // transfer data
    int transferButtonIndex;
} LinphoneCallAppData;

@interface LinphoneManager : NSObject <AVAudioSessionDelegate> {
@protected
	SCNetworkReachabilityRef proxyReachability;
@private
	NSTimer* mIterateTimer;
	id<LogView> mLogView;	
	bool isbackgroundModeEnabled;
    
	Connectivity connectivity;
    FastAddressBook* mFastAddressBook;
	const char*  frontCamId;
	const char*  backCamId;
    
    PhoneView currentView;
    
    id<IASKSettingsStore> settingsStore;
    
@public
    CallContext currentCallContextBeforeGoingBackground;
}
+ (LinphoneManager*) instance;
+ (LinphoneCore*) getLc;
+ (BOOL)isLcReady;
+ (BOOL)runningOnIpad;
- (void)registerLogView:(id<LogView>) view;

+ (NSString *)getPreferenceForCodec: (const char*) name withRate: (int) rate;
+ (BOOL)codecIsSupported:(NSString *) prefName;
    
- (void)startLibLinphone;
- (BOOL)isNotIphone3G;
- (void)destroyLibLinphone;
  
- (BOOL)enterBackgroundMode;
- (void)becomeActive;
- (void)kickOffNetworkConnection;
- (NSString*)getDisplayNameFromAddressBook:(NSString*) number andUpdateCallLog:(LinphoneCallLog*)log; 
- (UIImage*)getImageFromAddressBook:(NSString*) number;

- (void)setupNetworkReachabilityCallback;
- (void)refreshRegisters;

- (void)changeView:(PhoneView) view;
- (void)changeView:(PhoneView) view dict:(NSDictionary *)dict;
- (void)showTabBar:(BOOL) show;
- (void)fullScreen:(BOOL) enabled;
- (PhoneView) currentView;

- (void)enableSpeaker:(BOOL)enable;
- (BOOL)isSpeakerEnabled;

@property (nonatomic, retain) id<IASKSettingsStore> settingsStore;

@property Connectivity connectivity;
@property (nonatomic) int defaultExpires;
@property (readonly) const char*  frontCamId;
@property (readonly) const char*  backCamId;

@end

