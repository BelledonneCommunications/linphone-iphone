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
	[self fitScrollContentSize];

	if (!account_creator) {
		account_creator = linphone_account_creator_new(
			LC,
			[LinphoneManager.instance lpConfigStringForKey:@"xmlrpc_url" inSection:@"assistant" withDefault:@""]
				.UTF8String);
	}

	linphone_account_creator_set_user_data(account_creator, (__bridge void *)(self));
	linphone_account_creator_cbs_set_link_account(linphone_account_creator_get_callbacks(account_creator),
												  assistant_link_phone_number_with_account);
	linphone_account_creator_cbs_set_activate_alias(linphone_account_creator_get_callbacks(account_creator),
													assistant_activate_phone_number_link);

	LinphoneAccount *acc = linphone_core_get_default_account(LC);
	LinphoneAccountParams const *accParams = (acc) ? linphone_account_get_params(acc) : NULL;
	if (acc &&
		strcmp([LinphoneManager.instance lpConfigStringForKey:@"domain_name"
													inSection:@"app"
												  withDefault:@"sip.linphone.org"]
				   .UTF8String,
			   linphone_account_params_get_domain(accParams)) == 0) {
		linphone_account_creator_set_username(
			account_creator, linphone_address_get_username(linphone_account_params_get_identity_address(accParams)));
		const LinphoneAuthInfo *info = linphone_account_find_auth_info(acc);
		if (info) {
			if (linphone_auth_info_get_passwd(info))
				linphone_account_creator_set_password(account_creator, linphone_auth_info_get_passwd(info));
			else
				linphone_account_creator_set_ha1(account_creator, linphone_auth_info_get_ha1(info));
		}
		linphone_account_creator_set_domain(account_creator, linphone_account_params_get_domain(accParams));
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

- (void)fitScrollContentSize {
	// make view scrollable only if next button is too away
	CGRect viewframe = _linkAccountView.frame;
	if (UIInterfaceOrientationIsLandscape([[UIApplication sharedApplication] statusBarOrientation])) {
		viewframe.size.height += 60;
	}
	[_linkAccountView  setContentSize:viewframe.size];
}

- (void)didRotateFromInterfaceOrientation:(UIInterfaceOrientation)fromInterfaceOrientation {
	[self fitScrollContentSize];
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
	if (status == LinphoneAccountCreatorStatusRequestOk) {
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
	if (status == LinphoneAccountCreatorStatusAccountActivated) {
		[LinphoneManager.instance lpConfigSetInt:0 forKey:@"must_link_account_time"];
		// save country code prefix if none is already entered
		LinphoneAccount *acc = linphone_core_get_default_account(LC);
		LinphoneAccountParams const *accParams = linphone_account_get_params(acc);
		if (linphone_account_params_get_international_prefix(accParams) == NULL) {
			const char *prefix = thiz.countryCodeField.text.UTF8String;
			LinphoneAccountParams * newPrefixAccountParams = linphone_account_params_clone(accParams);
			linphone_account_params_set_international_prefix(newPrefixAccountParams, prefix[0] == '+' ? &prefix[1] : prefix);
			linphone_account_set_params(acc, newPrefixAccountParams);
			linphone_account_params_unref(newPrefixAccountParams);
		}
		[PhoneMainView.instance popToView:DialerView.compositeViewDescription];
		[[NSNotificationCenter defaultCenter] postNotificationName:kLinphoneAddressBookUpdate object:NULL];
		[LinphoneManager.instance.fastAddressBook fetchContactsInBackGroundThread];
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
	linphone_account_creator_link_account(account_creator);
}

- (IBAction)onCheckValidationButton:(id)sender {
	_waitView.hidden = NO;
	linphone_account_creator_set_activation_code(account_creator, _activationCodeField.text.UTF8String);
	linphone_account_creator_activate_alias(account_creator);
}

- (IBAction)onDialerClick:(id)sender {
	[PhoneMainView.instance popCurrentView];
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

- (IBAction)onMaybeLater:(id)sender {
	[PhoneMainView.instance popToView:DialerView.compositeViewDescription];
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
	if (status == LinphoneAccountCreatorPhoneNumberStatusTooLong ||
		status == LinphoneAccountCreatorPhoneNumberStatusTooShort) {
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
