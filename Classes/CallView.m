/* InCallViewController.h
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

#import <AddressBook/AddressBook.h>
#import <AudioToolbox/AudioToolbox.h>
#import <OpenGLES/EAGL.h>
#import <OpenGLES/EAGLDrawable.h>
#import <QuartzCore/CAAnimation.h>
#import <QuartzCore/QuartzCore.h>
#import <UserNotifications/UserNotifications.h>

#import "CallView.h"
#import "CallSideMenuView.h"
#import "LinphoneManager.h"
#import "PhoneMainView.h"
#import "Utils.h"

#include "linphone/linphonecore.h"

const NSInteger SECURE_BUTTON_TAG = 5;

@implementation CallView {
	BOOL hiddenVolume;
}

#pragma mark - Lifecycle Functions

- (id)init {
	self = [super initWithNibName:NSStringFromClass(self.class) bundle:[NSBundle mainBundle]];
	if (self != nil) {
		singleFingerTap = [[UITapGestureRecognizer alloc] initWithTarget:self action:@selector(toggleControls:)];
		videoZoomHandler = [[VideoZoomHandler alloc] init];
		videoHidden = TRUE;
	}
	return self;
}

#pragma mark - UICompositeViewDelegate Functions

static UICompositeViewDescription *compositeDescription = nil;

+ (UICompositeViewDescription *)compositeViewDescription {
	if (compositeDescription == nil) {
		compositeDescription = [[UICompositeViewDescription alloc] init:self.class
															  statusBar:StatusBarView.class
																 tabBar:nil
															   sideMenu:CallSideMenuView.class
															 fullscreen:false
														 isLeftFragment:YES
														   fragmentWith:nil];
		compositeDescription.darkBackground = true;
	}
	return compositeDescription;
}

- (UICompositeViewDescription *)compositeViewDescription {
	return self.class.compositeViewDescription;
}

#pragma mark - ViewController Functions

- (void)viewDidLoad {
	[super viewDidLoad];

	_routesEarpieceButton.enabled = !IPAD;

// TODO: fixme! video preview frame is too big compared to openGL preview
// frame, so until this is fixed, temporary disabled it.
#if 0
	_videoPreview.layer.borderColor = UIColor.whiteColor.CGColor;
	_videoPreview.layer.borderWidth = 1;
#endif
	[singleFingerTap setNumberOfTapsRequired:1];
	[singleFingerTap setCancelsTouchesInView:FALSE];
	[self.videoView addGestureRecognizer:singleFingerTap];

	[videoZoomHandler setup:_videoGroup];
	_videoGroup.alpha = 0;

	[_videoCameraSwitch setPreview:_videoPreview];

	UIPanGestureRecognizer *dragndrop =
		[[UIPanGestureRecognizer alloc] initWithTarget:self action:@selector(moveVideoPreview:)];
	dragndrop.minimumNumberOfTouches = 1;
	[_videoPreview addGestureRecognizer:dragndrop];

	[_zeroButton setDigit:'0'];
	[_zeroButton setDtmf:true];
	[_oneButton setDigit:'1'];
	[_oneButton setDtmf:true];
	[_twoButton setDigit:'2'];
	[_twoButton setDtmf:true];
	[_threeButton setDigit:'3'];
	[_threeButton setDtmf:true];
	[_fourButton setDigit:'4'];
	[_fourButton setDtmf:true];
	[_fiveButton setDigit:'5'];
	[_fiveButton setDtmf:true];
	[_sixButton setDigit:'6'];
	[_sixButton setDtmf:true];
	[_sevenButton setDigit:'7'];
	[_sevenButton setDtmf:true];
	[_eightButton setDigit:'8'];
	[_eightButton setDtmf:true];
	[_nineButton setDigit:'9'];
	[_nineButton setDtmf:true];
	[_starButton setDigit:'*'];
	[_starButton setDtmf:true];
	[_hashButton setDigit:'#'];
	[_hashButton setDtmf:true];
}

- (void)dealloc {
	[PhoneMainView.instance.view removeGestureRecognizer:singleFingerTap];
	// Remove all observer
	[NSNotificationCenter.defaultCenter removeObserver:self];
}

- (void)viewWillAppear:(BOOL)animated {
	[super viewWillAppear:animated];

	LinphoneManager.instance.nextCallIsTransfer = NO;

	[self updateUnreadMessage:FALSE];

	// Update on show
	[self hideRoutes:TRUE animated:FALSE];
	[self hideOptions:TRUE animated:FALSE];
	[self hidePad:TRUE animated:FALSE];
	[self hideSpeaker:LinphoneManager.instance.bluetoothAvailable];
	[self callDurationUpdate];
	[self onCurrentCallChange];
	// Set windows (warn memory leaks)
	linphone_core_set_native_video_window_id(LC, (__bridge void *)(_videoView));
	linphone_core_set_native_preview_window_id(LC, (__bridge void *)(_videoPreview));

	[self previewTouchLift];
	// Enable tap
	[singleFingerTap setEnabled:TRUE];

	[NSNotificationCenter.defaultCenter addObserver:self
										   selector:@selector(messageReceived:)
											   name:kLinphoneMessageReceived
											 object:nil];
	[NSNotificationCenter.defaultCenter addObserver:self
										   selector:@selector(bluetoothAvailabilityUpdateEvent:)
											   name:kLinphoneBluetoothAvailabilityUpdate
											 object:nil];
	[NSNotificationCenter.defaultCenter addObserver:self
										   selector:@selector(callUpdateEvent:)
											   name:kLinphoneCallUpdate
											 object:nil];

	[NSTimer scheduledTimerWithTimeInterval:1
									 target:self
								   selector:@selector(callDurationUpdate)
								   userInfo:nil
									repeats:YES];
}

- (void)viewDidAppear:(BOOL)animated {
	[super viewDidAppear:animated];

	[[UIApplication sharedApplication] setIdleTimerDisabled:YES];
	UIDevice.currentDevice.proximityMonitoringEnabled = YES;

	[PhoneMainView.instance setVolumeHidden:TRUE];
	hiddenVolume = TRUE;

	// we must wait didAppear to reset fullscreen mode because we cannot change it in viewwillappear
	LinphoneCall *call = linphone_core_get_current_call(LC);
	LinphoneCallState state = (call != NULL) ? linphone_call_get_state(call) : 0;
	[self callUpdate:call state:state animated:FALSE];
}

- (void)viewWillDisappear:(BOOL)animated {
	[super viewWillDisappear:animated];

	[self disableVideoDisplay:TRUE animated:NO];

	if (hideControlsTimer != nil) {
		[hideControlsTimer invalidate];
		hideControlsTimer = nil;
	}

	if (hiddenVolume) {
		[PhoneMainView.instance setVolumeHidden:FALSE];
		hiddenVolume = FALSE;
	}

	if (videoDismissTimer) {
		[self dismissVideoActionSheet:videoDismissTimer];
		[videoDismissTimer invalidate];
		videoDismissTimer = nil;
	}

	// Remove observer
	[NSNotificationCenter.defaultCenter removeObserver:self];
}

- (void)viewDidDisappear:(BOOL)animated {
	[super viewDidDisappear:animated];

	[[UIApplication sharedApplication] setIdleTimerDisabled:false];
	UIDevice.currentDevice.proximityMonitoringEnabled = NO;

	[PhoneMainView.instance fullScreen:false];
	// Disable tap
	[singleFingerTap setEnabled:FALSE];

	if (linphone_core_get_calls_nb(LC) == 0) {
		// reseting speaker button because no more call
		_speakerButton.selected = FALSE;
	}
}

- (void)didRotateFromInterfaceOrientation:(UIInterfaceOrientation)fromInterfaceOrientation {
	[super didRotateFromInterfaceOrientation:fromInterfaceOrientation];
	[self updateUnreadMessage:NO];
	[self previewTouchLift];
	[self hideStatusBar:!videoHidden && (_nameLabel.alpha <= 0.f)];
}

#pragma mark - UI modification

- (void)hideSpinnerIndicator:(LinphoneCall *)call {
	_videoWaitingForFirstImage.hidden = TRUE;
}

static void hideSpinner(LinphoneCall *call, void *user_data) {
	CallView *thiz = (__bridge CallView *)user_data;
	[thiz hideSpinnerIndicator:call];
}

- (void)updateBottomBar:(LinphoneCall *)call state:(LinphoneCallState)state {
	[_speakerButton update];
	[_microButton update];
	[_callPauseButton update];
	[_conferencePauseButton update];
	[_videoButton update];
	[_hangupButton update];

	_optionsButton.enabled = (!call || !linphone_core_sound_resources_locked(LC));
	_optionsTransferButton.enabled = call && !linphone_core_sound_resources_locked(LC);
	// enable conference button if 2 calls are presents and at least one is not in the conference
	int confSize = linphone_core_get_conference_size(LC) - (linphone_core_is_in_conference(LC) ? 1 : 0);
	_optionsConferenceButton.enabled =
		((linphone_core_get_calls_nb(LC) > 1) && (linphone_core_get_calls_nb(LC) != confSize));

	// Disable transfert in conference
	if (linphone_core_get_current_call(LC) == NULL) {
		[_optionsTransferButton setEnabled:FALSE];
	} else {
		[_optionsTransferButton setEnabled:TRUE];
	}

	switch (state) {
		case LinphoneCallEnd:
		case LinphoneCallError:
		case LinphoneCallIncoming:
		case LinphoneCallOutgoing:
			[self hidePad:TRUE animated:TRUE];
			[self hideOptions:TRUE animated:TRUE];
			[self hideRoutes:TRUE animated:TRUE];
		default:
			break;
	}
}

- (void)toggleControls:(id)sender {
	bool controlsHidden = (_bottomBar.alpha == 0.0);
	[self hideControls:!controlsHidden sender:sender];
}

- (void)timerHideControls:(id)sender {
	[self hideControls:TRUE sender:sender];
}

- (void)hideControls:(BOOL)hidden sender:(id)sender {
	if (videoHidden && hidden)
		return;

	if (hideControlsTimer) {
		[hideControlsTimer invalidate];
		hideControlsTimer = nil;
	}

	if ([[PhoneMainView.instance currentView] equal:CallView.compositeViewDescription]) {
		// show controls
		[UIView beginAnimations:nil context:nil];
		[UIView setAnimationDuration:0.35];
		_pausedCallsTable.tableView.alpha = _videoCameraSwitch.alpha = _callPauseButton.alpha = _routesView.alpha =
			_optionsView.alpha = _numpadView.alpha = _bottomBar.alpha = (hidden ? 0 : 1);
		_nameLabel.alpha = _durationLabel.alpha = (hidden ? 0 : .8f);

		[self hideStatusBar:hidden];

		[UIView commitAnimations];

		[PhoneMainView.instance hideTabBar:hidden];

		if (!hidden) {
			// hide controls in 5 sec
			hideControlsTimer = [NSTimer scheduledTimerWithTimeInterval:5.0
																 target:self
															   selector:@selector(timerHideControls:)
															   userInfo:nil
																repeats:NO];
		}
	}
}

- (void)disableVideoDisplay:(BOOL)disabled animated:(BOOL)animation {
	if (disabled == videoHidden && animation)
		return;
	videoHidden = disabled;

	if (!disabled) {
		[videoZoomHandler resetZoom];
	}
	if (animation) {
		[UIView beginAnimations:nil context:nil];
		[UIView setAnimationDuration:1.0];
	}

	[_videoGroup setAlpha:disabled ? 0 : 1];

	[self hideControls:!disabled sender:nil];

	if (animation) {
		[UIView commitAnimations];
	}

	// only show camera switch button if we have more than 1 camera
	_videoCameraSwitch.hidden = (disabled || !LinphoneManager.instance.frontCamId);
	_videoPreview.hidden = (disabled || !linphone_core_self_view_enabled(LC));

	if (hideControlsTimer != nil) {
		[hideControlsTimer invalidate];
		hideControlsTimer = nil;
	}

	[PhoneMainView.instance fullScreen:!disabled];
	[PhoneMainView.instance hideTabBar:!disabled];

	if (!disabled) {
#ifdef TEST_VIDEO_VIEW_CHANGE
		[NSTimer scheduledTimerWithTimeInterval:5.0
										 target:self
									   selector:@selector(_debugChangeVideoView)
									   userInfo:nil
										repeats:YES];
#endif
		// [self batteryLevelChanged:nil];

		[_videoWaitingForFirstImage setHidden:NO];
		[_videoWaitingForFirstImage startAnimating];

		LinphoneCall *call = linphone_core_get_current_call(LC);
		// linphone_call_params_get_used_video_codec return 0 if no video stream enabled
		if (call != NULL && linphone_call_params_get_used_video_codec(linphone_call_get_current_params(call))) {
			linphone_call_set_next_video_frame_decoded_callback(call, hideSpinner, (__bridge void *)(self));
		}
	}
}

- (void)displayVideoCall:(BOOL)animated {
	[self disableVideoDisplay:FALSE animated:animated];
}

- (void)displayAudioCall:(BOOL)animated {
	[self disableVideoDisplay:TRUE animated:animated];
}

- (void)hideStatusBar:(BOOL)hide {
	/* we cannot use [PhoneMainView.instance show]; because it will automatically
	 resize current view to fill empty space, which will resize video. This is
	 indesirable since we do not want to crop/rescale video view */
	PhoneMainView.instance.mainViewController.statusBarView.hidden = hide;
}

