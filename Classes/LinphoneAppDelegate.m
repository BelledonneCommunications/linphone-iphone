/*
 * Copyright (c) 2010-2020 Belledonne Communications SARL.
 *
 * This file is part of linphone-iphone
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
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

#import <Intents/Intents.h>
#import <IntentsUI/IntentsUI.h>
#import "linphoneapp-Swift.h"


#ifdef USE_CRASHLYTICS
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
	CallManager.instance.alreadyRegisteredForNotification = false;
    _onlyPortrait = FALSE;
	return self;
	[[UIApplication sharedApplication] setDelegate:self];
}

#pragma mark -

- (void)applicationDidEnterBackground:(UIApplication *)application {
	LOGI(@"%@", NSStringFromSelector(_cmd));
	if (linphone_core_get_global_state(LC) != LinphoneGlobalOff) {
		[LinphoneManager.instance enterBackgroundMode];
		[LinphoneManager.instance.fastAddressBook clearFriends];
		if (PhoneMainView.instance.currentView == ChatConversationView.compositeViewDescription) {
			ChatConversationView *view = VIEW(ChatConversationView);
			[view removeCallBacks];
		} else if (PhoneMainView.instance.currentView == ChatConversationInfoView.compositeViewDescription) {
			ChatConversationInfoView *view = VIEW(ChatConversationInfoView);
			[view removeCallbacks];
		} else if (PhoneMainView.instance.currentView == RecordingsListView.compositeViewDescription || PhoneMainView.instance.currentView == DevicesListView.compositeViewDescription) {
			// To avoid crash
			[PhoneMainView.instance changeCurrentView:DialerView.compositeViewDescription];
		}
		[CallManager.instance stopLinphoneCore];
	}
}

- (void)applicationWillEnterForeground:(UIApplication *)application {
	LOGI(@"%@", NSStringFromSelector(_cmd));
    [LinphoneManager.instance startLinphoneCore];
    [LinphoneManager.instance.fastAddressBook reloadFriends];
    [NSNotificationCenter.defaultCenter postNotificationName:kLinphoneMessageReceived object:nil];
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
	
	if (!startedInBackground || PhoneMainView.instance.currentView == nil) {
		startedInBackground = TRUE;
		// initialize UI
		[PhoneMainView.instance startUp];
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
            if ((floor(NSFoundationVersionNumber) <= NSFoundationVersionNumber_iOS_9_x_Max)) {
              if ([LinphoneManager.instance lpConfigBoolForKey:@"autoanswer_notif_preference"]) {
                linphone_call_accept(call);
                [PhoneMainView.instance changeCurrentView:CallView.compositeViewDescription];
              } else {
                [PhoneMainView.instance displayIncomingCall:call];
              }
            } else {
              // Click the call notification when callkit is disabled, show app view.
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
}

#pragma deploymate push "ignored-api-availability"

- (void)registerForNotifications {
	if (CallManager.instance.alreadyRegisteredForNotification && [[UIApplication sharedApplication] isRegisteredForRemoteNotifications])
		return;

	CallManager.instance.alreadyRegisteredForNotification = true;
	self.voipRegistry = [[PKPushRegistry alloc] initWithQueue:dispatch_get_main_queue()];
	self.voipRegistry.delegate = self;

	// Initiate registration.
	LOGI(@"[PushKit] Connecting for push notifications");
	self.voipRegistry.desiredPushTypes = [NSSet setWithObject:PKPushTypeVoIP];

	int num = [LinphoneManager.instance lpConfigIntForKey:@"unexpected_pushkit" withDefault:0];
	if (num > 3) {
		LOGI(@"[PushKit] unexpected pushkit notifications received %d, please clean your sip account.", num);
	}

    // Register for remote notifications.
    LOGI(@"[APNs] register for push notif");
    [[UIApplication sharedApplication] registerForRemoteNotifications];

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
#ifdef USE_CRASHLYTICS
	[FIRApp configure];
#endif


	if ([[LinphoneManager instance] lpConfigBoolForKey:@"vfs_enabled_preference"]) {
		if (TARGET_IPHONE_SIMULATOR) {
			LOGW(@"[VFS] Can not active for simulators.");
			[[LinphoneManager instance] lpConfigSetBool:FALSE forKey:@"vfs_enabled_preference"];
		} else if (!VFSUtil.activateVFS) {
			[VFSUtil oslogWithLog:@"[VFS] Error unable to activate." level:OS_LOG_TYPE_ERROR];
			[[LinphoneManager instance] lpConfigSetBool:FALSE forKey:@"vfs_enabled_preference"];
		}
	}

    UIApplication *app = [UIApplication sharedApplication];
	UIApplicationState state = app.applicationState;

	LinphoneManager *instance = [LinphoneManager instance];
	//init logs asap
	[Log enableLogs:[[LinphoneManager instance] lpConfigIntForKey:@"debugenable_preference"]];
	
	
	

	if ([PHPhotoLibrary authorizationStatus] != PHAuthorizationStatusAuthorized) {
		[PHPhotoLibrary requestAuthorization:^(PHAuthorizationStatus status) {
			dispatch_async(dispatch_get_main_queue(), ^{
				if ([PHPhotoLibrary authorizationStatus] != PHAuthorizationStatusAuthorized) {
					[[[UIAlertView alloc] initWithTitle:NSLocalizedString(@"Photo's permission", nil) message:NSLocalizedString(@"Photo not authorized", nil) delegate:nil cancelButtonTitle:nil otherButtonTitles:@"Continue", nil] show];
				}
			});
		}];
	}

	BOOL background_mode = [instance lpConfigBoolForKey:@"backgroundmode_preference"];
	BOOL start_at_boot = [instance lpConfigBoolForKey:@"start_at_boot_preference"];
	[self registerForNotifications]; // Register for notifications must be done ASAP to give a chance for first SIP register to be done with right token. Specially true in case of remote provisionning or re-install with new type of signing certificate, like debug to release.

	if (state == UIApplicationStateBackground) {
		// we've been woken up directly to background;
		if (!start_at_boot || !background_mode) {
			// autoboot disabled or no background, and no push: do nothing and wait for a real launch
			//output a log with NSLog, because the ortp logging system isn't activated yet at this time
			NSLog(@"Linphone launch doing nothing because start_at_boot or background_mode are not activated.", NULL);
			return YES;
		}
		startedInBackground = true;
	}
	bgStartId = [[UIApplication sharedApplication] beginBackgroundTaskWithExpirationHandler:^{
	  LOGW(@"Background task for application launching expired.");
	  [[UIApplication sharedApplication] endBackgroundTask:bgStartId];
	}];

	[LinphoneManager.instance launchLinphoneCore];
	LinphoneManager.instance.iapManager.notificationCategory = @"expiry_notification";
	// initialize UI
	[self.window makeKeyAndVisible];
	[RootViewManager setupWithPortrait:(PhoneMainView *)self.window.rootViewController];

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
	[CallManager.instance removeAllCallInfos];
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
    } else if ([scheme isEqualToString:@"sip"]||[scheme isEqualToString:@"sips"]) {
        // remove "sip://" from the URI, and do it correctly by taking resourceSpecifier and removing leading and
        // trailing "/"
        NSString *sipUri = [[url resourceSpecifier] stringByTrimmingCharactersInSet:[NSCharacterSet characterSetWithCharactersInString:@"/"]];
		[CallManager.instance performActionWhenCoreIsOnAction:^(void) {
			[LinphoneManager.instance call: [LinphoneUtils normalizeSipOrPhoneAddress:sipUri]];
		}];
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

// used for callkit. Called when active video.
- (BOOL)application:(UIApplication *)application continueUserActivity:(nonnull NSUserActivity *)userActivity restorationHandler:(nonnull void (^)(NSArray<id<UIUserActivityRestoring>> * _Nullable))restorationHandler
{
	
	
	if ([userActivity.activityType isEqualToString:@"INStartVideoCallIntent"]) {
		LOGI(@"CallKit: satrt video.");
		CallView *view = VIEW(CallView);
		[view.videoButton setOn];
	}
	if ([userActivity.activityType isEqualToString:@"INStartAudioCallIntent"]) { // tel URI handler.
		INInteraction *interaction = userActivity.interaction;
		INStartAudioCallIntent *startAudioCallIntent = (INStartAudioCallIntent *)interaction.intent;
		INPerson *contact = startAudioCallIntent.contacts[0];
		INPersonHandle *personHandle = contact.personHandle;
		[CallManager.instance performActionWhenCoreIsOnAction:^(void) {
			[LinphoneManager.instance call: [LinphoneUtils normalizeSipOrPhoneAddress:personHandle.value]];
		}];

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
	// support only for calls
	NSDictionary *aps = [userInfo objectForKey:@"aps"];
	//NSString *loc_key = [aps objectForKey:@"loc-key"] ?: [[aps objectForKey:@"alert"] objectForKey:@"loc-key"];
	NSString *callId = [aps objectForKey:@"call-id"] ?: @"";

	if([CallManager callKitEnabled]) {
		// Since ios13, a new Incoming call must be displayed when the callkit is enabled and app is in background.
		// Otherwise it will cause a crash.
		[CallManager.instance displayIncomingCallWithCallId:callId];
	} else {
		if (linphone_core_get_calls(LC)) {
			// if there are calls, obviously our TCP socket shall be working
			LOGD(@"Notification [%p] has no need to be processed because there already is an active call.", userInfo);
			return;
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
			}
		}
	}

    LOGI(@"Notification [%p] processed", userInfo);
	// Tell the core to make sure that we are registered.
	// It will initiate socket connections, which seems to be required.
	// Indeed it is observed that if no network action is done in the notification handler, then
	// iOS kills us.
	linphone_core_ensure_registered(LC);
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
	LOGI(@"[APNs] %@ : %@", NSStringFromSelector(_cmd), deviceToken);
	dispatch_async(dispatch_get_main_queue(), ^{
		[LinphoneManager.instance setRemoteNotificationToken:deviceToken];
	});
}

- (void)application:(UIApplication *)application didFailToRegisterForRemoteNotificationsWithError:(NSError *)error {
	LOGI(@"[APNs] %@ : %@", NSStringFromSelector(_cmd), [error localizedDescription]);
	[LinphoneManager.instance setRemoteNotificationToken:nil];
}

#pragma mark - PushKit Functions

- (void)pushRegistry:(PKPushRegistry *)registry didUpdatePushCredentials:(PKPushCredentials *)credentials forType:(PKPushType)type {
	LOGI(@"[PushKit] credentials updated with voip token: %@", credentials.token);
	dispatch_async(dispatch_get_main_queue(), ^{
		[LinphoneManager.instance setPushKitToken:credentials.token];
	});
}

- (void)pushRegistry:(PKPushRegistry *)registry didInvalidatePushTokenForType:(NSString *)type {
    LOGI(@"[PushKit] Token invalidated");
    dispatch_async(dispatch_get_main_queue(), ^{[LinphoneManager.instance setPushKitToken:nil];});
}

- (void)processPush:(NSDictionary *)userInfo {
	LOGI(@"[PushKit] Notification [%p] received with payload : %@", userInfo, userInfo.description);

	// prevent app to crash if PushKit received for msg
    if ([userInfo[@"aps"][@"loc-key"] isEqualToString:@"IM_MSG"]) {
		LOGE(@"Received a legacy PushKit notification for a chat message");
		[LinphoneManager.instance lpConfigSetInt:[LinphoneManager.instance lpConfigIntForKey:@"unexpected_pushkit" withDefault:0]+1 forKey:@"unexpected_pushkit"];
        return;
    }
    [LinphoneManager.instance startLinphoneCore];

	[self configureUINotification];
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
	// If an app extension launch a user notif while app is in fg, it is catch by the app
    NSString *category = [[[notification request] content] categoryIdentifier];
    if (category && [category isEqualToString:@"app_active"]) {
        return;
    }

	if (category && [category isEqualToString:@"msg_cat"] && [UIApplication sharedApplication].applicationState == UIApplicationStateActive) {
		if ((PhoneMainView.instance.currentView == ChatsListView.compositeViewDescription))
			return;

		if (PhoneMainView.instance.currentView == ChatConversationView.compositeViewDescription) {
			NSDictionary *userInfo = [[[notification request] content] userInfo];
			NSString *peerAddress = userInfo[@"peer_addr"];
			NSString *localAddress = userInfo[@"local_addr"];
			if (peerAddress && localAddress) {
				LinphoneAddress *peer = linphone_core_create_address([LinphoneManager getLc], peerAddress.UTF8String);
				LinphoneAddress *local = linphone_core_create_address([LinphoneManager getLc], localAddress.UTF8String);
				LinphoneChatRoom *room = linphone_core_find_chat_room([LinphoneManager getLc], peer, local);
				if (room == PhoneMainView.instance.currentRoom) return;
			}
		}
	}

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

	LinphoneCall *call = [CallManager.instance findCallWithCallId:callId];

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
		[CallManager.instance acceptVideoWithCall:call confirm:FALSE];
  	} else if ([response.actionIdentifier isEqual:@"Accept"]) {
		LOGI(@"User accept video proposal");
	  	if (call != linphone_core_get_current_call(LC))
			return;

		[[UNUserNotificationCenter currentNotificationCenter] removeAllDeliveredNotifications];
		[PhoneMainView.instance changeCurrentView:CallView.compositeViewDescription];
		[CallManager.instance acceptVideoWithCall:call confirm:TRUE];
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
            // prevent to go to chat room view when removing the notif
			if (![response.actionIdentifier isEqualToString:@"com.apple.UNNotificationDismissActionIdentifier"]) {
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
			}
		} else if ([response.notification.request.content.categoryIdentifier isEqual:@"video_request"]) {
			if (!call) return;
			[PhoneMainView.instance changeCurrentView:CallView.compositeViewDescription];
		  	NSTimer *videoDismissTimer = nil;
		  	UIConfirmationDialog *sheet = [UIConfirmationDialog ShowWithMessage:response.notification.request.content.body
																  cancelMessage:nil
																 confirmMessage:NSLocalizedString(@"ACCEPT", nil)
																  onCancelClick:^() {
																	  LOGI(@"User declined video proposal");
																	  if (call != linphone_core_get_current_call(LC))
																		  return;
																	  [CallManager.instance acceptVideoWithCall:call confirm:FALSE];
																	  [videoDismissTimer invalidate];
																  }
															onConfirmationClick:^() {
																LOGI(@"User accept video proposal");
																if (call != linphone_core_get_current_call(LC))
																	return;
																[CallManager.instance acceptVideoWithCall:call confirm:TRUE];
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
			 completionHandler:(void (^)(void))completionHandler {

	LinphoneCall *call = linphone_core_get_current_call(LC);
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
			 completionHandler:(void (^)(void))completionHandler {

	LinphoneCall *call = linphone_core_get_current_call(LC);
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
	[LinphoneManager.instance launchLinphoneCore];
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
