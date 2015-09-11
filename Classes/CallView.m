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

#import <AudioToolbox/AudioToolbox.h>
#import <AddressBook/AddressBook.h>
#import <QuartzCore/CAAnimation.h>
#import <QuartzCore/QuartzCore.h>
#import <OpenGLES/EAGL.h>
#import <OpenGLES/EAGLDrawable.h>

#import "CallView.h"
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
		singleFingerTap = [[UITapGestureRecognizer alloc] initWithTarget:self action:@selector(showControls:)];
		videoZoomHandler = [[VideoZoomHandler alloc] init];
	}
	return self;
}

- (void)dealloc {
	[PhoneMainView.instance.view removeGestureRecognizer:singleFingerTap];

	// Remove all observer
	[[NSNotificationCenter defaultCenter] removeObserver:self];
}

#pragma mark - UICompositeViewDelegate Functions

static UICompositeViewDescription *compositeDescription = nil;

+ (UICompositeViewDescription *)compositeViewDescription {
	if (compositeDescription == nil) {
		compositeDescription = [[UICompositeViewDescription alloc] init:self.class
															  statusBar:StatusBarView.class
																 tabBar:nil
															 fullscreen:false
														  landscapeMode:false
														   portraitMode:true];
		compositeDescription.darkBackground = true;
	}
	return compositeDescription;
}

- (UICompositeViewDescription *)compositeViewDescription {
	return self.class.compositeViewDescription;
}

#pragma mark - ViewController Functions

- (void)viewDidAppear:(BOOL)animated {
	[super viewDidAppear:animated];

	[[UIApplication sharedApplication] setIdleTimerDisabled:YES];
	UIDevice *device = [UIDevice currentDevice];
	device.proximityMonitoringEnabled = YES;

	[PhoneMainView.instance setVolumeHidden:TRUE];
	hiddenVolume = TRUE;
}

- (void)viewWillDisappear:(BOOL)animated {
	[super viewWillDisappear:animated];
	if (hideControlsTimer != nil) {
		[hideControlsTimer invalidate];
		hideControlsTimer = nil;
	}

	if (hiddenVolume) {
		[PhoneMainView.instance setVolumeHidden:FALSE];
		hiddenVolume = FALSE;
	}

	// Remove observer
	[[NSNotificationCenter defaultCenter] removeObserver:self name:kLinphoneCallUpdate object:nil];
}

- (void)viewWillAppear:(BOOL)animated {
	[super viewWillAppear:animated];
	// Set observer
	[[NSNotificationCenter defaultCenter] addObserver:self
											 selector:@selector(callUpdateEvent:)
												 name:kLinphoneCallUpdate
											   object:nil];

	// Update on show
	LinphoneCall *call = linphone_core_get_current_call([LinphoneManager getLc]);
	LinphoneCallState state = (call != NULL) ? linphone_call_get_state(call) : 0;
	[self callUpdate:call state:state animated:FALSE];
	[self hideRoutes:FALSE];
	[self hideOptions:FALSE];
	[self hidePad:FALSE];
	[self showSpeaker];

	// Set windows (warn memory leaks)
	linphone_core_set_native_video_window_id([LinphoneManager getLc], (__bridge void *)(_videoView));
	linphone_core_set_native_preview_window_id([LinphoneManager getLc], (__bridge void *)(_videoPreview));

	// Enable tap
	[singleFingerTap setEnabled:TRUE];

	[[NSNotificationCenter defaultCenter] addObserver:self
											 selector:@selector(callUpdateEvent:)
												 name:kLinphoneCallUpdate
											   object:nil];
	[[NSNotificationCenter defaultCenter] addObserver:self
											 selector:@selector(bluetoothAvailabilityUpdateEvent:)
												 name:kLinphoneBluetoothAvailabilityUpdate
											   object:nil];

	const LinphoneAddress *addr = linphone_call_get_remote_address(call);
	[ContactDisplay setDisplayNameLabel:_nameLabel forAddress:addr];
	char *uri = linphone_address_as_string_uri_only(addr);
	_addressLabel.text = [NSString stringWithUTF8String:uri];
	ms_free(uri);
	_avatarImage.image =
		[FastAddressBook getContactImage:[FastAddressBook getContactWithLinphoneAddress:addr] thumbnail:NO];
}

