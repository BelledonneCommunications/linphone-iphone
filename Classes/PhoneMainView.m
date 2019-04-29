/* PhoneMainView.m
 *
 * Copyright (C) 2012  Belledonne Comunications, Grenoble, France
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

#import <QuartzCore/QuartzCore.h>
#import <AudioToolbox/AudioServices.h>
#import "LinphoneAppDelegate.h"
#import "Log.h"
#import "PhoneMainView.h"

static RootViewManager *rootViewManagerInstance = nil;

@implementation RootViewManager {
	PhoneMainView *currentViewController;
}

+ (void)setupWithPortrait:(PhoneMainView *)portrait {
	assert(rootViewManagerInstance == nil);
	rootViewManagerInstance = [[RootViewManager alloc] initWithPortrait:portrait];
}

- (instancetype)initWithPortrait:(PhoneMainView *)portrait {
	self = [super init];
	if (self) {
		self.portraitViewController = portrait;
		self.rotatingViewController = [[PhoneMainView alloc] init];

		self.portraitViewController.name = @"Portrait";
		self.rotatingViewController.name = @"Rotating";

		currentViewController = portrait;
		self.viewDescriptionStack = [NSMutableArray array];
	}
	return self;
}

+ (RootViewManager *)instance {
	if (!rootViewManagerInstance) {
		@throw [NSException exceptionWithName:@"RootViewManager" reason:@"nil instance" userInfo:nil];
	}
	return rootViewManagerInstance;
}

- (PhoneMainView *)currentView {
	return currentViewController;
}

- (PhoneMainView *)setViewControllerForDescription:(UICompositeViewDescription *)description {
	return currentViewController;

// not sure what this code was doing... but since iphone does support rotation as well now...
#if 0
	if (IPAD)
		return currentViewController;

	PhoneMainView *newMainView = description.landscapeMode ? self.rotatingViewController : self.portraitViewController;
	if (newMainView != currentViewController) {
		PhoneMainView *previousMainView = currentViewController;
		UIInterfaceOrientation nextViewOrientation = newMainView.interfaceOrientation;
		UIInterfaceOrientation previousOrientation = currentViewController.interfaceOrientation;

		LOGI(@"Changing rootViewController: %@ -> %@", currentViewController.name, newMainView.name);
		currentViewController = newMainView;
		LinphoneAppDelegate *delegate = (LinphoneAppDelegate *)[UIApplication sharedApplication].delegate;

		if (ANIMATED) {
			[UIView transitionWithView:delegate.window
				duration:0.3
				options:UIViewAnimationOptionTransitionFlipFromLeft | UIViewAnimationOptionAllowAnimatedContent
				animations:^{
				  delegate.window.rootViewController = newMainView;
				  // when going to landscape-enabled view, we have to get the current portrait frame and orientation,
				  // because it could still have landscape-based size
				  if (nextViewOrientation != previousOrientation && newMainView == self.rotatingViewController) {
					  newMainView.view.frame = previousMainView.view.frame;
					  [newMainView.mainViewController.view setFrame:previousMainView.mainViewController.view.frame];
					  [newMainView willRotateToInterfaceOrientation:previousOrientation duration:0.3];
					  [newMainView willAnimateRotationToInterfaceOrientation:previousOrientation duration:0.3];
					  [newMainView didRotateFromInterfaceOrientation:nextViewOrientation];
				  }
				}
				completion:^(BOOL finished){
				}];
		} else {
			delegate.window.rootViewController = newMainView;
			// when going to landscape-enabled view, we have to get the current portrait frame and orientation,
			// because it could still have landscape-based size
			if (nextViewOrientation != previousOrientation && newMainView == self.rotatingViewController) {
				newMainView.view.frame = previousMainView.view.frame;
				[newMainView.mainViewController.view setFrame:previousMainView.mainViewController.view.frame];
				[newMainView willRotateToInterfaceOrientation:previousOrientation duration:0.];
				[newMainView willAnimateRotationToInterfaceOrientation:previousOrientation duration:0.];
				[newMainView didRotateFromInterfaceOrientation:nextViewOrientation];
			}
		}
	}
	return currentViewController;
#endif
}

@end

@implementation PhoneMainView

@synthesize mainViewController;
@synthesize currentView;
@synthesize statusBarBG;
@synthesize volumeView;

#pragma mark - Lifecycle Functions

- (void)initPhoneMainView {
	currentView = nil;
	_currentRoom = NULL;
	_currentName = NULL;
	_previousView = nil;
	inhibitedEvents = [[NSMutableArray alloc] init];
}

- (id)init {
	self = [super init];
	if (self) {
		[self initPhoneMainView];
	}
	return self;
}

- (id)initWithNibName:(NSString *)nibNameOrNil bundle:(NSBundle *)nibBundleOrNil {
	self = [super initWithNibName:nibNameOrNil bundle:nibBundleOrNil];
	if (self) {
		[self initPhoneMainView];
	}
	return self;
}

- (id)initWithCoder:(NSCoder *)decoder {
	self = [super initWithCoder:decoder];
	if (self) {
		[self initPhoneMainView];
	}
	return self;
}

- (void)dealloc {
	[NSNotificationCenter.defaultCenter removeObserver:self];
}

#pragma mark - ViewController Functions

- (void)viewDidLoad {
	[super viewDidLoad];

	volumeView = [[MPVolumeView alloc] initWithFrame:CGRectMake(-100, -100, 16, 16)];
	volumeView.showsRouteButton = false;
	volumeView.userInteractionEnabled = false;

	[self.view addSubview:mainViewController.view];
}

- (void)viewWillAppear:(BOOL)animated {
	[super viewWillAppear:animated];

	// Set observers
	[NSNotificationCenter.defaultCenter addObserver:self
										   selector:@selector(callUpdate:)
											   name:kLinphoneCallUpdate
											 object:nil];
	[NSNotificationCenter.defaultCenter addObserver:self
										   selector:@selector(registrationUpdate:)
											   name:kLinphoneRegistrationUpdate
											 object:nil];
	[NSNotificationCenter.defaultCenter addObserver:self
										   selector:@selector(textReceived:)
											   name:kLinphoneMessageReceived
											 object:nil];
	[NSNotificationCenter.defaultCenter addObserver:self
										   selector:@selector(onGlobalStateChanged:)
											   name:kLinphoneGlobalStateUpdate
											 object:nil];
	[[UIDevice currentDevice] setBatteryMonitoringEnabled:YES];
	[NSNotificationCenter.defaultCenter addObserver:self
										   selector:@selector(batteryLevelChanged:)
											   name:UIDeviceBatteryLevelDidChangeNotification
											 object:nil];
}

- (void)viewWillDisappear:(BOOL)animated {
	[super viewWillDisappear:animated];
	[NSNotificationCenter.defaultCenter removeObserver:self];
	[NSNotificationCenter.defaultCenter removeObserver:self name:UIDeviceBatteryLevelDidChangeNotification object:nil];
	[[UIDevice currentDevice] setBatteryMonitoringEnabled:NO];
}

/* IPHONE X specific : hide the HomeIndcator when not used */
#define IS_IPHONE (UI_USER_INTERFACE_IDIOM() == UIUserInterfaceIdiomPhone)
#define IS_IPHONE_X (IS_IPHONE && [[UIScreen mainScreen] bounds].size.height == 812.0)
#define IPHONE_STATUSBAR_HEIGHT (IS_IPHONE_X ? 35 : 20)

