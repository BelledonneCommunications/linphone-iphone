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
#import "ShopView.h"
#import "linphoneAppDelegate.h"
#import "AddressBook/ABPerson.h"
#import <PushKit/PushKit.h>

#import "CoreTelephony/CTCallCenter.h"
#import "CoreTelephony/CTCall.h"

#import "LinphoneCoreSettingsStore.h"

#include "LinphoneManager.h"
#include "linphone/linphonecore.h"

@implementation LinphoneAppDelegate

@synthesize configURL;
@synthesize window;

#pragma mark - Lifecycle Functions

- (id)init {
	self = [super init];
	if (self != nil) {
		startedInBackground = FALSE;
	}
	return self;
}

#pragma mark -

- (void)applicationDidEnterBackground:(UIApplication *)application {
	LOGI(@"%@", NSStringFromSelector(_cmd));
	[LinphoneManager.instance enterBackgroundMode];
}

- (void)applicationWillResignActive:(UIApplication *)application {
	LOGI(@"%@", NSStringFromSelector(_cmd));
	LinphoneCall *call = linphone_core_get_current_call(LC);

	if (call) {
		/* save call context */
		LinphoneManager *instance = LinphoneManager.instance;
		instance->currentCallContextBeforeGoingBackground.call = call;
		instance->currentCallContextBeforeGoingBackground.cameraIsEnabled = linphone_call_camera_enabled(call);

		const LinphoneCallParams *params = linphone_call_get_current_params(call);
		if (linphone_call_params_video_enabled(params)) {
			linphone_call_enable_camera(call, false);
		}
	}

	if (![LinphoneManager.instance resignActive]) {
	}
}

- (void)applicationDidBecomeActive:(UIApplication *)application {
	LOGI(@"%@", NSStringFromSelector(_cmd));

	if (startedInBackground) {
		startedInBackground = FALSE;
		[PhoneMainView.instance startUp];
		[PhoneMainView.instance updateStatusBar:nil];
	}
	LinphoneManager *instance = LinphoneManager.instance;

	[instance becomeActive];

	LinphoneCall *call = linphone_core_get_current_call(LC);

	if (call) {
		if (call == instance->currentCallContextBeforeGoingBackground.call) {
			const LinphoneCallParams *params = linphone_call_get_current_params(call);
			if (linphone_call_params_video_enabled(params)) {
				linphone_call_enable_camera(call, instance->currentCallContextBeforeGoingBackground.cameraIsEnabled);
			}
			instance->currentCallContextBeforeGoingBackground.call = 0;
		} else if (linphone_call_get_state(call) == LinphoneCallIncomingReceived) {
			[PhoneMainView.instance displayIncomingCall:call];
			// in this case, the ringing sound comes from the notification.
			// To stop it we have to do the iOS7 ring fix...
			[self fixRing];
		}
	}
	[LinphoneManager.instance.iapManager check];
}

#pragma deploymate push "ignored-api-availability"
- (UIUserNotificationCategory *)getMessageNotificationCategory {
	NSArray *actions;

	if ([[UIDevice.currentDevice systemVersion] floatValue] < 9 ||
		[LinphoneManager.instance lpConfigBoolForKey:@"show_msg_in_notif"] == NO) {

		UIMutableUserNotificationAction *reply = [[UIMutableUserNotificationAction alloc] init];
		reply.identifier = @"reply";
		reply.title = NSLocalizedString(@"Reply", nil);
		reply.activationMode = UIUserNotificationActivationModeForeground;
		reply.destructive = NO;
		reply.authenticationRequired = YES;

		UIMutableUserNotificationAction *mark_read = [[UIMutableUserNotificationAction alloc] init];
		mark_read.identifier = @"mark_read";
		mark_read.title = NSLocalizedString(@"Mark Read", nil);
		mark_read.activationMode = UIUserNotificationActivationModeBackground;
		mark_read.destructive = NO;
		mark_read.authenticationRequired = NO;

		actions = @[ mark_read, reply ];
	} else {
		// iOS 9 allows for inline reply. We don't propose mark_read in this case
		UIMutableUserNotificationAction *reply_inline = [[UIMutableUserNotificationAction alloc] init];

		reply_inline.identifier = @"reply_inline";
		reply_inline.title = NSLocalizedString(@"Reply", nil);
		reply_inline.activationMode = UIUserNotificationActivationModeBackground;
		reply_inline.destructive = NO;
		reply_inline.authenticationRequired = NO;
		reply_inline.behavior = UIUserNotificationActionBehaviorTextInput;

		actions = @[ reply_inline ];
	}

	UIMutableUserNotificationCategory *localRingNotifAction = [[UIMutableUserNotificationCategory alloc] init];
	localRingNotifAction.identifier = @"incoming_msg";
	[localRingNotifAction setActions:actions forContext:UIUserNotificationActionContextDefault];
	[localRingNotifAction setActions:actions forContext:UIUserNotificationActionContextMinimal];

	return localRingNotifAction;
}