- (void)viewDidDisappear:(BOOL)animated {
	[super viewDidDisappear:animated];

	[[NSNotificationCenter defaultCenter] removeObserver:self];

	[[UIApplication sharedApplication] setIdleTimerDisabled:false];
	UIDevice *device = [UIDevice currentDevice];
	device.proximityMonitoringEnabled = NO;

	[PhoneMainView.instance fullScreen:false];
	// Disable tap
	[singleFingerTap setEnabled:FALSE];

	if (linphone_core_get_calls_nb([LinphoneManager getLc]) == 0) {
		// reseting speaker button because no more call
		_speakerButton.selected = FALSE;
	}
}

- (void)viewDidLoad {
	[super viewDidLoad];

	[singleFingerTap setNumberOfTapsRequired:1];
	[singleFingerTap setCancelsTouchesInView:FALSE];
	[PhoneMainView.instance.view addGestureRecognizer:singleFingerTap];

	[videoZoomHandler setup:_videoGroup];
	_videoGroup.alpha = 0;

	[_videoCameraSwitch setPreview:_videoPreview];

	UIPanGestureRecognizer *dragndrop =
		[[UIPanGestureRecognizer alloc] initWithTarget:self action:@selector(moveVideoPreview:)];
	dragndrop.minimumNumberOfTouches = 1;
	[_videoPreview addGestureRecognizer:dragndrop];

	[_pauseButton setType:UIPauseButtonType_CurrentCall call:nil];

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
	[_sharpButton setDigit:'#'];
	[_sharpButton setDtmf:true];
}

- (void)viewDidUnload {
	[super viewDidUnload];
	[PhoneMainView.instance.view removeGestureRecognizer:singleFingerTap];
}

- (void)willAnimateRotationToInterfaceOrientation:(UIInterfaceOrientation)toInterfaceOrientation
										 duration:(NSTimeInterval)duration {
	[super willAnimateRotationToInterfaceOrientation:toInterfaceOrientation duration:duration];
	// in mode display_filter_auto_rotate=0, no need to rotate the preview
}

- (void)didRotateFromInterfaceOrientation:(UIInterfaceOrientation)fromInterfaceOrientation {
	[super didRotateFromInterfaceOrientation:fromInterfaceOrientation];
	[self previewTouchLift];
}

#pragma mark -