- (BOOL)isIphoneXDevice{
	return IS_IPHONE_X;
}

- (void)viewDidAppear:(BOOL)animated {
	[super viewDidAppear:animated];
	if([self isIphoneXDevice]){
		if(@available(iOS 11.0, *)) {
			[self childViewControllerForHomeIndicatorAutoHidden];
			[self prefersHomeIndicatorAutoHidden];
			[self setNeedsUpdateOfHomeIndicatorAutoHidden];
		}
	}

}

- (BOOL)prefersHomeIndicatorAutoHidden{
	return YES;
}

- (void)setVolumeHidden:(BOOL)hidden {
	// sometimes when placing a call, the volume view will appear. Inserting a
	// carefully hidden MPVolumeView into the view hierarchy will hide it
	if (hidden) {
		if (!(volumeView.superview == self.view)) {
			[self.view addSubview:volumeView];
		}
	} else {
		if (volumeView.superview == self.view) {
			[volumeView removeFromSuperview];
		}
	}
}

#if __IPHONE_OS_VERSION_MAX_ALLOWED >= 90000
- (UIInterfaceOrientationMask)supportedInterfaceOrientations
#else
- (NSUInteger)supportedInterfaceOrientations
#endif
{
	return UIInterfaceOrientationMaskAll;
}

- (void)willRotateToInterfaceOrientation:(UIInterfaceOrientation)toInterfaceOrientation
								duration:(NSTimeInterval)duration {
	if (toInterfaceOrientation == UIInterfaceOrientationPortraitUpsideDown)
		return;

	[super willRotateToInterfaceOrientation:toInterfaceOrientation duration:duration];
	[mainViewController willRotateToInterfaceOrientation:toInterfaceOrientation duration:duration];
	[self orientationUpdate:toInterfaceOrientation];
}

- (void)willAnimateRotationToInterfaceOrientation:(UIInterfaceOrientation)toInterfaceOrientation
										 duration:(NSTimeInterval)duration {
	if (toInterfaceOrientation == UIInterfaceOrientationPortraitUpsideDown)
		return;

	[super willAnimateRotationToInterfaceOrientation:toInterfaceOrientation duration:duration];
	[mainViewController willAnimateRotationToInterfaceOrientation:toInterfaceOrientation duration:duration];
}

- (void)didRotateFromInterfaceOrientation:(UIInterfaceOrientation)fromInterfaceOrientation {
	[super didRotateFromInterfaceOrientation:fromInterfaceOrientation];
	[mainViewController didRotateFromInterfaceOrientation:fromInterfaceOrientation];
}

- (UIInterfaceOrientation)interfaceOrientation {
	return [mainViewController currentOrientation];
}