- (void)callDurationUpdate {
	int duration =
		linphone_core_get_current_call(LC) ? linphone_call_get_duration(linphone_core_get_current_call(LC)) : 0;
	_durationLabel.text = [LinphoneUtils durationToString:duration];

	[_pausedCallsTable update];
	[_conferenceCallsTable update];
}

- (void)onCurrentCallChange {
	LinphoneCall *call = linphone_core_get_current_call(LC);

	_noActiveCallView.hidden = (call || linphone_core_is_in_conference(LC));
	_callView.hidden = !call;
	_conferenceView.hidden = !linphone_core_is_in_conference(LC);
	_callPauseButton.hidden = !call && !linphone_core_is_in_conference(LC);

	[_callPauseButton setType:UIPauseButtonType_CurrentCall call:call];
	[_conferencePauseButton setType:UIPauseButtonType_Conference call:call];

	if (!_callView.hidden) {
		const LinphoneAddress *addr = linphone_call_get_remote_address(call);
		[ContactDisplay setDisplayNameLabel:_nameLabel forAddress:addr];
		char *uri = linphone_address_as_string_uri_only(addr);
		ms_free(uri);
		[_avatarImage setImage:[FastAddressBook imageForAddress:addr thumbnail:NO] bordered:YES withRoundedRadius:YES];
	}
}