- (void)callUpdate:(LinphoneCall *)call state:(LinphoneCallState)state animated:(BOOL)animated {
	LinphoneCore *lc = [LinphoneManager getLc];
	[self updateBottomBar:call state:state];
	if (hiddenVolume) {
		[PhoneMainView.instance setVolumeHidden:FALSE];
		hiddenVolume = FALSE;
	}

	// Update table
	[_pausedCallsTableView.tableView reloadData];

	// Fake call update
	if (call == NULL) {
		return;
	}

	switch (state) {
		case LinphoneCallIncomingReceived:
		case LinphoneCallOutgoingInit:
		case LinphoneCallConnected:
		case LinphoneCallStreamsRunning: {
			// check video
			if (linphone_call_params_video_enabled(linphone_call_get_current_params(call))) {
				[self displayVideoCall:animated];
			} else {
				[self displayTableCall:animated];
				const LinphoneCallParams *param = linphone_call_get_current_params(call);
				const LinphoneCallAppData *callAppData =
					(__bridge const LinphoneCallAppData *)(linphone_call_get_user_pointer(call));
				if (state == LinphoneCallStreamsRunning && callAppData->videoRequested &&
					linphone_call_params_low_bandwidth_enabled(param)) {
					// too bad video was not enabled because low bandwidth
					UIAlertView *alert = [[UIAlertView alloc]
							initWithTitle:NSLocalizedString(@"Low bandwidth", nil)
								  message:NSLocalizedString(@"Video cannot be activated because of low bandwidth "
															@"condition, only audio is available",
															nil)
								 delegate:nil
						cancelButtonTitle:NSLocalizedString(@"Continue", nil)
						otherButtonTitles:nil];
					[alert show];
					callAppData->videoRequested = FALSE; /*reset field*/
				}
			}
			break;
		}
		case LinphoneCallUpdatedByRemote: {
			const LinphoneCallParams *current = linphone_call_get_current_params(call);
			const LinphoneCallParams *remote = linphone_call_get_remote_params(call);

			/* remote wants to add video */
			if (linphone_core_video_enabled(lc) && !linphone_call_params_video_enabled(current) &&
				linphone_call_params_video_enabled(remote) &&
				!linphone_core_get_video_policy(lc)->automatically_accept) {
				linphone_core_defer_call_update(lc, call);
				[self displayAskToEnableVideoCall:call];
			} else if (linphone_call_params_video_enabled(current) && !linphone_call_params_video_enabled(remote)) {
				[self displayTableCall:animated];
			}
			break;
		}
		case LinphoneCallPausing:
		case LinphoneCallPaused:
		case LinphoneCallPausedByRemote: {
			[self displayTableCall:animated];
			break;
		}
		case LinphoneCallEnd:
		case LinphoneCallError:
		default:
			break;
	}
}

- (void)updateBottomBar:(LinphoneCall *)call state:(LinphoneCallState)state {
	LinphoneCore *lc = [LinphoneManager getLc];

	[_speakerButton update];
	[_microButton update];
	[_pauseButton update];
	[_videoButton update];
	[_hangupButton update];

	_optionsButton.enabled =
		(state == LinphoneCallPaused || state == LinphoneCallPausing || state == LinphoneCallStreamsRunning);

	// Show Pause/Conference button following call count
	if (linphone_core_get_calls_nb(lc) > 1) {
		if (![_pauseButton isHidden]) {
			[_pauseButton setHidden:true];
			[_optionsConferenceButton setHidden:false];
		}
		bool enabled = true;
		const MSList *list = linphone_core_get_calls(lc);
		while (list != NULL) {
			LinphoneCall *call = (LinphoneCall *)list->data;
			LinphoneCallState state = linphone_call_get_state(call);
			if (state == LinphoneCallIncomingReceived || state == LinphoneCallOutgoingInit ||
				state == LinphoneCallOutgoingProgress || state == LinphoneCallOutgoingRinging ||
				state == LinphoneCallOutgoingEarlyMedia || state == LinphoneCallConnected) {
				enabled = false;
			}
			list = list->next;
		}
		[_optionsConferenceButton setEnabled:enabled];
	} else {
		if ([_pauseButton isHidden]) {
			[_pauseButton setHidden:false];
			[_optionsConferenceButton setHidden:true];
		}
	}

	// Disable transfert in conference
	if (linphone_core_get_current_call(lc) == NULL) {
		[_optionsTransferButton setEnabled:FALSE];
	} else {
		[_optionsTransferButton setEnabled:TRUE];
	}

	switch (state) {
		case LinphoneCallEnd:
		case LinphoneCallError:
		case LinphoneCallIncoming:
		case LinphoneCallOutgoing:
			[self hidePad:TRUE];
			[self hideOptions:TRUE];
			[self hideRoutes:TRUE];
		default:
			break;
	}
}

- (void)bluetoothAvailabilityUpdate:(bool)available {
	if (available) {
		[self hideSpeaker];
	} else {
		[self showSpeaker];
	}
}