- (void)didReceiveMemoryWarning {
	[super didReceiveMemoryWarning];
	[mainViewController clearCache:[RootViewManager instance].viewDescriptionStack];
}

#pragma mark - Event Functions

- (void)textReceived:(NSNotification *)notif {
	LinphoneChatMessage *msg = [[notif.userInfo objectForKey:@"message"] pointerValue];
	NSString *callID = [notif.userInfo objectForKey:@"call-id"];
	[self updateApplicationBadgeNumber];

	if (!msg)
		return;

	if (linphone_chat_message_is_outgoing(msg))
		return;

	ChatConversationView *view = VIEW(ChatConversationView);
	// if we already are in the conversation, we should not ring/vibrate
	if (view.chatRoom && _currentRoom == view.chatRoom)
		return;

	if ([UIApplication sharedApplication].applicationState != UIApplicationStateActive)
		return;

	LinphoneManager *lm = LinphoneManager.instance;
	// if the message was already received through a push notif, we don't need to ring
	if (![lm popPushCallID:callID]) {
		[lm playMessageSound];
	}
}

- (void)registrationUpdate:(NSNotification *)notif {
	LinphoneRegistrationState state = [[notif.userInfo objectForKey:@"state"] intValue];
	if (state == LinphoneRegistrationFailed && ![currentView equal:AssistantView.compositeViewDescription] &&
		[UIApplication sharedApplication].applicationState == UIApplicationStateActive) {
		UIAlertController *errView = [UIAlertController alertControllerWithTitle:NSLocalizedString(@"Connection failure", nil)
																		 message:[notif.userInfo objectForKey:@"message"]
																  preferredStyle:UIAlertControllerStyleAlert];
		
		UIAlertAction* defaultAction = [UIAlertAction actionWithTitle:NSLocalizedString(@"Continue", nil)
																style:UIAlertActionStyleDefault
															  handler:^(UIAlertAction * action) {}];
		
		[errView addAction:defaultAction];
		[self presentViewController:errView animated:YES completion:nil];
    } else if (state == LinphoneRegistrationOk && [currentView equal:ChatsListView.compositeViewDescription]) {
        // update avatarImages
        //ChatsListView *view = VIEW(ChatsListView);
        //[view.tableController loadData];
    }
}

- (void)onGlobalStateChanged:(NSNotification *)notif {
	LinphoneGlobalState state = (LinphoneGlobalState)[[[notif userInfo] valueForKey:@"state"] integerValue];
	static BOOL already_shown = FALSE;
	if (state == LinphoneGlobalOn && !already_shown && LinphoneManager.instance.wasRemoteProvisioned) {
		LinphoneProxyConfig *conf = linphone_core_get_default_proxy_config(LC);
		if ([LinphoneManager.instance lpConfigBoolForKey:@"show_login_view" inSection:@"app"] && conf == NULL) {
			already_shown = TRUE;
			AssistantView *view = VIEW(AssistantView);
			[PhoneMainView.instance changeCurrentView:view.compositeViewDescription];
			[view fillDefaultValues];
		}
	}
}

