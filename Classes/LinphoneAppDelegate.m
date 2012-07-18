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
#import "MoreViewController.h"
#import "LinphoneCoreSettingsStore.h"

#include "LinphoneManager.h"
#include "linphonecore.h"

#if __clang__ && __arm__
extern int __divsi3(int a, int b);
int __aeabi_idiv(int a, int b) {
	return __divsi3(a,b);
}
#endif

@implementation LinphoneAppDelegate

@synthesize window;

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

- (void)applicationWillResignActive:(UIApplication *)application {
    LinphoneCore* lc = [LinphoneManager getLc];
    LinphoneCall* call = linphone_core_get_current_call(lc);
    if (call == NULL)
        return;
    
    /* save call context */
    LinphoneManager* instance = [LinphoneManager instance];
    instance->currentCallContextBeforeGoingBackground.call = call;
    instance->currentCallContextBeforeGoingBackground.cameraIsEnabled = linphone_call_camera_enabled(call);
    
    const LinphoneCallParams* params = linphone_call_get_current_params(call);
    if (linphone_call_params_video_enabled(params)) {
        linphone_call_enable_camera(call, false);
    }
    
}
- (void)applicationDidEnterBackground:(UIApplication *)application {
	if ([[LinphoneManager instance] settingsStore]!=Nil)
		[[[LinphoneManager instance] settingsStore] synchronize];
    if (![[LinphoneManager instance] enterBackgroundMode]) {
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
    if ([[UIDevice currentDevice] respondsToSelector:@selector(isMultitaskingSupported)] 
		&& [UIApplication sharedApplication].applicationState ==  UIApplicationStateBackground 
        && ![[NSUserDefaults standardUserDefaults] boolForKey:@"start_at_boot_preference"]) {
		// autoboot disabled, doing nothing
        return;
    } else if ([LinphoneManager instance] == nil) {
        [self startApplication];
    }
    
	[[LinphoneManager instance] becomeActive];
    
    if (callCenter == nil) {
        callCenter = [[CTCallCenter alloc] init];
        callCenter.callEventHandler = ^(CTCall* call) {
            // post on main thread
            [self performSelectorOnMainThread:@selector(handleGSMCallInteration:)
                                   withObject:callCenter
                                waitUntilDone:YES];
        };
    }
    // check call state at startup
    [self handleGSMCallInteration:callCenter];
    
    LinphoneCore* lc = [LinphoneManager getLc];
    LinphoneCall* call = linphone_core_get_current_call(lc);
    if (call == NULL)
        return;
    
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

- (void)loadDefaultSettings:(NSDictionary *) appDefaults {
    for(NSString* key in appDefaults){
        [LinphoneLogger log:LinphoneLoggerLog format:@"Overload %@ to in app settings.", key];
        [[[LinphoneManager instance] settingsStore] setObject:[appDefaults objectForKey:key] forKey:key];
    }
    [[[LinphoneManager instance] settingsStore] synchronize];
}

- (void)setupUI {
	if ([[LinphoneManager instance].settingsStore boolForKey:@"enable_first_login_view_preference"] == true) {
        // Change to fist login view
        [[PhoneMainView instance] changeCurrentView: [FirstLoginViewController compositeViewDescription]];
    } else {
        // Change to default view
        const MSList *list = linphone_core_get_proxy_config_list([LinphoneManager getLc]);
        if(list != NULL) {
            [[PhoneMainView instance] changeCurrentView: [DialerViewController compositeViewDescription]];
        } else {
            [[PhoneMainView instance] changeCurrentView: [WizardViewController compositeViewDescription]];
        }
    }
	
	[UIDevice currentDevice].batteryMonitoringEnabled = YES;
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

- (BOOL)application:(UIApplication *)application didFinishLaunchingWithOptions:(NSDictionary *)launchOptions{    
    NSDictionary *appDefaults = [NSDictionary dictionaryWithObjectsAndKeys: nil];
		// Put your default NSUserDefaults settings in the dictionary above.
    
    if ([[UIDevice currentDevice] respondsToSelector:@selector(isMultitaskingSupported)] 
		&& [UIApplication sharedApplication].applicationState ==  UIApplicationStateBackground 
        && ![[NSUserDefaults standardUserDefaults] boolForKey:@"start_at_boot_preference"]) {
		// autoboot disabled, doing nothing
	} else {
        [self startApplication];
        [self loadDefaultSettings: appDefaults];
    }

    return YES;
}

- (void)startApplication {
    /* explicitely instanciate LinphoneManager */
    LinphoneManager* lm = [[LinphoneManager alloc] init];
    assert(lm == [LinphoneManager instance]);

	[[LinphoneManager instance]	startLibLinphone];
    
    [self setupUI];
    
	[[UIApplication sharedApplication] registerForRemoteNotificationTypes:UIRemoteNotificationTypeAlert|UIRemoteNotificationTypeSound];
    
    [self setupGSMInteraction];
}


- (void)applicationWillTerminate:(UIApplication *)application {
}

- (void)dealloc {
	[window release];
	[super dealloc];
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
    } else if([notification.userInfo objectForKey:@"chat"] != nil) {
        NSString *remoteContact = (NSString*)[notification.userInfo objectForKey:@"chat"];
        // Go to ChatRoom view
        ChatRoomViewController *controller = DYNAMIC_CAST([[PhoneMainView instance] changeCurrentView:[ChatRoomViewController compositeViewDescription] push:TRUE], ChatRoomViewController);
        if(controller != nil) {
            [controller setRemoteAddress:remoteContact];
        }
    }
}

@end
