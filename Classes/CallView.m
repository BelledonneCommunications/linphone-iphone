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

	[self disableVideoDisplay:NO];

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
	[[NSNotificationCenter defaultCenter] removeObserver:self name:kLinphoneCallUpdate object:nil];
}

- (void)viewWillAppear:(BOOL)animated {
	[super viewWillAppear:animated];

	[[NSNotificationCenter defaultCenter] addObserver:self
											 selector:@selector(messageReceived:)
												 name:kLinphoneMessageReceived
											   object:nil];
	[self updateUnreadMessage:FALSE];

	// Update on show
	LinphoneCall *call = linphone_core_get_current_call([LinphoneManager getLc]);
	LinphoneCallState state = (call != NULL) ? linphone_call_get_state(call) : 0;
	[self callUpdate:call state:state animated:FALSE];
	[self hideRoutes:FALSE];
	[self hideOptions:FALSE];
	[self hidePad:FALSE];
	[self showSpeaker];
	[self callDurationUpdate];
	[self onCurrentCallChange];

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

	[NSTimer scheduledTimerWithTimeInterval:1
									 target:self
								   selector:@selector(callDurationUpdate)
								   userInfo:nil
									repeats:YES];
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

- (void)viewDidUnload {
	[PhoneMainView.instance.view removeGestureRecognizer:singleFingerTap];
	// Remove all observer
	[[NSNotificationCenter defaultCenter] removeObserver:self];
	[super viewDidUnload];
}

- (void)didRotateFromInterfaceOrientation:(UIInterfaceOrientation)fromInterfaceOrientation {
	[super didRotateFromInterfaceOrientation:fromInterfaceOrientation];
	[self updateUnreadMessage:NO];
	[self previewTouchLift];
	[self showStatusBar:!videoShown || (_nameLabel.alpha > 0.f)];
}

#pragma mark -

- (void)updateBottomBar:(LinphoneCall *)call state:(LinphoneCallState)state {
	LinphoneCore *lc = [LinphoneManager getLc];

	[_speakerButton update];
	[_microButton update];
	[_callPauseButton update];
	[_conferencePauseButton update];
	[_videoButton update];
	[_hangupButton update];

	_optionsButton.enabled = (!call || !linphone_call_media_in_progress(call));
	_optionsTransferButton.enabled = call && !linphone_call_media_in_progress(call);
	// Show Pause/Conference button following call count
	if (linphone_core_get_calls_nb(lc) > 1) {
		bool enabled = ((linphone_core_get_current_call(lc) != NULL) || linphone_core_is_in_conference(lc));
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
		_optionsConferenceButton.enabled = enabled;
	} else {
		_optionsConferenceButton.enabled = NO;
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

- (void)toggleControls:(id)sender {
	bool controlsHidden = (_bottomBar.alpha == 0.0);
	if (controlsHidden) {
		[self showControls:sender];
	} else {
		[self hideControls:sender];
	}
}

- (void)showControls:(id)sender {
	if (hideControlsTimer) {
		[hideControlsTimer invalidate];
		hideControlsTimer = nil;
	}

	if ([[PhoneMainView.instance currentView] equal:CallView.compositeViewDescription]) {
		// show controls
		[UIView beginAnimations:nil context:nil];
		[UIView setAnimationDuration:0.35];
		_pausedCallsTable.tableView.alpha = _videoCameraSwitch.alpha = _callPauseButton.alpha = 1.0;
		_routesView.alpha = _optionsView.alpha = _numpadView.alpha = _bottomBar.alpha = 1.0;
		_nameLabel.alpha = _durationLabel.alpha = .8;

		[self showStatusBar:true];

		[UIView commitAnimations];

		[PhoneMainView.instance showTabBar:true];

		// hide controls in 5 sec
		hideControlsTimer = [NSTimer scheduledTimerWithTimeInterval:5.0
															 target:self
														   selector:@selector(hideControls:)
														   userInfo:nil
															repeats:NO];
	}
}

- (void)hideControls:(id)sender {
	if (!videoShown)
		return;

	if (hideControlsTimer) {
		[hideControlsTimer invalidate];
		hideControlsTimer = nil;
	}

	if ([[PhoneMainView.instance currentView] equal:CallView.compositeViewDescription]) {

		[PhoneMainView.instance showTabBar:false];

		[UIView beginAnimations:nil context:nil];
		[UIView setAnimationDuration:0.3];
		_pausedCallsTable.tableView.alpha = _videoCameraSwitch.alpha = _nameLabel.alpha = _durationLabel.alpha =
			_callPauseButton.alpha = 0.0;
		_routesView.alpha = _optionsView.alpha = _numpadView.alpha = _bottomBar.alpha = 0.0;
		[self showStatusBar:false];
		[UIView commitAnimations];
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

	[self hideControls:nil];

	if (animation) {
		[UIView commitAnimations];
	}

	_videoPreview.hidden = (!linphone_core_self_view_enabled([LinphoneManager getLc]));

	if ([LinphoneManager instance].frontCamId != nil) {
		// only show camera switch button if we have more than 1 camera
		[_videoCameraSwitch setHidden:FALSE];
	}

	[PhoneMainView.instance fullScreen:true];
	[PhoneMainView.instance showTabBar:false];
	[self showStatusBar:false];

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

	[self showControls:nil];

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

- (void)showStatusBar:(BOOL)show {
	/* we cannot use [PhoneMainView.instance show]; because it will automatically
	 resize current view to fill empty space, which will resize video. This is
	 indesirable since we do not want to crop/rescale video view */
	PhoneMainView.instance.mainViewController.statusBarView.hidden = !show;
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

- (void)callDurationUpdate {
	int duration = linphone_core_get_current_call([LinphoneManager getLc])
					   ? linphone_call_get_duration(linphone_core_get_current_call([LinphoneManager getLc]))
					   : 0;
	_durationLabel.text = [LinphoneUtils durationToString:duration];
}

- (void)onCurrentCallChange {
	LinphoneCore *lc = [LinphoneManager getLc];
	LinphoneCall *call = linphone_core_get_current_call(lc);

	_noActiveCallView.hidden = (call || linphone_core_is_in_conference(lc));
	_callView.hidden = !call;
	_conferenceView.hidden = !linphone_core_is_in_conference(lc);
	_callPauseButton.hidden = !call && !linphone_core_is_in_conference(lc);

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

- (void)showPad:(BOOL)animated {
	[_numpadButton setOn];
	if ([_numpadView isHidden]) {
		if (animated) {
			[self showAnimation:_numpadView
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
			[self hideAnimation:_numpadView
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
		[_routesEarpieceButton setSelected:!([[LinphoneManager instance] bluetoothEnabled] ||
											 [[LinphoneManager instance] speakerEnabled])];
		if ([_routesView isHidden]) {
			if (animated) {
				[self showAnimation:_routesView
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
				[self hideAnimation:_routesView
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
			[self showAnimation:_optionsView
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
			[self hideAnimation:_optionsView
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

- (void)callUpdate:(LinphoneCall *)call state:(LinphoneCallState)state animated:(BOOL)animated {
	LinphoneCore *lc = [LinphoneManager getLc];
	[self updateBottomBar:call state:state];
	if (hiddenVolume) {
		[PhoneMainView.instance setVolumeHidden:FALSE];
		hiddenVolume = FALSE;
	}

	// Update tables
	[_pausedCallsTable.tableView reloadData];
	[_conferenceCallsTable.tableView reloadData];

	static LinphoneCall *currentCall = NULL;
	if (!currentCall || linphone_core_get_current_call(lc) != currentCall) {
		currentCall = linphone_core_get_current_call(lc);
		[self onCurrentCallChange];
	}

	// Fake call update
	if (call == NULL) {
		return;
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
			if (linphone_core_video_display_enabled(lc) && !linphone_call_params_video_enabled(current) &&
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
			[self displayTableCall:animated];
			break;
		case LinphoneCallPausedByRemote:
			if (call == linphone_core_get_current_call(lc)) {
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
	if (linphone_core_get_video_policy([LinphoneManager getLc])->automatically_accept)
		return;

	NSString *username = [FastAddressBook displayNameForAddress:linphone_call_get_remote_address(call)];
	NSString *title = [NSString stringWithFormat:NSLocalizedString(@"%@ would like to enable video", nil), username];
	UIConfirmationDialog *sheet = [UIConfirmationDialog ShowWithMessage:title
		cancelMessage:nil
		confirmMessage:NSLocalizedString(@"ACCEPT", nil)
		onCancelClick:^() {
		  LOGI(@"User declined video proposal");
		  if (call == linphone_core_get_current_call([LinphoneManager getLc])) {
			  LinphoneCallParams *paramsCopy = linphone_call_params_copy(linphone_call_get_current_params(call));
			  linphone_core_accept_call_update([LinphoneManager getLc], call, paramsCopy);
			  linphone_call_params_destroy(paramsCopy);
			  [videoDismissTimer invalidate];
			  videoDismissTimer = nil;
		  }
		}
		onConfirmationClick:^() {
		  LOGI(@"User accept video proposal");
		  if (call == linphone_core_get_current_call([LinphoneManager getLc])) {
			  LinphoneCallParams *paramsCopy = linphone_call_params_copy(linphone_call_get_current_params(call));
			  linphone_call_params_enable_video(paramsCopy, TRUE);
			  linphone_core_accept_call_update([LinphoneManager getLc], call, paramsCopy);
			  linphone_call_params_destroy(paramsCopy);
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

- (IBAction)onChatClick:(id)sender {
	ChatsListView *view = VIEW(ChatsListView);
	[PhoneMainView.instance changeCurrentView:view.compositeViewDescription];
}

- (IBAction)onRoutesBluetoothClick:(id)sender {
	[self hideRoutes:TRUE];
	[[LinphoneManager instance] setBluetoothEnabled:TRUE];
}

- (IBAction)onRoutesEarpieceClick:(id)sender {
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

- (void)showAnimation:(UIView *)target completion:(void (^)(BOOL finished))completion {
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
		  completion(finished);
		}];
}

- (void)hideAnimation:(UIView *)target completion:(void (^)(BOOL finished))completion {
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
		  completion(finished);
		}];
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
