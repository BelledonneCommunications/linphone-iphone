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

#import "LinphoneAppDelegate.h"
#import "ContactDetailsView.h"
#import "ContactsListView.h"
#import "PhoneMainView.h"
#import "ShopView.h"

#import "CoreTelephony/CTCallCenter.h"
#import "CoreTelephony/CTCall.h"

#import "LinphoneCoreSettingsStore.h"

#include "LinphoneManager.h"
#include "linphone/linphonecore.h"

#ifdef USE_CRASHLYTHICSS
#include "FIRApp.h"
#endif

@implementation LinphoneAppDelegate

@synthesize configURL;
@synthesize window;

#pragma mark - Lifecycle Functions

- (id)init {
	self = [super init];
	if (self != nil) {
		startedInBackground = FALSE;
	}
	_alreadyRegisteredForNotification = false;
    _onlyPortrait = FALSE;
	return self;
	[[UIApplication sharedApplication] setDelegate:self];
}

#pragma mark -

- (void)applicationDidEnterBackground:(UIApplication *)application {
	LOGI(@"%@", NSStringFromSelector(_cmd));
	[LinphoneManager.instance enterBackgroundMode];
}

- (void)applicationWillResignActive:(UIApplication *)application {
	LOGI(@"%@", NSStringFromSelector(_cmd));
	LinphoneCall *call = linphone_core_get_current_call(LC);

	if (!call)
		return;

	/* save call context */
	LinphoneManager *instance = LinphoneManager.instance;
	instance->currentCallContextBeforeGoingBackground.call = call;
	instance->currentCallContextBeforeGoingBackground.cameraIsEnabled = linphone_call_camera_enabled(call);

	const LinphoneCallParams *params = linphone_call_get_current_params(call);
	if (linphone_call_params_video_enabled(params))
		linphone_call_enable_camera(call, false);
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

	if (instance.fastAddressBook.needToUpdate) {
		//Update address book for external changes
		if (PhoneMainView.instance.currentView == ContactsListView.compositeViewDescription || PhoneMainView.instance.currentView == ContactDetailsView.compositeViewDescription) {
			[PhoneMainView.instance changeCurrentView:DialerView.compositeViewDescription];
		}
                [instance.fastAddressBook fetchContactsInBackGroundThread];
                instance.fastAddressBook.needToUpdate = FALSE;
        }

        LinphoneCall *call = linphone_core_get_current_call(LC);

        if (call) {
          if (call == instance->currentCallContextBeforeGoingBackground.call) {
            const LinphoneCallParams *params =
                linphone_call_get_current_params(call);
            if (linphone_call_params_video_enabled(params)) {
              linphone_call_enable_camera(
                  call, instance->currentCallContextBeforeGoingBackground
                            .cameraIsEnabled);
            }
            instance->currentCallContextBeforeGoingBackground.call = 0;
          } else if (linphone_call_get_state(call) ==
                     LinphoneCallIncomingReceived) {
            LinphoneCallAppData *data =
                (__bridge LinphoneCallAppData *)linphone_call_get_user_data(
                    call);
            if (data && data->timer) {
              [data->timer invalidate];
              data->timer = nil;
            }
            if ((floor(NSFoundationVersionNumber) <= NSFoundationVersionNumber_iOS_9_x_Max)) {
              if ([LinphoneManager.instance lpConfigBoolForKey:@"autoanswer_notif_preference"]) {
                linphone_call_accept(call);
                [PhoneMainView.instance changeCurrentView:CallView.compositeViewDescription];
              } else {
                [PhoneMainView.instance displayIncomingCall:call];
              }
            } else if (linphone_core_get_calls_nb(LC) > 1) {
              [PhoneMainView.instance displayIncomingCall:call];
            }

            // in this case, the ringing sound comes from the notification.
            // To stop it we have to do the iOS7 ring fix...
            [self fixRing];
          }
        }
        [LinphoneManager.instance.iapManager check];
    if (_shortcutItem) {
        [self handleShortcut:_shortcutItem];
        _shortcutItem = nil;
    }
    [HistoryListTableView saveDataToUserDefaults];
    [ChatsListTableView saveDataToUserDefaults];
}

#pragma deploymate push "ignored-api-availability"

- (void)registerForNotifications {
	if (_alreadyRegisteredForNotification)
		return;

	_alreadyRegisteredForNotification = true;
	self.voipRegistry = [[PKPushRegistry alloc] initWithQueue:dispatch_get_main_queue()];
	self.voipRegistry.delegate = self;

	// Initiate registration.
	LOGI(@"[PushKit] Connecting for push notifications");
	self.voipRegistry.desiredPushTypes = [NSSet setWithObject:PKPushTypeVoIP];

	[self configureUINotification];
}