- (void)callUpdate:(NSNotification *)notif {
	LinphoneCall *call = [[notif.userInfo objectForKey:@"call"] pointerValue];
	LinphoneCallState state = [[notif.userInfo objectForKey:@"state"] intValue];
	NSString *message = [notif.userInfo objectForKey:@"message"];

	switch (state) {
		case LinphoneCallIncomingReceived:
		case LinphoneCallIncomingEarlyMedia: {
			if (linphone_core_get_calls_nb(LC) > 1 ||
				(floor(NSFoundationVersionNumber) <= NSFoundationVersionNumber_iOS_9_x_Max)) {
				[self displayIncomingCall:call];
			}
			break;
		}
		case LinphoneCallOutgoingInit: {
			[self changeCurrentView:CallOutgoingView.compositeViewDescription];
			break;
		}
		case LinphoneCallPausedByRemote:
		case LinphoneCallConnected: {
			if (floor(NSFoundationVersionNumber) > NSFoundationVersionNumber_iOS_9_x_Max && call) {
				NSString *callId =
					[NSString stringWithUTF8String:linphone_call_log_get_call_id(linphone_call_get_call_log(call))];
				NSUUID *uuid = [LinphoneManager.instance.providerDelegate.uuids objectForKey:callId];
				if (uuid) {
					[LinphoneManager.instance.providerDelegate.provider reportOutgoingCallWithUUID:uuid
																		   startedConnectingAtDate:nil];
				}
			}
			break;
		}
		case LinphoneCallStreamsRunning: {
			[self changeCurrentView:CallView.compositeViewDescription];
			if (floor(NSFoundationVersionNumber) > NSFoundationVersionNumber_iOS_9_x_Max && call) {
				NSString *callId =
					[NSString stringWithUTF8String:linphone_call_log_get_call_id(linphone_call_get_call_log(call))];
				NSUUID *uuid = [LinphoneManager.instance.providerDelegate.uuids objectForKey:callId];
				if (uuid) {
					[LinphoneManager.instance.providerDelegate.provider reportOutgoingCallWithUUID:uuid
																				   connectedAtDate:nil];
					NSString *address = [FastAddressBook displayNameForAddress:linphone_call_get_remote_address(call)];
					CXCallUpdate *update = [[CXCallUpdate alloc] init];
					update.remoteHandle = [[CXHandle alloc] initWithType:CXHandleTypeGeneric value:address];
					update.supportsGrouping = TRUE;
					update.supportsDTMF = TRUE;
					update.supportsHolding = TRUE;
					update.supportsUngrouping = TRUE;
					[LinphoneManager.instance.providerDelegate.provider reportCallWithUUID:uuid updated:update];
				}
			}
			break;
		}
		case LinphoneCallUpdatedByRemote: {
			const LinphoneCallParams *current = linphone_call_get_current_params(call);
			const LinphoneCallParams *remote = linphone_call_get_remote_params(call);

			if (linphone_call_params_video_enabled(current) && !linphone_call_params_video_enabled(remote)) {
				[self changeCurrentView:CallView.compositeViewDescription];
			}
			break;
		}
		case LinphoneCallError: {
			[self displayCallError:call message:message];
		}
		case LinphoneCallEnd: {
			const MSList *calls = linphone_core_get_calls(LC);
			if (!calls) {
				while ((currentView == CallView.compositeViewDescription) ||
					   (currentView == CallIncomingView.compositeViewDescription) ||
					   (currentView == CallOutgoingView.compositeViewDescription)) {
					[self popCurrentView];
				}
			} else {
				[self changeCurrentView:CallView.compositeViewDescription];
			}
			break;
		}
		case LinphoneCallEarlyUpdatedByRemote:
		case LinphoneCallEarlyUpdating:
		case LinphoneCallIdle:
			break;
		case LinphoneCallOutgoingEarlyMedia:
		case LinphoneCallOutgoingProgress: {
			if (floor(NSFoundationVersionNumber) > NSFoundationVersionNumber_iOS_9_x_Max && call &&
				(linphone_core_get_calls_nb(LC) < 2)) {
				// Link call ID to UUID
				NSString *callId =
					[NSString stringWithUTF8String:linphone_call_log_get_call_id(linphone_call_get_call_log(call))];
				NSUUID *uuid = [LinphoneManager.instance.providerDelegate.uuids objectForKey:@""];
				if (uuid) {
					[LinphoneManager.instance.providerDelegate.uuids removeObjectForKey:@""];
					[LinphoneManager.instance.providerDelegate.uuids setObject:uuid forKey:callId];
					[LinphoneManager.instance.providerDelegate.calls setObject:callId forKey:uuid];
				}
			}
			break;
		}
		case LinphoneCallOutgoingRinging:
		case LinphoneCallPaused:
		case LinphoneCallPausing:
		case LinphoneCallRefered:
		case LinphoneCallReleased:
			break;
		case LinphoneCallResuming: {
			if (floor(NSFoundationVersionNumber) > NSFoundationVersionNumber_iOS_9_x_Max && call) {
				NSUUID *uuid = (NSUUID *)[LinphoneManager.instance.providerDelegate.uuids
					objectForKey:[NSString stringWithUTF8String:linphone_call_log_get_call_id(
																	linphone_call_get_call_log(call))]];
				if (!uuid) {
					break;
				}
				CXSetHeldCallAction *act = [[CXSetHeldCallAction alloc] initWithCallUUID:uuid onHold:NO];
				CXTransaction *tr = [[CXTransaction alloc] initWithAction:act];
				[LinphoneManager.instance.providerDelegate.controller requestTransaction:tr
																			  completion:^(NSError *err){
																			  }];
			}
			break;
		}
		case LinphoneCallUpdating:
			break;
	}
	if (state == LinphoneCallEnd || state == LinphoneCallError || floor(NSFoundationVersionNumber) <= NSFoundationVersionNumber_iOS_9_x_Max)
		[self updateApplicationBadgeNumber];
}

#pragma mark -

