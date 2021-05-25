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

#import <AudioToolbox/AudioToolbox.h>

#import "LinphoneManager.h"
#import "PhoneMainView.h"

@implementation DialerView

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

	// Update on show
	LinphoneManager *mgr = LinphoneManager.instance;
	LinphoneCall *call = linphone_core_get_current_call(LC);
	LinphoneCallState state = (call != NULL) ? linphone_call_get_state(call) : 0;
	[self callUpdate:call state:state];

	if (IPAD) {
		BOOL videoEnabled = linphone_core_video_display_enabled(LC);
		BOOL previewPref = [mgr lpConfigBoolForKey:@"preview_preference"];

		if (videoEnabled && previewPref) {
			linphone_core_set_native_preview_window_id(LC, (__bridge void *)(_videoPreview));

			if (!linphone_core_video_preview_enabled(LC)) {
				linphone_core_enable_video_preview(LC, TRUE);
			}

			[_backgroundView setHidden:FALSE];
			[_videoCameraSwitch setHidden:FALSE];
		} else {
			linphone_core_set_native_preview_window_id(LC, NULL);
			linphone_core_enable_video_preview(LC, FALSE);
			[_backgroundView setHidden:TRUE];
			[_videoCameraSwitch setHidden:TRUE];
		}
	} else {
		linphone_core_enable_video_preview(LC, FALSE);
	}
	[_addressField setText:@""];
}

- (void)viewWillDisappear:(BOOL)animated {
	[super viewWillDisappear:animated];
	[NSNotificationCenter.defaultCenter removeObserver:self];
}

- (void)viewDidLoad {
	[super viewDidLoad];

	[_zeroButton setDigit:'0'];
	[_oneButton setDigit:'1'];
	[_twoButton setDigit:'2'];
	[_threeButton setDigit:'3'];
	[_fourButton setDigit:'4'];
	[_fiveButton setDigit:'5'];
	[_sixButton setDigit:'6'];
	[_sevenButton setDigit:'7'];
	[_eightButton setDigit:'8'];
	[_nineButton setDigit:'9'];
	[_starButton setDigit:'*'];
	[_hashButton setDigit:'#'];

	[_addressField setAdjustsFontSizeToFitWidth:TRUE]; // Not put it in IB: issue with placeholder size

	UILongPressGestureRecognizer *backspaceLongGesture =
		[[UILongPressGestureRecognizer alloc] initWithTarget:self action:@selector(onBackspaceLongClick:)];
	[_backspaceButton addGestureRecognizer:backspaceLongGesture];

	UILongPressGestureRecognizer *zeroLongGesture =
		[[UILongPressGestureRecognizer alloc] initWithTarget:self action:@selector(onZeroLongClick:)];
	[_zeroButton addGestureRecognizer:zeroLongGesture];

	UILongPressGestureRecognizer *oneLongGesture =
		[[UILongPressGestureRecognizer alloc] initWithTarget:self action:@selector(onOneLongClick:)];
	[_oneButton addGestureRecognizer:oneLongGesture];
	
	UITapGestureRecognizer *tap = [[UITapGestureRecognizer alloc]
																					initWithTarget:self
																					action:@selector(dismissKeyboards)];
	
	[self.view addGestureRecognizer:tap];

	if (IPAD) {
		if (LinphoneManager.instance.frontCamId != nil) {
			// only show camera switch button if we have more than 1 camera
			[_videoCameraSwitch setHidden:FALSE];
		}
	}
}

- (void)willAnimateRotationToInterfaceOrientation:(UIInterfaceOrientation)toInterfaceOrientation
										 duration:(NSTimeInterval)duration {
	[super willAnimateRotationToInterfaceOrientation:toInterfaceOrientation duration:duration];
	switch (toInterfaceOrientation) {
		case UIInterfaceOrientationPortrait:
			[_videoPreview setTransform:CGAffineTransformMakeRotation(0)];
			break;
		case UIInterfaceOrientationPortraitUpsideDown:
			[_videoPreview setTransform:CGAffineTransformMakeRotation(M_PI)];
			break;
		case UIInterfaceOrientationLandscapeLeft:
			[_videoPreview setTransform:CGAffineTransformMakeRotation(M_PI / 2)];
			break;
		case UIInterfaceOrientationLandscapeRight:
			[_videoPreview setTransform:CGAffineTransformMakeRotation(-M_PI / 2)];
			break;
		default:
			break;
	}
	CGRect frame = self.view.frame;
	frame.origin = CGPointMake(0, 0);
	_videoPreview.frame = frame;
	_padView.hidden = !IPAD && UIInterfaceOrientationIsLandscape(toInterfaceOrientation);
	if (linphone_core_get_calls_nb(LC)) {
		_backButton.hidden = FALSE;
		_addContactButton.hidden = TRUE;
	} else {
		_backButton.hidden = TRUE;
		_addContactButton.hidden = FALSE;
	}
}