- (void)hidePad:(BOOL)hidden animated:(BOOL)animated {
	if (hidden) {
		[_numpadButton setOff];
	} else {
		[_numpadButton setOn];
	}
	if (hidden != _numpadView.hidden) {
		if (animated) {
			[self hideAnimation:hidden forView:_numpadView completion:nil];
		} else {
			[_numpadView setHidden:hidden];
		}
	}
}

- (void)hideRoutes:(BOOL)hidden animated:(BOOL)animated {
	if (hidden) {
		[_routesButton setOff];
	} else {
		[_routesButton setOn];
	}

	_routesBluetoothButton.selected = LinphoneManager.instance.bluetoothEnabled;
	_routesSpeakerButton.selected = LinphoneManager.instance.speakerEnabled;
	_routesEarpieceButton.selected = !_routesBluetoothButton.selected && !_routesSpeakerButton.selected;

	if (hidden != _routesView.hidden) {
		if (animated) {
			[self hideAnimation:hidden forView:_routesView completion:nil];
		} else {
			[_routesView setHidden:hidden];
		}
	}
}

- (void)hideOptions:(BOOL)hidden animated:(BOOL)animated {
	if (hidden) {
		[_optionsButton setOff];
	} else {
		[_optionsButton setOn];
	}
	if (hidden != _optionsView.hidden) {
		if (animated) {
			[self hideAnimation:hidden forView:_optionsView completion:nil];
		} else {
			[_optionsView setHidden:hidden];
		}
	}
}

