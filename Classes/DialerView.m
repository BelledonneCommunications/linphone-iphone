/* DialerViewController.h
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

#import <AVFoundation/AVAudioSession.h>
#import <AudioToolbox/AudioToolbox.h>

#import "LinphoneManager.h"
#import "PhoneMainView.h"

@implementation DialerView

@synthesize transferMode;

@synthesize addressField;
@synthesize addContactButton;
@synthesize backButton;
@synthesize addCallButton;
@synthesize transferButton;
@synthesize callButton;
@synthesize backspaceButton;

@synthesize oneButton;
@synthesize twoButton;
@synthesize threeButton;
@synthesize fourButton;
@synthesize fiveButton;
@synthesize sixButton;
@synthesize sevenButton;
@synthesize eightButton;
@synthesize nineButton;
@synthesize starButton;
@synthesize zeroButton;
@synthesize hashButton;

@synthesize backgroundView;
@synthesize videoPreview;
@synthesize videoCameraSwitch;

#pragma mark - Lifecycle Functions

- (id)init {
	self = [super initWithNibName:NSStringFromClass(self.class) bundle:[NSBundle mainBundle]];
	if (self) {
		transferMode = FALSE;
	}
	return self;
}

- (void)dealloc {
	// Remove all observers
	[NSNotificationCenter.defaultCenter removeObserver:self];
}

#pragma mark - UICompositeViewDelegate Functions

static UICompositeViewDescription *compositeDescription = nil;

+ (UICompositeViewDescription *)compositeViewDescription {
	if (compositeDescription == nil) {
		compositeDescription = [[UICompositeViewDescription alloc] init:self.class
															  statusBar:StatusBarView.class
																 tabBar:TabBarView.class
															   sideMenu:SideMenuView.class
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

- (void)viewWillAppear:(BOOL)animated {
	[super viewWillAppear:animated];

	_padView.hidden =
		!IPAD && UIInterfaceOrientationIsLandscape(PhoneMainView.instance.mainViewController.currentOrientation);

	// Set observer
	[NSNotificationCenter.defaultCenter addObserver:self
										   selector:@selector(callUpdateEvent:)
											   name:kLinphoneCallUpdate
											 object:nil];

	[NSNotificationCenter.defaultCenter addObserver:self
										   selector:@selector(coreUpdateEvent:)
											   name:kLinphoneCoreUpdate
											 object:nil];

	// technically not needed, but older versions of linphone had this button
	// disabled by default. In this case, updating by pushing a new version with
	// xcode would result in the callbutton being disabled all the time.
	// We force it enabled anyway now.
	[callButton setEnabled:TRUE];

	// Update on show
	LinphoneManager *mgr = LinphoneManager.instance;
	LinphoneCore *lc = LC;
	LinphoneCall *call = linphone_core_get_current_call(lc);
	LinphoneCallState state = (call != NULL) ? linphone_call_get_state(call) : 0;
	[self callUpdate:call state:state];

	if (IPAD) {
		BOOL videoEnabled = linphone_core_video_display_enabled(lc);
		BOOL previewPref = [mgr lpConfigBoolForKey:@"preview_preference"];

		if (videoEnabled && previewPref) {
			linphone_core_set_native_preview_window_id(lc, (__bridge void *)(videoPreview));

			if (!linphone_core_video_preview_enabled(lc)) {
				linphone_core_enable_video_preview(lc, TRUE);
			}

			[backgroundView setHidden:FALSE];
			[videoCameraSwitch setHidden:FALSE];
		} else {
			linphone_core_set_native_preview_window_id(lc, NULL);
			linphone_core_enable_video_preview(lc, FALSE);
			[backgroundView setHidden:TRUE];
			[videoCameraSwitch setHidden:TRUE];
		}
	}
	[addressField setText:@""];
}

- (void)viewWillDisappear:(BOOL)animated {
	[super viewWillDisappear:animated];
	[NSNotificationCenter.defaultCenter removeObserver:self];
}

- (void)viewDidLoad {
	[super viewDidLoad];

	[zeroButton setDigit:'0'];
	[oneButton setDigit:'1'];
	[twoButton setDigit:'2'];
	[threeButton setDigit:'3'];
	[fourButton setDigit:'4'];
	[fiveButton setDigit:'5'];
	[sixButton setDigit:'6'];
	[sevenButton setDigit:'7'];
	[eightButton setDigit:'8'];
	[nineButton setDigit:'9'];
	[starButton setDigit:'*'];
	[hashButton setDigit:'#'];

	[addressField setAdjustsFontSizeToFitWidth:TRUE]; // Not put it in IB: issue with placeholder size

	UILongPressGestureRecognizer *backspaceLongGesture =
		[[UILongPressGestureRecognizer alloc] initWithTarget:self action:@selector(onBackspaceLongClick:)];
	[backspaceButton addGestureRecognizer:backspaceLongGesture];

	UILongPressGestureRecognizer *zeroLongGesture =
		[[UILongPressGestureRecognizer alloc] initWithTarget:self action:@selector(onZeroLongClick:)];
	[zeroButton addGestureRecognizer:zeroLongGesture];

	UILongPressGestureRecognizer *oneLongGesture =
		[[UILongPressGestureRecognizer alloc] initWithTarget:self action:@selector(onOneLongClick:)];
	[oneButton addGestureRecognizer:oneLongGesture];

	if (IPAD) {
		if (LinphoneManager.instance.frontCamId != nil) {
			// only show camera switch button if we have more than 1 camera
			[videoCameraSwitch setHidden:FALSE];
		}
	}
}

- (void)willAnimateRotationToInterfaceOrientation:(UIInterfaceOrientation)toInterfaceOrientation
										 duration:(NSTimeInterval)duration {
	[super willAnimateRotationToInterfaceOrientation:toInterfaceOrientation duration:duration];
	switch (toInterfaceOrientation) {
		case UIInterfaceOrientationPortrait:
			[videoPreview setTransform:CGAffineTransformMakeRotation(0)];
			break;
		case UIInterfaceOrientationPortraitUpsideDown:
			[videoPreview setTransform:CGAffineTransformMakeRotation(M_PI)];
			break;
		case UIInterfaceOrientationLandscapeLeft:
			[videoPreview setTransform:CGAffineTransformMakeRotation(M_PI / 2)];
			break;
		case UIInterfaceOrientationLandscapeRight:
			[videoPreview setTransform:CGAffineTransformMakeRotation(-M_PI / 2)];
			break;
		default:
			break;
	}
	CGRect frame = self.view.frame;
	frame.origin = CGPointMake(0, 0);
	videoPreview.frame = frame;
	_padView.hidden = !IPAD && UIInterfaceOrientationIsLandscape(toInterfaceOrientation);
}

#pragma mark - Event Functions

- (void)callUpdateEvent:(NSNotification *)notif {
	LinphoneCall *call = [[notif.userInfo objectForKey:@"call"] pointerValue];
	LinphoneCallState state = [[notif.userInfo objectForKey:@"state"] intValue];
	[self callUpdate:call state:state];
}

- (void)coreUpdateEvent:(NSNotification *)notif {
	if (IPAD) {
		LinphoneCore *lc = LC;
		if (linphone_core_video_display_enabled(lc) && linphone_core_video_preview_enabled(lc)) {
			linphone_core_set_native_preview_window_id(lc, (__bridge void *)(videoPreview));
			[backgroundView setHidden:FALSE];
			[videoCameraSwitch setHidden:FALSE];
		} else {
			linphone_core_set_native_preview_window_id(lc, NULL);
			[backgroundView setHidden:TRUE];
			[videoCameraSwitch setHidden:TRUE];
		}
	}
}

#pragma mark - Debug Functions
- (void)presentMailViewWithTitle:(NSString *)subject forRecipients:(NSArray *)recipients attachLogs:(BOOL)attachLogs {
	if ([MFMailComposeViewController canSendMail]) {
		MFMailComposeViewController *controller = [[MFMailComposeViewController alloc] init];
		if (controller) {
			controller.mailComposeDelegate = self;
			[controller setSubject:subject];
			[controller setToRecipients:recipients];

			if (attachLogs) {
				char *filepath = linphone_core_compress_log_collection();
				if (filepath == NULL) {
					LOGE(@"Cannot sent logs: file is NULL");
					return;
				}

				NSString *appName = [[NSBundle mainBundle] objectForInfoDictionaryKey:@"CFBundleDisplayName"];
				NSString *filename = [appName stringByAppendingString:@".gz"];
				NSString *mimeType = @"text/plain";

				if ([filename hasSuffix:@".gz"]) {
					mimeType = @"application/gzip";
					filename = [appName stringByAppendingString:@".gz"];
				} else {
					LOGE(@"Unknown extension type: %@, cancelling email", filename);
					return;
				}
				[controller setMessageBody:NSLocalizedString(@"Application logs", nil) isHTML:NO];
				[controller addAttachmentData:[NSData dataWithContentsOfFile:[NSString stringWithUTF8String:filepath]]
									 mimeType:mimeType
									 fileName:filename];

				ms_free(filepath);
			}
			self.modalPresentationStyle = UIModalPresentationPageSheet;
			[self.view.window.rootViewController presentViewController:controller
															  animated:TRUE
															completion:^{
															}];
		}

	} else {
		UIAlertView *alert =
			[[UIAlertView alloc] initWithTitle:subject
									   message:NSLocalizedString(@"Error: no mail account configured", nil)
									  delegate:nil
							 cancelButtonTitle:NSLocalizedString(@"OK", nil)
							 otherButtonTitles:nil];
		[alert show];
	}
}

- (BOOL)displayDebugPopup:(NSString *)address {
	LinphoneManager *mgr = LinphoneManager.instance;
	NSString *debugAddress = [mgr lpConfigStringForKey:@"debug_popup_magic" withDefault:@""];
	if (![debugAddress isEqualToString:@""] && [address isEqualToString:debugAddress]) {

		DTAlertView *alertView = [[DTAlertView alloc] initWithTitle:NSLocalizedString(@"Debug", nil)
															message:NSLocalizedString(@"Choose an action", nil)];

		[alertView addCancelButtonWithTitle:NSLocalizedString(@"Cancel", nil) block:nil];

		[alertView
			addButtonWithTitle:NSLocalizedString(@"Send logs", nil)
						 block:^{
						   NSString *appName =
							   [[NSBundle mainBundle] objectForInfoDictionaryKey:@"CFBundleDisplayName"];
						   NSString *logsAddress =
							   [mgr lpConfigStringForKey:@"debug_popup_email" withDefault:@"linphone-ios@linphone.org"];
						   [self presentMailViewWithTitle:appName forRecipients:@[ logsAddress ] attachLogs:true];
						 }];

		BOOL debugEnabled = [LinphoneManager.instance lpConfigBoolForKey:@"debugenable_preference"];
		NSString *actionLog =
			(debugEnabled ? NSLocalizedString(@"Disable logs", nil) : NSLocalizedString(@"Enable logs", nil));
		[alertView addButtonWithTitle:actionLog
								block:^{
								  // enable / disable
								  BOOL enableDebug = !debugEnabled;
								  [Log enableLogs:enableDebug];
								}];

		[alertView show];
		return true;
	}
	return false;
}

#pragma mark -

- (void)callUpdate:(LinphoneCall *)call state:(LinphoneCallState)state {
	LinphoneCore *lc = LC;
	BOOL callInProgress = (linphone_core_get_calls_nb(lc) > 0);
	addCallButton.hidden = (!callInProgress || transferMode);
	transferButton.hidden = (!callInProgress || !transferMode);
	addContactButton.hidden = callButton.hidden = callInProgress;
	backButton.hidden = !callInProgress;
	[callButton updateVideoPolicy];
}

- (void)setAddress:(NSString *)address {
	[addressField setText:address];
}

- (void)setTransferMode:(BOOL)atransferMode {
	transferMode = atransferMode;
	LinphoneCall *call = linphone_core_get_current_call(LC);
	LinphoneCallState state = (call != NULL) ? linphone_call_get_state(call) : 0;
	[self callUpdate:call state:state];
}

#pragma mark - UITextFieldDelegate Functions

- (BOOL)textField:(UITextField *)textField
	shouldChangeCharactersInRange:(NSRange)range
				replacementString:(NSString *)string {
	//[textField performSelector:@selector() withObject:nil afterDelay:0];
	return YES;
}

- (BOOL)textFieldShouldReturn:(UITextField *)textField {
	if (textField == addressField) {
		[addressField resignFirstResponder];
	}
	return YES;
}

#pragma mark - MFComposeMailDelegate

- (void)mailComposeController:(MFMailComposeViewController *)controller
		  didFinishWithResult:(MFMailComposeResult)result
						error:(NSError *)error {
	[controller dismissViewControllerAnimated:TRUE
								   completion:^{
								   }];
	[self.navigationController setNavigationBarHidden:TRUE animated:FALSE];
}

#pragma mark - Action Functions

- (IBAction)onAddContactClick:(id)event {
	[ContactSelection setSelectionMode:ContactSelectionModeEdit];
	[ContactSelection setAddAddress:[addressField text]];
	[ContactSelection setSipFilter:nil];
	[ContactSelection setNameOrEmailFilter:nil];
	[ContactSelection enableEmailFilter:FALSE];
	[PhoneMainView.instance changeCurrentView:ContactsListView.compositeViewDescription];
}

- (IBAction)onBackClick:(id)event {
	[PhoneMainView.instance popToView:CallView.compositeViewDescription];
}

- (IBAction)onAddressChange:(id)sender {
	if ([self displayDebugPopup:self.addressField.text]) {
		self.addressField.text = @"";
	}
	addContactButton.enabled = backspaceButton.enabled = addCallButton.enabled = transferButton.enabled =
		([[addressField text] length] > 0);
}

- (IBAction)onBackspaceClick:(id)sender {
	if ([addressField.text length] > 0) {
		[addressField setText:[addressField.text substringToIndex:[addressField.text length] - 1]];
	}
}

- (void)onBackspaceLongClick:(id)sender {
	[addressField setText:@""];
}

- (void)onZeroLongClick:(id)sender {
	// replace last character with a '+'
	NSString *newAddress =
		[[self.addressField.text substringToIndex:[self.addressField.text length] - 1] stringByAppendingString:@"+"];
	[self.addressField setText:newAddress];
}

- (void)onOneLongClick:(id)sender {
	LinphoneManager *lm = LinphoneManager.instance;
	NSString *voiceMail = [lm lpConfigStringForKey:@"voice_mail_uri"];
	LinphoneAddress *addr = linphone_core_interpret_url(LC, voiceMail ? voiceMail.UTF8String : NULL);
	if (addr) {
		linphone_address_set_display_name(addr, NSLocalizedString(@"Voice mail", nil).UTF8String);
		[lm call:addr transfer:FALSE];
		linphone_address_destroy(addr);
	} else {
		LOGE(@"Cannot call voice mail because URI not set or invalid!");
	}
}
@end