- (UIUserNotificationCategory *)getCallNotificationCategory {
	UIMutableUserNotificationAction *answer = [[UIMutableUserNotificationAction alloc] init];
	answer.identifier = @"answer";
	answer.title = NSLocalizedString(@"Answer", nil);
	answer.activationMode = UIUserNotificationActivationModeForeground;
	answer.destructive = NO;
	answer.authenticationRequired = YES;

	UIMutableUserNotificationAction *decline = [[UIMutableUserNotificationAction alloc] init];
	decline.identifier = @"decline";
	decline.title = NSLocalizedString(@"Decline", nil);
	decline.activationMode = UIUserNotificationActivationModeBackground;
	decline.destructive = YES;
	decline.authenticationRequired = NO;

	NSArray *localRingActions = @[ decline, answer ];

	UIMutableUserNotificationCategory *localRingNotifAction = [[UIMutableUserNotificationCategory alloc] init];
	localRingNotifAction.identifier = @"incoming_call";
	[localRingNotifAction setActions:localRingActions forContext:UIUserNotificationActionContextDefault];
	[localRingNotifAction setActions:localRingActions forContext:UIUserNotificationActionContextMinimal];

	return localRingNotifAction;
}

- (UIUserNotificationCategory *)getAccountExpiryNotificationCategory {
	
	UIMutableUserNotificationCategory *expiryNotification = [[UIMutableUserNotificationCategory alloc] init];
	expiryNotification.identifier = @"expiry_notification";
	return expiryNotification;
}


- (void)registerForNotifications:(UIApplication *)app {
	LinphoneManager *instance = [LinphoneManager instance];
	if (floor(NSFoundationVersionNumber) >= NSFoundationVersionNumber_iOS_8_0) {
        //[app unregisterForRemoteNotifications];
		// iOS8 and more : PushKit
        self.voipRegistry = [[PKPushRegistry alloc] initWithQueue:dispatch_get_main_queue()];
        self.voipRegistry.delegate = self;
        
        // Initiate registration.
        self.voipRegistry.desiredPushTypes = [NSSet setWithObject:PKPushTypeVoIP];
	} else {
		/* iOS7 and below */
        self.voipRegistry.delegate = NULL;
		if (!instance.isTesting) {
			NSUInteger notifTypes =
				UIRemoteNotificationTypeAlert | UIRemoteNotificationTypeSound | UIRemoteNotificationTypeBadge;
			[app registerForRemoteNotificationTypes:notifTypes];
		}
	}
}
#pragma deploymate pop