- (void)hideSpeaker:(BOOL)hidden {
	_speakerButton.hidden = hidden;
	_routesButton.hidden = !hidden;
}

#pragma mark - Event Functions

- (void)bluetoothAvailabilityUpdateEvent:(NSNotification *)notif {
	bool available = [[notif.userInfo objectForKey:@"available"] intValue];
	[self hideSpeaker:available];
}

- (void)callUpdateEvent:(NSNotification *)notif {
	LinphoneCall *call = [[notif.userInfo objectForKey:@"call"] pointerValue];
	LinphoneCallState state = [[notif.userInfo objectForKey:@"state"] intValue];
	[self callUpdate:call state:state animated:TRUE];
}

- (void)callUpdate:(LinphoneCall *)call state:(LinphoneCallState)state animated:(BOOL)animated {
	[self updateBottomBar:call state:state];
	if (hiddenVolume) {
		[PhoneMainView.instance setVolumeHidden:FALSE];
		hiddenVolume = FALSE;
	}

	// Update tables
	[_pausedCallsTable update];
	[_conferenceCallsTable update];

	static LinphoneCall *currentCall = NULL;
	if (!currentCall || linphone_core_get_current_call(LC) != currentCall) {
		currentCall = linphone_core_get_current_call(LC);
		[self onCurrentCallChange];
	}

	// Fake call update
	if (call == NULL) {
		return;
	}

	BOOL shouldDisableVideo =
		(!currentCall || !linphone_call_params_video_enabled(linphone_call_get_current_params(currentCall)));
	if (videoHidden != shouldDisableVideo) {
		if (!shouldDisableVideo) {
			[self displayVideoCall:animated];
		} else {
			[self displayAudioCall:animated];
		}
	}

	if (state != LinphoneCallPausedByRemote) {
		_pausedByRemoteView.hidden = YES;
	}

	switch (state) {
		case LinphoneCallIncomingReceived:
		case LinphoneCallOutgoingInit:
		case LinphoneCallConnected:
		case LinphoneCallStreamsRunning: {
			// check video
			if (!linphone_call_params_video_enabled(linphone_call_get_current_params(call))) {
				const LinphoneCallParams *param = linphone_call_get_current_params(call);
				const LinphoneCallAppData *callAppData =
					(__bridge const LinphoneCallAppData *)(linphone_call_get_user_pointer(call));
				if (state == LinphoneCallStreamsRunning && callAppData->videoRequested &&
					linphone_call_params_low_bandwidth_enabled(param)) {
					// too bad video was not enabled because low bandwidth
					UIAlertController *errView = [UIAlertController alertControllerWithTitle:NSLocalizedString(@"Low bandwidth", nil)
																					 message:NSLocalizedString(@"Video cannot be activated because of low bandwidth "
																											   @"condition, only audio is available",
																											   nil)
																			  preferredStyle:UIAlertControllerStyleAlert];
						
					UIAlertAction* defaultAction = [UIAlertAction actionWithTitle:NSLocalizedString(@"Continue", nil)
																			style:UIAlertActionStyleDefault
																		  handler:^(UIAlertAction * action) {}];
						
					[errView addAction:defaultAction];
					[self presentViewController:errView animated:YES completion:nil];
					callAppData->videoRequested = FALSE; /*reset field*/
				}
			}
			break;
		}
		case LinphoneCallUpdatedByRemote: {
			const LinphoneCallParams *current = linphone_call_get_current_params(call);
			const LinphoneCallParams *remote = linphone_call_get_remote_params(call);

			/* remote wants to add video */
			if ((linphone_core_video_display_enabled(LC) && !linphone_call_params_video_enabled(current) &&
				 linphone_call_params_video_enabled(remote)) &&
				(!linphone_core_get_video_policy(LC)->automatically_accept ||
				 (([UIApplication sharedApplication].applicationState == UIApplicationStateBackground) &&
				  floor(NSFoundationVersionNumber) > NSFoundationVersionNumber_iOS_9_x_Max))) {
				linphone_core_defer_call_update(LC, call);
				[self displayAskToEnableVideoCall:call];
			} else if (linphone_call_params_video_enabled(current) && !linphone_call_params_video_enabled(remote)) {
				[self displayAudioCall:animated];
			}
			break;
		}
		case LinphoneCallPausing:
		case LinphoneCallPaused:
			[self displayAudioCall:animated];
			break;
		case LinphoneCallPausedByRemote:
			[self displayAudioCall:animated];
			if (call == linphone_core_get_current_call(LC)) {
				_pausedByRemoteView.hidden = NO;
			}
			break;
		case LinphoneCallEnd:
		case LinphoneCallError:
		default:
			break;
	}
}

