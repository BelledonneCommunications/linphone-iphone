
/* AssistantViewController.m
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
 *  GNU Library General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */

#import "linphone/linphonecore_utils.h"
#import <CoreTelephony/CTCarrier.h>
#import <CoreTelephony/CTTelephonyNetworkInfo.h>

#import "AssistantView.h"
#import "CountryListView.h"
#import "LinphoneManager.h"
#import "PhoneMainView.h"
#import "UIAssistantTextField.h"
#import "UITextField+DoneButton.h"
#import "LinphoneAppDelegate.h"

typedef enum _ViewElement {
	ViewElement_Username = 100,
	ViewElement_Password = 101,
	ViewElement_Password2 = 102,
	ViewElement_Email = 103,
	ViewElement_Domain = 104,
	ViewElement_URL = 105,
	ViewElement_DisplayName = 106,
	ViewElement_Phone = 107,
	ViewElement_SMSCode = 108,
	ViewElement_PhoneCC = 109,
	ViewElement_TextFieldCount = ViewElement_PhoneCC - 100 + 1,
	ViewElement_Transport = 110,
	ViewElement_Username_Label = 120,
	ViewElement_NextButton = 130,

	ViewElement_PhoneButton = 150,

	ViewElement_UsernameFormView = 181,
	ViewElement_EmailFormView = 182,
} ViewElement;

@implementation AssistantView

#pragma mark - Lifecycle Functions