- (void)configureUINotification {
	if (floor(NSFoundationVersionNumber) <= NSFoundationVersionNumber_iOS_9_x_Max)
		return;

	LOGI(@"Connecting for UNNotifications");
	// Call category
	UNNotificationAction *act_ans =
	[UNNotificationAction actionWithIdentifier:@"Answer"
										 title:NSLocalizedString(@"Answer", nil)
									   options:UNNotificationActionOptionForeground];
	UNNotificationAction *act_dec = [UNNotificationAction actionWithIdentifier:@"Decline"
																		 title:NSLocalizedString(@"Decline", nil)
																	   options:UNNotificationActionOptionNone];
	UNNotificationCategory *cat_call =
	[UNNotificationCategory categoryWithIdentifier:@"call_cat"
										   actions:[NSArray arrayWithObjects:act_ans, act_dec, nil]
								 intentIdentifiers:[[NSMutableArray alloc] init]
										   options:UNNotificationCategoryOptionCustomDismissAction];
	// Msg category
	UNTextInputNotificationAction *act_reply =
	[UNTextInputNotificationAction actionWithIdentifier:@"Reply"
												  title:NSLocalizedString(@"Reply", nil)
												options:UNNotificationActionOptionNone];
	UNNotificationAction *act_seen =
	[UNNotificationAction actionWithIdentifier:@"Seen"
										 title:NSLocalizedString(@"Mark as seen", nil)
									   options:UNNotificationActionOptionNone];
	UNNotificationCategory *cat_msg =
	[UNNotificationCategory categoryWithIdentifier:@"msg_cat"
										   actions:[NSArray arrayWithObjects:act_reply, act_seen, nil]
								 intentIdentifiers:[[NSMutableArray alloc] init]
										   options:UNNotificationCategoryOptionCustomDismissAction];

	// Video Request Category
	UNNotificationAction *act_accept =
	[UNNotificationAction actionWithIdentifier:@"Accept"
										 title:NSLocalizedString(@"Accept", nil)
									   options:UNNotificationActionOptionForeground];

	UNNotificationAction *act_refuse = [UNNotificationAction actionWithIdentifier:@"Cancel"
																			title:NSLocalizedString(@"Cancel", nil)
																		  options:UNNotificationActionOptionNone];
	UNNotificationCategory *video_call =
	[UNNotificationCategory categoryWithIdentifier:@"video_request"
										   actions:[NSArray arrayWithObjects:act_accept, act_refuse, nil]
								 intentIdentifiers:[[NSMutableArray alloc] init]
										   options:UNNotificationCategoryOptionCustomDismissAction];

	// ZRTP verification category
	UNNotificationAction *act_confirm = [UNNotificationAction actionWithIdentifier:@"Confirm"
																			 title:NSLocalizedString(@"Accept", nil)
																		   options:UNNotificationActionOptionNone];

	UNNotificationAction *act_deny = [UNNotificationAction actionWithIdentifier:@"Deny"
																		  title:NSLocalizedString(@"Deny", nil)
																		options:UNNotificationActionOptionNone];
	UNNotificationCategory *cat_zrtp =
	[UNNotificationCategory categoryWithIdentifier:@"zrtp_request"
										   actions:[NSArray arrayWithObjects:act_confirm, act_deny, nil]
								 intentIdentifiers:[[NSMutableArray alloc] init]
										   options:UNNotificationCategoryOptionCustomDismissAction];

	[UNUserNotificationCenter currentNotificationCenter].delegate = self;
	[[UNUserNotificationCenter currentNotificationCenter] requestAuthorizationWithOptions:(UNAuthorizationOptionAlert | UNAuthorizationOptionSound | UNAuthorizationOptionBadge)
																		completionHandler:^(BOOL granted, NSError *_Nullable error) {
																			// Enable or disable features based on authorization.
																			if (error)
																				LOGD(error.description);
																		}];
    
	NSSet *categories = [NSSet setWithObjects:cat_call, cat_msg, video_call, cat_zrtp, nil];
	[[UNUserNotificationCenter currentNotificationCenter] setNotificationCategories:categories];
}

#pragma deploymate pop