- (void)orientationUpdate:(UIInterfaceOrientation)orientation {
	int oldLinphoneOrientation = linphone_core_get_device_rotation(LC);
	int newRotation = 0;
	switch (orientation) {
		case UIInterfaceOrientationPortrait:
			newRotation = 0;
			break;
		case UIInterfaceOrientationPortraitUpsideDown:
			newRotation = 180;
			break;
		case UIInterfaceOrientationLandscapeRight:
			newRotation = 270;
			break;
		case UIInterfaceOrientationLandscapeLeft:
			newRotation = 90;
			break;
		default:
			newRotation = oldLinphoneOrientation;
	}
	if (oldLinphoneOrientation != newRotation) {
		linphone_core_set_device_rotation(LC, newRotation);
		LinphoneCall *call = linphone_core_get_current_call(LC);
		if (call && linphone_call_params_video_enabled(linphone_call_get_current_params(call))) {
			// Orientation has changed, must call update call
			linphone_core_update_call(LC, call, NULL);
		}
	}
}
- (void)startUp {
	@try {
		LinphoneManager *lm = LinphoneManager.instance;
                LOGI(@"%s", linphone_global_state_to_string(
                                linphone_core_get_global_state(LC)));
                if (linphone_core_get_global_state(LC) != LinphoneGlobalOn) {
                  [self changeCurrentView:DialerView.compositeViewDescription];
                } else if ([LinphoneManager.instance
                               lpConfigBoolForKey:
                                   @"enable_first_login_view_preference"] ==
                           true) {
                  [PhoneMainView.instance
                      changeCurrentView:FirstLoginView
                                            .compositeViewDescription];
                } else {
                  // always start to dialer when testing
                  // Change to default view
                  const MSList *list = linphone_core_get_proxy_config_list(LC);
                  if (list != NULL ||
                      ([lm lpConfigBoolForKey:@"hide_assistant_preference"] ==
                       true) ||
                      lm.isTesting) {
                    [self
                        changeCurrentView:DialerView.compositeViewDescription];
                  } else {
                    AssistantView *view = VIEW(AssistantView);
                    [PhoneMainView.instance
                        changeCurrentView:view.compositeViewDescription];
                    [view reset];
                  }
                }
                [self updateApplicationBadgeNumber]; // Update Badge at startup
        } @catch (NSException *exception) {
          // we'll wait until the app transitions correctly
        }
}

- (void)updateApplicationBadgeNumber {
	int count = 0;
	count += linphone_core_get_missed_calls_count(LC);
	count += [LinphoneManager unreadMessageCount];
	count += linphone_core_get_calls_nb(LC);
	[[UIApplication sharedApplication] setApplicationIconBadgeNumber:count];
	TabBarView *view = (TabBarView *)[PhoneMainView.instance.mainViewController getCachedController:NSStringFromClass(TabBarView.class)];
	[view update:TRUE];
}

+ (CATransition *)getBackwardTransition {
	BOOL RTL = [LinphoneManager langageDirectionIsRTL];
	BOOL land = UIInterfaceOrientationIsLandscape([self.instance interfaceOrientation]);
	NSString *transition = land ? kCATransitionFromBottom : (RTL ? kCATransitionFromRight : kCATransitionFromLeft);
	CATransition *trans = [CATransition animation];
	[trans setType:kCATransitionPush];
	[trans setDuration:0.35];
	[trans setTimingFunction:[CAMediaTimingFunction functionWithName:kCAMediaTimingFunctionEaseInEaseOut]];
	[trans setSubtype:transition];

	return trans;
}

+ (CATransition *)getForwardTransition {
	BOOL RTL = [LinphoneManager langageDirectionIsRTL];
	BOOL land = UIInterfaceOrientationIsLandscape([self.instance interfaceOrientation]);
	NSString *transition = land ? kCATransitionFromTop : (RTL ? kCATransitionFromLeft : kCATransitionFromRight);
	CATransition *trans = [CATransition animation];
	[trans setType:kCATransitionPush];
	[trans setDuration:0.35];
	[trans setTimingFunction:[CAMediaTimingFunction functionWithName:kCAMediaTimingFunctionEaseInEaseOut]];
	[trans setSubtype:transition];

	return trans;
}

+ (CATransition *)getTransition:(UICompositeViewDescription *)old new:(UICompositeViewDescription *) new {
	bool left = false;

	if ([old equal:ChatsListView.compositeViewDescription]) {
		if ([new equal:ContactsListView.compositeViewDescription] || [new equal:DialerView.compositeViewDescription] ||
			[new equal:HistoryListView.compositeViewDescription]) {
			left = true;
		}
	} else if ([old equal:SettingsView.compositeViewDescription]) {
		if ([new equal:DialerView.compositeViewDescription] || [new equal:ContactsListView.compositeViewDescription] ||
			[new equal:HistoryListView.compositeViewDescription] ||
			[new equal:ChatsListView.compositeViewDescription]) {
			left = true;
		}
	} else if ([old equal:DialerView.compositeViewDescription]) {
		if ([new equal:ContactsListView.compositeViewDescription] ||
			[new equal:HistoryListView.compositeViewDescription]) {
			left = true;
		}
	} else if ([old equal:ContactsListView.compositeViewDescription]) {
		if ([new equal:HistoryListView.compositeViewDescription]) {
			left = true;
		}
	}

	if (left) {
		return [PhoneMainView getBackwardTransition];
	} else {
		return [PhoneMainView getForwardTransition];
	}
}

+ (PhoneMainView *)instance {
	return [[RootViewManager instance] currentView];
}

- (void)hideTabBar:(BOOL)hide {
	[mainViewController hideTabBar:hide];
}

- (void)hideStatusBar:(BOOL)hide {
	[mainViewController hideStatusBar:hide];
}