- (void)showControls:(id)sender {
	if (hideControlsTimer) {
		[hideControlsTimer invalidate];
		hideControlsTimer = nil;
	}

	if ([[PhoneMainView.instance currentView] equal:CallView.compositeViewDescription] && videoShown) {
		// show controls
		[UIView beginAnimations:nil context:nil];
		[UIView setAnimationDuration:0.3];
		[PhoneMainView.instance showTabBar:true];
		[PhoneMainView.instance showStatusBar:true];
		[_pausedCallsTableView.tableView setAlpha:1.0];
		[_videoCameraSwitch setAlpha:1.0];
		[UIView commitAnimations];

		// hide controls in 5 sec
		hideControlsTimer = [NSTimer scheduledTimerWithTimeInterval:5.0
															 target:self
														   selector:@selector(hideControls:)
														   userInfo:nil
															repeats:NO];
	}
}

- (void)hideControls:(id)sender {
	if (hideControlsTimer) {
		[hideControlsTimer invalidate];
		hideControlsTimer = nil;
	}

	if ([[PhoneMainView.instance currentView] equal:CallView.compositeViewDescription] && videoShown) {
		[UIView beginAnimations:nil context:nil];
		[UIView setAnimationDuration:0.3];
		[_videoCameraSwitch setAlpha:0.0];
		[_pausedCallsTableView.tableView setAlpha:0.0];
		[UIView commitAnimations];

		[PhoneMainView.instance showTabBar:false];
		[PhoneMainView.instance showStatusBar:false];
	}
}

- (void)enableVideoDisplay:(BOOL)animation {
	if (videoShown && animation)
		return;

	videoShown = true;

	[videoZoomHandler resetZoom];

	if (animation) {
		[UIView beginAnimations:nil context:nil];
		[UIView setAnimationDuration:1.0];
	}

	[_videoGroup setAlpha:1.0];
	[_pausedCallsTableView.tableView setAlpha:0.0];

	UIEdgeInsets insets = {33, 0, 25, 0};
	[_pausedCallsTableView.tableView setContentInset:insets];
	[_pausedCallsTableView.tableView setScrollIndicatorInsets:insets];

	if (animation) {
		[UIView commitAnimations];
	}

	if (linphone_core_self_view_enabled([LinphoneManager getLc])) {
		[_videoPreview setHidden:FALSE];
	} else {
		[_videoPreview setHidden:TRUE];
	}

	if ([LinphoneManager instance].frontCamId != nil) {
		// only show camera switch button if we have more than 1 camera
		[_videoCameraSwitch setHidden:FALSE];
	}
	[_videoCameraSwitch setAlpha:0.0];

	[PhoneMainView.instance fullScreen:true];
	[PhoneMainView.instance showTabBar:false];
	[PhoneMainView.instance showStatusBar:false];

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

	LinphoneCall *call = linphone_core_get_current_call([LinphoneManager getLc]);
	// linphone_call_params_get_used_video_codec return 0 if no video stream enabled
	if (call != NULL && linphone_call_params_get_used_video_codec(linphone_call_get_current_params(call))) {
		linphone_call_set_next_video_frame_decoded_callback(call, hideSpinner, (__bridge void *)(self));
	}
}

- (void)disableVideoDisplay:(BOOL)animation {
	if (!videoShown && animation)
		return;

	videoShown = false;
	if (animation) {
		[UIView beginAnimations:nil context:nil];
		[UIView setAnimationDuration:1.0];
	}

	[_videoGroup setAlpha:0.0];
	[PhoneMainView.instance showTabBar:true];

	UIEdgeInsets insets = {10, 0, 25, 0};
	[_pausedCallsTableView.tableView setContentInset:insets];
	[_pausedCallsTableView.tableView setScrollIndicatorInsets:insets];
	[_pausedCallsTableView.tableView setAlpha:1.0];

	[_pausedCallsTableView.tableView setAlpha:1.0];
	[_videoCameraSwitch setHidden:TRUE];

	if (animation) {
		[UIView commitAnimations];
	}

	if (hideControlsTimer != nil) {
		[hideControlsTimer invalidate];
		hideControlsTimer = nil;
	}

	[PhoneMainView.instance fullScreen:false];
}