- (void)viewDidAppear:(BOOL)animated {
	[super viewDidAppear:animated];
	[LinphoneManager.instance shouldPresentLinkPopup];
}

#pragma mark - Event Functions

- (void)callUpdateEvent:(NSNotification *)notif {
	LinphoneCall *call = [[notif.userInfo objectForKey:@"call"] pointerValue];
	LinphoneCallState state = [[notif.userInfo objectForKey:@"state"] intValue];
	[self callUpdate:call state:state];
}

- (void)coreUpdateEvent:(NSNotification *)notif {
	@try {
		if (IPAD) {
			if (linphone_core_video_display_enabled(LC) && linphone_core_video_preview_enabled(LC)) {
				linphone_core_set_native_preview_window_id(LC, (__bridge void *)(_videoPreview));
				[_backgroundView setHidden:FALSE];
				[_videoCameraSwitch setHidden:FALSE];
			} else {
				linphone_core_set_native_preview_window_id(LC, NULL);
				[_backgroundView setHidden:TRUE];
				[_videoCameraSwitch setHidden:TRUE];
			}
		}
	} @catch (NSException *exception) {
		if ([exception.name isEqualToString:@"LinphoneCoreException"]) {
			LOGE(@"Core already destroyed");
			return;
		}
		LOGE(@"Uncaught exception : %@", exception.description);
		abort();
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
		UIAlertController *errView = [UIAlertController alertControllerWithTitle:subject
																		 message:NSLocalizedString(@"Error: no mail account configured",
																								   nil)
																  preferredStyle:UIAlertControllerStyleAlert];
		
		UIAlertAction* defaultAction = [UIAlertAction actionWithTitle:@"OK"
																style:UIAlertActionStyleDefault
															  handler:^(UIAlertAction * action) {}];
		
		[errView addAction:defaultAction];
		[self presentViewController:errView animated:YES completion:nil];
	}
}

- (BOOL)displayDebugPopup:(NSString *)address {
	LinphoneManager *mgr = LinphoneManager.instance;
	NSString *debugAddress = [mgr lpConfigStringForKey:@"debug_popup_magic" withDefault:@""];
	if (![debugAddress isEqualToString:@""] && [address isEqualToString:debugAddress]) {
		UIAlertController *errView = [UIAlertController alertControllerWithTitle:NSLocalizedString(@"Debug", nil)
																		 message:NSLocalizedString(@"Choose an action", nil)
																  preferredStyle:UIAlertControllerStyleAlert];
		
		UIAlertAction* defaultAction = [UIAlertAction actionWithTitle:NSLocalizedString(@"Cancel", nil)
																style:UIAlertActionStyleDefault
															  handler:^(UIAlertAction * action) {}];
		
		[errView addAction:defaultAction];

		int debugLevel = [LinphoneManager.instance lpConfigIntForKey:@"debugenable_preference"];
		BOOL debugEnabled = (debugLevel >= ORTP_DEBUG && debugLevel < ORTP_ERROR);

		if (debugEnabled) {
			UIAlertAction* continueAction = [UIAlertAction actionWithTitle:NSLocalizedString(@"Send logs", nil)
																	 style:UIAlertActionStyleDefault
																   handler:^(UIAlertAction * action) {
																	   NSString *appName =
																	   [[NSBundle mainBundle] objectForInfoDictionaryKey:@"CFBundleDisplayName"];
																	   NSString *logsAddress = [mgr lpConfigStringForKey:@"debug_popup_email" withDefault:@""];
																	   [self presentMailViewWithTitle:appName forRecipients:@[ logsAddress ] attachLogs:true];
																   }];
			[errView addAction:continueAction];
		}
		NSString *actionLog =
			(debugEnabled ? NSLocalizedString(@"Disable logs", nil) : NSLocalizedString(@"Enable logs", nil));
		
		UIAlertAction* logAction = [UIAlertAction actionWithTitle:actionLog
															style:UIAlertActionStyleDefault
														  handler:^(UIAlertAction * action) {
																   int newDebugLevel = debugEnabled ? 0 : ORTP_DEBUG;
																   [LinphoneManager.instance lpConfigSetInt:newDebugLevel forKey:@"debugenable_preference"];
																   [Log enableLogs:newDebugLevel];
															   }];
		[errView addAction:logAction];
		
		UIAlertAction* remAction = [UIAlertAction actionWithTitle:NSLocalizedString(@"Remove account(s) and self destruct", nil)
															style:UIAlertActionStyleDefault
														  handler:^(UIAlertAction * action) {
															  linphone_core_clear_accounts([LinphoneManager getLc]);
															  linphone_core_clear_all_auth_info([LinphoneManager getLc]);
															  @try {
																  [LinphoneManager.instance destroyLinphoneCore];
															  } @catch (NSException *e) {
																  LOGW(@"Exception while destroying linphone core: %@", e);
															  } @finally {
																  if ([NSFileManager.defaultManager
																	   isDeletableFileAtPath:[LinphoneManager preferenceFile:@"linphonerc"]] == YES) {
																	  [NSFileManager.defaultManager
																	   removeItemAtPath:[LinphoneManager preferenceFile:@"linphonerc"]
																	   error:nil];
																  }
#ifdef DEBUG
																  [LinphoneManager instanceRelease];
#endif
															  }
															  [UIApplication sharedApplication].keyWindow.rootViewController = nil;
															  // make the application crash to be sure that user restart it properly
															  LOGF(@"Self-destructing in 3..2..1..0!");
														  }];
		[errView addAction:remAction];
		
		[self presentViewController:errView animated:YES completion:nil];
		return true;
	}
	return false;
}