- (void)updateStatusBar:(UICompositeViewDescription *)to_view {
#pragma deploymate push "ignored-api-availability"
	if (UIDevice.currentDevice.systemVersion.doubleValue >= 7.) {
		// In iOS7, the app has a black background on dialer, incoming and incall, so we have to adjust the
		// status bar style for each transition to/from these views
		BOOL toLightStatus = (to_view != NULL) && ![to_view darkBackground];
		if (!toLightStatus) {
			// black bg: white text on black background
			[[UIApplication sharedApplication] setStatusBarStyle:UIStatusBarStyleLightContent];

			[UIView animateWithDuration:0.3f
							 animations:^{
							   statusBarBG.backgroundColor = [UIColor blackColor];
							 }];

		} else {
			// light bg: black text on white bg
			[[UIApplication sharedApplication] setStatusBarStyle:UIStatusBarStyleDefault];
			[UIView animateWithDuration:0.3f
							 animations:^{
							   statusBarBG.backgroundColor = [UIColor colorWithWhite:0.935 alpha:1];
							 }];
		}
	}
#pragma deploymate pop
}

- (void)fullScreen:(BOOL)enabled {
	[statusBarBG setHidden:enabled];
	[mainViewController setFullscreen:enabled];
}

- (UIViewController *)popCurrentView {
	NSMutableArray *viewStack = [RootViewManager instance].viewDescriptionStack;
	if (viewStack.count <= 1) {
		[viewStack removeAllObjects];
		LOGW(@"PhoneMainView: Trying to pop view but none stacked, going to %@!",
			 DialerView.compositeViewDescription.name);
	} else {
		[viewStack removeLastObject];
		LOGI(@"PhoneMainView: Popping view %@, going to %@", currentView.name,
			 ((UICompositeViewDescription *)(viewStack.lastObject ?: DialerView.compositeViewDescription)).name);
	}
	[self _changeCurrentView:viewStack.lastObject ?: DialerView.compositeViewDescription
				  transition:[PhoneMainView getBackwardTransition]
					animated:ANIMATED];
	return [mainViewController getCurrentViewController];
}

- (void)changeCurrentView:(UICompositeViewDescription *)view {
	[self _changeCurrentView:view transition:nil animated:ANIMATED];
}

- (UIViewController *)_changeCurrentView:(UICompositeViewDescription *)view
							  transition:(CATransition *)transition
								animated:(BOOL)animated {
	PhoneMainView *vc = [[RootViewManager instance] setViewControllerForDescription:view];
	if (![view equal:vc.currentView] || vc != self) {
		LOGI(@"Change current view to %@", view.name);
		[self setPreviousViewName:vc.currentView.name];
		NSMutableArray *viewStack = [RootViewManager instance].viewDescriptionStack;
		[viewStack addObject:view];
		if (animated && transition == nil)
			transition = [PhoneMainView getTransition:vc.currentView new:view];
		[vc.mainViewController setViewTransition:(animated ? transition : nil)];
		[vc updateStatusBar:view];
		[vc.mainViewController changeView:view];
		vc->currentView = view;
	}

	//[[RootViewManager instance] setViewControllerForDescription:view];

	NSDictionary *mdict = [NSMutableDictionary dictionaryWithObject:vc->currentView forKey:@"view"];
	[NSNotificationCenter.defaultCenter postNotificationName:kLinphoneMainViewChange object:self userInfo:mdict];

	return [vc->mainViewController getCurrentViewController];
}

- (UIViewController *)popToView:(UICompositeViewDescription *)view {
	NSMutableArray *viewStack = [RootViewManager instance].viewDescriptionStack;
	while (viewStack.count > 0 && ![[viewStack lastObject] equal:view]) {
		[viewStack removeLastObject];
	}
	return [self _changeCurrentView:view transition:[PhoneMainView getBackwardTransition] animated:ANIMATED];
}

- (void) setPreviousViewName:(NSString*)previous{
	_previousView = previous;
}

- (NSString*) getPreviousViewName {
	return _previousView;
}

+ (NSString*) getPreviousViewName {
	return [self getPreviousViewName];
}


- (UICompositeViewDescription *)firstView {
	UICompositeViewDescription *view = nil;
	NSArray *viewStack = [RootViewManager instance].viewDescriptionStack;
	if ([viewStack count]) {
		view = [viewStack objectAtIndex:0];
	}
	return view;
}

- (void)displayCallError:(LinphoneCall *)call message:(NSString *)message {
	const char *lUserNameChars = linphone_address_get_username(linphone_call_get_remote_address(call));
	NSString *lUserName =
		lUserNameChars ? [[NSString alloc] initWithUTF8String:lUserNameChars] : NSLocalizedString(@"Unknown", nil);
	NSString *lMessage;
	NSString *lTitle;

	// get default proxy
	LinphoneProxyConfig *proxyCfg = linphone_core_get_default_proxy_config(LC);
	if (proxyCfg == nil) {
		lMessage = NSLocalizedString(@"Please make sure your device is connected to the internet and double check your "
									 @"SIP account configuration in the settings.",
									 nil);
	} else {
		lMessage = [NSString stringWithFormat:NSLocalizedString(@"Cannot call %@.", nil), lUserName];
	}

	switch (linphone_call_get_reason(call)) {
		case LinphoneReasonNotFound:
			lMessage = [NSString stringWithFormat:NSLocalizedString(@"%@ is not connected.", nil), lUserName];
			break;
		case LinphoneReasonBusy:
			lMessage = [NSString stringWithFormat:NSLocalizedString(@"%@ is busy.", nil), lUserName];
			break;
		default:
			if (message != nil) {
				lMessage = [NSString stringWithFormat:NSLocalizedString(@"%@\nReason was: %@", nil), lMessage, message];
			}
			break;
	}

	lTitle = NSLocalizedString(@"Call failed", nil);
	UIAlertController *errView = [UIAlertController alertControllerWithTitle:lTitle
																	 message:lMessage
															  preferredStyle:UIAlertControllerStyleAlert];
	
	UIAlertAction* defaultAction = [UIAlertAction actionWithTitle:NSLocalizedString(@"Cancel", nil)
															style:UIAlertActionStyleDefault
														  handler:^(UIAlertAction * action) {}];
	
	[errView addAction:defaultAction];
	[self presentViewController:errView animated:YES completion:nil];
}