- (void)displayVideoCall:(BOOL)animated {
	[self enableVideoDisplay:animated];
}

- (void)displayTableCall:(BOOL)animated {
	[self disableVideoDisplay:animated];
}

#pragma mark - Spinner Functions

- (void)hideSpinnerIndicator:(LinphoneCall *)call {
	_videoWaitingForFirstImage.hidden = TRUE;
}

static void hideSpinner(LinphoneCall *call, void *user_data) {
	CallView *thiz = (__bridge CallView *)user_data;
	[thiz hideSpinnerIndicator:call];
}

#pragma mark - UI modification

- (void)showPad:(BOOL)animated {
	[_numpadButton setOn];
	if ([_numpadView isHidden]) {
		if (animated) {
			[self showAnimation:@"show"
						 target:_numpadView
					 completion:^(BOOL finished){
					 }];
		} else {
			[_numpadView setHidden:FALSE];
		}
	}
}

- (void)hidePad:(BOOL)animated {
	[_numpadButton setOff];
	if (![_numpadView isHidden]) {
		if (animated) {
			[self hideAnimation:@"hide"
						 target:_numpadView
					 completion:^(BOOL finished){
					 }];
		} else {
			[_numpadView setHidden:TRUE];
		}
	}
}

- (void)showRoutes:(BOOL)animated {
	if (![LinphoneManager runningOnIpad]) {
		[_routesButton setOn];
		[_routesBluetoothButton setSelected:[[LinphoneManager instance] bluetoothEnabled]];
		[_routesSpeakerButton setSelected:[[LinphoneManager instance] speakerEnabled]];
		[_routesReceiverButton setSelected:!([[LinphoneManager instance] bluetoothEnabled] ||
											 [[LinphoneManager instance] speakerEnabled])];
		if ([_routesView isHidden]) {
			if (animated) {
				[self showAnimation:@"show"
							 target:_routesView
						 completion:^(BOOL finished){
						 }];
			} else {
				[_routesView setHidden:FALSE];
			}
		}
	}
}

- (void)hideRoutes:(BOOL)animated {
	if (![LinphoneManager runningOnIpad]) {
		[_routesButton setOff];
		if (![_routesView isHidden]) {
			if (animated) {
				[self hideAnimation:@"hide"
							 target:_routesView
						 completion:^(BOOL finished){
						 }];
			} else {
				[_routesView setHidden:TRUE];
			}
		}
	}
}

- (void)showOptions:(BOOL)animated {
	[_optionsButton setOn];
	if ([_optionsView isHidden]) {
		if (animated) {
			[self showAnimation:@"show"
						 target:_optionsView
					 completion:^(BOOL finished){
					 }];
		} else {
			[_optionsView setHidden:FALSE];
		}
	}
}

- (void)hideOptions:(BOOL)animated {
	[_optionsButton setOff];
	if (![_optionsView isHidden]) {
		if (animated) {
			[self hideAnimation:@"hide"
						 target:_optionsView
					 completion:^(BOOL finished){
					 }];
		} else {
			[_optionsView setHidden:TRUE];
		}
	}
}

- (void)showSpeaker {
	if (![LinphoneManager runningOnIpad]) {
		[_speakerButton setHidden:FALSE];
		[_routesButton setHidden:TRUE];
	}
}

- (void)hideSpeaker {
	if (![LinphoneManager runningOnIpad]) {
		[_speakerButton setHidden:TRUE];
		[_routesButton setHidden:FALSE];
	}
}

#pragma mark - Event Functions

- (void)bluetoothAvailabilityUpdateEvent:(NSNotification *)notif {
	bool available = [[notif.userInfo objectForKey:@"available"] intValue];
	[self bluetoothAvailabilityUpdate:available];
}

- (void)callUpdateEvent:(NSNotification *)notif {
	LinphoneCall *call = [[notif.userInfo objectForKey:@"call"] pointerValue];
	LinphoneCallState state = [[notif.userInfo objectForKey:@"state"] intValue];
	[self callUpdate:call state:state animated:TRUE];
}

