/* linphoneAppDelegate.m
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

#import "PhoneViewController.h"
#import "linphoneAppDelegate.h"
#import "ContactPickerDelegate.h"
#import "AddressBook/ABPerson.h"

#import "CoreTelephony/CTCallCenter.h"
#import "CoreTelephony/CTCall.h"

#import "ConsoleViewController.h"
#import "MoreViewController.h"
#include "CallHistoryTableViewController.h"
#import "LinphoneCoreSettingsStore.h"

#include "LinphoneManager.h"
#include "linphonecore.h"

#if __clang__ && __arm__
extern int __divsi3(int a, int b);
int __aeabi_idiv(int a, int b) {
	return __divsi3(a,b);
}
#endif

@implementation linphoneAppDelegate

@synthesize window;
@synthesize myTabBarController;
@synthesize myPeoplePickerController;
@synthesize myPhoneViewController;
@synthesize moreNavigationController;
@synthesize settingsController;

-(void) handleGSMCallInteration: (id) cCenter {
    CTCallCenter* ct = (CTCallCenter*) cCenter;
    
    int callCount = [ct.currentCalls count];
    if (!callCount) {
        NSLog(@"No GSM call -> enabling SIP calls");
        linphone_core_set_max_calls([LinphoneManager getLc], 3);
    } else {
        NSLog(@"%d GSM call(s) -> disabling SIP calls", callCount);
        /* pause current call, if any */
        LinphoneCall* call = linphone_core_get_current_call([LinphoneManager getLc]);
        if (call) {
            NSLog(@"Pausing SIP call");
            linphone_core_pause_call([LinphoneManager getLc], call);
        }
        linphone_core_set_max_calls([LinphoneManager getLc], 0);
    }
}

-(void)applicationWillResignActive:(UIApplication *)application {
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
	if (settingsController.settingsStore!=Nil)
		[settingsController.settingsStore synchronize];
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
        && [[NSUserDefaults standardUserDefaults] boolForKey:@"disable_autoboot_preference"]) {
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

- (void) loadDefaultSettings:(NSDictionary *) appDefaults {
    [[NSUserDefaults standardUserDefaults] registerDefaults:appDefaults];
    [appDefaults release];
    [[NSUserDefaults standardUserDefaults] synchronize];
}

-(void) setupUI {
    // Contacts
	myContactPickerDelegate = [[ContactPickerDelegate alloc] init];
    //people picker
    myPeoplePickerController = [[[ABPeoplePickerNavigationController alloc] init] autorelease];
    [myPeoplePickerController setDisplayedProperties:[NSArray arrayWithObject:[NSNumber numberWithInt:kABPersonPhoneProperty]]];
    [myPeoplePickerController setPeoplePickerDelegate:myContactPickerDelegate];
    //copy tab bar item
    myPeoplePickerController.tabBarItem = [(UIViewController*)[myTabBarController.viewControllers objectAtIndex:CONTACTS_TAB_INDEX] tabBarItem];
    
    NSMutableArray* newArray = [NSMutableArray arrayWithArray:self.myTabBarController.viewControllers];
    [newArray replaceObjectAtIndex:CONTACTS_TAB_INDEX withObject:myPeoplePickerController];
    [myTabBarController setViewControllers:newArray animated:NO];
    
    [myTabBarController setSelectedIndex:DIALER_TAB_INDEX];
	[window addSubview:myTabBarController.view];
	
	[window makeKeyAndVisible];
	
	[[LinphoneManager instance] setCallDelegate:myPhoneViewController];
    
    [UIDevice currentDevice].batteryMonitoringEnabled = YES;
}

-(void) setupGSMInteraction {
	callCenter = [[CTCallCenter alloc] init];
    callCenter.callEventHandler = ^(CTCall* call) {
        // post on main thread
        [self performSelectorOnMainThread:@selector(handleGSMCallInteration:)
                               withObject:callCenter
                            waitUntilDone:YES];
    };    
}

- (BOOL)application:(UIApplication *)application didFinishLaunchingWithOptions:(NSDictionary *)launchOptions{    
    NSDictionary *appDefaults = [NSDictionary dictionaryWithObjectsAndKeys: nil];
		// Put your default NSUserDefaults settings in the dictionary above.
	
    [self loadDefaultSettings: appDefaults];
    
    if ([[UIDevice currentDevice] respondsToSelector:@selector(isMultitaskingSupported)] 
		&& [UIApplication sharedApplication].applicationState ==  UIApplicationStateBackground 
        && ![[NSUserDefaults standardUserDefaults] boolForKey:@"start_at_boot_preference"]) {
		// autoboot disabled, doing nothing
	} else {
        [self startApplication];
    }

    return YES;
}

-(void) startApplication {
    /* explicitely instanciate LinphoneManager */
    LinphoneManager* lm = [[LinphoneManager alloc] init];
    assert(lm == [LinphoneManager instance]);
    
    [self setupUI];

	[[LinphoneManager instance]	startLibLinphone];

    // Settings, setup delegate
    settingsController.delegate = [LinphoneManager instance];
    settingsController.settingsReaderDelegate = [LinphoneManager instance];
	[LinphoneManager instance].settingsStore=settingsController.settingsStore=[[LinphoneCoreSettingsStore alloc] init];
	settingsController.showCreditsFooter=FALSE;
    
	[[UIApplication sharedApplication] registerForRemoteNotificationTypes:UIRemoteNotificationTypeAlert|UIRemoteNotificationTypeSound];
    
    [self setupGSMInteraction];
}


- (void)applicationWillTerminate:(UIApplication *)application {
}

- (void)dealloc {
	[window release];
	[myPeoplePickerController release];
	[super dealloc];
}

- (void)application:(UIApplication *)application didReceiveLocalNotification:(UILocalNotification *)notification {
    LinphoneCall* call;
	[(NSData*)([notification.userInfo objectForKey:@"call"])  getBytes:&call];
    if (!call) {
        ms_warning("Local notification received with nil call");
        return;
    }
	linphone_core_accept_call([LinphoneManager getLc], call);	
}

@end
