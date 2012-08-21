/* LinphoneAppDelegate.m
 *
 * Copyright (C) 2009  Belledonne Comunications, Grenoble, France
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

#import "PhoneMainView.h"
#import "linphoneAppDelegate.h"
#import "AddressBook/ABPerson.h"

#import "CoreTelephony/CTCallCenter.h"
#import "CoreTelephony/CTCall.h"

#import "ConsoleViewController.h"
#import "LinphoneCoreSettingsStore.h"

#include "LinphoneManager.h"
#include "linphonecore.h"

@implementation UILinphoneWindow

@end

@implementation LinphoneAppDelegate

@synthesize started;


#pragma mark - Lifecycle Functions

- (id)init {
    self = [super init];
    if(self != nil) {
        self->started = FALSE;
    }
    return self;
}

- (void)dealloc {
	[super dealloc];
}


#pragma mark - 

- (void)handleGSMCallInteration: (id) cCenter {
    CTCallCenter* ct = (CTCallCenter*) cCenter;
    
    int callCount = [ct.currentCalls count];
    if (!callCount) {
        [LinphoneLogger logc:LinphoneLoggerLog format:"No GSM call -> enabling SIP calls"];
        linphone_core_set_max_calls([LinphoneManager getLc], 3);
    } else {
        [LinphoneLogger logc:LinphoneLoggerLog format:"%d GSM call(s) -> disabling SIP calls", callCount];
        /* pause current call, if any */
        LinphoneCall* call = linphone_core_get_current_call([LinphoneManager getLc]);
        if (call) {
            [LinphoneLogger logc:LinphoneLoggerLog format:"Pausing SIP call"];
            linphone_core_pause_call([LinphoneManager getLc], call);
        }
        linphone_core_set_max_calls([LinphoneManager getLc], 0);
    }
}

- (void)applicationDidEnterBackground:(UIApplication *)application{
	[LinphoneLogger logc:LinphoneLoggerLog format:"applicationDidEnterBackground"];
	if(![LinphoneManager isLcReady]) return;
	[[LinphoneManager instance] enterBackgroundMode];
}

- (void)applicationWillResignActive:(UIApplication *)application {
	[LinphoneLogger logc:LinphoneLoggerLog format:"applicationWillResignActive"];
    if(![LinphoneManager isLcReady]) return;
    LinphoneCore* lc = [LinphoneManager getLc];
    LinphoneCall* call = linphone_core_get_current_call(lc);
	
	
    if (call){
		/* save call context */
		LinphoneManager* instance = [LinphoneManager instance];
		instance->currentCallContextBeforeGoingBackground.call = call;
		instance->currentCallContextBeforeGoingBackground.cameraIsEnabled = linphone_call_camera_enabled(call);
    
		const LinphoneCallParams* params = linphone_call_get_current_params(call);
		if (linphone_call_params_video_enabled(params)) {
			linphone_call_enable_camera(call, false);
		}
	}
    
    if (![[LinphoneManager instance] resignActive]) {
        // destroying eventHandler if app cannot go in background.
        // Otherwise if a GSM call happen and Linphone is resumed,
        // the handler will be called before LinphoneCore is built.
        // Then handler will be restored in appDidBecomeActive cb
        callCenter.callEventHandler = nil;
        [callCenter release];
        callCenter = nil;
    }
    
}

- (void)applicationDidBecomeActive:(UIApplication *)application {
	[LinphoneLogger logc:LinphoneLoggerLog format:"applicationDidBecomeActive"];
    [self startApplication];
    
	[[LinphoneManager instance] becomeActive];
    
    // check call state at startup
    [self handleGSMCallInteration:callCenter];
    
    LinphoneCore* lc = [LinphoneManager getLc];
    LinphoneCall* call = linphone_core_get_current_call(lc);
    
	if (call){
		LinphoneManager* instance = [LinphoneManager instance];
		if (call == instance->currentCallContextBeforeGoingBackground.call) {
			const LinphoneCallParams* params = linphone_call_get_current_params(call);
			if (linphone_call_params_video_enabled(params)) {
				linphone_call_enable_camera(
                                        call, 
                                        instance->currentCallContextBeforeGoingBackground.cameraIsEnabled);
			}
			instance->currentCallContextBeforeGoingBackground.call = 0;
		}
	}
}


- (void)setupGSMInteraction {
    if (callCenter == nil) {
        callCenter = [[CTCallCenter alloc] init];
        callCenter.callEventHandler = ^(CTCall* call) {
            // post on main thread
            [self performSelectorOnMainThread:@selector(handleGSMCallInteration:)
                               withObject:callCenter
                            waitUntilDone:YES];
        };    
    }
}

