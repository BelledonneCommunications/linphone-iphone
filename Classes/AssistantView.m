
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


#import <CoreTelephony/CTCarrier.h>
#import <CoreTelephony/CTTelephonyNetworkInfo.h>

#import "AssistantView.h"
#import "LinphoneManager.h"
#import "PhoneMainView.h"
#import "UITextField+DoneButton.h"
#import "UIAssistantTextField.h"
#import "CountryListViewController.h"

#import <XMLRPCConnection.h>
#import <XMLRPCConnectionManager.h>
#import <XMLRPCResponse.h>
#import <XMLRPCRequest.h>

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

- (void)viewDidLoad {
	[super viewDidLoad];
	_countryButton.layer.borderWidth = .8;
	_countryButton.layer.borderColor = _countryButton.backgroundColor.CGColor;
	_countryButton.layer.cornerRadius = 4.f;
	_countryButton.layer.masksToBounds = YES;
}

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

	if (!mustRestoreView) {
		new_config = NULL;
		number_of_configs_before = bctbx_list_size(linphone_core_get_proxy_config_list(LC));
		[self resetTextFields];
		[self changeView:_welcomeView back:FALSE animation:FALSE];
	}
	mustRestoreView = NO;
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
}

- (void)didRotateFromInterfaceOrientation:(UIInterfaceOrientation)fromInterfaceOrientation {
	[self fitContent];
}

#pragma mark - Utils