- (BOOL)application:(UIApplication *)application didFinishLaunchingWithOptions:(NSDictionary *)launchOptions {
	UIApplication *app = [UIApplication sharedApplication];
	UIApplicationState state = app.applicationState;

	LinphoneManager *instance = [LinphoneManager instance];
	BOOL background_mode = [instance lpConfigBoolForKey:@"backgroundmode_preference"];
	BOOL start_at_boot = [instance lpConfigBoolForKey:@"start_at_boot_preference"];
	[self registerForNotifications:app];

	if (state == UIApplicationStateBackground) {
		// we've been woken up directly to background;
		if (!start_at_boot || !background_mode) {
			// autoboot disabled or no background, and no push: do nothing and wait for a real launch
			//output a log with NSLog, because the ortp logging system isn't activated yet at this time
			NSLog(@"Linphone launch doing nothing because start_at_boot or background_mode are not activated.", NULL);
			return YES;
		}
	}
	bgStartId = [[UIApplication sharedApplication] beginBackgroundTaskWithExpirationHandler:^{
	  LOGW(@"Background task for application launching expired.");
	  [[UIApplication sharedApplication] endBackgroundTask:bgStartId];
	}];

	[LinphoneManager.instance startLinphoneCore];
	LinphoneManager.instance.iapManager.notificationCategory = @"expiry_notification";
	// initialize UI
	[self.window makeKeyAndVisible];
	[RootViewManager setupWithPortrait:(PhoneMainView *)self.window.rootViewController];
	[PhoneMainView.instance startUp];
	[PhoneMainView.instance updateStatusBar:nil];

	NSDictionary *remoteNotif = [launchOptions objectForKey:UIApplicationLaunchOptionsRemoteNotificationKey];
	if (remoteNotif) {
		LOGI(@"PushNotification from launch received.");
		[self processRemoteNotification:remoteNotif];
	}
	if (bgStartId != UIBackgroundTaskInvalid)
		[[UIApplication sharedApplication] endBackgroundTask:bgStartId];
    
    //Enable all notification type. VoIP Notifications don't present a UI but we will use this to show local nofications later
    UIUserNotificationSettings *notificationSettings = [UIUserNotificationSettings settingsForTypes:UIUserNotificationTypeAlert| UIUserNotificationTypeBadge | UIUserNotificationTypeSound categories:nil];
    
    //register the notification settings
    [application registerUserNotificationSettings:notificationSettings];
    
    //output what state the app is in. This will be used to see when the app is started in the background
    NSLog(@"app launched with state : %li", (long)application.applicationState);
    
	return YES;
}

- (void)applicationWillTerminate:(UIApplication *)application {
	LOGI(@"%@", NSStringFromSelector(_cmd));

	linphone_core_terminate_all_calls(LC);

	// destroyLinphoneCore automatically unregister proxies but if we are using
	// remote push notifications, we want to continue receiving them
	if (LinphoneManager.instance.pushNotificationToken != nil) {
		// trick me! setting network reachable to false will avoid sending unregister
		const MSList *proxies = linphone_core_get_proxy_config_list(LC);
		BOOL pushNotifEnabled = NO;
		while (proxies) {
			const char *refkey = linphone_proxy_config_get_ref_key(proxies->data);
			pushNotifEnabled = pushNotifEnabled || (refkey && strcmp(refkey, "push_notification") == 0);
			proxies = proxies->next;
		}
		// but we only want to hack if at least one proxy config uses remote push..
		if (pushNotifEnabled) {
			linphone_core_set_network_reachable(LC, FALSE);
		}
	}

	[LinphoneManager.instance destroyLinphoneCore];
}

- (BOOL)application:(UIApplication *)application handleOpenURL:(NSURL *)url {
	NSString *scheme = [[url scheme] lowercaseString];
	if ([scheme isEqualToString:@"linphone-config"] || [scheme isEqualToString:@"linphone-config"]) {
		NSString *encodedURL =
			[[url absoluteString] stringByReplacingOccurrencesOfString:@"linphone-config://" withString:@""];
		self.configURL = [encodedURL stringByReplacingPercentEscapesUsingEncoding:NSUTF8StringEncoding];
		UIAlertView *confirmation = [[UIAlertView alloc]
				initWithTitle:NSLocalizedString(@"Remote configuration", nil)
					  message:NSLocalizedString(@"This operation will load a remote configuration. Continue ?", nil)
					 delegate:self
			cancelButtonTitle:NSLocalizedString(@"No", nil)
			otherButtonTitles:NSLocalizedString(@"Yes", nil), nil];
		confirmation.tag = 1;
		[confirmation show];
	} else {
		if ([[url scheme] isEqualToString:@"sip"]) {
			// remove "sip://" from the URI, and do it correctly by taking resourceSpecifier and removing leading and
			// trailing "/"
			NSString *sipUri = [[url resourceSpecifier]
				stringByTrimmingCharactersInSet:[NSCharacterSet characterSetWithCharactersInString:@"/"]];
			[VIEW(DialerView) setAddress:sipUri];
		}
	}
	return YES;
}