- (void)addInhibitedEvent:(id)event {
	[inhibitedEvents addObject:event];
}

- (BOOL)removeInhibitedEvent:(id)event {
	NSUInteger index = [inhibitedEvents indexOfObject:event];
	if (index != NSNotFound) {
		[inhibitedEvents removeObjectAtIndex:index];
		return TRUE;
	}
	return FALSE;
}

#pragma mark - ActionSheet Functions

- (void)displayIncomingCall:(LinphoneCall *)call {
	LinphoneCallLog *callLog = linphone_call_get_call_log(call);
	NSString *callId = [NSString stringWithUTF8String:linphone_call_log_get_call_id(callLog)];

	if ([UIApplication sharedApplication].applicationState == UIApplicationStateActive) {
		LinphoneManager *lm = LinphoneManager.instance;
		BOOL callIDFromPush = [lm popPushCallID:callId];
		BOOL autoAnswer = [lm lpConfigBoolForKey:@"autoanswer_notif_preference"];

		if (callIDFromPush && autoAnswer) {
			// accept call automatically
			[lm acceptCall:call evenWithVideo:YES];
		} else {
			AudioServicesPlaySystemSound(lm.sounds.vibrate);
			CallIncomingView *view = VIEW(CallIncomingView);
			[self changeCurrentView:view.compositeViewDescription];
			[view setCall:call];
			[view setDelegate:self];
		}
	}
}

- (void)batteryLevelChanged:(NSNotification *)notif {
	float level = [UIDevice currentDevice].batteryLevel;
	UIDeviceBatteryState state = [UIDevice currentDevice].batteryState;
	LOGD(@"Battery state:%d level:%.2f", state, level);

	LinphoneCall *call = linphone_core_get_current_call(LC);
	if (call && linphone_call_params_video_enabled(linphone_call_get_current_params(call))) {
		LinphoneCallAppData *callData = (__bridge LinphoneCallAppData *)linphone_call_get_user_data(call);
		if (callData != nil) {
			if (state == UIDeviceBatteryStateUnplugged) {
				if (level <= 0.2f && !callData->batteryWarningShown) {
					LOGI(@"Battery warning");
					DTActionSheet *sheet = [[DTActionSheet alloc]
						initWithTitle:NSLocalizedString(@"Battery is running low. Stop video ?", nil)];
					[sheet addCancelButtonWithTitle:NSLocalizedString(@"Continue video", nil) block:nil];
					[sheet
						addDestructiveButtonWithTitle:NSLocalizedString(@"Stop video", nil)
												block:^() {
												  LinphoneCallParams *params =
													  linphone_core_create_call_params(LC,call);
												  // stop video
												  linphone_call_params_enable_video(params, FALSE);
												  linphone_core_update_call(LC, call, params);
												}];
					[sheet showInView:self.view];
					callData->batteryWarningShown = TRUE;
				}
			}
			if (level > 0.2f) {
				callData->batteryWarningShown = FALSE;
			}
		}
	}
}

#pragma mark - IncomingCallDelegate Functions

- (void)incomingCallAborted:(LinphoneCall *)call {
}

- (void)incomingCallAccepted:(LinphoneCall *)call evenWithVideo:(BOOL)video {
	[LinphoneManager.instance acceptCall:call evenWithVideo:video];
}

- (void)incomingCallDeclined:(LinphoneCall *)call {
	linphone_call_terminate(call);
}

#pragma mark - Chat room Functions

- (void)getOrCreateOneToOneChatRoom:(const LinphoneAddress *)remoteAddress waitView:(UIView *)waitView isEncrypted:(BOOL)isEncrypted{
	if (!remoteAddress) {
		[self changeCurrentView:ChatsListView.compositeViewDescription];
		return;
	}
    
    if (!linphone_core_is_network_reachable(LC)) {
        [PhoneMainView.instance presentViewController:[LinphoneUtils networkErrorView] animated:YES completion:nil];
        return;
    }
    
	const LinphoneAddress *local = linphone_proxy_config_get_contact(linphone_core_get_default_proxy_config(LC));
	LinphoneChatRoom *room = linphone_core_find_one_to_one_chat_room_2(LC, local, remoteAddress, isEncrypted);
	if (!room) {
		bctbx_list_t *addresses = bctbx_list_new((void*)remoteAddress);
		[self createChatRoom:LINPHONE_DUMMY_SUBJECT addresses:addresses andWaitView:waitView isEncrypted:isEncrypted isGroup:FALSE];
		bctbx_list_free(addresses);
		return;
	}

	[self goToChatRoom:room];
}