- (BOOL)application:(UIApplication *)application didFinishLaunchingWithOptions:(NSDictionary *)launchOptions {
#ifdef USE_CRASHLYTHICSS
	[FIRApp configure];
#endif
    
    UIApplication *app = [UIApplication sharedApplication];
	UIApplicationState state = app.applicationState;

	LinphoneManager *instance = [LinphoneManager instance];
	//init logs asap
	[Log enableLogs:[[LinphoneManager instance] lpConfigIntForKey:@"debugenable_preference"]];
	
	BOOL background_mode = [instance lpConfigBoolForKey:@"backgroundmode_preference"];
	BOOL start_at_boot = [instance lpConfigBoolForKey:@"start_at_boot_preference"];
	[self registerForNotifications]; // Register for notifications must be done ASAP to give a chance for first SIP register to be done with right token. Specially true in case of remote provisionning or re-install with new type of signing certificate, like debug to release.
	if (floor(NSFoundationVersionNumber) > NSFoundationVersionNumber_iOS_9_x_Max) {
		self.del = [[ProviderDelegate alloc] init];
		[LinphoneManager.instance setProviderDelegate:self.del];
	}

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

	if (bgStartId != UIBackgroundTaskInvalid)
		[[UIApplication sharedApplication] endBackgroundTask:bgStartId];

    //Enable all notification type. VoIP Notifications don't present a UI but we will use this to show local nofications later
    UIUserNotificationSettings *notificationSettings = [UIUserNotificationSettings settingsForTypes:UIUserNotificationTypeAlert| UIUserNotificationTypeBadge | UIUserNotificationTypeSound categories:nil];

    //register the notification settings
    [application registerUserNotificationSettings:notificationSettings];

    //output what state the app is in. This will be used to see when the app is started in the background
    LOGI(@"app launched with state : %li", (long)application.applicationState);
    LOGI(@"FINISH LAUNCHING WITH OPTION : %@", launchOptions.description);
    
    UIApplicationShortcutItem *shortcutItem = [launchOptions objectForKey:@"UIApplicationLaunchOptionsShortcutItemKey"];
    if (shortcutItem) {
        _shortcutItem = shortcutItem;
        return NO;
    }

	return YES;
}