- (void)fixRing {
	if ([[[UIDevice currentDevice] systemVersion] floatValue] >= 7) {
		// iOS7 fix for notification sound not stopping.
		// see http://stackoverflow.com/questions/19124882/stopping-ios-7-remote-notification-sound
		[[UIApplication sharedApplication] setApplicationIconBadgeNumber:1];
		[[UIApplication sharedApplication] setApplicationIconBadgeNumber:0];
	}
}

- (void)processRemoteNotification:(NSDictionary *)userInfo {

	NSDictionary *aps = [userInfo objectForKey:@"aps"];

	if (aps != nil) {
		NSDictionary *alert = [aps objectForKey:@"alert"];
		if (alert != nil) {
			NSString *loc_key = [alert objectForKey:@"loc-key"];
			/*if we receive a remote notification, it is probably because our TCP background socket was no more working.
			 As a result, break it and refresh registers in order to make sure to receive incoming INVITE or MESSAGE*/
			if (linphone_core_get_calls(LC) == NULL) { // if there are calls, obviously our TCP socket shall be working
				linphone_core_set_network_reachable(LC, FALSE);
                LinphoneManager.instance.connectivity = none; /*force connectivity to be discovered again*/
                LOGI(@"Registers refreshing");
				[LinphoneManager.instance refreshRegisters];
                LOGI(@"Registers refreshed");
				if (loc_key != nil) {

					NSString *callId = [userInfo objectForKey:@"call-id"];
					if (callId != nil) {
						[LinphoneManager.instance addPushCallId:callId];
					} else {
						LOGE(@"PushNotification: does not have call-id yet, fix it !");
					}

					if ([loc_key isEqualToString:@"IM_MSG"] || [loc_key isEqualToString:@"IM_FULLMSG"]) {
						[PhoneMainView.instance changeCurrentView:ChatsListView.compositeViewDescription];
					} else if ([loc_key isEqualToString:@"IC_MSG"]) {
						[self fixRing];
					}
				}
			}
		}
	}
    LOGI(@"Notification %@ processed", userInfo.description);
}

- (void)application:(UIApplication *)application didReceiveRemoteNotification:(NSDictionary *)userInfo {
	LOGI(@"%@ : %@", NSStringFromSelector(_cmd), userInfo);

	[self processRemoteNotification:userInfo];
}

- (LinphoneChatRoom *)findChatRoomForContact:(NSString *)contact {
	const MSList *rooms = linphone_core_get_chat_rooms(LC);
	const char *from = [contact UTF8String];
	while (rooms) {
		const LinphoneAddress *room_from_address = linphone_chat_room_get_peer_address((LinphoneChatRoom *)rooms->data);
		char *room_from = linphone_address_as_string_uri_only(room_from_address);
		if (room_from && strcmp(from, room_from) == 0) {
			return rooms->data;
		}
		rooms = rooms->next;
	}
	return NULL;
}