#pragma mark -

- (void)callUpdate:(LinphoneCall *)call state:(LinphoneCallState)state {
	BOOL callInProgress = (linphone_core_get_calls_nb(LC) > 0);
	_addContactButton.hidden = callInProgress;
	_backButton.hidden = !callInProgress;
	[_callButton updateIcon];
}

- (void)setAddress:(NSString *)address {
	[_addressField setText:address];
}

#pragma mark - UITextFieldDelegate Functions

- (BOOL)textField:(UITextField *)textField
	shouldChangeCharactersInRange:(NSRange)range
				replacementString:(NSString *)string {
	//[textField performSelector:@selector() withObject:nil afterDelay:0];
	return YES;
}

- (BOOL)textFieldShouldReturn:(UITextField *)textField {
	if (textField == _addressField) {
		[_addressField resignFirstResponder];
	}
	if (textField.text.length > 0) {
		LinphoneAddress *addr = [LinphoneUtils normalizeSipOrPhoneAddress:textField.text];
		[LinphoneManager.instance call:addr];
		if (addr)
			linphone_address_destroy(addr);
	}
	return YES;
}

#pragma mark - MFComposeMailDelegate

- (void)mailComposeController:(MFMailComposeViewController *)controller
		  didFinishWithResult:(MFMailComposeResult)result
						error:(NSError *)error {
	[controller dismissViewControllerAnimated:TRUE completion:nil];
	[self.navigationController setNavigationBarHidden:TRUE animated:FALSE];
}

#pragma mark - Action Functions

- (IBAction)onAddContactClick:(id)event {
	[ContactSelection setSelectionMode:ContactSelectionModeEdit];
	[ContactSelection setAddAddress:[_addressField text]];
	[ContactSelection setSipFilter:nil];
	[ContactSelection setNameOrEmailFilter:nil];
	[ContactSelection enableEmailFilter:FALSE];
	[PhoneMainView.instance changeCurrentView:ContactsListView.compositeViewDescription];
}

- (IBAction)onBackClick:(id)event {
	[PhoneMainView.instance popToView:CallView.compositeViewDescription];
}

- (IBAction)onAddressChange:(id)sender {
	if ([self displayDebugPopup:_addressField.text]) {
		_addressField.text = @"";
	}
	_addContactButton.enabled = _backspaceButton.enabled = ([[_addressField text] length] > 0);
    if ([_addressField.text length] == 0) {
        [self.view endEditing:YES];
    }
}

- (IBAction)onBackspaceClick:(id)sender {
	if ([_addressField.text length] > 0) {
		[_addressField setText:[_addressField.text substringToIndex:[_addressField.text length] - 1]];
    }
}

- (void)onBackspaceLongClick:(id)sender {
	[_addressField setText:@""];
}

- (void)onZeroLongClick:(id)sender {
	// replace last character with a '+'
	NSString *newAddress =
		[[_addressField.text substringToIndex:[_addressField.text length] - 1] stringByAppendingString:@"+"];
	[_addressField setText:newAddress];
	linphone_core_stop_dtmf(LC);
}

- (void)onOneLongClick:(id)sender {
	LinphoneManager *lm = LinphoneManager.instance;
	NSString *voiceMail = [lm lpConfigStringForKey:@"voice_mail_uri"];
	LinphoneAddress *addr = [LinphoneUtils normalizeSipOrPhoneAddress:voiceMail];
	if (addr) {
		linphone_address_set_display_name(addr, NSLocalizedString(@"Voice mail", nil).UTF8String);
		[lm call:addr];
		linphone_address_destroy(addr);
	} else {
		LOGE(@"Cannot call voice mail because URI not set or invalid!");
	}
	linphone_core_stop_dtmf(LC);
}

- (void)dismissKeyboards {
	[self.addressField resignFirstResponder];
}
@end
