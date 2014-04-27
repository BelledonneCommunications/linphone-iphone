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

@synthesize started,configURL;


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

    }
    
}

- (void)applicationDidBecomeActive:(UIApplication *)application {
	[LinphoneLogger logc:LinphoneLoggerLog format:"applicationDidBecomeActive"];
    
    [self startApplication];
    LinphoneManager* instance = [LinphoneManager instance];
    
    [instance becomeActive];
    
    LinphoneCore* lc = [LinphoneManager getLc];
    LinphoneCall* call = linphone_core_get_current_call(lc);
    
    if (call){
        if (call == instance->currentCallContextBeforeGoingBackground.call) {
            const LinphoneCallParams* params = linphone_call_get_current_params(call);
            if (linphone_call_params_video_enabled(params)) {
                linphone_call_enable_camera(
                                            call,
                                            instance->currentCallContextBeforeGoingBackground.cameraIsEnabled);
            }
            instance->currentCallContextBeforeGoingBackground.call = 0;
        } else if ( linphone_call_get_state(call) == LinphoneCallIncomingReceived ) {
            [[PhoneMainView  instance ] displayIncomingCall:call];
            // in this case, the ringing sound comes from the notification.
            // To stop it we have to do the iOS7 ring fix...
            [self fixRing];
        }
    }
}



- (BOOL)application:(UIApplication *)application didFinishLaunchingWithOptions:(NSDictionary *)launchOptions {
    [[UIApplication sharedApplication] registerForRemoteNotificationTypes:UIRemoteNotificationTypeAlert|UIRemoteNotificationTypeSound|UIRemoteNotificationTypeBadge|UIRemoteNotificationTypeNewsstandContentAvailability];
    

	LinphoneManager* instance = [LinphoneManager instance];
    BOOL background_mode = [instance lpConfigBoolForKey:@"backgroundmode_preference"];
    BOOL start_at_boot   = [instance lpConfigBoolForKey:@"start_at_boot_preference"];

    if ([[UIDevice currentDevice] respondsToSelector:@selector(isMultitaskingSupported)]
		&& [UIApplication sharedApplication].applicationState ==  UIApplicationStateBackground)
    {
        // we've been woken up directly to background;
        if( !start_at_boot || !background_mode ) {
            // autoboot disabled or no background, and no push: do nothing and wait for a real launch
			/*output a log with NSLog, because the ortp logging system isn't activated yet at this time*/
			NSLog(@"Linphone launch doing nothing because start_at_boot or background_mode are not activated.", NULL);
            return YES;
        }

    }
	bgStartId = [[UIApplication sharedApplication] beginBackgroundTaskWithExpirationHandler:^{
		[LinphoneLogger log:LinphoneLoggerWarning format:@"Background task for application launching expired."];
		[[UIApplication sharedApplication] endBackgroundTask:bgStartId];
	}];
    [self startApplication];
	NSDictionary *remoteNotif =[launchOptions objectForKey:UIApplicationLaunchOptionsRemoteNotificationKey];
    if (remoteNotif){
		[LinphoneLogger log:LinphoneLoggerLog format:@"PushNotification from launch received."];
		[self processRemoteNotification:remoteNotif];
	}
    if (bgStartId!=UIBackgroundTaskInvalid) [[UIApplication sharedApplication] endBackgroundTask:bgStartId];
    [[PhoneMainView instance] updateStatusBar:nil];

    return YES;
}

- (void)startApplication {
    // Restart Linphone Core if needed
    if(![LinphoneManager isLcReady]) {
        [[LinphoneManager instance]	startLibLinphone];
    }
    if([LinphoneManager isLcReady]) {
        
        
        // Only execute one time at application start
        if(!started) {
            started = TRUE;
            [[PhoneMainView instance] startUp];
        }
    }
}

- (void)applicationWillTerminate:(UIApplication *)application {
    [LinphoneLogger log:LinphoneLoggerLog format:@"Application Will Terminate"];
}