#pragma mark - ActionSheet Functions

- (void)displayAskToEnableVideoCall:(LinphoneCall *)call {
	if (linphone_core_get_video_policy([LinphoneManager getLc])->automatically_accept)
		return;

	const char *lUserNameChars = linphone_address_get_username(linphone_call_get_remote_address(call));
	NSString *lUserName =
		lUserNameChars ? [[NSString alloc] initWithUTF8String:lUserNameChars] : NSLocalizedString(@"Unknown", nil);
	const char *lDisplayNameChars = linphone_address_get_display_name(linphone_call_get_remote_address(call));
	NSString *lDisplayName = lDisplayNameChars ? [[NSString alloc] initWithUTF8String:lDisplayNameChars] : @"";

	NSString *title = [NSString stringWithFormat:NSLocalizedString(@"'%@' would like to enable video", nil),
												 ([lDisplayName length] > 0) ? lDisplayName : lUserName];
	DTActionSheet *sheet = [[DTActionSheet alloc] initWithTitle:title];
	NSTimer *timer = [NSTimer scheduledTimerWithTimeInterval:30
													  target:self
													selector:@selector(dismissVideoActionSheet:)
													userInfo:sheet
													 repeats:NO];
	[sheet addButtonWithTitle:NSLocalizedString(@"Accept", nil)
						block:^() {
						  LOGI(@"User accept video proposal");
						  LinphoneCallParams *paramsCopy =
							  linphone_call_params_copy(linphone_call_get_current_params(call));
						  linphone_call_params_enable_video(paramsCopy, TRUE);
						  linphone_core_accept_call_update([LinphoneManager getLc], call, paramsCopy);
						  linphone_call_params_destroy(paramsCopy);
						  [timer invalidate];
						}];
	DTActionSheetBlock cancelBlock = ^() {
	  LOGI(@"User declined video proposal");
	  LinphoneCallParams *paramsCopy = linphone_call_params_copy(linphone_call_get_current_params(call));
	  linphone_core_accept_call_update([LinphoneManager getLc], call, paramsCopy);
	  linphone_call_params_destroy(paramsCopy);
	  [timer invalidate];
	};
	[sheet addDestructiveButtonWithTitle:NSLocalizedString(@"Decline", nil) block:cancelBlock];
	if (LinphoneManager.runningOnIpad) {
		[sheet addCancelButtonWithTitle:NSLocalizedString(@"Decline", nil) block:cancelBlock];
	}
	[sheet showInView:PhoneMainView.instance.view];
}

- (void)dismissVideoActionSheet:(NSTimer *)timer {
	DTActionSheet *sheet = (DTActionSheet *)timer.userInfo;
	[sheet dismissWithClickedButtonIndex:sheet.destructiveButtonIndex animated:TRUE];
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
	if (value > max) {
		value = max;
	}
	if (value < min) {
		value = min;
	}
	return value;
}

- (void)previewTouchLift {
	CGRect previewFrame = _videoPreview.frame;
	previewFrame.origin.x = [self coerce:previewFrame.origin.x
							  betweenMin:5
								  andMax:(self.view.frame.size.width - previewFrame.size.width - 5)];
	previewFrame.origin.y = [self coerce:previewFrame.origin.y
							  betweenMin:5
								  andMax:(self.view.frame.size.height - previewFrame.size.height - 5)];

	if (!CGRectEqualToRect(previewFrame, _videoPreview.frame)) {
		dispatch_after(dispatch_time(DISPATCH_TIME_NOW, (int64_t)(0.3 * NSEC_PER_SEC)), dispatch_get_main_queue(), ^{
		  [UIView animateWithDuration:0.3
						   animations:^{
							 LOGI(@"Recentering preview to %@", NSStringFromCGRect(previewFrame));
							 _videoPreview.frame = previewFrame;
						   }];
		});
	}
}

#pragma mark - Action Functions