- (void)applicationWillTerminate:(UIApplication *)application {
	LOGI(@"%@", NSStringFromSelector(_cmd));
	LinphoneManager.instance.conf = TRUE;
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

- (BOOL)handleShortcut:(UIApplicationShortcutItem *)shortcutItem {
    BOOL success = NO;
    if ([shortcutItem.type isEqualToString:@"linphone.phone.action.newMessage"]) {
        [PhoneMainView.instance changeCurrentView:ChatConversationCreateView.compositeViewDescription];
        success = YES;
    }
    return success;
}

- (void)application:(UIApplication *)application performActionForShortcutItem:(UIApplicationShortcutItem *)shortcutItem completionHandler:(void (^)(BOOL))completionHandler {
    completionHandler([self handleShortcut:shortcutItem]);
}

- (BOOL)application:(UIApplication *)application openURL:(NSURL *)url options:(NSDictionary<UIApplicationOpenURLOptionsKey, id> *)options{
	NSString *scheme = [[url scheme] lowercaseString];
	if ([scheme isEqualToString:@"linphone-config"] || [scheme isEqualToString:@"linphone-config"]) {
		NSString *encodedURL =
			[[url absoluteString] stringByReplacingOccurrencesOfString:@"linphone-config://" withString:@""];
		self.configURL = [encodedURL stringByReplacingPercentEscapesUsingEncoding:NSUTF8StringEncoding];
		UIAlertController *errView = [UIAlertController alertControllerWithTitle:NSLocalizedString(@"Remote configuration", nil)
																		 message:NSLocalizedString(@"This operation will load a remote configuration. Continue ?", nil)
																  preferredStyle:UIAlertControllerStyleAlert];

		UIAlertAction* defaultAction = [UIAlertAction actionWithTitle:NSLocalizedString(@"No", nil)
																style:UIAlertActionStyleDefault
															  handler:^(UIAlertAction * action) {}];

		UIAlertAction* yesAction = [UIAlertAction actionWithTitle:NSLocalizedString(@"Yes", nil)
																style:UIAlertActionStyleDefault
														  handler:^(UIAlertAction * action) {
															  [self showWaitingIndicator];
															  [self attemptRemoteConfiguration];
														  }];

		[errView addAction:defaultAction];
		[errView addAction:yesAction];

		[PhoneMainView.instance presentViewController:errView animated:YES completion:nil];
    } else if([[url scheme] isEqualToString:@"message-linphone"]) {
        [PhoneMainView.instance popToView:ChatsListView.compositeViewDescription];
    } else if ([scheme isEqualToString:@"sip"]) {
        // remove "sip://" from the URI, and do it correctly by taking resourceSpecifier and removing leading and
        // trailing "/"
        NSString *sipUri = [[url resourceSpecifier] stringByTrimmingCharactersInSet:[NSCharacterSet characterSetWithCharactersInString:@"/"]];
        [VIEW(DialerView) setAddress:sipUri];
    } else if ([scheme isEqualToString:@"linphone-widget"]) {
        if ([[url host] isEqualToString:@"call_log"] &&
            [[url path] isEqualToString:@"/show"]) {
            [VIEW(HistoryDetailsView) setCallLogId:[url query]];
            [PhoneMainView.instance changeCurrentView:HistoryDetailsView.compositeViewDescription];
        } else if ([[url host] isEqualToString:@"chatroom"] && [[url path] isEqualToString:@"/show"]) {
            NSURLComponents *urlComponents = [NSURLComponents componentsWithURL:url
                                                        resolvingAgainstBaseURL:NO];
            NSArray *queryItems = urlComponents.queryItems;
            NSString *peerAddress = [self valueForKey:@"peer" fromQueryItems:queryItems];
            NSString *localAddress = [self valueForKey:@"local" fromQueryItems:queryItems];
            LinphoneAddress *peer = linphone_address_new(peerAddress.UTF8String);
            LinphoneAddress *local = linphone_address_new(localAddress.UTF8String);
            LinphoneChatRoom *cr = linphone_core_find_chat_room(LC, peer, local);
            linphone_address_unref(peer);
            linphone_address_unref(local);
            // TODO : Find a better fix
            VIEW(ChatConversationView).markAsRead = FALSE;
            [PhoneMainView.instance goToChatRoom:cr];
        }
    }
    return YES;
}

- (NSString *)valueForKey:(NSString *)key fromQueryItems:(NSArray *)queryItems {
    NSPredicate *predicate = [NSPredicate predicateWithFormat:@"name=%@", key];
    NSURLQueryItem *queryItem = [[queryItems filteredArrayUsingPredicate:predicate] firstObject];
    return queryItem.value;
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
	if (linphone_core_get_calls(LC)) {
		// if there are calls, obviously our TCP socket shall be working
		LOGD(@"Notification [%p] has no need to be processed because there already is an active call.", userInfo);
		return;
	}

	NSDictionary *aps = [userInfo objectForKey:@"aps"];
	if (!aps) {
		LOGE(@"Notification [%p] was empy, it's impossible to process it.", userInfo);
		return;
	}

	NSString *loc_key = [aps objectForKey:@"loc-key"] ?: [[aps objectForKey:@"alert"] objectForKey:@"loc-key"];
	if (!loc_key) {
		LOGE(@"Notification [%p] has no loc_key, it's impossible to process it.", userInfo);
		return;
	}

	NSString *uuid = [NSString stringWithFormat:@"<urn:uuid:%@>", [LinphoneManager.instance lpConfigStringForKey:@"uuid" inSection:@"misc" withDefault:NULL]];
	NSString *sipInstance = [aps objectForKey:@"uuid"];
	if (sipInstance && uuid && ![sipInstance isEqualToString:uuid]) {
		LOGE(@"Notification [%p] was intended for another device, ignoring it.", userInfo);
        LOGD(@"My sip instance is: [%@], push was intended for: [%@].", uuid, sipInstance);
		return;
	}

	NSString *callId = [aps objectForKey:@"call-id"] ?: @"";
	if ([UIApplication sharedApplication].applicationState != UIApplicationStateActive && [self addLongTaskIDforCallID:callId])
		[LinphoneManager.instance startPushLongRunningTask:loc_key callId:callId];

	// if we receive a push notification, it is probably because our TCP background socket was no more working.
	// As a result, break it and refresh registers in order to make sure to receive incoming INVITE or MESSAGE
	if (!linphone_core_is_network_reachable(LC)) {
		LOGI(@"Notification [%p] network is down, restarting it.", userInfo);
		LinphoneManager.instance.connectivity = none; // Force connectivity to be discovered again
		[LinphoneManager.instance setupNetworkReachabilityCallback];
	}

	if ([callId isEqualToString:@""]) {
		// Present apn pusher notifications for info
		LOGD(@"Notification [%p] came from flexisip-pusher.", userInfo);
		if (floor(NSFoundationVersionNumber) > NSFoundationVersionNumber_iOS_9_x_Max) {
			UNMutableNotificationContent* content = [[UNMutableNotificationContent alloc] init];
			content.title = @"APN Pusher";
			content.body = @"Push notification received !";

			UNNotificationRequest *req = [UNNotificationRequest requestWithIdentifier:@"call_request" content:content trigger:NULL];
			[[UNUserNotificationCenter currentNotificationCenter] addNotificationRequest:req withCompletionHandler:^(NSError * _Nullable error) {
				// Enable or disable features based on authorization.
				if (error) {
					LOGD(@"Error while adding notification request :");
					LOGD(error.description);
				}
			}];
		} else {
			UILocalNotification *notification = [[UILocalNotification alloc] init];
			notification.repeatInterval = 0;
			notification.alertBody = @"Push notification received !";
			notification.alertTitle = @"APN Pusher";
			[[UIApplication sharedApplication] presentLocalNotificationNow:notification];
		}
	} else
		[LinphoneManager.instance addPushCallId:callId];

    LOGI(@"Notification [%p] processed", userInfo);
}

- (BOOL)addLongTaskIDforCallID:(NSString *)callId {
	if (!callId)
		return FALSE;

	if ([callId isEqualToString:@""])
		return FALSE;

	NSDictionary *dict = LinphoneManager.instance.pushDict;
	if ([[dict allKeys] indexOfObject:callId] != NSNotFound)
		return FALSE;

	LOGI(@"Adding long running task for call id : %@ with index : 1", callId);
	[dict setValue:[NSNumber numberWithInt:1] forKey:callId];
	return TRUE;
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
		if (room_from && strcmp(from, room_from) == 0){
			ms_free(room_from);
			return rooms->data;
		}
		if (room_from) ms_free(room_from);
		rooms = rooms->next;
	}
	return NULL;
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

- (void)pushRegistry:(PKPushRegistry *)registry didUpdatePushCredentials:(PKPushCredentials *)credentials forType:(PKPushType)type {
	LOGI(@"[PushKit] credentials updated with voip token: %@", credentials.token);
	dispatch_async(dispatch_get_main_queue(), ^{
		[LinphoneManager.instance setPushNotificationToken:credentials.token];
	});
}

- (void)pushRegistry:(PKPushRegistry *)registry didInvalidatePushTokenForType:(NSString *)type {
    LOGI(@"[PushKit] Token invalidated");
    dispatch_async(dispatch_get_main_queue(), ^{[LinphoneManager.instance setPushNotificationToken:nil];});
}

- (void)processPush:(NSDictionary *)userInfo {
	LOGI(@"[PushKit] Notification [%p] received with pay load : %@", userInfo, userInfo.description);
	[self configureUINotification];
	[LinphoneManager.instance setupNetworkReachabilityCallback];
	//to avoid IOS to suspend the app before being able to launch long running task
	[self processRemoteNotification:userInfo];
}

- (void)pushRegistry:(PKPushRegistry *)registry didReceiveIncomingPushWithPayload:(PKPushPayload *)payload forType:(PKPushType)type withCompletionHandler:(void (^)(void))completion {
	[self processPush:payload.dictionaryPayload];
	dispatch_async(dispatch_get_main_queue(), ^{completion();});
}

- (void)pushRegistry:(PKPushRegistry *)registry didReceiveIncomingPushWithPayload:(PKPushPayload *)payload forType:(NSString *)type {
	[self processPush:payload.dictionaryPayload];
}

#pragma mark - UNUserNotifications Framework

- (void) userNotificationCenter:(UNUserNotificationCenter *)center willPresentNotification:(UNNotification *)notification withCompletionHandler:(void (^)(UNNotificationPresentationOptions))completionHandler {
	completionHandler(UNNotificationPresentationOptionAlert | UNNotificationPresentationOptionAlert);
}

- (void)userNotificationCenter:(UNUserNotificationCenter *)center
    didReceiveNotificationResponse:(UNNotificationResponse *)response
             withCompletionHandler:(void (^)(void))completionHandler {
	LOGD(@"UN : response received");
	LOGD(response.description);

	NSString *callId = (NSString *)[response.notification.request.content.userInfo objectForKey:@"CallId"];
	if (!callId)
		return;

	LinphoneCall *call = [LinphoneManager.instance callByCallId:callId];
	if (call) {
		LinphoneCallAppData *data = (__bridge LinphoneCallAppData *)linphone_call_get_user_data(call);
		if (data->timer) {
			[data->timer invalidate];
			data->timer = nil;
		}
	}

	if ([response.actionIdentifier isEqual:@"Answer"]) {
		// use the standard handler
		[PhoneMainView.instance changeCurrentView:CallView.compositeViewDescription];
		linphone_call_accept(call);
	} else if ([response.actionIdentifier isEqual:@"Decline"]) {
		linphone_call_decline(call, LinphoneReasonDeclined);
	} else if ([response.actionIdentifier isEqual:@"Reply"]) {
	  	NSString *replyText = [(UNTextInputNotificationResponse *)response userText];
	  	NSString *peer_address = [response.notification.request.content.userInfo objectForKey:@"peer_addr"];
	  	NSString *local_address = [response.notification.request.content.userInfo objectForKey:@"local_addr"];
	  	LinphoneAddress *peer = linphone_address_new(peer_address.UTF8String);
		LinphoneAddress *local = linphone_address_new(local_address.UTF8String);
	  	LinphoneChatRoom *room = linphone_core_find_chat_room(LC, peer, local);
	  	if(room)
		  	[LinphoneManager.instance send:replyText toChatRoom:room];

	  	linphone_address_unref(peer);
	  	linphone_address_unref(local);
  	} else if ([response.actionIdentifier isEqual:@"Seen"]) {
	  	NSString *peer_address = [response.notification.request.content.userInfo objectForKey:@"peer_addr"];
	  	NSString *local_address = [response.notification.request.content.userInfo objectForKey:@"local_addr"];
	  	LinphoneAddress *peer = linphone_address_new(peer_address.UTF8String);
	  	LinphoneAddress *local = linphone_address_new(local_address.UTF8String);
	  	LinphoneChatRoom *room = linphone_core_find_chat_room(LC, peer, local);
	  	if (room)
		  	[ChatConversationView markAsRead:room];

	  	linphone_address_unref(peer);
	  	linphone_address_unref(local);
	} else if ([response.actionIdentifier isEqual:@"Cancel"]) {
	  	LOGI(@"User declined video proposal");
	  	if (call != linphone_core_get_current_call(LC))
		  	return;

	  	LinphoneCallParams *params = linphone_core_create_call_params(LC, call);
	  	linphone_call_accept_update(call, params);
	  	linphone_call_params_destroy(params);
  	} else if ([response.actionIdentifier isEqual:@"Accept"]) {
		LOGI(@"User accept video proposal");
	  	if (call != linphone_core_get_current_call(LC))
			return;

		[[UNUserNotificationCenter currentNotificationCenter] removeAllDeliveredNotifications];
	  	[PhoneMainView.instance changeCurrentView:CallView.compositeViewDescription];
      	LinphoneCallParams *params = linphone_core_create_call_params(LC, call);
      	linphone_call_params_enable_video(params, TRUE);
      	linphone_call_accept_update(call, params);
      	linphone_call_params_destroy(params);
  	} else if ([response.actionIdentifier isEqual:@"Confirm"]) {
	  	if (linphone_core_get_current_call(LC) == call)
		  	linphone_call_set_authentication_token_verified(call, YES);
  	} else if ([response.actionIdentifier isEqual:@"Deny"]) {
	  	if (linphone_core_get_current_call(LC) == call)
		  	linphone_call_set_authentication_token_verified(call, NO);
  	} else if ([response.actionIdentifier isEqual:@"Call"]) {
	  	return;
  	} else { // in this case the value is : com.apple.UNNotificationDefaultActionIdentifier or com.apple.UNNotificationDismissActionIdentifier
	  	if ([response.notification.request.content.categoryIdentifier isEqual:@"call_cat"]) {
			if ([response.actionIdentifier isEqualToString:@"com.apple.UNNotificationDismissActionIdentifier"])
				// clear notification
				linphone_call_decline(call, LinphoneReasonDeclined);
			else
		  		[PhoneMainView.instance displayIncomingCall:call];
	  	} else if ([response.notification.request.content.categoryIdentifier isEqual:@"msg_cat"]) {
		  	NSString *peer_address = [response.notification.request.content.userInfo objectForKey:@"peer_addr"];
		  	NSString *local_address = [response.notification.request.content.userInfo objectForKey:@"local_addr"];
		  	LinphoneAddress *peer = linphone_address_new(peer_address.UTF8String);
		  	LinphoneAddress *local = linphone_address_new(local_address.UTF8String);
		  	LinphoneChatRoom *room = linphone_core_find_chat_room(LC, peer, local);
		  	if (room) {
				[PhoneMainView.instance goToChatRoom:room];
			  	return;
		  	}
		  	[PhoneMainView.instance changeCurrentView:ChatsListView.compositeViewDescription];
		} else if ([response.notification.request.content.categoryIdentifier isEqual:@"video_request"]) {
			[PhoneMainView.instance changeCurrentView:CallView.compositeViewDescription];
		  	NSTimer *videoDismissTimer = nil;
		  	UIConfirmationDialog *sheet = [UIConfirmationDialog ShowWithMessage:response.notification.request.content.body
																  cancelMessage:nil
																 confirmMessage:NSLocalizedString(@"ACCEPT", nil)
																  onCancelClick:^() {
																	  LOGI(@"User declined video proposal");
																	  if (call != linphone_core_get_current_call(LC))
																		  return;

																	  LinphoneCallParams *params = linphone_core_create_call_params(LC, call);
																	  linphone_call_accept_update(call, params);
																	  linphone_call_params_destroy(params);
																	  [videoDismissTimer invalidate];
																  }
															onConfirmationClick:^() {
																LOGI(@"User accept video proposal");
																if (call != linphone_core_get_current_call(LC))
																	return;

																LinphoneCallParams *params = linphone_core_create_call_params(LC, call);
																linphone_call_params_enable_video(params, TRUE);
																linphone_call_accept_update(call, params);
																linphone_call_params_destroy(params);
																[videoDismissTimer invalidate];
															}
																   inController:PhoneMainView.instance];

			videoDismissTimer = [NSTimer scheduledTimerWithTimeInterval:30
																 target:self
															   selector:@selector(dismissVideoActionSheet:)
															   userInfo:sheet
																repeats:NO];
		} else if ([response.notification.request.content.categoryIdentifier isEqual:@"zrtp_request"]) {
			if (!call)
				return;
			
			NSString *code = [NSString stringWithUTF8String:linphone_call_get_authentication_token(call)];
			NSString *myCode;
			NSString *correspondantCode;
			if (linphone_call_get_dir(call) == LinphoneCallIncoming) {
				myCode = [code substringToIndex:2];
				correspondantCode = [code substringFromIndex:2];
			} else {
				correspondantCode = [code substringToIndex:2];
				myCode = [code substringFromIndex:2];
			}
		  	NSString *message = [NSString stringWithFormat:NSLocalizedString(@"Confirm the following SAS with peer:\n"
																			 @"Say : %@\n"
																			 @"Your correspondant should say : %@", nil), myCode, correspondantCode];
			UIConfirmationDialog *securityDialog = [UIConfirmationDialog ShowWithMessage:message
									cancelMessage:NSLocalizedString(@"DENY", nil)
								   confirmMessage:NSLocalizedString(@"ACCEPT", nil)
									onCancelClick:^() {
										if (linphone_core_get_current_call(LC) == call)
											linphone_call_set_authentication_token_verified(call, NO);
								  	}
							  onConfirmationClick:^() {
								  if (linphone_core_get_current_call(LC) == call)
									  linphone_call_set_authentication_token_verified(call, YES);
							  }];
			[securityDialog setSpecialColor];
		} else if ([response.notification.request.content.categoryIdentifier isEqual:@"lime"]) {
			return;
        } else { // Missed call
			[PhoneMainView.instance changeCurrentView:HistoryListView.compositeViewDescription];
		}
	}
}

- (void)dismissVideoActionSheet:(NSTimer *)timer {
	UIConfirmationDialog *sheet = (UIConfirmationDialog *)timer.userInfo;
	[sheet dismiss];
}

#pragma mark - NSUser notifications
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wstrict-prototypes"

- (void)application:(UIApplication *)application
	handleActionWithIdentifier:(NSString *)identifier
		  forLocalNotification:(UILocalNotification *)notification
			 completionHandler:(void (^)())completionHandler {

	LinphoneCall *call = linphone_core_get_current_call(LC);
	if (call) {
		LinphoneCallAppData *data = (__bridge LinphoneCallAppData *)linphone_call_get_user_data(call);
		if (data->timer) {
			[data->timer invalidate];
			data->timer = nil;
		}
	}
	LOGI(@"%@", NSStringFromSelector(_cmd));
	if (floor(NSFoundationVersionNumber) < NSFoundationVersionNumber_iOS_9_0) {
		LOGI(@"%@", NSStringFromSelector(_cmd));
		if ([notification.category isEqualToString:@"incoming_call"]) {
			if ([identifier isEqualToString:@"answer"]) {
				// use the standard handler
				[PhoneMainView.instance changeCurrentView:CallView.compositeViewDescription];
				linphone_call_accept(call);
			} else if ([identifier isEqualToString:@"decline"]) {
				LinphoneCall *call = linphone_core_get_current_call(LC);
				if (call)
					linphone_call_decline(call, LinphoneReasonDeclined);
			}
		} else if ([notification.category isEqualToString:@"incoming_msg"]) {
			if ([identifier isEqualToString:@"reply"]) {
				// use the standard handler
				[PhoneMainView.instance changeCurrentView:ChatsListView.compositeViewDescription];
			} else if ([identifier isEqualToString:@"mark_read"]) {
				NSString *peer_address = [notification.userInfo objectForKey:@"peer_addr"];
				NSString *local_address = [notification.userInfo objectForKey:@"local_addr"];
				LinphoneAddress *peer = linphone_address_new(peer_address.UTF8String);
				LinphoneAddress *local = linphone_address_new(local_address.UTF8String);
				LinphoneChatRoom *room = linphone_core_find_chat_room(LC, peer, local);
				if (room)
					[ChatConversationView markAsRead:room];

				linphone_address_unref(peer);
				linphone_address_unref(local);
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

	LinphoneCall *call = linphone_core_get_current_call(LC);
	if (call) {
		LinphoneCallAppData *data = (__bridge LinphoneCallAppData *)linphone_call_get_user_data(call);
		if (data->timer) {
			[data->timer invalidate];
			data->timer = nil;
		}
	}
	if ([notification.category isEqualToString:@"incoming_call"]) {
		if ([identifier isEqualToString:@"answer"]) {
			// use the standard handler
			[PhoneMainView.instance changeCurrentView:CallView.compositeViewDescription];
			linphone_call_accept(call);
		} else if ([identifier isEqualToString:@"decline"]) {
			LinphoneCall *call = linphone_core_get_current_call(LC);
			if (call)
				linphone_call_decline(call, LinphoneReasonDeclined);
		}
	} else if ([notification.category isEqualToString:@"incoming_msg"] &&
			   [identifier isEqualToString:@"reply_inline"]) {
		NSString *replyText = [responseInfo objectForKey:UIUserNotificationActionResponseTypedTextKey];
		NSString *peer_address = [responseInfo objectForKey:@"peer_addr"];
		NSString *local_address = [responseInfo objectForKey:@"local_addr"];
		LinphoneAddress *peer = linphone_address_new(peer_address.UTF8String);
		LinphoneAddress *local = linphone_address_new(local_address.UTF8String);
		LinphoneChatRoom *room = linphone_core_find_chat_room(LC, peer, local);
		if (room)
			[LinphoneManager.instance send:replyText toChatRoom:room];

		linphone_address_unref(peer);
		linphone_address_unref(local);
	}
	completionHandler();
}
#pragma clang diagnostic pop
#pragma deploymate pop

#pragma mark - Remote configuration Functions (URL Handler)

- (void)ConfigurationStateUpdateEvent:(NSNotification *)notif {
	LinphoneConfiguringState state = [[notif.userInfo objectForKey:@"state"] intValue];
	if (state == LinphoneConfiguringSuccessful) {
		[NSNotificationCenter.defaultCenter removeObserver:self name:kLinphoneConfiguringStateUpdate object:nil];
		[_waitingIndicator dismissViewControllerAnimated:YES completion:nil];
		UIAlertController *errView = [UIAlertController alertControllerWithTitle:NSLocalizedString(@"Success", nil)
																		 message:NSLocalizedString(@"Remote configuration successfully fetched and applied.", nil)
																  preferredStyle:UIAlertControllerStyleAlert];

		UIAlertAction* defaultAction = [UIAlertAction actionWithTitle:NSLocalizedString(@"OK", nil)
																style:UIAlertActionStyleDefault
															  handler:^(UIAlertAction * action) {}];

		[errView addAction:defaultAction];
		[PhoneMainView.instance presentViewController:errView animated:YES completion:nil];

		[PhoneMainView.instance startUp];
	}
	if (state == LinphoneConfiguringFailed) {
		[NSNotificationCenter.defaultCenter removeObserver:self name:kLinphoneConfiguringStateUpdate object:nil];
		[_waitingIndicator dismissViewControllerAnimated:YES completion:nil];
		UIAlertController *errView = [UIAlertController alertControllerWithTitle:NSLocalizedString(@"Failure", nil)
																		 message:NSLocalizedString(@"Failed configuring from the specified URL.", nil)
																  preferredStyle:UIAlertControllerStyleAlert];

		UIAlertAction* defaultAction = [UIAlertAction actionWithTitle:NSLocalizedString(@"OK", nil)
																style:UIAlertActionStyleDefault
															  handler:^(UIAlertAction * action) {}];

		[errView addAction:defaultAction];
		[PhoneMainView.instance presentViewController:errView animated:YES completion:nil];
	}
}

- (void)showWaitingIndicator {
	_waitingIndicator = [UIAlertController alertControllerWithTitle:NSLocalizedString(@"Fetching remote configuration...", nil)
															message:@""
													 preferredStyle:UIAlertControllerStyleAlert];

	UIActivityIndicatorView *progress = [[UIActivityIndicatorView alloc] initWithFrame:CGRectMake(125, 60, 30, 30)];
	progress.activityIndicatorViewStyle = UIActivityIndicatorViewStyleWhiteLarge;

	[_waitingIndicator setValue:progress forKey:@"accessoryView"];
	[progress setColor:[UIColor blackColor]];

	[progress startAnimating];
	[PhoneMainView.instance presentViewController:_waitingIndicator animated:YES completion:nil];
}

- (void)attemptRemoteConfiguration {

	[NSNotificationCenter.defaultCenter addObserver:self
										   selector:@selector(ConfigurationStateUpdateEvent:)
											   name:kLinphoneConfiguringStateUpdate
											 object:nil];
	linphone_core_set_provisioning_uri(LC, [configURL UTF8String]);
	[LinphoneManager.instance destroyLinphoneCore];
	[LinphoneManager.instance startLinphoneCore];
        [LinphoneManager.instance.fastAddressBook fetchContactsInBackGroundThread];
}

#pragma mark - Prevent ImagePickerView from rotating

- (UIInterfaceOrientationMask)application:(UIApplication *)application supportedInterfaceOrientationsForWindow:(UIWindow *)window {
	if ([[(PhoneMainView*)self.window.rootViewController currentView] equal:ImagePickerView.compositeViewDescription] || _onlyPortrait)
	{
		//Prevent rotation of camera
		NSNumber *value = [NSNumber numberWithInt:UIInterfaceOrientationPortrait];
		[[UIDevice currentDevice] setValue:value forKey:@"orientation"];
		return UIInterfaceOrientationMaskPortrait;
	} else return UIInterfaceOrientationMaskAllButUpsideDown;
}

@end
