//
//  AssistantLinkView.m
//  linphone
//
//  Created by Gautier Pelloux-Prayer on 29/08/16.
//
//

#import <CoreTelephony/CTCarrier.h>
#import <CoreTelephony/CTTelephonyNetworkInfo.h>

#import "AssistantLinkView.h"
#import "UITextField+DoneButton.h"
#import "UIAssistantTextField.h"

@implementation AssistantLinkView {
	LinphoneAccountCreator *account_creator;
}

- (void)viewDidLoad {
	[super viewDidLoad];
	// every UITextField subviews with phone keyboard must be tweaked to have a done button
	[self addDoneButtonRecursivelyInView:self.view];
	self.phoneField.delegate = self; self.firstTime = TRUE;
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

- (void)viewDidAppear:(BOOL)animated {
	[super viewDidAppear:animated];

	_linkAccountView.hidden = _activateSMSView.userInteractionEnabled = NO;
	_activateSMSView.hidden = _linkAccountView.userInteractionEnabled = YES;

	account_creator = linphone_account_creator_new(
		LC, [LinphoneManager.instance lpConfigStringForKey:@"xmlrpc_url" inSection:@"assistant" withDefault:@""]
				.UTF8String);

	linphone_account_creator_set_user_data(account_creator, (__bridge void *)(self));
	linphone_account_creator_cbs_set_link_phone_number_with_account(
		linphone_account_creator_get_callbacks(account_creator), assistant_link_phone_number_with_account);
	linphone_account_creator_cbs_set_activate_phone_number_link(linphone_account_creator_get_callbacks(account_creator),
																assistant_activate_phone_number_link);

	LinphoneProxyConfig *cfg = linphone_core_get_default_proxy_config(LC);
	if (cfg && strcmp("sip.linphone.org", linphone_proxy_config_get_domain(cfg)) == 0) {
		linphone_account_creator_set_username(
			account_creator, linphone_address_get_username(linphone_proxy_config_get_identity_address(cfg)));
		const LinphoneAuthInfo *info = linphone_proxy_config_find_auth_info(cfg);
		if (info) {
			if (linphone_auth_info_get_passwd(info))
				linphone_account_creator_set_password(account_creator, linphone_auth_info_get_passwd(info));
			else
				linphone_account_creator_set_ha1(account_creator, linphone_auth_info_get_ha1(info));
		}
		linphone_account_creator_set_domain(account_creator, linphone_proxy_config_get_domain(cfg));
	} else {
		LOGW(@"Default proxy is NOT a sip.linphone.org, aborting");
		[PhoneMainView.instance popToView:DialerView.compositeViewDescription];
	}

	CTTelephonyNetworkInfo *networkInfo = [CTTelephonyNetworkInfo new];
	CTCarrier *carrier = networkInfo.subscriberCellularProvider;
	NSDictionary *country = [CountryListView countryWithIso:carrier.isoCountryCode];
	if (!country) {
		// fetch phone locale
		for (NSString *lang in [NSLocale preferredLanguages]) {
			NSUInteger idx = [lang rangeOfString:@"-"].location;
			idx = (idx == NSNotFound) ? idx = 0 : idx + 1;
			if ((country = [CountryListView countryWithIso:[lang substringFromIndex:idx]]) != nil)
				break;
		}
	}

	if (country && self.firstTime) {
		[self didSelectCountry:country];
	}
}

- (void)viewDidDisappear:(BOOL)animated {
	if (account_creator) {
		linphone_account_creator_unref(account_creator);
	}
	account_creator = NULL;
	[super viewDidDisappear:animated];
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

#pragma mark - popup

- (void)showErrorPopup:(const char *)err {
	if (strcmp(err, "ERROR_KEY_DOESNT_MATCH") == 0) {
		UIAlertController *errView =
			[UIAlertController alertControllerWithTitle:NSLocalizedString(@"Account configuration issue", nil)
												message:[AssistantView StringForXMLRPCError:err]
										 preferredStyle:UIAlertControllerStyleAlert];

		UIAlertAction *defaultAction = [UIAlertAction actionWithTitle:@"OK"
																style:UIAlertActionStyleDefault
															  handler:^(UIAlertAction *action) {
																self.linkAccountView.hidden = NO;
																self.linkAccountView.userInteractionEnabled = YES;
																self.activateSMSView.userInteractionEnabled = NO;
																self.activateSMSView.hidden = YES;
																self.activationCodeField.text = @"";
															  }];

		[errView addAction:defaultAction];
		[self presentViewController:errView animated:YES completion:nil];
	} else {
		UIAlertController *errView =
			[UIAlertController alertControllerWithTitle:NSLocalizedString(@"Account configuration issue", nil)
												message:[AssistantView StringForXMLRPCError:err]
										 preferredStyle:UIAlertControllerStyleAlert];

		UIAlertAction *defaultAction = [UIAlertAction actionWithTitle:@"OK"
																style:UIAlertActionStyleDefault
															  handler:^(UIAlertAction *action){
															  }];

		[errView addAction:defaultAction];
		[self presentViewController:errView animated:YES completion:nil];
	}
}

#pragma mark - cbs

void assistant_link_phone_number_with_account(LinphoneAccountCreator *creator, LinphoneAccountCreatorStatus status,
											  const char *resp) {
	AssistantLinkView *thiz = (__bridge AssistantLinkView *)(linphone_account_creator_get_user_data(creator));
	thiz.waitView.hidden = YES;
	if (status == LinphoneAccountCreatorOK) {
		thiz.linkAccountView.hidden = thiz.activateSMSView.userInteractionEnabled = YES;
		NSString* phoneNumber = [NSString stringWithUTF8String:linphone_account_creator_get_phone_number(creator)];
		thiz.linkSMSText.text = [NSString stringWithFormat:NSLocalizedString(@"We have sent a SMS with a validation code to %@. To complete your phone number verification, please enter the 4 digit code below:",nil), phoneNumber];
		thiz.activateSMSView.hidden = thiz.linkAccountView.userInteractionEnabled = NO;
	} else {
		if (strcmp(resp, "Missing required parameters") == 0) {
			[thiz showErrorPopup:"ERROR_NO_PHONE_NUMBER"];
		} else {
			[thiz showErrorPopup:resp];
		}
	}
}

void assistant_activate_phone_number_link(LinphoneAccountCreator *creator, LinphoneAccountCreatorStatus status,
										  const char *resp) {
	AssistantLinkView *thiz = (__bridge AssistantLinkView *)(linphone_account_creator_get_user_data(creator));
	thiz.waitView.hidden = YES;
	if (status == LinphoneAccountCreatorOK) {
		[LinphoneManager.instance lpConfigSetInt:0 forKey:@"must_link_account_time"];
		// save country code prefix if none is already entered
		LinphoneProxyConfig *cfg = linphone_core_get_default_proxy_config(LC);
		if (linphone_proxy_config_get_dial_prefix(cfg) == NULL) {
			const char *prefix = thiz.countryCodeField.text.UTF8String;
			linphone_proxy_config_edit(cfg);
			linphone_proxy_config_set_dial_prefix(cfg, prefix[0] == '+' ? &prefix[1] : prefix);
			linphone_proxy_config_done(cfg);
		}
		[PhoneMainView.instance popToView:DialerView.compositeViewDescription];
		[[NSNotificationCenter defaultCenter] postNotificationName:kLinphoneAddressBookUpdate object:NULL];
		[LinphoneManager.instance.fastAddressBook reload];
	} else {
		[thiz showErrorPopup:resp];
	}
}

#pragma mark - other
- (void)updateCountry:(BOOL)force {
	NSDictionary *c = [CountryListView countryWithCountryCode:_countryCodeField.text];
	if (c || force) {
		[_countryButton setTitle:c ? [c objectForKey:@"name"] : NSLocalizedString(@"Unknown country code", nil)
						forState:UIControlStateNormal];
	}
	if ([[_countryButton currentTitle] isEqualToString:NSLocalizedString(@"Unknown country code", nil)]) {
		_countryCodeField.layer.borderWidth = .8;
		_countryCodeField.layer.cornerRadius = 4.f;
		_countryCodeField.layer.borderColor = [[UIColor redColor] CGColor];
		self.linkAccountButton.enabled = FALSE;
	} else {
		_countryCodeField.layer.borderColor = [[UIColor clearColor] CGColor];
		if (_phoneField.layer.borderColor != [[UIColor redColor] CGColor]) {
			self.linkAccountButton.enabled = TRUE;
		}
	}
}

- (IBAction)onCountryCodeFieldChange:(id)sender {
	[self updateCountry:NO];
}

- (IBAction)onCountryCodeFieldEnd:(id)sender {
	[self updateCountry:YES];
}

- (IBAction)onCountryClick:(id)sender {
	self.firstTime = FALSE;
	CountryListView *view = VIEW(CountryListView);
	[view setDelegate:(id)self];
	[PhoneMainView.instance changeCurrentView:view.compositeViewDescription];
}

- (IBAction)onLinkAccount:(id)sender {
	_waitView.hidden = NO;
	NSString *newStr = [_countryCodeField.text substringWithRange:NSMakeRange(1, [_countryCodeField.text length]-1)];
	linphone_account_creator_set_phone_number(account_creator, _phoneField.text.UTF8String,
											  newStr.UTF8String);
	
	NSString * language = [[NSLocale preferredLanguages] objectAtIndex:0];
	linphone_account_creator_set_language(account_creator, [[language substringToIndex:2] UTF8String]);
	linphone_account_creator_link_phone_number_with_account(account_creator);
}

- (IBAction)onCheckValidationButton:(id)sender {
	_waitView.hidden = NO;
	linphone_account_creator_set_activation_code(account_creator, _activationCodeField.text.UTF8String);
	linphone_account_creator_activate_phone_number_link(account_creator);
}

- (IBAction)onDialerClick:(id)sender {
	[PhoneMainView.instance popToView:DialerView.compositeViewDescription];
}

- (IBAction)onPhoneNumberDisclosureClick:(id)sender {
	UIAlertController *errView = [UIAlertController alertControllerWithTitle:NSLocalizedString(@"What will my phone number be used for?", nil)
																	 message:NSLocalizedString(@"Your friends will find your more easily if you link your account to your "
																							   @"phone number. \n\nYou will see in your address book who is using "
																							   @"Linphone and your friends will know that they can reach you on Linphone "
																							   @"as well. \n\nYou can use your phone number with only one Linphone "
																							   @"account. If you had already linked your number to an other account but "
																							   @"you prefer to use this one, simply link it now and your number will "
																							   @"automatically be moved to this account.",
																							   nil)
															   preferredStyle:UIAlertControllerStyleAlert];
	
	UIAlertAction* defaultAction = [UIAlertAction actionWithTitle:@"OK"
															style:UIAlertActionStyleDefault
														  handler:^(UIAlertAction * action) {}];
	[errView addAction:defaultAction];
	[self presentViewController:errView animated:YES completion:nil];
}

#pragma mark - select country delegate

- (void)didSelectCountry:(NSDictionary *)country {
	[_countryButton setTitle:[country objectForKey:@"name"] forState:UIControlStateNormal];
	_countryCodeField.text = [country objectForKey:@"code"];
}

#pragma mark - UITextFieldDelegate Functions

- (BOOL)textField:(UITextField *)textField shouldChangeCharactersInRange:(NSRange)range replacementString:(NSString *)string {
	//remove the + from the country code to avoir error when checking its validity
	NSString *newStr = [_countryCodeField.text substringWithRange:NSMakeRange(1, [_countryCodeField.text length]-1)];
	LinphoneAccountCreatorStatus status = linphone_account_creator_set_phone_number(account_creator, [_phoneField.text UTF8String], [newStr UTF8String]);
	if (status == LinphoneAccountCreatorPhoneNumberTooLong || self.phoneField.text.length < 8) {
		self.phoneField.layer.borderWidth = .8;
		self.phoneField.layer.cornerRadius = 4.f;
		self.phoneField.layer.borderColor = [[UIColor redColor] CGColor];
		self.linkAccountButton.enabled = FALSE;
	} else {
		self.phoneField.layer.borderColor = [[UIColor clearColor] CGColor];
		if (_countryCodeField.layer.borderColor != [[UIColor redColor] CGColor]){
			self.linkAccountButton.enabled = TRUE;
		}
	}
	return YES;
}

@end