- (BOOL)application:(UIApplication *)application handleOpenURL:(NSURL *)url {
    NSString *scheme = [[url scheme] lowercaseString];
    if ([scheme isEqualToString:@"linphone-config-http"] || [scheme isEqualToString:@"linphone-config-https"]) {
        configURL = [[NSString alloc] initWithString:[[url absoluteString] stringByReplacingOccurrencesOfString:@"linphone-config-" withString:@""]];
        UIAlertView* confirmation = [[UIAlertView alloc] initWithTitle:NSLocalizedString(@"Remote configuration",nil)
                                                        message:NSLocalizedString(@"This operation will load a remote configuration. Continue ?",nil)
                                                       delegate:self
                                              cancelButtonTitle:NSLocalizedString(@"No",nil)
                                              otherButtonTitles:NSLocalizedString(@"Yes",nil),nil];
        confirmation.tag = 1;
        [confirmation show];
        [confirmation release];
    } else {
        [self startApplication];
        if([LinphoneManager isLcReady]) {
            if([[url scheme] isEqualToString:@"sip"]) {
                // Go to Dialer view
                DialerViewController *controller = DYNAMIC_CAST([[PhoneMainView instance] changeCurrentView:[DialerViewController compositeViewDescription]], DialerViewController);
                if(controller != nil) {
                    [controller setAddress:[url absoluteString]];
                }
            }
        }
    }
	return YES;
}

- (void)fixRing{
    if ([[[UIDevice currentDevice] systemVersion] floatValue] >= 7) {
        // iOS7 fix for notification sound not stopping.
        // see http://stackoverflow.com/questions/19124882/stopping-ios-7-remote-notification-sound
        [[UIApplication sharedApplication] setApplicationIconBadgeNumber: 1];
        [[UIApplication sharedApplication] setApplicationIconBadgeNumber: 0];
    }
}

- (void)processRemoteNotification:(NSDictionary*)userInfo{
	if ([LinphoneManager instance].pushNotificationToken==Nil){
		[LinphoneLogger log:LinphoneLoggerLog format:@"Ignoring push notification we did not subscribed."];
		return;
	}
	
	NSDictionary *aps = [userInfo objectForKey:@"aps"];
	
    if(aps != nil) {
        NSDictionary *alert = [aps objectForKey:@"alert"];
        if(alert != nil) {
            NSString *loc_key = [alert objectForKey:@"loc-key"];
			/*if we receive a remote notification, it is probably because our TCP background socket was no more working.
			 As a result, break it and refresh registers in order to make sure to receive incoming INVITE or MESSAGE*/
			LinphoneCore *lc = [LinphoneManager getLc];
			if (linphone_core_get_calls(lc)==NULL){ //if there are calls, obviously our TCP socket shall be working
				linphone_core_set_network_reachable(lc, FALSE);
				[LinphoneManager instance].connectivity=none; /*force connectivity to be discovered again*/
				if(loc_key != nil) {
					if([loc_key isEqualToString:@"IM_MSG"]) {
						[[PhoneMainView instance] addInhibitedEvent:kLinphoneTextReceived];
						[[PhoneMainView instance] changeCurrentView:[ChatViewController compositeViewDescription]];
					} else if([loc_key isEqualToString:@"IC_MSG"]) {
						//it's a call
						NSString *callid=[userInfo objectForKey:@"call-id"];
						if (callid)
							[[LinphoneManager instance] enableAutoAnswerForCallId:callid];
						else
							[LinphoneLogger log:LinphoneLoggerError format:@"PushNotification: does not have call-id yet, fix it !"];

						[self fixRing];
					}
				}
			}
        }
    }
}

- (void)application:(UIApplication *)application didReceiveRemoteNotification:(NSDictionary *)userInfo {
	[LinphoneLogger log:LinphoneLoggerLog format:@"PushNotification: Receive %@", userInfo];
	
	[self processRemoteNotification:userInfo];
}