- (IBAction)onNumpadClick:(id)sender {
	if ([_numpadView isHidden]) {
		[self showPad:[[LinphoneManager instance] lpConfigBoolForKey:@"animations_preference"]];
	} else {
		[self hidePad:[[LinphoneManager instance] lpConfigBoolForKey:@"animations_preference"]];
	}
}

- (IBAction)onRoutesBluetoothClick:(id)sender {
	[self hideRoutes:TRUE];
	[[LinphoneManager instance] setBluetoothEnabled:TRUE];
}

- (IBAction)onRoutesReceiverClick:(id)sender {
	[self hideRoutes:TRUE];
	[[LinphoneManager instance] setSpeakerEnabled:FALSE];
	[[LinphoneManager instance] setBluetoothEnabled:FALSE];
}

- (IBAction)onRoutesSpeakerClick:(id)sender {
	[self hideRoutes:TRUE];
	[[LinphoneManager instance] setSpeakerEnabled:TRUE];
}

- (IBAction)onRoutesClick:(id)sender {
	if ([_routesView isHidden]) {
		[self showRoutes:[[LinphoneManager instance] lpConfigBoolForKey:@"animations_preference"]];
	} else {
		[self hideRoutes:[[LinphoneManager instance] lpConfigBoolForKey:@"animations_preference"]];
	}
}

- (IBAction)onOptionsTransferClick:(id)sender {
	[self hideOptions:TRUE];
	// Go to dialer view
	DialerView *view = VIEW(DialerView);
	[PhoneMainView.instance changeCurrentView:view.compositeViewDescription];
	if (view != nil) {
		[view setAddress:@""];
		[view setTransferMode:TRUE];
	}
}

- (IBAction)onOptionsAddClick:(id)sender {
	[self hideOptions:TRUE];
	// Go to dialer view
	DialerView *view = VIEW(DialerView);
	[PhoneMainView.instance changeCurrentView:view.compositeViewDescription];
	[view setAddress:@""];
	[view setTransferMode:FALSE];
}

- (IBAction)onOptionsClick:(id)sender {
	if ([_optionsView isHidden]) {
		[self showOptions:[[LinphoneManager instance] lpConfigBoolForKey:@"animations_preference"]];
	} else {
		[self hideOptions:[[LinphoneManager instance] lpConfigBoolForKey:@"animations_preference"]];
	}
}

- (IBAction)onOptionsConferenceClick:(id)sender {
	linphone_core_add_all_to_conference([LinphoneManager getLc]);
}

#pragma mark - Animation

- (void)showAnimation:(NSString *)animationID target:(UIView *)target completion:(void (^)(BOOL finished))completion {
	CGRect frame = [target frame];
	int original_y = frame.origin.y;
	frame.origin.y = [[self view] frame].size.height;
	[target setFrame:frame];
	[target setHidden:FALSE];
	[UIView animateWithDuration:0.5
		delay:0.0
		options:UIViewAnimationOptionCurveEaseOut
		animations:^{
		  CGRect frame = [target frame];
		  frame.origin.y = original_y;
		  [target setFrame:frame];
		}
		completion:^(BOOL finished) {
		  CGRect frame = [target frame];
		  frame.origin.y = original_y;
		  [target setFrame:frame];
		  completion(finished);
		}];
}

- (void)hideAnimation:(NSString *)animationID target:(UIView *)target completion:(void (^)(BOOL finished))completion {
	CGRect frame = [target frame];
	int original_y = frame.origin.y;
	[UIView animateWithDuration:0.5
		delay:0.0
		options:UIViewAnimationOptionCurveEaseIn
		animations:^{
		  CGRect frame = [target frame];
		  frame.origin.y = [[self view] frame].size.height;
		  [target setFrame:frame];
		}
		completion:^(BOOL finished) {
		  CGRect frame = [target frame];
		  frame.origin.y = original_y;
		  [target setHidden:TRUE];
		  [target setFrame:frame];
		  completion(finished);
		}];
}

@end
