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
 *  GNU Library General Public License for more details.                
 *                                                                      
 *  You should have received a copy of the GNU General Public License   
 *  along with this program; if not, write to the Free Software         
 *  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */                                                                           

#import "PhoneViewController.h"
#import "linphoneAppDelegate.h"
#import "ContactPickerDelegate.h"
#import "AddressBook/ABPerson.h"

#import "ConsoleViewController.h"
#import "MoreViewController.h"

#include "LinphoneManager.h"


@implementation linphoneAppDelegate

@synthesize window;
@synthesize myTabBarController;
@synthesize myPeoplePickerController;
@synthesize myPhoneViewController;


- (void)applicationDidEnterBackground:(UIApplication *)application {
	[[LinphoneManager instance] enterBackgroundMode];
}
- (void)applicationDidBecomeActive:(UIApplication *)application {
	[[LinphoneManager instance] becomeActive];
}

- (void)registerDefaultsFromSettingsBundle {
	// source: http://stackoverflow.com/questions/510216/
    NSString *settingsBundle = [[NSBundle mainBundle] pathForResource:@"Settings" ofType:@"bundle"];
    if(!settingsBundle) {
        NSLog(@"Could not find Settings.bundle");
        return;
    }

    NSDictionary *settings = [NSDictionary dictionaryWithContentsOfFile:[settingsBundle stringByAppendingPathComponent:@"Root.plist"]];
    NSArray *preferences = [settings objectForKey:@"PreferenceSpecifiers"];

    NSMutableDictionary *defaultsToRegister = [[NSMutableDictionary alloc] initWithCapacity:[preferences count]];
    for(NSDictionary *prefSpecification in preferences) {
        NSString *key = [prefSpecification objectForKey:@"Key"];
        if(key) {
            [defaultsToRegister setObject:[prefSpecification objectForKey:@"DefaultValue"] forKey:key];
        }
    }

    [[NSUserDefaults standardUserDefaults] registerDefaults:defaultsToRegister];
    [defaultsToRegister release];
}

- (void) loadDefaultSettings {

		// if this is the first launch after installing, we would like to load default settings values from Settings.bundle
		NSObject *somePrefVal = [[NSUserDefaults standardUserDefaults] objectForKey:@"debugenable_preference"];
		if(!somePrefVal) {
			// has no value, so defaults have not been loaded yet
			[self registerDefaultsFromSettingsBundle];
		}

		NSDictionary *appDefaults = [NSDictionary dictionaryWithObjectsAndKeys:
									 @"NO", @"enable_first_login_view_preference", //
									 nil];
		
		[[NSUserDefaults standardUserDefaults] registerDefaults:appDefaults];
		[[NSUserDefaults standardUserDefaults] synchronize];
	
}
- (BOOL)application:(UIApplication *)application didFinishLaunchingWithOptions:(NSDictionary *)launchOptions{    
	
	/*
	 *Custumization
	 */
	[self loadDefaultSettings];
	//as defined in PhoneMainView.xib		
	//dialer
	myPhoneViewController = (PhoneViewController*) [myTabBarController.viewControllers objectAtIndex: DIALER_TAB_INDEX];
	myPhoneViewController.myTabBarController =  myTabBarController;
	//Call history
	myCallHistoryTableViewController = [[CallHistoryTableViewController alloc]  initWithNibName:@"CallHistoryTableViewController" 
																						 bundle:[NSBundle mainBundle]];
	UINavigationController *aCallHistNavigationController = [[UINavigationController alloc] initWithRootViewController:myCallHistoryTableViewController];
	aCallHistNavigationController.tabBarItem = [(UIViewController*)[myTabBarController.viewControllers objectAtIndex:HISTORY_TAB_INDEX] tabBarItem];
	
	//people picker delegates
	myContactPickerDelegate = [[ContactPickerDelegate alloc] init];
	//people picker
	myPeoplePickerController = [[[ABPeoplePickerNavigationController alloc] init] autorelease];
	[myPeoplePickerController setDisplayedProperties:[NSArray arrayWithObject:[NSNumber numberWithInt:kABPersonPhoneProperty]]];
	[myPeoplePickerController setPeoplePickerDelegate:myContactPickerDelegate];
	//copy tab bar item
	myPeoplePickerController.tabBarItem = [(UIViewController*)[myTabBarController.viewControllers objectAtIndex:CONTACTS_TAB_INDEX] tabBarItem]; 
	
	//more tab 
	MoreViewController *moreViewController = [[MoreViewController alloc] initWithNibName:@"MoreViewController" bundle:[NSBundle mainBundle]];
	UINavigationController *aNavigationController = [[UINavigationController alloc] initWithRootViewController:moreViewController];
	//copy tab bar item
	aNavigationController.tabBarItem = [(UIViewController*)[myTabBarController.viewControllers objectAtIndex:MORE_TAB_INDEX] tabBarItem]; 
	
	//insert contact controller
	NSMutableArray* newArray = [NSMutableArray arrayWithArray:self.myTabBarController.viewControllers];
	[newArray replaceObjectAtIndex:CONTACTS_TAB_INDEX withObject:myPeoplePickerController];
	[newArray replaceObjectAtIndex:MORE_TAB_INDEX withObject:aNavigationController];
	[newArray replaceObjectAtIndex:HISTORY_TAB_INDEX withObject:aCallHistNavigationController];
	
	[myTabBarController setSelectedIndex:DIALER_TAB_INDEX];
	[myTabBarController setViewControllers:newArray animated:NO];
	
	[window addSubview:myTabBarController.view];
	
	[window makeKeyAndVisible];
	
	[[LinphoneManager instance] setCallDelegate:myPhoneViewController];
	[[LinphoneManager instance]	startLibLinphone];
	
	[ [UIDevice currentDevice] setProximityMonitoringEnabled:true];
	
	
	
	return YES;
}


- (void)applicationWillTerminate:(UIApplication *)application {
}

- (void)dealloc {
	[window release];
	[myPeoplePickerController release];
	[super dealloc];
}


- (void)application:(UIApplication *)application didReceiveLocalNotification:(UILocalNotification *)notification {
	linphone_core_accept_call([LinphoneManager getLc],linphone_core_get_current_call([LinphoneManager getLc]));	
}



@end