- (void)application:(UIApplication *)application didReceiveLocalNotification:(UILocalNotification *)notification {

    [self fixRing];
    

    if([notification.userInfo objectForKey:@"callId"] != nil) {
        // some local notifications have an internal timer to relaunch themselves at specified intervals
        if( [[notification.userInfo objectForKey:@"timer"] intValue] == 1 ){
            [[LinphoneManager instance] cancelLocalNotifTimerForCallId:[notification.userInfo objectForKey:@"callId"]];
        } else {
            // auto answer only for non-timed local notifications
            [[LinphoneManager instance] acceptCallForCallId:[notification.userInfo objectForKey:@"callId"]];
        }
    } else if([notification.userInfo objectForKey:@"chat"] != nil) {
        NSString *remoteContact = (NSString*)[notification.userInfo objectForKey:@"chat"];
        // Go to ChatRoom view
        [[PhoneMainView instance] changeCurrentView:[ChatViewController compositeViewDescription]];
        ChatRoomViewController *controller = DYNAMIC_CAST([[PhoneMainView instance] changeCurrentView:[ChatRoomViewController compositeViewDescription] push:TRUE], ChatRoomViewController);
        if(controller != nil) {
            [controller setRemoteAddress:remoteContact];
        }
    } else if([notification.userInfo objectForKey:@"callLog"] != nil) {
        NSString *callLog = (NSString*)[notification.userInfo objectForKey:@"callLog"];
        // Go to HistoryDetails view
        [[PhoneMainView instance] changeCurrentView:[HistoryViewController compositeViewDescription]];
        HistoryDetailsViewController *controller = DYNAMIC_CAST([[PhoneMainView instance] changeCurrentView:[HistoryDetailsViewController compositeViewDescription] push:TRUE], HistoryDetailsViewController);
        if(controller != nil) {
            [controller setCallLogId:callLog];
        }
    }
}

// this method is implemented for iOS7. It is invoked when receiving a push notification for a call and it has "content-available" in the aps section.
- (void)application:(UIApplication *)application didReceiveRemoteNotification:(NSDictionary *)userInfo fetchCompletionHandler:(void (^)(UIBackgroundFetchResult))completionHandler
{
    LinphoneManager* lm = [LinphoneManager instance];
	
	if (lm.pushNotificationToken==Nil){
		[LinphoneLogger log:LinphoneLoggerLog format:@"Ignoring push notification we did not subscribed."];
		return;
	}

    // check that linphone is still running
    if( ![LinphoneManager isLcReady] )
        [lm startLibLinphone];

	[LinphoneLogger log:LinphoneLoggerLog format:@"Silent PushNotification; userInfo %@", userInfo];

    // save the completion handler for later execution.
    // 2 outcomes:
    // - if a new call/message is received, the completion handler will be called with "NEWDATA"
    // - if nothing happens for 15 seconds, the completion handler will be called with "NODATA"
    lm.silentPushCompletion = completionHandler;
    [NSTimer scheduledTimerWithTimeInterval:15.0 target:lm selector:@selector(silentPushFailed:) userInfo:nil repeats:FALSE];

	LinphoneCore *lc=[LinphoneManager getLc];
	// If no call is yet received at this time, then force Linphone to drop the current socket and make new one to register, so that we get
	// a better chance to receive the INVITE.
	if (linphone_core_get_calls(lc)==NULL){
		linphone_core_set_network_reachable(lc, FALSE);
		lm.connectivity=none; /*force connectivity to be discovered again*/
		[lm refreshRegisters];
	}
}


#pragma mark - PushNotification Functions

- (void)application:(UIApplication*)application didRegisterForRemoteNotificationsWithDeviceToken:(NSData*)deviceToken {
    [LinphoneLogger log:LinphoneLoggerLog format:@"PushNotification: Token %@", deviceToken];
    [[LinphoneManager instance] setPushNotificationToken:deviceToken];
}

- (void)application:(UIApplication*)application didFailToRegisterForRemoteNotificationsWithError:(NSError*)error {
    [LinphoneLogger log:LinphoneLoggerError format:@"PushNotification: Error %@", [error localizedDescription]];
    [[LinphoneManager instance] setPushNotificationToken:nil];
}

#pragma mark - Remote configuration Functions (URL Handler)