#pragma mark - ActionSheet Functions

- (void)displayAskToEnableVideoCall:(LinphoneCall *)call {
	if (linphone_core_get_video_policy(LC)->automatically_accept &&
		!([UIApplication sharedApplication].applicationState == UIApplicationStateBackground))
		return;

	NSString *username = [FastAddressBook displayNameForAddress:linphone_call_get_remote_address(call)];
	NSString *title = [NSString stringWithFormat:NSLocalizedString(@"%@ would like to enable video", nil), username];
	if ([UIApplication sharedApplication].applicationState == UIApplicationStateBackground &&
		floor(NSFoundationVersionNumber) > NSFoundationVersionNumber_iOS_9_x_Max) {
		UNMutableNotificationContent *content = [[UNMutableNotificationContent alloc] init];
		content.title = NSLocalizedString(@"Video request", nil);
		content.body = title;
		content.categoryIdentifier = @"video_request";
		content.userInfo = @{
			@"CallId" : [NSString stringWithUTF8String:linphone_call_log_get_call_id(linphone_call_get_call_log(call))]
		};

		UNNotificationRequest *req =
			[UNNotificationRequest requestWithIdentifier:@"video_request" content:content trigger:NULL];
		[[UNUserNotificationCenter currentNotificationCenter] addNotificationRequest:req
															   withCompletionHandler:^(NSError *_Nullable error) {
																 // Enable or disable features based on authorization.
																 if (error) {
																	 LOGD(@"Error while adding notification request :");
																	 LOGD(error.description);
																 }
															   }];
	} else {
		UIConfirmationDialog *sheet = [UIConfirmationDialog ShowWithMessage:title
			cancelMessage:nil
			confirmMessage:NSLocalizedString(@"ACCEPT", nil)
			onCancelClick:^() {
			  LOGI(@"User declined video proposal");
			  if (call == linphone_core_get_current_call(LC)) {
				  LinphoneCallParams *params = linphone_core_create_call_params(LC, call);
				  linphone_core_accept_call_update(LC, call, params);
				  linphone_call_params_destroy(params);
				  [videoDismissTimer invalidate];
				  videoDismissTimer = nil;
			  }
			}
			onConfirmationClick:^() {
			  LOGI(@"User accept video proposal");
			  if (call == linphone_core_get_current_call(LC)) {
				  LinphoneCallParams *params = linphone_core_create_call_params(LC, call);
				  linphone_call_params_enable_video(params, TRUE);
				  linphone_core_accept_call_update(LC, call, params);
				  linphone_call_params_destroy(params);
				  [videoDismissTimer invalidate];
				  videoDismissTimer = nil;
			  }
			}
			inController:self];
		videoDismissTimer = [NSTimer scheduledTimerWithTimeInterval:30
															 target:self
														   selector:@selector(dismissVideoActionSheet:)
														   userInfo:sheet
															repeats:NO];
	}
}