- (void)application:(UIApplication *)application didReceiveLocalNotification:(UILocalNotification *)notification {
	LOGI(@"%@ - state = %ld", NSStringFromSelector(_cmd), (long)application.applicationState);
	
	if ([notification.category isEqual:LinphoneManager.instance.iapManager.notificationCategory]){
		[PhoneMainView.instance changeCurrentView:ShopView.compositeViewDescription];
		return;
	}

	[self fixRing];

	if ([notification.userInfo objectForKey:@"callId"] != nil) {
		BOOL bypass_incoming_view = TRUE;
		// some local notifications have an internal timer to relaunch themselves at specified intervals
		if ([[notification.userInfo objectForKey:@"timer"] intValue] == 1) {
			[LinphoneManager.instance cancelLocalNotifTimerForCallId:[notification.userInfo objectForKey:@"callId"]];
			bypass_incoming_view = [LinphoneManager.instance lpConfigBoolForKey:@"autoanswer_notif_preference"];
		}
		if (bypass_incoming_view) {
			[LinphoneManager.instance acceptCallForCallId:[notification.userInfo objectForKey:@"callId"]];
		}
	} else if ([notification.userInfo objectForKey:@"from_addr"] != nil) {
		NSString *remoteContact = (NSString *)[notification.userInfo objectForKey:@"from_addr"];
		LinphoneChatRoom *room = [self findChatRoomForContact:remoteContact];
		ChatConversationView *view = VIEW(ChatConversationView);
		[view setChatRoom:room];
		[PhoneMainView.instance changeCurrentView:view.compositeViewDescription];
	} else if ([notification.userInfo objectForKey:@"callLog"] != nil) {
		NSString *callLog = (NSString *)[notification.userInfo objectForKey:@"callLog"];
		HistoryDetailsView *view = VIEW(HistoryDetailsView);
		[view setCallLogId:callLog];
		[PhoneMainView.instance changeCurrentView:view.compositeViewDescription];
	}
}

// this method is implemented for iOS7. It is invoked when receiving a push notification for a call and it has
// "content-available" in the aps section.
- (void)application:(UIApplication *)application
	didReceiveRemoteNotification:(NSDictionary *)userInfo
		  fetchCompletionHandler:(void (^)(UIBackgroundFetchResult))completionHandler {
	LOGI(@"%@ : %@", NSStringFromSelector(_cmd), userInfo);
	LinphoneManager *lm = LinphoneManager.instance;

	// save the completion handler for later execution.
	// 2 outcomes:
	// - if a new call/message is received, the completion handler will be called with "NEWDATA"
	// - if nothing happens for 15 seconds, the completion handler will be called with "NODATA"
	lm.silentPushCompletion = completionHandler;
	[NSTimer scheduledTimerWithTimeInterval:15.0
									 target:lm
								   selector:@selector(silentPushFailed:)
								   userInfo:nil
									repeats:FALSE];

	// If no call is yet received at this time, then force Linphone to drop the current socket and make new one to
	// register, so that we get
	// a better chance to receive the INVITE.
	if (linphone_core_get_calls(LC) == NULL) {
		linphone_core_set_network_reachable(LC, FALSE);
		lm.connectivity = none; /*force connectivity to be discovered again*/
		[lm refreshRegisters];
	}
}

#pragma mark - PushNotification Functions

- (void)application:(UIApplication *)application
	didRegisterForRemoteNotificationsWithDeviceToken:(NSData *)deviceToken {
	LOGI(@"%@ : %@", NSStringFromSelector(_cmd), deviceToken);
	[LinphoneManager.instance setPushNotificationToken:deviceToken];
}

- (void)application:(UIApplication *)application didFailToRegisterForRemoteNotificationsWithError:(NSError *)error {
	LOGI(@"%@ : %@", NSStringFromSelector(_cmd), [error localizedDescription]);
	[LinphoneManager.instance setPushNotificationToken:nil];
}

#pragma mark - PushKit Functions

- (void)pushRegistry:(PKPushRegistry *)registry
didInvalidatePushTokenForType:(NSString *)type {
    LOGI(@"PushKit Token invalidated");
    [LinphoneManager.instance setPushNotificationToken:nil];
}

- (void)pushRegistry:(PKPushRegistry *)registry
    didReceiveIncomingPushWithPayload:(PKPushPayload *)payload
             forType:(NSString *)type {
    LOGI(@"PushKit received with payload : %@", payload.description);
    
    /*NSDictionary *payloadDict = payload.dictionaryPayload[@"aps"];
    NSString *message = payloadDict[@"alert"];
    
    //present a local notifcation to visually see when we are recieving a VoIP Notification
    if ([[UIApplication sharedApplication] applicationState] == UIApplicationStateBackground) {
        
        UILocalNotification *localNotification = [UILocalNotification new];
        localNotification.alertBody = message;
        localNotification.applicationIconBadgeNumber = 1;
        localNotification.soundName = UILocalNotificationDefaultSoundName;
        
        [[UIApplication sharedApplication] presentLocalNotificationNow:localNotification];
    }*/

    LOGI(@"incoming voip notfication: %@ ", payload.dictionaryPayload);
    dispatch_async(dispatch_get_main_queue(), ^{[self processRemoteNotification:payload.dictionaryPayload];});
}