- (void)resetLiblinphone {
	if (account_creator) {
		linphone_account_creator_unref(account_creator);
		account_creator = NULL;
	}
	[LinphoneManager.instance resetLinphoneCore];
	account_creator = linphone_account_creator_new(
		LC, [LinphoneManager.instance lpConfigStringForKey:@"xmlrpc_url" inSection:@"assistant" withDefault:@""]
				.UTF8String);
	linphone_account_creator_set_user_data(account_creator, (__bridge void *)(self));
	linphone_account_creator_cbs_set_is_account_used(linphone_account_creator_get_callbacks(account_creator),
													  assistant_is_account_used);
	linphone_account_creator_cbs_set_create_account(linphone_account_creator_get_callbacks(account_creator),
													assistant_create_account);
	linphone_account_creator_cbs_set_activate_account(linphone_account_creator_get_callbacks(account_creator),
													assistant_activate_account);
	linphone_account_creator_cbs_set_is_account_activated(linphone_account_creator_get_callbacks(account_creator),
													   assistant_is_account_activated);

	linphone_account_creator_cbs_set_link_phone_number_with_account(linphone_account_creator_get_callbacks(account_creator),
													   assistant_link_phone_number_with_account);
	linphone_account_creator_cbs_set_activate_phone_number_link(linphone_account_creator_get_callbacks(account_creator),
													   assistant_activate_phone_number_link);
	linphone_account_creator_cbs_set_recover_phone_account(linphone_account_creator_get_callbacks(account_creator),
													   assistant_recover_phone_account);
}
- (void)loadAssistantConfig:(NSString *)rcFilename {
	NSString *fullPath = [@"file://" stringByAppendingString:[LinphoneManager bundleFile:rcFilename]];
	linphone_core_set_provisioning_uri(LC, fullPath.UTF8String);
	[LinphoneManager.instance lpConfigSetInt:1 forKey:@"transient_provisioning" inSection:@"misc"];

	[self resetLiblinphone];
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

+ (NSString *)errorForStatus:(LinphoneAccountCreatorStatus)status {
	switch (status) {
		case LinphoneAccountCreatorEmailInvalid:
			return NSLocalizedString(@"Invalid email.", nil);
		case LinphoneAccountCreatorUsernameInvalid:
			return NSLocalizedString(@"Invalid username.", nil);
		case LinphoneAccountCreatorUsernameTooShort:
			return NSLocalizedString(@"Username too short.", nil);
		case LinphoneAccountCreatorUsernameTooLong:
			return NSLocalizedString(@"Username too long.", nil);
		case LinphoneAccountCreatorUsernameInvalidSize:
			return NSLocalizedString(@"Username length invalid.", nil);
		case LinphoneAccountCreatorPhoneNumberTooShort:
		case LinphoneAccountCreatorPhoneNumberTooLong:
			return nil; /* this is not an error, just user has to finish typing */
		case LinphoneAccountCreatorPhoneNumberInvalid:
			return NSLocalizedString(@"Invalid phone number.", nil);
		case LinphoneAccountCreatorPasswordTooShort:
			return NSLocalizedString(@"Password too short.", nil);
		case LinphoneAccountCreatorPasswordTooLong:
			return NSLocalizedString(@"Password too long.", nil);
		case LinphoneAccountCreatorDomainInvalid:
			return NSLocalizedString(@"Invalid domain.", nil);
		case LinphoneAccountCreatorRouteInvalid:
			return NSLocalizedString(@"Invalid route.", nil);
		case LinphoneAccountCreatorDisplayNameInvalid:
			return NSLocalizedString(@"Invalid display name.", nil);
		case LinphoneAccountCreatorReqFailed:
			return NSLocalizedString(@"Failed to query the server. Please try again later", nil);
		case LinphoneAccountCreatorTransportNotSupported:
			return NSLocalizedString(@"Unsupported transport", nil);
		case LinphoneAccountCreatorAccountCreated:
		case LinphoneAccountCreatorAccountExist:
		case LinphoneAccountCreatorAccountExistWithAlias:
		case LinphoneAccountCreatorAccountNotCreated:
		case LinphoneAccountCreatorAccountNotExist:
		case LinphoneAccountCreatorAccountNotActivated:
		case LinphoneAccountCreatorAccountAlreadyActivated:
		case LinphoneAccountCreatorAccountActivated:
		case LinphoneAccountCreatorOK:
			break;
	}
	return nil;
}

- (void)configureProxyConfig {
	LinphoneManager *lm = LinphoneManager.instance;

	if (!linphone_core_is_network_reachable(LC)) {
		UIAlertView *error =
			[[UIAlertView alloc] initWithTitle:NSLocalizedString(@"Network Error", nil)
									   message:NSLocalizedString(@"There is no network connection available, enable "
																 @"WIFI or WWAN prior to configure an account",
																 nil)
									  delegate:nil
							 cancelButtonTitle:NSLocalizedString(@"Cancel", nil)
							 otherButtonTitles:nil];
		[error show];
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

	// set transport
	UISegmentedControl *transports = (UISegmentedControl *)[self findView:ViewElement_Transport
																   inView:self.contentView
																   ofType:UISegmentedControl.class];
	if (transports) {
		NSString *type = [transports titleForSegmentAtIndex:[transports selectedSegmentIndex]];
		linphone_account_creator_set_transport(account_creator,
											   linphone_transport_parse(type.lowercaseString.UTF8String));
	}

	new_config = linphone_account_creator_configure(account_creator);

	if (new_config) {
		[lm configurePushTokenForProxyConfig:new_config];
		linphone_core_set_default_proxy_config(LC, new_config);
		// reload address book to prepend proxy config domain to contacts' phone number
		// todo: STOP doing that!
		[[LinphoneManager.instance fastAddressBook] reload];
	} else {
		UIAlertView *error = [[UIAlertView alloc]
				initWithTitle:NSLocalizedString(@"Assistant error", nil)
					  message:NSLocalizedString(
								  @"Could not configure your account, please check parameters or try again later", nil)
					 delegate:nil
			cancelButtonTitle:NSLocalizedString(@"Cancel", nil)
			otherButtonTitles:nil];
		[error show];
		_waitView.hidden = YES;
		return;
	}
}

#pragma mark - UI update

- (void)changeView:(UIView *)view back:(BOOL)back animation:(BOOL)animation {

	static BOOL placement_done = NO; // indicates if the button placement has been done in the assistant choice view

	_backButton.hidden = (view == _welcomeView);

	if (view == _welcomeView) {
		BOOL show_logo = [LinphoneManager.instance lpConfigBoolForKey:@"show_assistant_logo_in_choice_view_preference"];
		BOOL show_extern = ![LinphoneManager.instance lpConfigBoolForKey:@"hide_assistant_custom_account"];
		BOOL show_new = ![LinphoneManager.instance lpConfigBoolForKey:@"hide_assistant_create_account"];

		if (!placement_done) {
			// visibility
			_welcomeLogoImage.hidden = !show_logo;
			_gotoLoginButton.hidden = !show_extern;
			_gotoCreateAccountButton.hidden = !show_new;

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
	if (currentView == _createAccountView) {
		CTTelephonyNetworkInfo *networkInfo = [CTTelephonyNetworkInfo new];
		CTCarrier *carrier = networkInfo.subscriberCellularProvider;
		NSDictionary *country = [CountryListViewController countryWithIso:carrier.isoCountryCode];

		if (!IPAD) {
			UISwitch *emailSwitch = (UISwitch *)[self findView:ViewElement_EmailFormView inView:self.contentView ofType:UISwitch.class];
			UILabel *emailLabel = (UILabel *)[self findView:ViewElement_EmailFormView inView:self.contentView ofType:UILabel.class];
			emailSwitch.hidden = emailLabel.hidden = YES;
		}

		if (!country) {
			//fetch phone locale
			for (NSString* lang in [NSLocale preferredLanguages]) {
				NSUInteger idx = [lang rangeOfString:@"-"].location;
				idx = (idx == NSNotFound) ? idx = 0 : idx + 1;
				if ((country = [CountryListViewController countryWithIso:[lang substringFromIndex:idx]]) != nil)
					break;
			}
		}

		if (country) {
			[self didSelectCountry:country];
		}
		[self onFormSwitchToggle:nil];
	}

	[self prepareErrorLabels];

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
		atf.text = @"test.linphone.org";
#endif
	}
	phone_number_length = 0;
}

+ (void)cleanTextField:(UIView *)view {
	if ([view isKindOfClass:UIAssistantTextField.class]) {
		[(UIAssistantTextField *)view setText:@""];
		((UIAssistantTextField *)view).canShowError = NO;
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
	[createUsername showError:[AssistantView errorForStatus:LinphoneAccountCreatorUsernameInvalid]
						 when:^BOOL(NSString *inputEntry) {
						   LinphoneAccountCreatorStatus s =
							   linphone_account_creator_set_username(account_creator, inputEntry.UTF8String);
							 if (s != LinphoneAccountCreatorOK) linphone_account_creator_set_username(account_creator, NULL);
						   createUsername.errorLabel.text = [AssistantView errorForStatus:s];
						   return s != LinphoneAccountCreatorOK;
						 }];
	UIAssistantTextField *createPhone = [self findTextField:ViewElement_Phone];
	[createPhone showError:[AssistantView errorForStatus:LinphoneAccountCreatorPhoneNumberInvalid]
						 when:^BOOL(NSString *inputEntry) {
							 UIAssistantTextField* countryCodeField = [self findTextField:ViewElement_PhoneCC];
							 NSString* prefix = countryCodeField.text.length > 0 ? [countryCodeField.text substringFromIndex:1] : nil;
							 LinphoneAccountCreatorStatus s =
							 linphone_account_creator_set_phone_number(account_creator, inputEntry.length > 0 ? inputEntry.UTF8String : NULL, prefix.UTF8String);
							 if (s != LinphoneAccountCreatorOK) linphone_account_creator_set_phone_number(account_creator, NULL, NULL);

							 // if phone is empty and username is empty, this is wrong
							 if (linphone_account_creator_get_phone_number(account_creator) == NULL) {
								 s = LinphoneAccountCreatorPhoneNumberTooShort;
							 }

							 createPhone.errorLabel.text = [AssistantView errorForStatus:s];


							 return s != LinphoneAccountCreatorOK;
						 }];

	UIAssistantTextField *password = [self findTextField:ViewElement_Password];
	[password showError:[AssistantView errorForStatus:LinphoneAccountCreatorPasswordTooShort]
				   when:^BOOL(NSString *inputEntry) {
					 LinphoneAccountCreatorStatus s =
					   linphone_account_creator_set_password(account_creator, inputEntry.UTF8String);
					 password.errorLabel.text = [AssistantView errorForStatus:s];
					 return s != LinphoneAccountCreatorOK;
				   }];

	UIAssistantTextField *password2 = [self findTextField:ViewElement_Password2];
	[password2 showError:NSLocalizedString(@"Passwords do not match.", nil)
					when:^BOOL(NSString *inputEntry) {
					  return ![inputEntry isEqualToString:[self findTextField:ViewElement_Password].text];
					}];

	UIAssistantTextField *email = [self findTextField:ViewElement_Email];
	[email showError:[AssistantView errorForStatus:LinphoneAccountCreatorEmailInvalid]
				when:^BOOL(NSString *inputEntry) {
				  LinphoneAccountCreatorStatus s =
					  linphone_account_creator_set_email(account_creator, inputEntry.UTF8String);
				  email.errorLabel.text = [AssistantView errorForStatus:s];
				  return s != LinphoneAccountCreatorOK;
				}];

	UIAssistantTextField *domain = [self findTextField:ViewElement_Domain];
	[domain showError:[AssistantView errorForStatus:LinphoneAccountCreatorDomainInvalid]
				 when:^BOOL(NSString *inputEntry) {
				   LinphoneAccountCreatorStatus s =
					   linphone_account_creator_set_domain(account_creator, inputEntry.UTF8String);
				   domain.errorLabel.text = [AssistantView errorForStatus:s];
				   return s != LinphoneAccountCreatorOK;
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
	[displayName showError:[AssistantView errorForStatus:LinphoneAccountCreatorDisplayNameInvalid]
					  when:^BOOL(NSString *inputEntry) {
						LinphoneAccountCreatorStatus s = LinphoneAccountCreatorOK;
						if (inputEntry.length > 0) {
							s = linphone_account_creator_set_display_name(account_creator, inputEntry.UTF8String);
							displayName.errorLabel.text = [AssistantView errorForStatus:s];
						}
						return s != LinphoneAccountCreatorOK;
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

	[self findButton:ViewElement_NextButton].enabled = !invalidInputs;
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
			[PhoneMainView.instance popToView:DialerView.compositeViewDescription];
			break;
		}
		case LinphoneRegistrationNone:
		case LinphoneRegistrationCleared: {
			_waitView.hidden = true;
			break;
		}
		case LinphoneRegistrationFailed: {
			_waitView.hidden = true;
			DTAlertView *alert = [[DTAlertView alloc] initWithTitle:NSLocalizedString(@"Registration failure", nil)
															message:message
														   delegate:nil
												  cancelButtonTitle:@"Cancel"
												  otherButtonTitles:nil];
			[alert addButtonWithTitle:@"Continue"
								block:^(void) {
								  [PhoneMainView.instance popToView:DialerView.compositeViewDescription];
								}];
			[alert show];
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
			UIAlertView *alert = [[UIAlertView alloc] initWithTitle:NSLocalizedString(@"Provisioning Load error", nil)
															message:error_message
														   delegate:nil
												  cancelButtonTitle:NSLocalizedString(@"OK", nil)
												  otherButtonTitles:nil];
			[alert show];
			break;
		}

		case LinphoneConfiguringSkipped:
		default:
			break;
	}
}

- (void)genericError {
	UIAlertView *errorView = [[UIAlertView alloc]
							  initWithTitle:NSLocalizedString(@"Account creation issue", nil)
							  message:NSLocalizedString(@"Your account could not be created, please try again later.", nil)
							  delegate:nil
							  cancelButtonTitle:NSLocalizedString(@"Continue", nil)
							  otherButtonTitles:nil, nil];
	[errorView show];
}

- (void)isAccountUsed:(LinphoneAccountCreatorStatus)status {
	if (currentView == _createAccountView) {
		if (status == LinphoneAccountCreatorAccountExist || status == LinphoneAccountCreatorAccountExistWithAlias) {
			ViewElement ve =  ([self findTextField:ViewElement_Username].isVisible) ? ViewElement_Username : ViewElement_Phone;
			[[self findTextField:ve] showError:NSLocalizedString(@"This account already exists.", nil)];
			[self findButton:ViewElement_NextButton].enabled = NO;
		} else if (status == LinphoneAccountCreatorAccountNotExist) {
			linphone_account_creator_create_account(account_creator);
		} else {
			[self genericError];
		}
	} else if (currentView ==  _linphoneLoginView) {
		[self findTextField:ViewElement_PhoneCC].enabled =
		[self findTextField:ViewElement_Phone].enabled =
		[self findTextField:ViewElement_DisplayName].enabled =
		[self findTextField:ViewElement_Password].enabled = (status == LinphoneAccountCreatorAccountExist);
		if (status == LinphoneAccountCreatorAccountExistWithAlias) {
			[self findButton:ViewElement_NextButton].enabled = YES;
		} else {
			[self shouldEnableNextButton];
		}
	}
}

#pragma mark - Account creator callbacks

void assistant_is_account_used(LinphoneAccountCreator *creator, LinphoneAccountCreatorStatus status) {
	AssistantView *thiz = (__bridge AssistantView *)(linphone_account_creator_get_user_data(creator));
	thiz.waitView.hidden = YES;
	[thiz isAccountUsed:status];
}

void assistant_create_account(LinphoneAccountCreator *creator, LinphoneAccountCreatorStatus status) {
	AssistantView *thiz = (__bridge AssistantView *)(linphone_account_creator_get_user_data(creator));
	thiz.waitView.hidden = YES;
	if (status == LinphoneAccountCreatorAccountCreated) {
		if (linphone_account_creator_get_phone_number(creator)) {
			[thiz changeView:thiz.createAccountActivateSMSView back:FALSE animation:TRUE];
		} else {
			[thiz changeView:thiz.createAccountActivateEmailView back:FALSE animation:TRUE];
		}
	} else {
		[thiz genericError];
	}
}

void assistant_recover_phone_account(LinphoneAccountCreator *creator, LinphoneAccountCreatorStatus status) {
	AssistantView *thiz = (__bridge AssistantView *)(linphone_account_creator_get_user_data(creator));
	thiz.waitView.hidden = YES;
	if (status == LinphoneAccountCreatorOK) {
		[thiz changeView:thiz.createAccountActivateSMSView back:FALSE animation:TRUE];
	} else {
		[thiz genericError];
	}
}

void assistant_activate_account(LinphoneAccountCreator *creator, LinphoneAccountCreatorStatus status) {
	AssistantView *thiz = (__bridge AssistantView *)(linphone_account_creator_get_user_data(creator));
	thiz.waitView.hidden = YES;
	if (status == LinphoneAccountCreatorAccountActivated) {
		[thiz configureProxyConfig];
	} else if (status == LinphoneAccountCreatorAccountNotActivated) {
		UIAlertView *errorView = [[UIAlertView alloc]
								  initWithTitle:NSLocalizedString(@"Account activation issue", nil)
								  message:NSLocalizedString(@"Your account could not be activated, please check SMS code.", nil)
								  delegate:nil
								  cancelButtonTitle:NSLocalizedString(@"Continue", nil)
								  otherButtonTitles:nil, nil];
		[errorView show];
	} else if (status == LinphoneAccountCreatorAccountAlreadyActivated) {
		// in case we are actually trying to link account, let's try it now
		linphone_account_creator_activate_phone_number_link(creator);
	} else {
		[thiz genericError];
	}
}

void assistant_is_account_activated(LinphoneAccountCreator *creator, LinphoneAccountCreatorStatus status) {
	AssistantView *thiz = (__bridge AssistantView *)(linphone_account_creator_get_user_data(creator));
	thiz.waitView.hidden = YES;
	if (status == LinphoneAccountCreatorAccountActivated) {
		[thiz configureProxyConfig];
	} else if (status == LinphoneAccountCreatorAccountNotActivated) {
		DTAlertView *alert = [[DTAlertView alloc]
			initWithTitle:NSLocalizedString(@"Account validation failed", nil)
				  message:
					  NSLocalizedString(
						  @"Your account could not be checked yet. You can skip this validation or try again later.",
						  nil)];
		[alert addCancelButtonWithTitle:NSLocalizedString(@"Back", nil) block:nil];
		[alert addButtonWithTitle:NSLocalizedString(@"Skip verification", nil)
							block:^{
							  [thiz configureProxyConfig];
							  [PhoneMainView.instance popToView:DialerView.compositeViewDescription];
							}];
		[alert show];
	} else {
		[thiz genericError];
	}
}

void assistant_link_phone_number_with_account(LinphoneAccountCreator *creator, LinphoneAccountCreatorStatus status) {
	AssistantView *thiz = (__bridge AssistantView *)(linphone_account_creator_get_user_data(creator));
	thiz.waitView.hidden = YES;
	if (status == LinphoneAccountCreatorOK) {
		[thiz changeView:thiz.createAccountActivateSMSView back:FALSE animation:TRUE];
	} else {
		UIAlertView *errorView = [[UIAlertView alloc]
								  initWithTitle:NSLocalizedString(@"Account link issue", nil)
								  message:NSLocalizedString(@"Could not link your phone number with your account, please try again later.", nil)
								  delegate:nil
								  cancelButtonTitle:NSLocalizedString(@"Continue", nil)
								  otherButtonTitles:nil, nil];
		[errorView show];
	}
}

void assistant_activate_phone_number_link(LinphoneAccountCreator *creator, LinphoneAccountCreatorStatus status) {
	AssistantView *thiz = (__bridge AssistantView *)(linphone_account_creator_get_user_data(creator));
	thiz.waitView.hidden = YES;
	if (status == LinphoneAccountCreatorOK) {
		[thiz configureProxyConfig];
	} else {
		UIAlertView *errorView = [[UIAlertView alloc]
								  initWithTitle:NSLocalizedString(@"Account link issue", nil)
								  message:NSLocalizedString(@"Could not link your phone number with your account, please try again later.", nil)
								  delegate:nil
								  cancelButtonTitle:NSLocalizedString(@"Continue", nil)
								  otherButtonTitles:nil, nil];
		[errorView show];
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

	if (textField.tag == ViewElement_Username && currentView == _linphoneLoginView) {
		linphone_account_creator_is_account_used(account_creator);
	}

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
        [self loadAssistantConfig:@"assistant_remote.rc"];
        [self findTextField:ViewElement_URL].text =
        [LinphoneManager.instance lpConfigStringForKey:@"config-uri" inSection:@"misc"];
    });
}

- (IBAction)onCreateAccountClick:(id)sender {
	ONCLICKBUTTON(sender, 100, {
        _waitView.hidden = NO;
        linphone_account_creator_is_account_used(account_creator);
    });
}

- (IBAction)onCreateAccountActivationClick:(id)sender {
    ONCLICKBUTTON(sender, 100, {
        _waitView.hidden = NO;
        linphone_account_creator_set_activation_code(account_creator, ((UITextField*)[self findView:ViewElement_SMSCode inView:_contentView ofType:UITextField.class]).text.UTF8String);
        linphone_account_creator_activate_account(account_creator);
    });
}

- (IBAction)onCreateAccountCheckActivatedClick:(id)sender {
	ONCLICKBUTTON(sender, 100, {
        _waitView.hidden = NO;
        linphone_account_creator_is_account_activated(account_creator);
    });
}

- (IBAction)onLinphoneLoginClick:(id)sender {
	ONCLICKBUTTON(sender, 100, {
        _waitView.hidden = NO;

        NSString *phone = [self findTextField:ViewElement_Phone].text;
        if (phone.length > 0) {
            linphone_account_creator_link_phone_number_with_account(account_creator);
        } else {
			if ((linphone_account_creator_get_phone_number(account_creator) != NULL)
				&& [self findTextField:ViewElement_Password].text.length == 0) {
				linphone_account_creator_recover_phone_account(account_creator);
			} else {
				[self configureProxyConfig];
			}
        }
    });
}

- (IBAction)onLoginClick:(id)sender {
	ONCLICKBUTTON(sender, 100, {
        _waitView.hidden = NO;
        [self configureProxyConfig];
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
        [self resetLiblinphone];
    });
}

- (void)refreshYourUsername {
	UIAssistantTextField *username = [self findTextField:ViewElement_Username];
	UIAssistantTextField *phone = [self findTextField:ViewElement_Phone];
	const char* uri = NULL;
	if (!username.superview.hidden) {
		uri = linphone_account_creator_get_username(account_creator);
	} else if (!phone.superview.hidden) {
		uri = linphone_account_creator_get_phone_number(account_creator);
	}

	if (uri) {
		_accountLabel.text = [NSString stringWithFormat:NSLocalizedString(@"Your SIP address will be sip:%s@sip.linphone.org", nil), uri];
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

	UIAssistantTextField* countryCodeField = [self findTextField:ViewElement_PhoneCC];
	usernameSwitch.enabled = _countryButton.enabled = countryCodeField.enabled = countryCodeField.userInteractionEnabled = [self findTextField:ViewElement_Phone].userInteractionEnabled = [self findTextField:ViewElement_Phone].enabled = !emailSwitch.isOn;

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
	[self shouldEnableNextButton];
}

- (IBAction)onCountryCodeClick:(id)sender {
	mustRestoreView = YES;

	CountryListViewController *view = VIEW(CountryListViewController);
	[view setDelegate:(id)self];
	[PhoneMainView.instance changeCurrentView:view.compositeViewDescription];
}

- (void)updateCountry:(BOOL)force {
	UIAssistantTextField* countryCodeField = [self findTextField:ViewElement_PhoneCC];
	NSDictionary *c = [CountryListViewController countryWithCountryCode:countryCodeField.text];
	if (c || force) {
		[_countryButton setTitle:c ? [c objectForKey:@"name"] : NSLocalizedString(@"Unknown country code", nil) forState:UIControlStateNormal];
	}
}

- (IBAction)onCountryCodeFieldChange:(id)sender {
	[self updateCountry:NO];
}

- (IBAction)onCountryCodeFieldEnd:(id)sender {
	[self updateCountry:YES];
}

- (IBAction)onPhoneNumberDisclosureClick:(id)sender {
	UIAlertView *errorView = [[UIAlertView alloc]
							  initWithTitle:NSLocalizedString(@"What is my phone number for?", nil)
							  message:NSLocalizedString(@"A SMS code will be sent to your phone number to validate your account.", nil)
							  delegate:nil
							  cancelButtonTitle:NSLocalizedString(@"OK", nil)
							  otherButtonTitles:nil, nil];
	[errorView show];
}

- (IBAction)onBackClick:(id)sender {
	if ([historyViews count] > 0) {
		UIView *view = [historyViews lastObject];
		[historyViews removeLastObject];
		[self changeView:view back:TRUE animation:TRUE];
	}
}

- (IBAction)onDialerClick:(id)sender {
	[PhoneMainView.instance popToView:DialerView.compositeViewDescription];
}

- (IBAction)onLinkTap:(id)sender {
	NSString *url = @"http://linphone.org/free-sip-service.html&action=recover";
	if (![UIApplication.sharedApplication openURL:[NSURL URLWithString:url]]) {
		LOGE(@"Failed to open %@, invalid URL", url);
	}
}

#pragma mark - select country delegate

- (void)didSelectCountry:(NSDictionary *)country{
	[_countryButton setTitle:[country objectForKey:@"name"] forState:UIControlStateNormal];
	UIAssistantTextField* countryCodeField = [self findTextField:ViewElement_PhoneCC];
	countryCodeField.text = countryCodeField.lastText = [country objectForKey:@"code"];
	phone_number_length = [[country objectForKey:@"phone_length"] integerValue];
	[self shouldEnableNextButton];
}

@end