- (void)dismissVideoActionSheet:(NSTimer *)timer {
	UIConfirmationDialog *sheet = (UIConfirmationDialog *)timer.userInfo;
	[sheet dismiss];
}

#pragma mark VideoPreviewMoving

- (void)moveVideoPreview:(UIPanGestureRecognizer *)dragndrop {
	CGPoint center = [dragndrop locationInView:_videoPreview.superview];
	_videoPreview.center = center;
	if (dragndrop.state == UIGestureRecognizerStateEnded) {
		[self previewTouchLift];
	}
}

- (CGFloat)coerce:(CGFloat)value betweenMin:(CGFloat)min andMax:(CGFloat)max {
	return MAX(min, MIN(value, max));
}

- (void)previewTouchLift {
	CGRect previewFrame = _videoPreview.frame;
	previewFrame.origin.x = [self coerce:previewFrame.origin.x
							  betweenMin:5
								  andMax:(UIScreen.mainScreen.bounds.size.width - 5 - previewFrame.size.width)];
	previewFrame.origin.y = [self coerce:previewFrame.origin.y
							  betweenMin:5
								  andMax:(UIScreen.mainScreen.bounds.size.height - 5 - previewFrame.size.height)];

	if (!CGRectEqualToRect(previewFrame, _videoPreview.frame)) {
		dispatch_after(dispatch_time(DISPATCH_TIME_NOW, (int64_t)(0.3 * NSEC_PER_SEC)), dispatch_get_main_queue(), ^{
		  [UIView animateWithDuration:0.3
						   animations:^{
							 LOGD(@"Recentering preview to %@", NSStringFromCGRect(previewFrame));
							 _videoPreview.frame = previewFrame;
						   }];
		});
	}
}