- (BOOL)application:(UIApplication *)application didFinishLaunchingWithOptions:(NSDictionary *)launchOptions {
    [[UIApplication sharedApplication] registerForRemoteNotificationTypes:UIRemoteNotificationTypeAlert|UIRemoteNotificationTypeSound];
    
    if ([[UIDevice currentDevice] respondsToSelector:@selector(isMultitaskingSupported)]
		&& [UIApplication sharedApplication].applicationState ==  UIApplicationStateBackground
        && (![[NSUserDefaults standardUserDefaults] boolForKey:@"start_at_boot_preference"] ||
            ![[NSUserDefaults standardUserDefaults] boolForKey:@"backgroundmode_preference"])) {
            // autoboot disabled, doing nothing
            return YES;
        }
    
    [self startApplication];
    
    return YES;
}

- (void)startApplication {
    // Restart Linphone Core if needed
    if(![LinphoneManager isLcReady]) {
        [[LinphoneManager instance]	startLibLinphone];
    }
    if([LinphoneManager isLcReady]) {
        [self setupGSMInteraction];
        
        // Only execute one time at application start
        if(!started) {
            started = TRUE;
            [[PhoneMainView instance] startUp];
        }
    }
}


- (void)applicationWillTerminate:(UIApplication *)application {
}

- (BOOL)application:(UIApplication *)application handleOpenURL:(NSURL *)url {
    [self startApplication];
    if([LinphoneManager isLcReady]) {
        if([[url scheme] isEqualToString:@"sip"]) {
            // Go to ChatRoom view
            DialerViewController *controller = DYNAMIC_CAST([[PhoneMainView instance] changeCurrentView:[DialerViewController compositeViewDescription]], DialerViewController);
            if(controller != nil) {
                [controller setAddress:[url absoluteString]];
            }
        }
    }
	return YES;
}

- (void)application:(UIApplication *)application didReceiveRemoteNotification:(NSDictionary *)userInfo {
    [LinphoneLogger log:LinphoneLoggerDebug format:@"PushNotification: Receive %@", userInfo];
    NSDictionary *aps = [userInfo objectForKey:@"aps"];
    if(aps != nil) {
        NSDictionary *alert = [aps objectForKey:@"alert"];
        if(alert != nil) {
            NSString *loc_key = [alert objectForKey:@"loc-key"];
            if(loc_key != nil) {
                /* MODIFICATION disable chat
                if([loc_key isEqualToString:@"IM_MSG"]) {
                    [[LinphoneManager instance] addInhibitedEvent:kLinphoneTextReceivedSound];
                    [[PhoneMainView instance] changeCurrentView:[ChatViewController compositeViewDescription]];
                }
                */
            }
        }
    }
}

- (void)application:(UIApplication *)application didReceiveLocalNotification:(UILocalNotification *)notification {
    if([notification.userInfo objectForKey:@"call"] != nil) {
        LinphoneCall* call;
        [(NSData*)[notification.userInfo objectForKey:@"call"] getBytes:&call];
        if (!call) {
            [LinphoneLogger logc:LinphoneLoggerWarning format:"Local notification received with nil call"];
            return;
        }
        linphone_core_accept_call([LinphoneManager getLc], call);
    } /* MODIFICATION: Remove chat
		 else if([notification.userInfo objectForKey:@"chat"] != nil) {
        NSString *remoteContact = (NSString*)[notification.userInfo objectForKey:@"chat"];
        // Go to ChatRoom view
        ChatRoomViewController *controller = DYNAMIC_CAST([[PhoneMainView instance] changeCurrentView:[ChatRoomViewController compositeViewDescription] push:TRUE], ChatRoomViewController);
        if(controller != nil) {
            [controller setRemoteAddress:remoteContact];
        }
    } */
}


#pragma mark - PushNotification Functions

- (void)application:(UIApplication*)application didRegisterForRemoteNotificationsWithDeviceToken:(NSData*)deviceToken {
    [LinphoneLogger log:LinphoneLoggerDebug format:@"PushNotification: Token %@", deviceToken];
    [[LinphoneManager instance] setPushNotificationToken:deviceToken];
}

- (void)application:(UIApplication*)application didFailToRegisterForRemoteNotificationsWithError:(NSError*)error {
    [LinphoneLogger log:LinphoneLoggerDebug format:@"PushNotification: Error %@", error];
    [[LinphoneManager instance] setPushNotificationToken:nil];
}

@end
