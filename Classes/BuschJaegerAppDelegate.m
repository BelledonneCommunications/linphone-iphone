/* BuschJaegerAppDelegate.m
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
#import "BuschJaegerAppDelegate.h"

@implementation BuschJaegerAppDelegate
@synthesize window;
@synthesize buschJaegerMainView;
@synthesize buschJaegerCallView;
@synthesize buschJaegerSettingsView;
@synthesize navigationController;

- (void) loadDefaultSettings {
    NSString *settingsBundle = [[NSBundle mainBundle] pathForResource:@"Settings" ofType:@"bundle"];
    if(!settingsBundle) {
        NSLog(@"Could not find Settings.bundle");
        return;
    }
    
    NSMutableDictionary *rootSettings = [NSDictionary dictionaryWithContentsOfFile:[settingsBundle stringByAppendingPathComponent:@"Root.plist"]];
    NSMutableArray *preferences = [rootSettings objectForKey:@"PreferenceSpecifiers"];
    NSMutableDictionary *defaultsToRegister = [[NSMutableDictionary alloc] initWithCapacity:[preferences count]];
    
    NSDictionary *appDefaults = [NSDictionary dictionaryWithObjectsAndKeys:
                                 @"YES", @"debugenable_preference",
                                 @"YES", @"enable_video_preference",
                                 @"YES", @"start_video_preference",
                                 @"YES", @"h264_preference",
                                 @"YES", @"vp8_preference",
                                 @"NO", @"mpeg4_preference",
                                 @"YES", @"pcmu_preference",
                                 @"YES", @"pcma_preference",
                                 @"tcp", @"transport_preference",
                                 @"NO", @"enable_srtp_preference",
                                 @"YES", @"backgroundmode_preference",
                                 @"YES", @"outbound_proxy_preference",
                                 nil];
    
    [defaultsToRegister addEntriesFromDictionary:appDefaults];
    [[NSUserDefaults standardUserDefaults] registerDefaults:defaultsToRegister];
    [defaultsToRegister release];
    [[NSUserDefaults standardUserDefaults] synchronize];
	
}

- (BOOL)application:(UIApplication *)application didFinishLaunchingWithOptions:(NSDictionary *)launchOptions{
    [[UIApplication sharedApplication] setStatusBarHidden:YES];
    
    [self loadDefaultSettings];
    
    /* explicitely instanciate LinphoneManager */
    LinphoneManager* lm = [[LinphoneManager alloc] init];
    assert(lm == [LinphoneManager instance]);
    
	[[LinphoneManager instance]	startLibLinphone];
    [[LinphoneManager instance] setCallDelegate:self];
    
	[window addSubview:buschJaegerMainView.view];
	[window makeKeyAndVisible];
    
	[[UIApplication sharedApplication] registerForRemoteNotificationTypes:UIRemoteNotificationTypeAlert|UIRemoteNotificationTypeSound];
    
    
    linphone_core_set_device_rotation([LinphoneManager getLc], 0);
    linphone_core_set_video_device([LinphoneManager getLc], "DummyImage: Dummy (no) picture");
    
    linphone_core_set_ring([LinphoneManager getLc], NULL ); //so that we don't attempt to play ring by the core
    return YES;
}

- (void)applicationDidEnterBackground:(UIApplication *)application {
    [buschJaegerCallView activateVideoView:FALSE];
	[[LinphoneManager instance] enterBackgroundMode];
}

- (void)applicationDidBecomeActive:(UIApplication *)application {
	[[LinphoneManager instance] becomeActive];
    [buschJaegerCallView activateVideoView:TRUE];
}

- (void)application:(UIApplication *)application didReceiveLocalNotification:(UILocalNotification *)notification {

}

// UI changes
-(void) displayDialerFromUI:(UIViewController*) viewCtrl forUser:(NSString*) username withDisplayName:(NSString*) displayName {
    [buschJaegerCallView displayDialerFromUI:viewCtrl forUser:username withDisplayName:displayName];
}
-(void) displayCall: (LinphoneCall*) call InProgressFromUI:(UIViewController*) viewCtrl forUser:(NSString*) username withDisplayName:(NSString*) displayName {
    [buschJaegerCallView displayCall:call InProgressFromUI:viewCtrl forUser:username withDisplayName:displayName];
}
-(void) displayIncomingCall: (LinphoneCall*) call NotificationFromUI:(UIViewController*) viewCtrl forUser:(NSString*) username withDisplayName:(NSString*) displayName {
    [buschJaegerCallView displayIncomingCall:call NotificationFromUI:viewCtrl forUser:username withDisplayName:displayName];
}
-(void) displayInCall: (LinphoneCall*) call FromUI:(UIViewController*) viewCtrl forUser:(NSString*) username withDisplayName:(NSString*) displayName {
    [buschJaegerCallView displayInCall:call FromUI:viewCtrl forUser:username withDisplayName:displayName];
}
-(void) displayVideoCall:(LinphoneCall*) call  FromUI:(UIViewController*) viewCtrl forUser:(NSString*) username withDisplayName:(NSString*) displayName {
    [buschJaegerCallView displayVideoCall:call FromUI:viewCtrl forUser:username withDisplayName:displayName];
}

//status reporting
-(void) displayStatus:(NSString*) message {
    [buschJaegerCallView displayStatus:message];
}

-(void) displayAskToEnableVideoCall:(LinphoneCall*) call forUser:(NSString*) username withDisplayName:(NSString*) displayName {
    [buschJaegerCallView displayAskToEnableVideoCall:call forUser:username withDisplayName:displayName];
}

-(void) firstVideoFrameDecoded:(LinphoneCall*) call {
    [buschJaegerCallView firstVideoFrameDecoded:call];
}

@end