- (void)pushRegistry:(PKPushRegistry *)registry
    didUpdatePushCredentials:(PKPushCredentials *)credentials
             forType:(NSString *)type {
    LOGI(@"PushKit credentials updated");
    LOGI(@"voip token: %@", (credentials.token));
    LOGI(@"%@ : %@", NSStringFromSelector(_cmd), credentials.token);
    [LinphoneManager.instance setPushNotificationToken:credentials.token];
}

#pragma mark - User notifications

/*#pragma deploymate push "ignored-api-availability"
- (void)application:(UIApplication *)application
	didRegisterUserNotificationSettings:(UIUserNotificationSettings *)notificationSettings {
    //register for voip notifiactions
    self.voipRegistry = [[PKPushRegistry alloc] initWithQueue:dispatch_get_main_queue()];
    
    // Initiate registration.
    NSLog(@"Initiating PushKit registration");
    self.voipRegistry.desiredPushTypes = [NSSet setWithObject:PKPushTypeVoIP];
    self.voipRegistry.delegate = self;
	LOGI(@"%@", NSStringFromSelector(_cmd));
}*/

- (void)application:(UIApplication *)application
	handleActionWithIdentifier:(NSString *)identifier
		  forLocalNotification:(UILocalNotification *)notification
			 completionHandler:(void (^)())completionHandler {
	LOGI(@"%@", NSStringFromSelector(_cmd));
	if ([[UIDevice currentDevice].systemVersion floatValue] >= 8) {
		LOGI(@"%@", NSStringFromSelector(_cmd));
		if ([notification.category isEqualToString:@"incoming_call"]) {
			if ([identifier isEqualToString:@"answer"]) {
				// use the standard handler
				[self application:application didReceiveLocalNotification:notification];
			} else if ([identifier isEqualToString:@"decline"]) {
				LinphoneCall *call = linphone_core_get_current_call(LC);
				if (call)
					linphone_core_decline_call(LC, call, LinphoneReasonDeclined);
			}
		} else if ([notification.category isEqualToString:@"incoming_msg"]) {
			if ([identifier isEqualToString:@"reply"]) {
				// use the standard handler
				[self application:application didReceiveLocalNotification:notification];
			} else if ([identifier isEqualToString:@"mark_read"]) {
				NSString *from = [notification.userInfo objectForKey:@"from_addr"];
				LinphoneChatRoom *room = linphone_core_get_chat_room_from_uri(LC, [from UTF8String]);
				if (room) {
					linphone_chat_room_mark_as_read(room);
					TabBarView *tab = (TabBarView *)[PhoneMainView.instance.mainViewController
						getCachedController:NSStringFromClass(TabBarView.class)];
					[tab update:YES];
					[PhoneMainView.instance updateApplicationBadgeNumber];
				}
			}
		}
	}
	completionHandler();
}

- (void)application:(UIApplication *)application
	handleActionWithIdentifier:(NSString *)identifier
		  forLocalNotification:(UILocalNotification *)notification
			  withResponseInfo:(NSDictionary *)responseInfo
			 completionHandler:(void (^)())completionHandler {

	if ([notification.category isEqualToString:@"incoming_call"]) {
		if ([identifier isEqualToString:@"answer"]) {
			// use the standard handler
			[self application:application didReceiveLocalNotification:notification];
		} else if ([identifier isEqualToString:@"decline"]) {
			LinphoneCall *call = linphone_core_get_current_call(LC);
			if (call)
				linphone_core_decline_call(LC, call, LinphoneReasonDeclined);
		}
	} else if ([notification.category isEqualToString:@"incoming_msg"] &&
			   [identifier isEqualToString:@"reply_inline"]) {
		LinphoneCore *lc = [LinphoneManager getLc];
		NSString *replyText = [responseInfo objectForKey:UIUserNotificationActionResponseTypedTextKey];
		NSString *from = [notification.userInfo objectForKey:@"from_addr"];
		LinphoneChatRoom *room = linphone_core_get_chat_room_from_uri(lc, [from UTF8String]);
		if (room) {
			LinphoneChatMessage *msg = linphone_chat_room_create_message(room, replyText.UTF8String);
			linphone_chat_room_send_chat_message(room, msg);
			linphone_chat_room_mark_as_read(room);
			[PhoneMainView.instance updateApplicationBadgeNumber];
		}
	}
	completionHandler();
}