#pragma mark - Action Functions

- (IBAction)onNumpadClick:(id)sender {
	if ([_numpadView isHidden]) {
		[self hidePad:FALSE animated:ANIMATED];
	} else {
		[self hidePad:TRUE animated:ANIMATED];
	}
}

- (IBAction)onChatClick:(id)sender {
	[PhoneMainView.instance changeCurrentView:ChatsListView.compositeViewDescription];
}

- (IBAction)onRoutesBluetoothClick:(id)sender {
	[self hideRoutes:TRUE animated:TRUE];
	[LinphoneManager.instance setBluetoothEnabled:TRUE];
}

- (IBAction)onRoutesEarpieceClick:(id)sender {
	[self hideRoutes:TRUE animated:TRUE];
	[LinphoneManager.instance setSpeakerEnabled:FALSE];
	[LinphoneManager.instance setBluetoothEnabled:FALSE];
}

- (IBAction)onRoutesSpeakerClick:(id)sender {
	[self hideRoutes:TRUE animated:TRUE];
	[LinphoneManager.instance setSpeakerEnabled:TRUE];
}

- (IBAction)onRoutesClick:(id)sender {
	if ([_routesView isHidden]) {
		[self hideRoutes:FALSE animated:ANIMATED];
	} else {
		[self hideRoutes:TRUE animated:ANIMATED];
	}
}

- (IBAction)onOptionsClick:(id)sender {
	if ([_optionsView isHidden]) {
		[self hideOptions:FALSE animated:ANIMATED];
	} else {
		[self hideOptions:TRUE animated:ANIMATED];
	}
}

- (IBAction)onOptionsTransferClick:(id)sender {
	[self hideOptions:TRUE animated:TRUE];
	DialerView *view = VIEW(DialerView);
	[view setAddress:@""];
	LinphoneManager.instance.nextCallIsTransfer = YES;
	[PhoneMainView.instance changeCurrentView:view.compositeViewDescription];
}

- (IBAction)onOptionsAddClick:(id)sender {
	[self hideOptions:TRUE animated:TRUE];
	DialerView *view = VIEW(DialerView);
	[view setAddress:@""];
	LinphoneManager.instance.nextCallIsTransfer = NO;
	[PhoneMainView.instance changeCurrentView:view.compositeViewDescription];
}

- (IBAction)onOptionsConferenceClick:(id)sender {
	[self hideOptions:TRUE animated:TRUE];
	linphone_core_add_all_to_conference(LC);
}

#pragma mark - Animation

- (void)hideAnimation:(BOOL)hidden forView:(UIView *)target completion:(void (^)(BOOL finished))completion {
	if (hidden) {
	int original_y = target.frame.origin.y;
	CGRect newFrame = target.frame;
	newFrame.origin.y = self.view.frame.size.height;
	[UIView animateWithDuration:0.5
		delay:0.0
		options:UIViewAnimationOptionCurveEaseIn
		animations:^{
		  target.frame = newFrame;
		}
		completion:^(BOOL finished) {
		  CGRect originFrame = target.frame;
		  originFrame.origin.y = original_y;
		  target.hidden = YES;
		  target.frame = originFrame;
		  if (completion)
			  completion(finished);
		}];
	} else {
		CGRect frame = target.frame;
		int original_y = frame.origin.y;
		frame.origin.y = self.view.frame.size.height;
		target.frame = frame;
		frame.origin.y = original_y;
		target.hidden = NO;

		[UIView animateWithDuration:0.5
			delay:0.0
			options:UIViewAnimationOptionCurveEaseOut
			animations:^{
			  target.frame = frame;
			}
			completion:^(BOOL finished) {
			  target.frame = frame; // in case application did not finish
			  if (completion)
				  completion(finished);
			}];
	}
}

#pragma mark - Bounce
- (void)messageReceived:(NSNotification *)notif {
	[self updateUnreadMessage:TRUE];
}
- (void)updateUnreadMessage:(BOOL)appear {
	int unreadMessage = [LinphoneManager unreadMessageCount];
	if (unreadMessage > 0) {
		_chatNotificationLabel.text = [NSString stringWithFormat:@"%i", unreadMessage];
		[_chatNotificationView startAnimating:appear];
	} else {
		[_chatNotificationView stopAnimating:appear];
	}
}
@end