- (id)init {
	self = [super initWithNibName:NSStringFromClass(self.class) bundle:[NSBundle mainBundle]];
	if (self != nil) {
		[[NSBundle mainBundle] loadNibNamed:@"AssistantViewScreens" owner:self options:nil];
		historyViews = [[NSMutableArray alloc] init];
		currentView = nil;
		mustRestoreView = NO;
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
															   sideMenu:SideMenuView.class
															 fullscreen:false
														 isLeftFragment:NO
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

	[NSNotificationCenter.defaultCenter addObserver:self
										   selector:@selector(registrationUpdateEvent:)
											   name:kLinphoneRegistrationUpdate
											 object:nil];
	[NSNotificationCenter.defaultCenter addObserver:self
										   selector:@selector(configuringUpdate:)
											   name:kLinphoneConfiguringStateUpdate
											 object:nil];
	if (!account_creator) {
		account_creator = linphone_account_creator_new(
			LC,
			[LinphoneManager.instance lpConfigStringForKey:@"xmlrpc_url" inSection:@"assistant" withDefault:@""]
				.UTF8String);
	}

	if (!mustRestoreView) {
		new_config = NULL;
		number_of_configs_before = bctbx_list_size(linphone_core_get_proxy_config_list(LC));
		[self resetTextFields];
		[self changeView:_welcomeView back:FALSE animation:FALSE];
	}
	mustRestoreView = NO;
	_outgoingView = DialerView.compositeViewDescription;
    _qrCodeButton.hidden = !ENABLE_QRCODE;
	[self resetLiblinphone:FALSE];
}

- (void)viewWillDisappear:(BOOL)animated {
	[super viewWillDisappear:animated];
	[NSNotificationCenter.defaultCenter removeObserver:self];
	
}

- (void)fitContent {
	// always resize content view so that it fits whole available width
	CGRect frame = currentView.frame;
	frame.size.width = _contentView.bounds.size.width;
	currentView.frame = frame;

	[_contentView setContentSize:frame.size];
	[_contentView contentSizeToFit];
    
    _qrCodeView.frame = [[UIScreen mainScreen] bounds];
}

- (void)didRotateFromInterfaceOrientation:(UIInterfaceOrientation)fromInterfaceOrientation {
	[self fitContent];
}

#pragma mark - Utils

- (void)resetLiblinphone:(BOOL)core {
	if (account_creator) {
		linphone_account_creator_unref(account_creator);
		account_creator = NULL;
	}
	if (core) {
		[LinphoneManager.instance resetLinphoneCore];
	}
	account_creator = linphone_account_creator_new(
		LC, [LinphoneManager.instance lpConfigStringForKey:@"xmlrpc_url" inSection:@"assistant" withDefault:@""]
				.UTF8String);
	linphone_account_creator_set_user_data(account_creator, (__bridge void *)(self));
	linphone_account_creator_cbs_set_is_account_exist(linphone_account_creator_get_callbacks(account_creator),
													  assistant_is_account_used);
	linphone_account_creator_cbs_set_create_account(linphone_account_creator_get_callbacks(account_creator),
													assistant_create_account);
	linphone_account_creator_cbs_set_activate_account(linphone_account_creator_get_callbacks(account_creator),
													assistant_activate_account);
	linphone_account_creator_cbs_set_is_account_activated(linphone_account_creator_get_callbacks(account_creator),
													   //assistant_is_account_activated);
	//linphone_account_creator_cbs_set_recover_account(linphone_account_creator_get_callbacks(account_creator),
													 //assistant_recover_phone_account);
	//linphone_account_creator_cbs_set_is_account_linked(linphone_account_creator_get_callbacks(account_creator),
													   assistant_is_account_linked);
	
}
- (void)loadAssistantConfig:(NSString *)rcFilename {
	linphone_config_load_from_xml_file(linphone_core_get_config(LC),
									   [LinphoneManager bundleFile:rcFilename].UTF8String);
	[self changeView:nextView back:FALSE animation:TRUE];
}

- (void)reset {
	[LinphoneManager.instance removeAllAccounts];
	[self resetTextFields];
	[self changeView:_welcomeView back:FALSE animation:FALSE];
	_waitView.hidden = TRUE;
}

- (void)clearHistory {
	[historyViews removeAllObjects];
}

+ (NSString *)StringForXMLRPCError:(const char *)err {
#define IS(x) (strcmp(err, #x) == 0)
	if
		IS(ERROR_ACCOUNT_ALREADY_ACTIVATED)
	return NSLocalizedString(@"This account is already activated.", nil);
	if
		IS(ERROR_ACCOUNT_ALREADY_IN_USE)
	return NSLocalizedString(@"This account is already in use.", nil);
	if
		IS(ERROR_ACCOUNT_DOESNT_EXIST)
	return NSLocalizedString(@"This account does not exist.", nil);
	if
		IS(ERROR_ACCOUNT_NOT_ACTIVATED)
	return NSLocalizedString(@"This account is not activated yet.", nil);
	if
		IS(ERROR_ALIAS_ALREADY_IN_USE)
	return NSLocalizedString(@"This phone number is already used. Please type a different number. \nYou can delete "
							 @"your existing account if you want to reuse your phone number.",
							 nil);
	if
		IS(ERROR_ALIAS_DOESNT_EXIST)
	return NSLocalizedString(@"This alias does not exist.", nil);
	if
		IS(ERROR_EMAIL_ALREADY_IN_USE)
	return NSLocalizedString(@"This email address is already used.", nil);
	if
		IS(ERROR_EMAIL_DOESNT_EXIST)
	return NSLocalizedString(@"This email does not exist.", nil);
	if
		IS(ERROR_KEY_DOESNT_MATCH)
	return NSLocalizedString(@"The confirmation code is invalid. \nPlease try again.", nil);
	if
		IS(ERROR_PASSWORD_DOESNT_MATCH)
	return NSLocalizedString(@"Passwords do not match.", nil);
	if
		IS(ERROR_PHONE_ISNT_E164)
	return NSLocalizedString(@"Your phone number is invalid.", nil);
	if
		IS(ERROR_CANNOT_SEND_SMS)
	return NSLocalizedString(@"Server error, please try again later.", nil);
	if
		IS(ERROR_NO_PHONE_NUMBER)
	return NSLocalizedString(@"Please confirm your country code and enter your phone number.", nil);
	if
		IS(Missing required parameters)
	return NSLocalizedString(@"Missing required parameters", nil);
	if
		IS(ERROR_BAD_CREDENTIALS)
	return NSLocalizedString(@"Bad credentials, check your account settings", nil);
	if
		IS(ERROR_NO_PASSWORD)
	return NSLocalizedString(@"Please enter a password to your account", nil);
	if
		IS(ERROR_NO_EMAIL)
	return NSLocalizedString(@"Please enter your email", nil);
	if
		IS(ERROR_NO_USERNAME)
	return NSLocalizedString(@"Please enter a username", nil);
	if
		IS(ERROR_INVALID_CONFIRMATION)
	return NSLocalizedString(@"Your confirmation password doesn't match your password", nil);
	if
		IS(ERROR_INVALID_EMAIL)
	return NSLocalizedString(@"Your email is invalid", nil);

	if (!linphone_core_is_network_reachable(LC))
		return NSLocalizedString(@"There is no network connection available, enable "
								 @"WIFI or WWAN prior to configure an account.",
								 nil);

	return NSLocalizedString(@"Unknown error, please try again later.", nil);
}

+ (NSString *)errorForLinphoneAccountCreatorPhoneNumberStatus:(LinphoneAccountCreatorPhoneNumberStatus)status {
	switch (status) {
		case LinphoneAccountCreatorPhoneNumberStatusTooShort: /**< Phone number too short */
			return NSLocalizedString(@"Your country code is too short.", nil);
		case LinphoneAccountCreatorPhoneNumberStatusTooLong: /**< Phone number too long */
			return NSLocalizedString(@"Your country code is too long.", nil);
			return nil; /* this is not an error, just user has to finish typing */
		case LinphoneAccountCreatorPhoneNumberStatusInvalidCountryCode: /**< Country code invalid */
			return NSLocalizedString(@"Your country code is invalid.", nil);
		case LinphoneAccountCreatorPhoneNumberStatusInvalid: /**< Phone number invalid */
			return NSLocalizedString(@"Your phone number is invalid.", nil);
		default:
			return NSLocalizedString(@"Unknown error, please try again later.", nil);
	}
}

+ (NSString *)errorForLinphoneAccountCreatorUsernameStatus:(LinphoneAccountCreatorUsernameStatus)status {
	switch (status) {
		case LinphoneAccountCreatorUsernameStatusTooShort: /**< Username too short */
			return NSLocalizedString(@"Your username is too short.", nil);
		case LinphoneAccountCreatorUsernameStatusTooLong: /**< Username too long */
			return NSLocalizedString(@"Your username is too long.", nil);
		case LinphoneAccountCreatorUsernameStatusInvalidCharacters: /**< Contain invalid characters */
			return NSLocalizedString(@"Your username contains invalid characters.", nil);
		case LinphoneAccountCreatorUsernameStatusInvalid: /**< Invalid username */
			return NSLocalizedString(@"Your username is invalid.", nil);
		default:
			return NSLocalizedString(@"Unknown error, please try again later.", nil);
	}
}

+ (NSString *)errorForLinphoneAccountCreatorEmailStatus:(LinphoneAccountCreatorEmailStatus)status {
	switch (status) {
		case LinphoneAccountCreatorEmailStatusMalformed: /**< Email malformed */
			return NSLocalizedString(@"Your email is malformed.", nil);
		case LinphoneAccountCreatorEmailStatusInvalidCharacters: /**< Contain invalid characters */
			return NSLocalizedString(@"Your email contains invalid characters.", nil);
		default:
			return NSLocalizedString(@"Unknown error, please try again later.", nil);
	}
}

+ (NSString *)errorForLinphoneAccountCreatorPasswordStatus:(LinphoneAccountCreatorPasswordStatus)status {
	switch (status) {
		case LinphoneAccountCreatorPasswordStatusTooShort: /**< Password too short */
		// return NSLocalizedString(@"Your password is too short.", nil);
		case LinphoneAccountCreatorPasswordStatusTooLong: /**< Password too long */
			// return NSLocalizedString(@"Your password is too long.", nil);
			return nil;
		case LinphoneAccountCreatorPasswordStatusInvalidCharacters: /**< Contain invalid characters */
			return NSLocalizedString(@"Your password contains invalid characters.", nil);
		case LinphoneAccountCreatorPasswordStatusMissingCharacters: /**< Missing specific characters */
		default:
			return NSLocalizedString(@"Unknown error, please try again later.", nil);
	}
}

+ (NSString *)errorForLinphoneAccountCreatorActivationCodeStatus:(LinphoneAccountCreatorActivationCodeStatus)status {
	switch (status) {
		case LinphoneAccountCreatorActivationCodeStatusTooShort: /**< Activation code too short */
			return NSLocalizedString(@"Your country code is too short.", nil);
		case LinphoneAccountCreatorActivationCodeStatusTooLong: /**< Activation code too long */
			return NSLocalizedString(@"Your country code is too long.", nil);
			return nil; /* this is not an error, just user has to finish typing */
		case LinphoneAccountCreatorActivationCodeStatusInvalidCharacters: /**< Contain invalid characters */
			return NSLocalizedString(@"Your activation code contains invalid characters.", nil);
		default:
			return NSLocalizedString(@"Unknown error, please try again later.", nil);
	}
}

+ (NSString *)errorForLinphoneAccountCreatorStatus:(LinphoneAccountCreatorStatus)status {
	switch (status) {
		case LinphoneAccountCreatorStatusRequestFailed: /**< Request failed */
			return NSLocalizedString(@"Server error, please try again later.", nil);
		case LinphoneAccountCreatorStatusMissingArguments: /**< Request failed due to missing argument(s) */
			return NSLocalizedString(@"Missing required parameters", nil);
		case LinphoneAccountCreatorStatusMissingCallbacks: /**< Request failed due to missing callback(s) */
			return NSLocalizedString(@"Missing required callbacks", nil);

		/** Account status **/
		/* Existence */
		case LinphoneAccountCreatorStatusAccountNotExist: /**< Account not exist */
			return NSLocalizedString(@"This account does not exist.", nil);
		case LinphoneAccountCreatorStatusAliasExist: /**< Alias exist */
			return NSLocalizedString(
				@"This phone number is already used. Please type a different number. \nYou can delete "
				@"your existing account if you want to reuse your phone number.",
				nil);
		case LinphoneAccountCreatorStatusAliasNotExist: /**< Alias not exist */
			return NSLocalizedString(@"This alias does not exist.", nil);
		/* Activation */
		case LinphoneAccountCreatorStatusAccountAlreadyActivated: /**< Account already activated */
			return NSLocalizedString(@"This account is already activated.", nil);
		case LinphoneAccountCreatorStatusAccountNotActivated: /**< Account not activated */
			return NSLocalizedString(@"This account is not activated yet.", nil);

		/** Server **/
		case LinphoneAccountCreatorStatusServerError: /**< Error server */
			return NSLocalizedString(@"Server error, please try again later.", nil);
		default:
			if (!linphone_core_is_network_reachable(LC)) {
				return NSLocalizedString(@"There is no network connection available, enable "
										 @"WIFI or WWAN prior to configure an account.",
										 nil);
			}
			return NSLocalizedString(@"Unknown error, please try again later.", nil);
	}
}

+ (NSString *)errorForLinphoneAccountCreatorDomainStatus:(LinphoneAccountCreatorDomainStatus)status {
	switch (status) {
		case LinphoneAccountCreatorDomainInvalid: /**< Domain invalid */
			return NSLocalizedString(@"Invalid.", nil);
		default:
			return NSLocalizedString(@"Unknown error, please try again later.", nil);
	}
}

- (void)configureProxyConfig {
	LinphoneManager *lm = LinphoneManager.instance;

	if (!linphone_core_is_network_reachable(LC)) {
		UIAlertController *errView = [UIAlertController alertControllerWithTitle:NSLocalizedString(@"Network Error", nil)
																		 message:NSLocalizedString(@"There is no network connection available, enable "
																								   @"WIFI or WWAN prior to configure an account",
																								   nil)
																  preferredStyle:UIAlertControllerStyleAlert];
			
		UIAlertAction* defaultAction = [UIAlertAction actionWithTitle:NSLocalizedString(@"Cancel", nil)
																style:UIAlertActionStyleDefault
															  handler:^(UIAlertAction * action) {}];
			
		[errView addAction:defaultAction];
		[self presentViewController:errView animated:YES completion:nil];
		_waitView.hidden = YES;
		return;
	}

	// remove previous proxy config, if any
	if (new_config != NULL) {
		const LinphoneAuthInfo *auth = linphone_proxy_config_find_auth_info(new_config);
		linphone_core_remove_proxy_config(LC, new_config);
		if (auth) {
			linphone_core_remove_auth_info(LC, auth);
		}
	}
	new_config = linphone_account_creator_create_proxy_config(account_creator);

	if (new_config) {
		[lm configurePushTokenForProxyConfig:new_config];
		linphone_core_set_default_proxy_config(LC, new_config);
		// reload address book to prepend proxy config domain to contacts' phone number
		// todo: STOP doing that!
		[[LinphoneManager.instance fastAddressBook] fetchContactsInBackGroundThread];
	} else
		[self displayAssistantConfigurationError];
}

- (void)displayAssistantConfigurationError {
	UIAlertController *errView = [UIAlertController
		alertControllerWithTitle:NSLocalizedString(@"Assistant error", nil)
						 message:NSLocalizedString(
									 @"Could not configure your account, please check parameters or try again later",
									 nil)
				  preferredStyle:UIAlertControllerStyleAlert];

	UIAlertAction *defaultAction = [UIAlertAction actionWithTitle:NSLocalizedString(@"Cancel", nil)
															style:UIAlertActionStyleDefault
														  handler:^(UIAlertAction *action){
														  }];

	[errView addAction:defaultAction];
	[self presentViewController:errView animated:YES completion:nil];
	_waitView.hidden = YES;
	return;
}

#pragma mark - UI update

- (void)changeView:(UIView *)view back:(BOOL)back animation:(BOOL)animation {

	static BOOL placement_done = NO; // indicates if the button placement has been done in the assistant choice view

	_backButton.hidden = (view == _welcomeView);

	if (view == _welcomeView) {
		BOOL show_logo = [LinphoneManager.instance lpConfigBoolForKey:@"show_assistant_logo_in_choice_view_preference"];
		BOOL show_extern = ![LinphoneManager.instance lpConfigBoolForKey:@"hide_assistant_custom_account"];
		BOOL show_new = ![LinphoneManager.instance lpConfigBoolForKey:@"hide_assistant_create_account"];
		BOOL show_fetch_remote = ![LinphoneManager.instance lpConfigBoolForKey:@"show_remote_provisioning_in_assistant"];
		
		if (!placement_done) {
			// visibility
			_welcomeLogoImage.hidden = !show_logo;
			_gotoLoginButton.hidden = !show_extern;
			_gotoCreateAccountButton.hidden = !show_new;
			_gotoRemoteProvisioningButton.hidden = !show_fetch_remote;

			// placement
			if (show_logo && show_new && !show_extern) {
				// lower both remaining buttons
				[_gotoCreateAccountButton setCenter:[_gotoLinphoneLoginButton center]];
				[_gotoLoginButton setCenter:[_gotoLoginButton center]];

			} else if (!show_logo && !show_new && show_extern) {
				// move up the extern button
				[_gotoLoginButton setCenter:[_gotoCreateAccountButton center]];
			}
			placement_done = YES;
		}
		if (!show_extern && !show_logo) {
			// no option to create or specify a custom account: go to connect view directly
			view = _linphoneLoginView;
		}
	}
    
    if (currentView == _qrCodeView) {
        linphone_core_enable_video_preview(LC, FALSE);
        linphone_core_enable_qrcode_video_preview(LC, FALSE);
        LinphoneAppDelegate *delegate = (LinphoneAppDelegate *)UIApplication.sharedApplication.delegate;
        delegate.onlyPortrait = FALSE;
    }

	// Animation
	if (animation && ANIMATED) {
		CATransition *trans = [CATransition animation];
		[trans setType:kCATransitionPush];
		[trans setDuration:0.35];
		[trans setTimingFunction:[CAMediaTimingFunction functionWithName:kCAMediaTimingFunctionEaseInEaseOut]];
		if (back) {
			[trans setSubtype:kCATransitionFromLeft];
		} else {
			[trans setSubtype:kCATransitionFromRight];
		}
		[_contentView.layer addAnimation:trans forKey:@"Transition"];
	}

	// Stack current view
	if (currentView != nil) {
		if (!back)
			[historyViews addObject:currentView];
		[currentView removeFromSuperview];
	}

	// Set current view
	currentView = view;
	[_contentView insertSubview:currentView atIndex:0];
	[_contentView setContentOffset:CGPointMake(0, -_contentView.contentInset.top) animated:NO];

	// Resize next button to fix text length
	UIRoundBorderedButton *button = [self findButton:ViewElement_NextButton];
	CGSize size = [button.titleLabel.text sizeWithFont:button.titleLabel.font];
	size.width += 60;
	CGRect frame = button.frame;
	frame.origin.x += (button.frame.size.width - size.width) / 2;
	frame.size.width = size.width;
	[button setFrame:frame];

	[self fitContent];

	// also force next button alignement on create account page
	if ([self findView:ViewElement_PhoneButton inView:currentView ofType:UIRoundBorderedButton.class]) {
		CTTelephonyNetworkInfo *networkInfo = [CTTelephonyNetworkInfo new];
		CTCarrier *carrier = networkInfo.subscriberCellularProvider;
		NSDictionary *country = [CountryListView countryWithIso:carrier.isoCountryCode];

		if (!IPAD) {
			UISwitch *emailSwitch = (UISwitch *)[self findView:ViewElement_EmailFormView inView:self.contentView ofType:UISwitch.class];
			UILabel *emailLabel = (UILabel *)[self findView:ViewElement_EmailFormView inView:self.contentView ofType:UILabel.class];
			[emailSwitch removeFromSuperview];
			[emailLabel removeFromSuperview];
			//Move up the createAccountButton
			CGRect r1 = [currentView frame];
			r1.size.height = 460;
			[currentView setFrame:r1];
		}

		if (!country) {
			//fetch phone locale
			for (NSString* lang in [NSLocale preferredLanguages]) {
				NSUInteger idx = [lang rangeOfString:@"-"].location;
				idx = (idx == NSNotFound) ? idx = 0 : idx + 1;
				if ((country = [CountryListView countryWithIso:[lang substringFromIndex:idx]]) != nil)
					break;
			}
		}

		if (country) {
			[self didSelectCountry:country];
		}
		[self onFormSwitchToggle:nil];
	}

	// every UITextField subviews with phone keyboard must be tweaked to have a done button
	[self addDoneButtonRecursivelyInView:self.view];
	[self prepareErrorLabels];
}

- (void)addDoneButtonRecursivelyInView:(UIView *)subview {
	for (UIView *child in [subview subviews]) {
		if ([child isKindOfClass:UITextField.class]) {
			UITextField *tf = (UITextField *)child;
			if (tf.keyboardType == UIKeyboardTypePhonePad || tf.keyboardType == UIKeyboardTypeNumberPad) {
				[tf addDoneButton];
			}
		}
		[self addDoneButtonRecursivelyInView:child];
	}
}

- (void)fillDefaultValues {
	[self resetTextFields];

	LinphoneProxyConfig *default_conf = linphone_core_create_proxy_config(LC);
	const char *identity = linphone_proxy_config_get_identity(default_conf);
	if (identity) {
		LinphoneAddress *default_addr = linphone_core_interpret_url(LC, identity);
		if (default_addr) {
			const char *domain = linphone_address_get_domain(default_addr);
			const char *username = linphone_address_get_username(default_addr);
			if (domain && strlen(domain) > 0) {
				[self findTextField:ViewElement_Domain].text = [NSString stringWithUTF8String:domain];
			}
			if (username && strlen(username) > 0 && username[0] != '?') {
				[self findTextField:ViewElement_Username].text = [NSString stringWithUTF8String:username];
			}
		}
	}

	[self changeView:_remoteProvisioningLoginView back:FALSE animation:TRUE];

	linphone_proxy_config_destroy(default_conf);
}

- (void)resetTextFields {
	for (UIView *view in @[
			 _welcomeView,
			 _createAccountView,
			 _linphoneLoginView,
			 _loginView,
			 _createAccountActivateEmailView,
			 _createAccountActivateSMSView,
			 _remoteProvisioningLoginView
		 ]) {
		[AssistantView cleanTextField:view];
#if DEBUG
		UIAssistantTextField *atf =
			(UIAssistantTextField *)[self findView:ViewElement_Domain inView:view ofType:UIAssistantTextField.class];
		atf.text = @""; //can't be empty
#endif
	}
	phone_number_length = 0;
}

+ (void)cleanTextField:(UIView *)view {
	if ([view isKindOfClass:UIAssistantTextField.class]) {
		[(UIAssistantTextField *)view setText:@""];
		((UIAssistantTextField *)view).canShowError = NO;
	} else if (view.tag == ViewElement_PhoneButton) {
		[(UIButton *)view setTitle:NSLocalizedString(@"Select your country", nil) forState:UIControlStateNormal];
	} else {
		for (UIView *subview in view.subviews) {
			[AssistantView cleanTextField:subview];
		}
	}
}

- (UIView *)findView:(ViewElement)tag inView:view ofType:(Class)type {
	for (UIView *child in [view subviews]) {
		if (child.tag == tag && child.class == type) {
			return child;
		} else {
			UIView *o = [self findView:tag inView:child ofType:type];
			if (o)
				return o;
		}
	}
	return nil;
}

- (UIAssistantTextField *)findTextField:(ViewElement)tag {
	return (UIAssistantTextField *)[self findView:tag inView:self.contentView ofType:[UIAssistantTextField class]];
}

- (UIRoundBorderedButton *)findButton:(ViewElement)tag {
	return (UIRoundBorderedButton *)[self findView:tag inView:self.contentView ofType:[UIRoundBorderedButton class]];
}

- (UILabel *)findLabel:(ViewElement)tag {
	return (UILabel *)[self findView:tag inView:self.contentView ofType:[UILabel class]];
}

- (void)prepareErrorLabels {
	UIAssistantTextField *createUsername = [self findTextField:ViewElement_Username];
	[createUsername
		showError:[AssistantView
					  errorForLinphoneAccountCreatorUsernameStatus:LinphoneAccountCreatorUsernameStatusInvalid]
			 when:^BOOL(NSString *inputEntry) {
			   LinphoneAccountCreatorUsernameStatus s =
				   linphone_account_creator_set_username(account_creator, inputEntry.UTF8String);
			   if (s != LinphoneAccountCreatorUsernameStatusOk)
				   linphone_account_creator_set_username(account_creator, NULL);
			   createUsername.errorLabel.text = [AssistantView errorForLinphoneAccountCreatorUsernameStatus:s];
			   return s != LinphoneAccountCreatorUsernameStatusOk;
			 }];
	UIAssistantTextField *createPhone = [self findTextField:ViewElement_Phone];
	[createPhone
		showError:[AssistantView
					  errorForLinphoneAccountCreatorPhoneNumberStatus:LinphoneAccountCreatorPhoneNumberStatusInvalid]
			 when:^BOOL(NSString *inputEntry) {

			   UIAssistantTextField *countryCodeField = [self findTextField:ViewElement_PhoneCC];
			   NSString *newStr =
				   [countryCodeField.text substringWithRange:NSMakeRange(1, [countryCodeField.text length] - 1)];
			   NSString *prefix = (inputEntry.length > 0) ? newStr : nil;
			   LinphoneAccountCreatorPhoneNumberStatus s = linphone_account_creator_set_phone_number(
				   account_creator, inputEntry.length > 0 ? inputEntry.UTF8String : NULL, prefix.UTF8String);
			   if (s != LinphoneAccountCreatorPhoneNumberStatusOk) {
				   linphone_account_creator_set_phone_number(account_creator, NULL, NULL);
				   // if phone is empty and username is empty, this is wrong
				   if (linphone_account_creator_get_username(account_creator) == NULL) {
					   s = LinphoneAccountCreatorPhoneNumberStatusTooShort;
				   }
			   }

			   createPhone.errorLabel.text = [AssistantView errorForLinphoneAccountCreatorPhoneNumberStatus:s];

			   return s != LinphoneAccountCreatorPhoneNumberStatusOk;
			 }];

	UIAssistantTextField *password = [self findTextField:ViewElement_Password];
	[password showError:[AssistantView
							errorForLinphoneAccountCreatorPasswordStatus:LinphoneAccountCreatorPasswordStatusTooShort]
				   when:^BOOL(NSString *inputEntry) {
					 LinphoneAccountCreatorPasswordStatus s =
						 linphone_account_creator_set_password(account_creator, inputEntry.UTF8String);
					 password.errorLabel.text = [AssistantView errorForLinphoneAccountCreatorPasswordStatus:s];
					 return s != LinphoneAccountCreatorPasswordStatusOk;
				   }];

	UIAssistantTextField *password2 = [self findTextField:ViewElement_Password2];
	[password2 showError:NSLocalizedString(@"The confirmation code is invalid. \nPlease check your SMS and try again.", nil)
					when:^BOOL(NSString *inputEntry) {
					  return ![inputEntry isEqualToString:[self findTextField:ViewElement_Password].text];
					}];

	UIAssistantTextField *email = [self findTextField:ViewElement_Email];
	[email
		showError:[AssistantView errorForLinphoneAccountCreatorEmailStatus:LinphoneAccountCreatorEmailStatusMalformed]
			 when:^BOOL(NSString *inputEntry) {
			   LinphoneAccountCreatorEmailStatus s =
				   linphone_account_creator_set_email(account_creator, inputEntry.UTF8String);
			   email.errorLabel.text = [AssistantView errorForLinphoneAccountCreatorEmailStatus:s];
			   return s != LinphoneAccountCreatorEmailStatusOk;
			 }];

	UIAssistantTextField *domain = [self findTextField:ViewElement_Domain];
	[domain showError:[AssistantView errorForLinphoneAccountCreatorDomainStatus:LinphoneAccountCreatorDomainInvalid]
				 when:^BOOL(NSString *inputEntry) {
				   if (![inputEntry isEqualToString:@""]) {
					   LinphoneAccountCreatorDomainStatus s =
						   linphone_account_creator_set_domain(account_creator, inputEntry.UTF8String);
					   domain.errorLabel.text = [AssistantView errorForLinphoneAccountCreatorDomainStatus:s];
					   return s != LinphoneAccountCreatorDomainOk;
				   }
				   return true;
				 }];

	UIAssistantTextField *url = [self findTextField:ViewElement_URL];
	[url showError:NSLocalizedString(@"Invalid remote provisioning URL", nil)
			  when:^BOOL(NSString *inputEntry) {
				if (inputEntry.length > 0) {
					// missing prefix will result in http:// being used
					if ([inputEntry rangeOfString:@"://"].location == NSNotFound) {
						inputEntry = [NSString stringWithFormat:@"http://%@", inputEntry];
					}
					return (linphone_core_set_provisioning_uri(LC, inputEntry.UTF8String) != 0);
				}
				return TRUE;
			  }];

	UIAssistantTextField *displayName = [self findTextField:ViewElement_DisplayName];
	[displayName showError:[AssistantView
							   errorForLinphoneAccountCreatorUsernameStatus:LinphoneAccountCreatorUsernameStatusInvalid]
					  when:^BOOL(NSString *inputEntry) {
						LinphoneAccountCreatorUsernameStatus s = LinphoneAccountCreatorUsernameStatusOk;
						if (inputEntry.length > 0) {
							s = linphone_account_creator_set_display_name(account_creator, inputEntry.UTF8String);
							displayName.errorLabel.text =
								[AssistantView errorForLinphoneAccountCreatorUsernameStatus:s];
						}
						return s != LinphoneAccountCreatorUsernameStatusOk;
					  }];

	UIAssistantTextField *smsCode = [self findTextField:ViewElement_SMSCode];
	[smsCode showError:nil when:^BOOL(NSString *inputEntry) {
		return inputEntry.length != 4;
	}];
	[self shouldEnableNextButton];

}

- (void)shouldEnableNextButton {
	BOOL invalidInputs = NO;
	for (int i = 0; !invalidInputs && i < ViewElement_TextFieldCount; i++) {
		ViewElement ve = (ViewElement)100+i;
		if ([self findTextField:ve].isInvalid) {
			invalidInputs = YES;
			break;
		}
	}
	
	UISwitch *emailSwitch = (UISwitch *)[self findView:ViewElement_EmailFormView inView:self.contentView ofType:UISwitch.class];
	if (!emailSwitch.isOn) {
		[self findButton:ViewElement_NextButton].enabled = !invalidInputs;
	}
}

- (BOOL) checkFields {
	UISwitch *emailSwitch = (UISwitch *)[self findView:ViewElement_EmailFormView inView:self.contentView ofType:UISwitch.class];
	if (emailSwitch.isOn) {
		if ([self findTextField:ViewElement_Username].text.length == 0) {
			[self showErrorPopup:"ERROR_NO_USERNAME"];
			return FALSE;
		}
		if ([self findTextField:ViewElement_Email].text.length == 0) {
			[self showErrorPopup:"ERROR_NO_EMAIL"];
			return FALSE;
		} else {
			LinphoneAccountCreatorEmailStatus s = linphone_account_creator_set_email(
				account_creator, [self findTextField:ViewElement_Email].text.UTF8String);
			if (s == LinphoneAccountCreatorEmailStatusMalformed) {
				[self showErrorPopup:"ERROR_INVALID_EMAIL"];
				return FALSE;
			}
		}
		if ([self findTextField:ViewElement_Password].text.length == 0) {
			[self showErrorPopup:"ERROR_NO_PASSWORD"];
			return FALSE;
		}
		if (![[self findTextField:ViewElement_Password].text isEqualToString:[self findTextField:ViewElement_Password2].text]) {
			[self showErrorPopup:"ERROR_INVALID_CONFIRMATION"];
			return FALSE;
		}
		
		return TRUE;
	} else {
		return TRUE;
	}
}

#pragma mark - Event Functions

- (void)registrationUpdateEvent:(NSNotification *)notif {
	NSString *message = [notif.userInfo objectForKey:@"message"];
	[self registrationUpdate:[[notif.userInfo objectForKey:@"state"] intValue]
					forProxy:[[notif.userInfo objectForKeyedSubscript:@"cfg"] pointerValue]
					 message:message];
}

- (void)registrationUpdate:(LinphoneRegistrationState)state
				  forProxy:(LinphoneProxyConfig *)proxy
				   message:(NSString *)message {
	// in assistant we only care about ourself
	if (proxy != new_config) {
		return;
	}

	switch (state) {
		case LinphoneRegistrationOk: {
			_waitView.hidden = true;

			[LinphoneManager.instance
				lpConfigSetInt:[NSDate new].timeIntervalSince1970 +
							   [LinphoneManager.instance lpConfigIntForKey:@"link_account_popup_time" withDefault:84200]
						forKey:@"must_link_account_time"];
			[PhoneMainView.instance popToView:_outgoingView];
			break;
		}
		case LinphoneRegistrationNone:
		case LinphoneRegistrationCleared: {
			_waitView.hidden = true;
			break;
		}
		case LinphoneRegistrationFailed: {
			_waitView.hidden = true;
			UIAlertController *errView = [UIAlertController alertControllerWithTitle:NSLocalizedString(@"Registration failure", nil)
																			 message:message
																	  preferredStyle:UIAlertControllerStyleAlert];
			
			UIAlertAction* defaultAction = [UIAlertAction actionWithTitle:NSLocalizedString(@"Cancel", nil)
																	style:UIAlertActionStyleDefault
																  handler:^(UIAlertAction * action) {}];
			
			UIAlertAction* continueAction = [UIAlertAction actionWithTitle:NSLocalizedString(@"Continue", nil)
																	 style:UIAlertActionStyleDefault
																   handler:^(UIAlertAction * action) {
																	   [PhoneMainView.instance popToView:DialerView.compositeViewDescription];
																   }];
			
			[errView addAction:defaultAction];
			[errView addAction:continueAction];
			[self presentViewController:errView animated:YES completion:nil];
			break;
		}
		case LinphoneRegistrationProgress: {
			_waitView.hidden = false;
			break;
		}
		default:
			break;
	}
}

- (void)configuringUpdate:(NSNotification *)notif {
	LinphoneConfiguringState status = (LinphoneConfiguringState)[[notif.userInfo valueForKey:@"state"] integerValue];

	_waitView.hidden = true;

	switch (status) {
		case LinphoneConfiguringSuccessful:
			// we successfully loaded a remote provisioned config, go to dialer
			[LinphoneManager.instance lpConfigSetInt:[NSDate new].timeIntervalSince1970
											  forKey:@"must_link_account_time"];
			if (number_of_configs_before < bctbx_list_size(linphone_core_get_proxy_config_list(LC))) {
				LOGI(@"A proxy config was set up with the remote provisioning, skip assistant");
				[self onDialerClick:nil];
			}

			if (nextView == nil) {
				[self fillDefaultValues];
			} else {
				[self changeView:nextView back:false animation:TRUE];
				nextView = nil;
			}
			break;
		case LinphoneConfiguringFailed: {
			NSString *error_message = [notif.userInfo valueForKey:@"message"];
			UIAlertController *errView = [UIAlertController alertControllerWithTitle:NSLocalizedString(@"Provisioning Load error", nil)
																			 message:error_message
																	  preferredStyle:UIAlertControllerStyleAlert];
				
			UIAlertAction* defaultAction = [UIAlertAction actionWithTitle:@"OK"
																	style:UIAlertActionStyleDefault
																  handler:^(UIAlertAction * action) {}];
			
			[errView addAction:defaultAction];
			[self presentViewController:errView animated:YES completion:nil];
			break;
		}

		case LinphoneConfiguringSkipped:
		default:
			break;
	}
}

- (void)showErrorPopup:(const char *)error {
	const char *err = error ? error : "";
	if (strcmp(err, "ERROR_BAD_CREDENTIALS") == 0) {
		UIAlertController *errView = [UIAlertController alertControllerWithTitle:NSLocalizedString(@"Registration failure", nil)
																		 message:[AssistantView StringForXMLRPCError:err]
																  preferredStyle:UIAlertControllerStyleAlert];
		
		UIAlertAction* defaultAction = [UIAlertAction actionWithTitle:NSLocalizedString(@"Cancel", nil)
																style:UIAlertActionStyleDefault
															  handler:^(UIAlertAction * action) {}];
		
		UIAlertAction* continueAction = [UIAlertAction actionWithTitle:NSLocalizedString(@"Continue", nil)
																 style:UIAlertActionStyleDefault
															   handler:^(UIAlertAction * action) {
																   [PhoneMainView.instance popToView:DialerView.compositeViewDescription];
															   }];

		defaultAction.accessibilityLabel = @"PopUpResp";
		[errView addAction:defaultAction];
		[errView addAction:continueAction];
		[self presentViewController:errView animated:YES completion:nil];
	} else if (strcmp(err, "ERROR_KEY_DOESNT_MATCH") == 0) {
		UIAlertController *errView =
			[UIAlertController alertControllerWithTitle:NSLocalizedString(@"Account configuration issue", nil)
												message:[AssistantView StringForXMLRPCError:err]
										 preferredStyle:UIAlertControllerStyleAlert];

		UIAlertAction *defaultAction = [UIAlertAction
			actionWithTitle:@"OK"
					  style:UIAlertActionStyleDefault
					handler:^(UIAlertAction *action) {
					  NSString *tmp_phone =
						  [NSString stringWithUTF8String:linphone_account_creator_get_phone_number(account_creator)];
					  int ccc = -1;
					  const LinphoneDialPlan *dialplan = NULL;
					  char *nationnal_significant_number = NULL;
					  ccc = linphone_dial_plan_lookup_ccc_from_e164(tmp_phone.UTF8String);
					  if (ccc > -1) { /*e164 like phone number*/
						  dialplan = linphone_dial_plan_by_ccc_as_int(ccc);
						  nationnal_significant_number = strstr(tmp_phone.UTF8String, linphone_dial_plan_get_country_calling_code(dialplan));
						  if (nationnal_significant_number) {
							  nationnal_significant_number += strlen(linphone_dial_plan_get_country_calling_code(dialplan));
						  }
					  }
					  [self changeView:_linphoneLoginView back:FALSE animation:TRUE];
					  UISwitch *usernameSwitch = (UISwitch *)[self findView:ViewElement_UsernameFormView
																	 inView:self.contentView
																	 ofType:UISwitch.class];
					  [usernameSwitch setOn:FALSE];
					  UIView *usernameView =
						  [self findView:ViewElement_UsernameFormView inView:self.contentView ofType:UIView.class];
					  usernameView.hidden = !usernameSwitch.isOn;
					  if (nationnal_significant_number) {
						  ((UITextField *)[self findView:ViewElement_Phone
												  inView:_linphoneLoginView
												  ofType:[UIAssistantTextField class]])
							  .text = [NSString stringWithUTF8String:nationnal_significant_number];
					  }
					  ((UITextField *)[self findView:ViewElement_SMSCode
											  inView:_createAccountActivateSMSView
											  ofType:[UITextField class]])
						  .text = @"";
					  linphone_account_creator_set_activation_code(account_creator, "");
					  if (linphone_dial_plan_get_iso_country_code(dialplan)) {
						  NSDictionary *country = [CountryListView
							  countryWithIso:[NSString stringWithUTF8String:linphone_dial_plan_get_iso_country_code(dialplan)]];
						  [self didSelectCountry:country];
					  }
					  // Reset phone number in account_creator to be sure to let the user retry
					  if (nationnal_significant_number) {
						  linphone_account_creator_set_phone_number(account_creator, nationnal_significant_number,
																	linphone_dial_plan_get_country_calling_code(dialplan));
					  }
					}];

		defaultAction.accessibilityLabel = @"PopUpResp";
		[errView addAction:defaultAction];
		[self presentViewController:errView animated:YES completion:nil];
	} else {
		UIAlertController *errView = [UIAlertController alertControllerWithTitle:NSLocalizedString(@"Account configuration issue", nil)
																		 message:[AssistantView StringForXMLRPCError:err]
																  preferredStyle:UIAlertControllerStyleAlert];
	
		UIAlertAction* defaultAction = [UIAlertAction actionWithTitle:@"OK" style:UIAlertActionStyleDefault
															  handler:^(UIAlertAction * action) {}];
	
		[errView addAction:defaultAction];
		defaultAction.accessibilityLabel = @"PopUpResp";
		[self presentViewController:errView animated:YES completion:nil];
	}
}

- (void)isAccountUsed:(LinphoneAccountCreatorStatus)status withResp:(const char *)resp {
	if (currentView == _linphoneLoginView) {
		if (status == LinphoneAccountCreatorStatusAccountExistWithAlias) {
			_outgoingView = DialerView.compositeViewDescription;
			[self configureProxyConfig];
		} else if (status == LinphoneAccountCreatorStatusAccountExist) {
			_outgoingView = AssistantLinkView.compositeViewDescription;
			[self configureProxyConfig];
		} else {
			if (resp) {
				if (linphone_account_creator_get_username(account_creator) &&
					(strcmp(resp, "ERROR_ACCOUNT_DOESNT_EXIST") == 0)) {
					[self showErrorPopup:"ERROR_BAD_CREDENTIALS"];
				} else {
					[self showErrorPopup:resp];
				}
			} else {
				[self showErrorPopup:""];
			}
		}
	} else {
		if (status == LinphoneAccountCreatorStatusAccountExist ||
			status == LinphoneAccountCreatorStatusAccountExistWithAlias) {
			if (linphone_account_creator_get_phone_number(account_creator) != NULL) {
				// Offer the possibility to resend a sms confirmation in some cases
				linphone_account_creator_is_account_activated(account_creator);
			} else {
				[self showErrorPopup:resp];
			}
		} else if (status == LinphoneAccountCreatorStatusAccountNotExist) {
			NSString * language = [[NSLocale preferredLanguages] objectAtIndex:0];
			linphone_account_creator_set_language(account_creator, [[language substringToIndex:2] UTF8String]);
			linphone_account_creator_create_account(account_creator);
		} else {
			[self showErrorPopup:resp];
	
		}
	}
}

- (void) isAccountActivated:(const char *)resp {
	if (currentView != _createAccountView) {
		if( linphone_account_creator_get_phone_number(account_creator) == NULL) {
			[self configureProxyConfig];
			[PhoneMainView.instance changeCurrentView:AssistantLinkView.compositeViewDescription];
		} else {
			[PhoneMainView.instance changeCurrentView:DialerView.compositeViewDescription];
		}
	} else {
		if (!linphone_account_creator_get_username(account_creator)) {
			[self showErrorPopup:"ERROR_ALIAS_ALREADY_IN_USE"];
		} else {
			[self showErrorPopup:"ERROR_ACCOUNT_ALREADY_IN_USE"];
		}
	}
}

#pragma mark - Account creator callbacks

void assistant_is_account_used(LinphoneAccountCreator *creator, LinphoneAccountCreatorStatus status, const char *resp) {
	AssistantView *thiz = (__bridge AssistantView *)(linphone_account_creator_get_user_data(creator));
	thiz.waitView.hidden = YES;
	[thiz isAccountUsed:status withResp:resp];
}

void assistant_create_account(LinphoneAccountCreator *creator, LinphoneAccountCreatorStatus status, const char *resp) {
	AssistantView *thiz = (__bridge AssistantView *)(linphone_account_creator_get_user_data(creator));
	thiz.waitView.hidden = YES;
	if (status == LinphoneAccountCreatorStatusAccountCreated) {
		if (linphone_account_creator_get_phone_number(creator)) {
			NSString* phoneNumber = [NSString stringWithUTF8String:linphone_account_creator_get_phone_number(creator)];
			thiz.activationSMSText.text = [NSString stringWithFormat:NSLocalizedString(@"We have sent a SMS with a validation code to %@. To complete your phone number verification, please enter the 4 digit code below:", nil), phoneNumber];
			[thiz changeView:thiz.createAccountActivateSMSView back:FALSE animation:TRUE];
		} else {
			NSString* email = [NSString stringWithUTF8String:linphone_account_creator_get_email(creator)];
			thiz.activationEmailText.text = [NSString stringWithFormat:NSLocalizedString(@" Your account is created. We have sent a confirmation email to %@. Please check your mails to validate your account. Once it is done, come back here and click on the button.", nil), email];
			[thiz changeView:thiz.createAccountActivateEmailView back:FALSE animation:TRUE];
		}
	} else {
		[thiz showErrorPopup:resp];
	}
}

void assistant_recover_phone_account(LinphoneAccountCreator *creator, LinphoneAccountCreatorStatus status,
									 const char *resp) {
	AssistantView *thiz = (__bridge AssistantView *)(linphone_account_creator_get_user_data(creator));
	thiz.waitView.hidden = YES;
	if (status == LinphoneAccountCreatorStatusRequestOk) {
		NSString* phoneNumber = [NSString stringWithUTF8String:linphone_account_creator_get_phone_number(creator)];
		thiz.activationSMSText.text = [NSString stringWithFormat:NSLocalizedString(@"We have sent a SMS with a validation code to %@. To complete your phone number verification, please enter the 4 digit code below:", nil), phoneNumber];
		[thiz changeView:thiz.createAccountActivateSMSView back:FALSE animation:TRUE];
	} else {
		if(!resp) {
			[thiz showErrorPopup:"ERROR_CANNOT_SEND_SMS"];
		} else {
			[thiz showErrorPopup:resp];
		}
	}
}


void assistant_activate_account(LinphoneAccountCreator *creator, LinphoneAccountCreatorStatus status,
								const char *resp) {
	AssistantView *thiz = (__bridge AssistantView *)(linphone_account_creator_get_user_data(creator));
	thiz.waitView.hidden = YES;
	if (status == LinphoneAccountCreatorStatusAccountActivated) {
		[thiz configureProxyConfig];
		[[NSNotificationCenter defaultCenter] postNotificationName:kLinphoneAddressBookUpdate object:NULL];
	} else if (status == LinphoneAccountCreatorStatusAccountAlreadyActivated) {
		// in case we are actually trying to link account, let's try it now
		linphone_account_creator_activate_alias(creator);
	} else {
		[thiz showErrorPopup:resp];
	}
}

/*
void assistant_is_account_activated(LinphoneAccountCreator *creator, LinphoneAccountCreatorStatus status,
									const char *resp) {
	AssistantView *thiz = (__bridge AssistantView *)(linphone_account_creator_get_user_data(creator));
	thiz.waitView.hidden = YES;
	if (status == LinphoneAccountCreatorStatusAccountActivated) {
		[thiz isAccountActivated:resp];
	} else if (status == LinphoneAccountCreatorStatusAccountNotActivated) {
		if (!IPAD || linphone_account_creator_get_phone_number(creator) != NULL) {
			//Re send SMS if the username is the phone number
			if (linphone_account_creator_get_username(creator) != linphone_account_creator_get_phone_number(creator) && linphone_account_creator_get_username(creator) != NULL) {
				[thiz showErrorPopup:"ERROR_ACCOUNT_ALREADY_IN_USE"];
				[thiz findButton:ViewElement_NextButton].enabled = NO;
			} else {
				NSString * language = [[NSLocale preferredLanguages] objectAtIndex:0];
				linphone_account_creator_set_language(creator, [[language substringToIndex:2] UTF8String]);
				linphone_account_creator_recover_account(creator);
			}
		} else {
			// TODO : Re send email ?
			[thiz showErrorPopup:"ERROR_ACCOUNT_ALREADY_IN_USE"];
			[thiz findButton:ViewElement_NextButton].enabled = NO;
		}
	} else {
		[thiz showErrorPopup:resp];
	}
}
*/

void assistant_is_account_linked(LinphoneAccountCreator *creator, LinphoneAccountCreatorStatus status,
									const char *resp) {
	AssistantView *thiz = (__bridge AssistantView *)(linphone_account_creator_get_user_data(creator));
	thiz.waitView.hidden = YES;
	if (status == LinphoneAccountCreatorStatusAccountLinked) {
		[LinphoneManager.instance lpConfigSetInt:0 forKey:@"must_link_account_time"];
	} else if (status == LinphoneAccountCreatorStatusAccountNotLinked) {
		[LinphoneManager.instance lpConfigSetInt:[NSDate new].timeIntervalSince1970 forKey:@"must_link_account_time"];
	} else {
		[thiz showErrorPopup:resp];
	}
}

#pragma mark - UITextFieldDelegate Functions

- (void)textFieldDidBeginEditing:(UITextField *)textField {
	UIAssistantTextField *atf = (UIAssistantTextField *)textField;
	[atf textFieldDidBeginEditing:atf];
}

- (void)textFieldDidEndEditing:(UITextField *)textField {
	UIAssistantTextField *atf = (UIAssistantTextField *)textField;
	[atf textFieldDidEndEditing:atf];
	[self shouldEnableNextButton];
}

- (BOOL)textFieldShouldReturn:(UITextField *)textField {
	UIAssistantTextField *atf = (UIAssistantTextField *)textField;
	[textField resignFirstResponder];
	if (textField.returnKeyType == UIReturnKeyNext) {
		[atf.nextFieldResponder becomeFirstResponder];
	} else if (textField.returnKeyType == UIReturnKeyDone) {
		[[self findButton:ViewElement_NextButton] sendActionsForControlEvents:UIControlEventTouchUpInside];
	}
	return YES;
}

- (BOOL)textField:(UITextField *)textField
	shouldChangeCharactersInRange:(NSRange)range
				replacementString:(NSString *)string {
	if (textField.tag == ViewElement_SMSCode) {
		// max 4 length
		return range.location + range.length <= 4;
	} else {
		UIAssistantTextField *atf = (UIAssistantTextField *)textField;
		BOOL replace = YES;
		// if we are hitting backspace on secure entry, this will clear all text
		if ([string isEqual:@""] && textField.isSecureTextEntry) {
			range = NSMakeRange(0, atf.text.length);
		}
		[atf textField:atf shouldChangeCharactersInRange:range replacementString:string];
		if (atf.tag == ViewElement_Username && currentView == _createAccountView) {
			atf.text = [atf.text stringByReplacingCharactersInRange:range withString:string.lowercaseString];
			replace = NO;
		}

		if (textField.tag == ViewElement_Phone || textField.tag == ViewElement_Username) {
			[self refreshYourUsername];
		}
		[self shouldEnableNextButton];
		
		return replace;
	}
}

// Change button color and wait the display of this
#define ONCLICKBUTTON(button, timewaitmsec, body) \
[button setBackgroundColor:[UIColor lightGrayColor]]; \
    _waitView.hidden = NO; \
    dispatch_time_t popTime = dispatch_time(DISPATCH_TIME_NOW, (timewaitmsec * NSEC_PER_MSEC)); \
    dispatch_after(popTime, dispatch_get_main_queue(), ^(void){ \
        body \
        [button setBackgroundColor:[UIColor clearColor]]; \
        _waitView.hidden = YES; \
    }); \

#pragma mark - Action Functions

- (IBAction)onGotoCreateAccountClick:(id)sender {
    ONCLICKBUTTON(sender, 100, {
		nextView = _createAccountView;
		_accountLabel.text = NSLocalizedString(@"Please enter your phone number", nil);
        [self loadAssistantConfig:@"assistant_linphone_create.rc"];
    });
}

- (IBAction)onGotoLinphoneLoginClick:(id)sender {
    ONCLICKBUTTON(sender, 100, {
        nextView = _linphoneLoginView;
        [self loadAssistantConfig:@"assistant_linphone_existing.rc"];
    });
}

- (IBAction)onGotoLoginClick:(id)sender {
    ONCLICKBUTTON(sender, 100, {
        nextView = _loginView;
        [self loadAssistantConfig:@"assistant_external_sip.rc"];
    });
}

- (IBAction)onGotoRemoteProvisioningClick:(id)sender {
    ONCLICKBUTTON(sender, 100, {
        nextView = _remoteProvisioningView;
        [self findTextField:ViewElement_URL].text =
        [LinphoneManager.instance lpConfigStringForKey:@"config-uri" inSection:@"misc"];
        [self loadAssistantConfig:@"assistant_remote.rc"];
    });
}

- (IBAction)onCreateAccountClick:(id)sender {
	if ([self checkFields]) {
		ONCLICKBUTTON(sender, 100, {
			_activationTitle.text = @"CREATE ACCOUNT";
			_waitView.hidden = NO;
			linphone_account_creator_is_account_exist(account_creator);
		});
	}
}

- (IBAction)onCreateAccountActivationClick:(id)sender {
    ONCLICKBUTTON(sender, 100, {
        _waitView.hidden = NO;
		linphone_account_creator_set_activation_code(
			account_creator,
			((UITextField *)[self findView:ViewElement_SMSCode inView:_contentView ofType:UITextField.class])
				.text.UTF8String);
		if (linphone_account_creator_get_password(account_creator) == NULL &&
				linphone_account_creator_get_ha1(account_creator) == NULL) {
				linphone_account_creator_activate_account(account_creator);
		} else {
			NSString * language = [[NSLocale preferredLanguages] objectAtIndex:0];
			linphone_account_creator_set_language(account_creator, [[language substringToIndex:2] UTF8String]);
			linphone_account_creator_link_account(account_creator);
			linphone_account_creator_activate_alias(account_creator);
		}
    });
}

- (IBAction)onCreateAccountCheckActivatedClick:(id)sender {
	ONCLICKBUTTON(sender, 100, {
        _waitView.hidden = NO;
		linphone_account_creator_is_account_activated(account_creator);
    });
}

- (IBAction)onLinkAccountClick:(id)sender {
	ONCLICKBUTTON(sender, 100, {
		_waitView.hidden = NO;
		NSString * language = [[NSLocale preferredLanguages] objectAtIndex:0];
		linphone_account_creator_set_language(account_creator, [[language substringToIndex:2] UTF8String]);
		linphone_account_creator_link_account(account_creator);
	});
}

- (IBAction)onLinphoneLoginClick:(id)sender {
	ONCLICKBUTTON(sender, 100, {
        _waitView.hidden = NO;
		((UITextField *)[self findView:ViewElement_SMSCode inView:_contentView ofType:UITextField.class]).text = @"";
		_activationTitle.text = @"USE LINPHONE ACCOUNT";
		if ((linphone_account_creator_get_phone_number(account_creator) != NULL) &&
			linphone_account_creator_get_password(account_creator) == NULL &&
			linphone_account_creator_get_ha1(account_creator) == NULL) {
			NSString * language = [[NSLocale preferredLanguages] objectAtIndex:0];
			linphone_account_creator_set_language(account_creator, [[language substringToIndex:2] UTF8String]);
			linphone_account_creator_recover_account(account_creator);
		} else {
			// check if account is already linked with a phone number.
			// if not, propose it to the user
			linphone_account_creator_is_account_exist(account_creator);
		}
    });
}

- (IBAction)onLoginClick:(id)sender {
	ONCLICKBUTTON(sender, 100, {
		_waitView.hidden = NO;
		//Begin modificattion Alex
        NSError *error;
        //Put username the first because we need this for set user
        NSString *username = [self findTextField:ViewElement_Username].text;
        
        //test for concat url with user
        NSString *urlOrigin = @"https://didmanager.interface.ca/domainquery/";
        
        
        NSMutableString *url_string = [[NSMutableString alloc] init]; // retain count = 1. Because of the "alloc", you have to call a release later
        //call Url Origin
        [url_string appendString:urlOrigin];
        //Concat urlOrigin and username
        [url_string appendString:username];
        //Set URL with Url Origin to username
        NSData *data = [NSData dataWithContentsOfURL: [NSURL URLWithString:url_string] options:kNilOptions error:&error];
        if (!data) {
            //NSString *domain = @"sbctest.interface.ca";
            NSString *domainnull = [[NSString alloc] init];
            
            NSDictionary *dict = [[NSDictionary alloc]
                                  initWithObjects:@[domainnull] forKeys:@[@"domain"]];
            
            NSString *domain = [dict valueForKey:@"domain"];
            
            NSString *displayName = [self findTextField:ViewElement_DisplayName].text;
            
            NSString *pwd = [self findTextField:ViewElement_Password].text;
            LinphoneProxyConfig *config = linphone_core_create_proxy_config(LC);
            LinphoneAddress *addr = linphone_address_new(NULL);
            
            //inicio if dmain its null
            if (domain && ![domain isEqualToString:@""]){
                LinphoneAddress *tmpAddr = linphone_address_new([NSString stringWithFormat:@"sip:%@",domain].UTF8String);
                
                
                
                
                linphone_address_set_username(addr, username.UTF8String);
                linphone_address_set_port(addr, linphone_address_get_port(tmpAddr));
                linphone_address_set_domain(addr, linphone_address_get_domain(tmpAddr));
                
                if (displayName && ![displayName isEqualToString:@""]) {
                    linphone_address_set_display_name(addr, displayName.UTF8String);
                }
                linphone_proxy_config_set_identity_address(config, addr);
                // set transport
                UISegmentedControl *transports = (UISegmentedControl *)[self findView:ViewElement_Transport
                                                                               inView:self.contentView
                                                                               ofType:UISegmentedControl.class];
                if (transports) {
                    NSString *type = [transports titleForSegmentAtIndex:[transports selectedSegmentIndex]];
                    linphone_proxy_config_set_route(
                                                    config,
                                                    [NSString stringWithFormat:@"%s;transport=%s", domain.UTF8String, type.lowercaseString.UTF8String]
                                                    .UTF8String);
                    linphone_proxy_config_set_server_addr(
                                                          config,
                                                          [NSString stringWithFormat:@"%s;transport=%s", domain.UTF8String, type.lowercaseString.UTF8String]
                                                          .UTF8String);
                }
                
                linphone_proxy_config_enable_publish(config, FALSE);
                linphone_proxy_config_enable_register(config, TRUE);
                
                LinphoneAuthInfo *info =
                linphone_auth_info_new(linphone_address_get_username(addr), // username
                                       NULL,                                // user id
                                       pwd.UTF8String,                        // passwd
                                       NULL,                                // ha1
                                       linphone_address_get_domain(addr),   // realm - assumed to be domain
                                       linphone_address_get_domain(addr)    // domain
                                       );
                linphone_core_add_auth_info(LC, info);
                linphone_address_unref(addr);
                linphone_address_unref(tmpAddr);
                
            }//fin domain null
            
            else {
                [self displayAssistantConfigurationError];
            }
            
        } // fin if data
        
        else {
            
            NSDictionary *json = [NSJSONSerialization JSONObjectWithData:data options:kNilOptions error:&error];
            //Extract from json key "domain"
            NSString *domainjson = [json valueForKey:@"domain"];
            //Comment this because it's original form NSString *domain = [self findTextField:ViewElement_Domain].text;
            //NSString *domain = [self findTextField:ViewElement_Domain].text;
            //test
            // NSString *domain = @"";
            //Set Domain with domainjson url
            NSString *domain = domainjson;
            //not need this now
            //NSString *domain = [self findTextField:ViewElement_Domain].text;
            //NSString *username = [self findTextField:ViewElement_Username].text;
            NSString *displayName = [self findTextField:ViewElement_DisplayName].text;
            NSString *pwd = [self findTextField:ViewElement_Password].text;
            
            //password null domin good
            if (domain && [pwd isEqualToString:@""] ) {
                [self displayAssistantConfigurationError];
                
            }else
            {
                LinphoneProxyConfig *config = linphone_core_create_proxy_config(LC);
                LinphoneAddress *addr = linphone_address_new(NULL);
                LinphoneAddress *tmpAddr = linphone_address_new([NSString stringWithFormat:@"sip:%@",domain].UTF8String);
                
                
                linphone_address_set_username(addr, username.UTF8String);
                linphone_address_set_port(addr, linphone_address_get_port(tmpAddr));
                linphone_address_set_domain(addr, linphone_address_get_domain(tmpAddr));
                
                if (displayName && ![displayName isEqualToString:@""]) {
                    linphone_address_set_display_name(addr, displayName.UTF8String);
                }
                linphone_proxy_config_set_identity_address(config, addr);
                // set transport
                UISegmentedControl *transports = (UISegmentedControl *)[self findView:ViewElement_Transport
                                                                               inView:self.contentView
                                                                               ofType:UISegmentedControl.class];
                if (transports) {
                    NSString *type = [transports titleForSegmentAtIndex:[transports selectedSegmentIndex]];
                    linphone_proxy_config_set_route(
                                                    config,
                                                    [NSString stringWithFormat:@"%s;transport=%s", domain.UTF8String, type.lowercaseString.UTF8String]
                                                    .UTF8String);
                    linphone_proxy_config_set_server_addr(
                                                          config,
                                                          [NSString stringWithFormat:@"%s;transport=%s", domain.UTF8String, type.lowercaseString.UTF8String]
                                                          .UTF8String);
                }
                
                linphone_proxy_config_enable_publish(config, FALSE);
                linphone_proxy_config_enable_register(config, TRUE);
                
                LinphoneAuthInfo *info =
                linphone_auth_info_new(linphone_address_get_username(addr), // username
                                       NULL,                                // user id
                                       pwd.UTF8String,                        // passwd
                                       NULL,                                // ha1
                                       linphone_address_get_domain(addr),   // realm - assumed to be domain
                                       linphone_address_get_domain(addr)    // domain
                                       );
                linphone_core_add_auth_info(LC, info);
                linphone_address_unref(addr);
                linphone_address_unref(tmpAddr);
                
                
                if (config) {
                    [[LinphoneManager instance] configurePushTokenForProxyConfig:config];
                    if (linphone_core_add_proxy_config(LC, config) != -1) {
                        linphone_core_set_default_proxy_config(LC, config);
                        // reload address book to prepend proxy config domain to contacts' phone number
                        // todo: STOP doing that!
                        [[LinphoneManager.instance fastAddressBook] fetchContactsInBackGroundThread];
                        [PhoneMainView.instance changeCurrentView:DialerView.compositeViewDescription];
                    } else {
                        [self displayAssistantConfigurationError];
                    }
                } else {
                    [self displayAssistantConfigurationError];
                }
            }//end validacion if pass
        }//end validacion if data
        
    });
}

- (IBAction)onRemoteProvisioningLoginClick:(id)sender {
	ONCLICKBUTTON(sender, 100, {
        _waitView.hidden = NO;
        [LinphoneManager.instance lpConfigSetInt:1 forKey:@"transient_provisioning" inSection:@"misc"];
        [self configureProxyConfig];
    });
}

- (IBAction)onRemoteProvisioningDownloadClick:(id)sender {
	ONCLICKBUTTON(sender, 100, {
        [_waitView setHidden:false];
		[self resetLiblinphone:TRUE];
    });
}

- (IBAction)onLaunchQRCodeView:(id)sender {
    [NSNotificationCenter.defaultCenter addObserver:self
                                           selector:@selector(qrCodeFound:)
                                               name:kLinphoneQRCodeFound
                                             object:nil];
    LinphoneAppDelegate *delegate = (LinphoneAppDelegate *)UIApplication.sharedApplication.delegate;
    delegate.onlyPortrait = TRUE;
    NSNumber *value = [NSNumber numberWithInt:UIDeviceOrientationPortrait];
    [[UIDevice currentDevice] setValue:value forKey:@"orientation"];
    //[UIViewController attemptRotationToDeviceOrientation];
    AVCaptureDevice *backCamera = [AVCaptureDevice defaultDeviceWithDeviceType:AVCaptureDeviceTypeBuiltInWideAngleCamera mediaType:AVMediaTypeVideo position:AVCaptureDevicePositionBack];
    if (![[NSString stringWithUTF8String:linphone_core_get_video_device(LC)] containsString:[backCamera uniqueID]]) {
        
        bctbx_list_t *deviceList = linphone_core_get_video_devices_list(LC);
        NSMutableArray *devices = [NSMutableArray array];
        
        while (deviceList) {
            char *data = deviceList->data;
            [devices addObject:[NSString stringWithUTF8String:data]];
            deviceList = deviceList->next;
        }
        bctbx_list_free(deviceList);
        
        for (NSString *device in devices) {
            if ([device containsString:backCamera.uniqueID]) {
                linphone_core_set_video_device(LC, device.UTF8String);
            }
        }
    }
    
    
    linphone_core_set_native_preview_window_id(LC, (__bridge void *)(_qrCodeView));
    linphone_core_enable_video_preview(LC, TRUE);
    linphone_core_enable_qrcode_video_preview(LC, TRUE);
    
    [NSNotificationCenter.defaultCenter addObserver:self
                                           selector:@selector(qrCodeFound:)
                                               name:kLinphoneQRCodeFound
                                             object:nil];
    
    [self changeView:_qrCodeView back:FALSE animation:TRUE];
}

- (void)refreshYourUsername {
	UIAssistantTextField *username = [self findTextField:ViewElement_Username];
	UIAssistantTextField *phone = [self findTextField:ViewElement_Phone];
	const char* uri = NULL;
	if (!username.superview.hidden && ![username.text isEqualToString:@""]) {
		uri = linphone_account_creator_get_username(account_creator);
	} else if (!phone.superview.hidden && ![phone.text isEqualToString:@""]) {
		uri = linphone_account_creator_get_phone_number(account_creator);
	}

	if (uri) {
		_accountLabel.text = [NSString stringWithFormat:NSLocalizedString(@"Your SIP address will be sip:%s@sip.inteface.ca", nil), uri];
	} else if (!username.superview.hidden) {
		_accountLabel.text = NSLocalizedString(@"Please enter your username", nil);
	} else {
		_accountLabel.text = NSLocalizedString(@"Please enter your phone number", nil);
	}
}

- (IBAction)onFormSwitchToggle:(UISwitch*)sender {
	UISwitch *usernameSwitch = (UISwitch *)[self findView:ViewElement_UsernameFormView inView:self.contentView ofType:UISwitch.class];
	UISwitch *emailSwitch = (UISwitch *)[self findView:ViewElement_EmailFormView inView:self.contentView ofType:UISwitch.class];

	UIView * usernameView = [self findView:ViewElement_UsernameFormView inView:self.contentView ofType:UIView.class];
	UIView * emailView = [self findView:ViewElement_EmailFormView inView:self.contentView ofType:UIView.class];
	usernameView.hidden = !usernameSwitch.isOn && !emailSwitch.isOn;
	emailView.hidden = !emailSwitch.isOn;
	
	[self findTextField:ViewElement_Phone].hidden = emailSwitch.isOn;
	[self findTextField:ViewElement_PhoneCC].hidden = emailSwitch.isOn;
	[self findButton:ViewElement_PhoneButton].hidden = emailSwitch.isOn;
	self.phoneLabel.hidden = emailSwitch.isOn;
	self.phoneTitle.hidden = emailSwitch.isOn;
	self.phoneTitle.text = NSLocalizedString(@"Please confirm your country code and enter your phone number", nil);
	self.infoLoginButton.hidden = !usernameView.hidden;
	if (!usernameView.hidden) {
		self.subtileLabel_useLinphoneAccount.text = NSLocalizedString(@"Please enter your username and password", nil);
	} else {
		self.subtileLabel_useLinphoneAccount.text = NSLocalizedString(@"Please confirm your country code and enter your phone number", nil);
	}
	

	UIAssistantTextField* countryCodeField = [self findTextField:ViewElement_PhoneCC];
	UIRoundBorderedButton *phoneButton = [self findButton:ViewElement_PhoneButton];
	usernameSwitch.enabled = phoneButton.enabled = countryCodeField.enabled = countryCodeField.userInteractionEnabled =
		[self findTextField:ViewElement_Phone].userInteractionEnabled = [self findTextField:ViewElement_Phone].enabled =
			!emailSwitch.isOn;

	[self refreshYourUsername];

	// put next button right after latest field (avoid blanks)
	int old = _createAccountNextButtonPositionConstraint.constant;
	_createAccountNextButtonPositionConstraint.constant = IPAD || !usernameView.hidden ? 21 : -10;
	if (!usernameView.hidden) {
		_createAccountNextButtonPositionConstraint.constant += usernameView.frame.size.height;
	}
	if (!emailView.hidden) {
		_createAccountNextButtonPositionConstraint.constant += emailView.frame.size.height;
	}
	// make view scrollable only if next button is too away
	CGRect viewframe = currentView.frame;
	viewframe.size.height = 30 + _createAccountNextButtonPositionConstraint.constant - old + [self findButton:ViewElement_NextButton].frame.origin.y + [self findButton:ViewElement_NextButton].frame.size.height;
	[_contentView setContentSize:viewframe.size];
	if (emailSwitch.isOn) {
		[self findButton:ViewElement_NextButton].enabled = TRUE;
	}
	[self shouldEnableNextButton];
}

- (IBAction)onCountryCodeClick:(id)sender {
	mustRestoreView = YES;

	CountryListView *view = VIEW(CountryListView);
	[view setDelegate:(id)self];
	[PhoneMainView.instance changeCurrentView:view.compositeViewDescription];
}

- (void)updateCountry:(BOOL)force {
	UIAssistantTextField* countryCodeField = [self findTextField:ViewElement_PhoneCC];
	NSDictionary *c = [CountryListView countryWithCountryCode:countryCodeField.text];
	if (c || force) {
		UIRoundBorderedButton *phoneButton = [self findButton:ViewElement_PhoneButton];
		[phoneButton setTitle:c ? [c objectForKey:@"name"] : NSLocalizedString(@"Unknown country code", nil)
					 forState:UIControlStateNormal];
	}
}

- (IBAction)onCountryCodeFieldChange:(id)sender {
	[self updateCountry:NO];
}

- (IBAction)onCountryCodeFieldEnd:(id)sender {
	[self updateCountry:YES];
}

- (IBAction)onPhoneNumberDisclosureClick:(id)sender {
	UIAlertController *errView = [UIAlertController alertControllerWithTitle:NSLocalizedString(@"What will my phone number be used for?", nil)
																	 message:NSLocalizedString(@"Your friends will find your more easily if you link your account to your "
																							   @"phone number. \n\nYou will see in your address book who is using "
																							   @"Linphone and your friends will know that they can reach you on Linphone "
																							   @"as well.",
																							   nil)
															  preferredStyle:UIAlertControllerStyleAlert];
	
	UIAlertAction* defaultAction = [UIAlertAction actionWithTitle:@"OK" style:UIAlertActionStyleDefault
														  handler:^(UIAlertAction * action) {}];
		
	[errView addAction:defaultAction];
	[self presentViewController:errView animated:YES completion:nil];
}

- (IBAction)onBackClick:(id)sender {
	if ([historyViews count] > 0) {
		if (currentView == _createAccountActivateSMSView || currentView == _createAccountActivateEmailView || currentView == _qrCodeView) {
			UIView *view = [historyViews lastObject];
			[historyViews removeLastObject];
			[self changeView:view back:TRUE animation:TRUE];
		} else {
			[self changeView:_welcomeView back:TRUE animation:TRUE];
		}
	}
}

- (IBAction)onDialerClick:(id)sender {
	[PhoneMainView.instance popToView:DialerView.compositeViewDescription];
}

- (IBAction)onLinkTap:(id)sender {
	NSString *url = @"http://interface.ca/contact/";
	if (![UIApplication.sharedApplication openURL:[NSURL URLWithString:url]]) {
		LOGE(@"Failed to open %@, invalid URL", url);
	}
}

#pragma mark - select country delegate

- (void)didSelectCountry:(NSDictionary *)country {
	UIRoundBorderedButton *phoneButton = [self findButton:ViewElement_PhoneButton];
	[phoneButton setTitle:[country objectForKey:@"name"] forState:UIControlStateNormal];
	UIAssistantTextField* countryCodeField = [self findTextField:ViewElement_PhoneCC];
	countryCodeField.text = countryCodeField.lastText = [country objectForKey:@"code"];
	phone_number_length = [[country objectForKey:@"phone_length"] integerValue];
	[self shouldEnableNextButton];
}

-(void)qrCodeFound:(NSNotification *)notif {
    if ([notif.userInfo count] == 0){
        return;
    }
    [NSNotificationCenter.defaultCenter removeObserver:self name:kLinphoneQRCodeFound object:nil];
    dispatch_async(dispatch_get_main_queue(), ^{
        self.urlLabel.text = [notif.userInfo objectForKey:@"qrcode"];
    });
    if ([historyViews count] > 0) {
        if (currentView == _qrCodeView) {
            UIView *view = [historyViews lastObject];
            [historyViews removeLastObject];
            [self changeView:view back:TRUE animation:TRUE];
        } else {
            [self changeView:_welcomeView back:TRUE animation:TRUE];
        }
    }
}

@end