- (void)ConfigurationStateUpdateEvent: (NSNotification*) notif {
    LinphoneConfiguringState state = [[notif.userInfo objectForKey: @"state"] intValue];
       if (state == LinphoneConfiguringSuccessful) {
        [[NSNotificationCenter defaultCenter] removeObserver:self
                                                    name:kLinphoneConfiguringStateUpdate
                                                  object:nil];
        [_waitingIndicator dismissWithClickedButtonIndex:0 animated:true];

        UIAlertView* error = [[UIAlertView alloc] initWithTitle:NSLocalizedString(@"Success",nil)
                                                        message:NSLocalizedString(@"Remote configuration successfully fetched and applied.",nil)
                                                       delegate:nil
                                              cancelButtonTitle:NSLocalizedString(@"OK",nil)
                                              otherButtonTitles:nil];
        [error show];
        [error release];
        [[PhoneMainView instance] startUp];
    }
    if (state == LinphoneConfiguringFailed) {
        [[NSNotificationCenter defaultCenter] removeObserver:self
                                                    name:kLinphoneConfiguringStateUpdate
                                                  object:nil];
        [_waitingIndicator dismissWithClickedButtonIndex:0 animated:true];
        UIAlertView* error = [[UIAlertView alloc] initWithTitle:NSLocalizedString(@"Failure",nil)
                                                        message:NSLocalizedString(@"Failed configuring from the specified URL." ,nil)
                                                       delegate:nil
                                              cancelButtonTitle:NSLocalizedString(@"OK",nil)
                                              otherButtonTitles:nil];
        [error show];
        [error release];
        
    }
}


- (void) showWaitingIndicator {
    _waitingIndicator = [[UIAlertView alloc] initWithTitle:NSLocalizedString(@"Fetching remote configuration...",nil) message:@"" delegate:self cancelButtonTitle:nil otherButtonTitles:nil];
    UIActivityIndicatorView *progress= [[UIActivityIndicatorView alloc] initWithFrame:CGRectMake(125, 60, 30, 30)];
    progress.activityIndicatorViewStyle = UIActivityIndicatorViewStyleWhiteLarge;
    if ([[[UIDevice currentDevice] systemVersion] floatValue] >= 7.0){
        [_waitingIndicator setValue:progress forKey:@"accessoryView"];
        [progress setColor:[UIColor blackColor]];
    } else {
        [_waitingIndicator addSubview:progress];
    }
    [progress startAnimating];
    [_waitingIndicator show];

}


- (void)alertView:(UIAlertView *)alertView clickedButtonAtIndex:(NSInteger)buttonIndex
{
    if ((alertView.tag == 1) && (buttonIndex==1))  {
        [self showWaitingIndicator];
        if([LinphoneManager isLcReady]) {
            [self attemptRemoteConfiguration];
        } else {
            [[LinphoneManager instance] startLibLinphone];
            [self performSelector:@selector(attemptRemoteConfiguration) withObject:NULL afterDelay:5.0];
        }
    }
    
}

- (void)attemptRemoteConfiguration {

    if ([LinphoneManager isLcReady]) {
        [[NSNotificationCenter defaultCenter] addObserver:self
                                                 selector:@selector(ConfigurationStateUpdateEvent:)
                                                     name:kLinphoneConfiguringStateUpdate
                                                   object:nil];
        linphone_core_set_provisioning_uri([LinphoneManager getLc] , [configURL UTF8String]);
        [[LinphoneManager instance] destroyLibLinphone];
        [[LinphoneManager instance] startLibLinphone];
    } else {
        [_waitingIndicator dismissWithClickedButtonIndex:0 animated:true];
        UIAlertView* error = [[UIAlertView alloc] initWithTitle:NSLocalizedString(@"Failure",nil)
                                                        message:NSLocalizedString(@"Linphone is not ready.",nil)
                                                       delegate:nil
                                              cancelButtonTitle:NSLocalizedString(@"OK",nil)
                                              otherButtonTitles:nil];
        [error show];
        [error release];
        
    }
}


@end