- (void)application:(UIApplication *)application
	handleActionWithIdentifier:(NSString *)identifier
		 forRemoteNotification:(NSDictionary *)userInfo
			 completionHandler:(void (^)())completionHandler {
	LOGI(@"%@", NSStringFromSelector(_cmd));
	completionHandler();
}
#pragma deploymate pop

#pragma mark - Remote configuration Functions (URL Handler)

- (void)ConfigurationStateUpdateEvent:(NSNotification *)notif {
	LinphoneConfiguringState state = [[notif.userInfo objectForKey:@"state"] intValue];
	if (state == LinphoneConfiguringSuccessful) {
		[NSNotificationCenter.defaultCenter removeObserver:self name:kLinphoneConfiguringStateUpdate object:nil];
		[_waitingIndicator dismissWithClickedButtonIndex:0 animated:true];

		UIAlertView *error = [[UIAlertView alloc]
				initWithTitle:NSLocalizedString(@"Success", nil)
					  message:NSLocalizedString(@"Remote configuration successfully fetched and applied.", nil)
					 delegate:nil
			cancelButtonTitle:NSLocalizedString(@"OK", nil)
			otherButtonTitles:nil];
		[error show];
		[PhoneMainView.instance startUp];
	}
	if (state == LinphoneConfiguringFailed) {
		[NSNotificationCenter.defaultCenter removeObserver:self name:kLinphoneConfiguringStateUpdate object:nil];
		[_waitingIndicator dismissWithClickedButtonIndex:0 animated:true];
		UIAlertView *error =
			[[UIAlertView alloc] initWithTitle:NSLocalizedString(@"Failure", nil)
									   message:NSLocalizedString(@"Failed configuring from the specified URL.", nil)
									  delegate:nil
							 cancelButtonTitle:NSLocalizedString(@"OK", nil)
							 otherButtonTitles:nil];
		[error show];
	}
}

- (void)showWaitingIndicator {
	_waitingIndicator = [[UIAlertView alloc] initWithTitle:NSLocalizedString(@"Fetching remote configuration...", nil)
												   message:@""
												  delegate:self
										 cancelButtonTitle:nil
										 otherButtonTitles:nil];
	UIActivityIndicatorView *progress = [[UIActivityIndicatorView alloc] initWithFrame:CGRectMake(125, 60, 30, 30)];
	progress.activityIndicatorViewStyle = UIActivityIndicatorViewStyleWhiteLarge;
	if ([[[UIDevice currentDevice] systemVersion] floatValue] >= 7.0) {
		[_waitingIndicator setValue:progress forKey:@"accessoryView"];
		[progress setColor:[UIColor blackColor]];
	} else {
		[_waitingIndicator addSubview:progress];
	}
	[progress startAnimating];
	[_waitingIndicator show];
}

- (void)alertView:(UIAlertView *)alertView clickedButtonAtIndex:(NSInteger)buttonIndex {
	if ((alertView.tag == 1) && (buttonIndex == 1)) {
		[self showWaitingIndicator];
		[self attemptRemoteConfiguration];
	}
}

- (void)attemptRemoteConfiguration {

	[NSNotificationCenter.defaultCenter addObserver:self
										   selector:@selector(ConfigurationStateUpdateEvent:)
											   name:kLinphoneConfiguringStateUpdate
											 object:nil];
	linphone_core_set_provisioning_uri(LC, [configURL UTF8String]);
	[LinphoneManager.instance destroyLinphoneCore];
	[LinphoneManager.instance startLinphoneCore];
}

@end