- (LinphoneChatRoom *)createChatRoom:(const char *)subject addresses:(bctbx_list_t *)addresses andWaitView:(UIView *)waitView isEncrypted:(BOOL)isEncrypted isGroup:(BOOL)isGroup{
    if (!linphone_proxy_config_get_conference_factory_uri(linphone_core_get_default_proxy_config(LC))
        || ((bctbx_list_size(addresses) == 1) && !isGroup && ([[LinphoneManager instance] lpConfigBoolForKey:@"prefer_basic_chat_room" inSection:@"misc"] || !isEncrypted))) {
        // If there's no factory uri, create a basic chat room
        if (bctbx_list_size(addresses) != 1) {
            // Display Error: unsuported group chat
            UIAlertController *errView =
            [UIAlertController alertControllerWithTitle:NSLocalizedString(@"Conversation creation error", nil)
                                                message:NSLocalizedString(@"Group conversation is not supported.", nil)
                                         preferredStyle:UIAlertControllerStyleAlert];
            
            UIAlertAction *defaultAction = [UIAlertAction actionWithTitle:@"OK"
                                                                    style:UIAlertActionStyleDefault
                                                                  handler:^(UIAlertAction *action) {}];
            [errView addAction:defaultAction];
            [self presentViewController:errView animated:YES completion:nil];
            return nil;
        }
		LinphoneChatRoom *basicRoom = linphone_core_create_chat_room_5(LC, addresses->data);
        [self goToChatRoom:basicRoom];
        return nil;
    }
    
    _waitView = waitView;
    _waitView.hidden = NO;
    // always use group chatroom
	LinphoneChatRoomParams *param = linphone_core_create_default_chat_room_params(LC);
	linphone_chat_room_params_enable_group(param, isGroup);
	linphone_chat_room_params_enable_encryption(param, isEncrypted);
	
	LinphoneChatRoom *room = linphone_core_create_chat_room_2(LC, param, subject ?: LINPHONE_DUMMY_SUBJECT, addresses);
	
    if (!room) {
        _waitView.hidden = YES;
        return nil;
    }
    
    LinphoneChatRoomCbs *cbs = linphone_factory_create_chat_room_cbs(linphone_factory_get());
    linphone_chat_room_cbs_set_state_changed(cbs, main_view_chat_room_state_changed);
    linphone_chat_room_add_callbacks(room, cbs);
    
    return room;
}

- (void)goToChatRoom:(LinphoneChatRoom *)cr {
	_waitView.hidden = YES;
	_waitView = NULL;
	ChatConversationView *view = VIEW(ChatConversationView);
	if (view.chatRoom && view.chatRoomCbs)
		linphone_chat_room_remove_callbacks(view.chatRoom, view.chatRoomCbs);

	view.chatRoomCbs = NULL;
    if (view.chatRoom != cr)
        [view clearMessageView];
	view.chatRoom = cr;
	self.currentRoom = view.chatRoom;
	if (PhoneMainView.instance.currentView == view.compositeViewDescription)
		[view configureForRoom:FALSE];
	else
		[PhoneMainView.instance changeCurrentView:view.compositeViewDescription];
}

void main_view_chat_room_state_changed(LinphoneChatRoom *cr, LinphoneChatRoomState newState) {
	PhoneMainView *view = PhoneMainView.instance;
	switch (newState) {
		case LinphoneChatRoomStateCreated: {
			LOGI(@"Chat room [%p] created on server.", cr);
			linphone_chat_room_remove_callbacks(cr, linphone_chat_room_get_current_callbacks(cr));
			[view goToChatRoom:cr];
			if (!IPAD)
				break;

			if (PhoneMainView.instance.currentView != ChatsListView.compositeViewDescription && PhoneMainView.instance.currentView != ChatConversationView.compositeViewDescription)
				break;

			ChatsListView *mainView = VIEW(ChatsListView);
			[mainView.tableController loadData];
			[mainView.tableController selectFirstRow];
			break;
		}
		case LinphoneChatRoomStateCreationFailed:
			LOGE(@"Chat room [%p] could not be created on server.", cr);
			linphone_chat_room_remove_callbacks(cr, linphone_chat_room_get_current_callbacks(cr));
			view.waitView.hidden = YES;
			[ChatConversationInfoView displayCreationError];
			break;
		case LinphoneChatRoomStateTerminated:
			LOGI(@"Chat room [%p] has been terminated.", cr);
			[view goToChatRoom:cr];
			break;
		default:
			break;
	}
}

#pragma mark - SMS invite callback

- (void)messageComposeViewController:(MFMessageComposeViewController *)controller didFinishWithResult:(MessageComposeResult)result {
    [controller dismissModalViewControllerAnimated:YES];
}

@end
