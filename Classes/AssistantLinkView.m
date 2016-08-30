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

@implementation AssistantLinkView {
	LinphoneAccountCreator *account_creator;
}

- (void)viewDidLoad {
	[super viewDidLoad];
	[_activationCodeField addDoneButton];
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
		linphone_account_creator_set_password(account_creator, linphone_auth_info_get_passwd(info));
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

	if (country) {
		[self didSelectCountry:country];
	}
}

- (void)viewDidDisappear:(BOOL)animated {
	linphone_account_creator_unref(account_creator);
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

- (NSString *)stringForError:(const char *)err {
#define IS(x) (err && (strcmp(err, #x) == 0))
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
	return NSLocalizedString(@"This alias is already used.", nil);
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
	return NSLocalizedString(@"The confirmation code is invalid.", nil);
	if
		IS(ERROR_PASSWORD_DOESNT_MATCH)
	return NSLocalizedString(@"Passwords do not match.", nil);
	if
		IS(ERROR_PHONE_ISNT_E164)
	return NSLocalizedString(@"Your phone number is invalid.", nil);

	if (!linphone_core_is_network_reachable(LC))
		return NSLocalizedString(@"There is no network connection available, enable "
								 @"WIFI or WWAN prior to configure an account",
								 nil);

	if (err)
		LOGW(@"Unhandled error %s", err);
	return NSLocalizedString(@"Unknown error, please try again later", nil);
}

- (void)showErrorPopup:(const char *)err {
	UIAlertView *errorView = [[UIAlertView alloc] initWithTitle:NSLocalizedString(@"Account configuration issue", nil)
														message:[self stringForError:err]
													   delegate:nil
											  cancelButtonTitle:NSLocalizedString(@"Cancel", nil)
											  otherButtonTitles:nil, nil];
	[errorView show];
}

#pragma mark - cbs

void assistant_link_phone_number_with_account(LinphoneAccountCreator *creator, LinphoneAccountCreatorStatus status,
											  const char *resp) {
	AssistantLinkView *thiz = (__bridge AssistantLinkView *)(linphone_account_creator_get_user_data(creator));
	thiz.waitView.hidden = YES;
	if (status == LinphoneAccountCreatorOK) {
		thiz.linkAccountView.hidden = thiz.activateSMSView.userInteractionEnabled = YES;
		thiz.activateSMSView.hidden = thiz.linkAccountView.userInteractionEnabled = NO;
	} else {
		[thiz showErrorPopup:resp];
	}
}

void assistant_activate_phone_number_link(LinphoneAccountCreator *creator, LinphoneAccountCreatorStatus status,
										  const char *resp) {
	AssistantLinkView *thiz = (__bridge AssistantLinkView *)(linphone_account_creator_get_user_data(creator));
	thiz.waitView.hidden = YES;
	if (status == LinphoneAccountCreatorOK) {
		[PhoneMainView.instance popToView:DialerView.compositeViewDescription];
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
}

- (IBAction)onCountryCodeFieldChange:(id)sender {
	[self updateCountry:NO];
}

- (IBAction)onCountryCodeFieldEnd:(id)sender {
	[self updateCountry:YES];
}

- (IBAction)onCountryClick:(id)sender {
	CountryListView *view = VIEW(CountryListView);
	[view setDelegate:(id)self];
	[PhoneMainView.instance changeCurrentView:view.compositeViewDescription];
}

- (IBAction)onLinkAccount:(id)sender {
	_waitView.hidden = NO;
	linphone_account_creator_set_phone_number(account_creator, _phoneField.text.UTF8String,
											  _countryCodeField.text.UTF8String);
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

#pragma mark - select country delegate

- (void)didSelectCountry:(NSDictionary *)country {
	[_countryButton setTitle:[country objectForKey:@"name"] forState:UIControlStateNormal];
	_countryCodeField.text = [country objectForKey:@"code"];
}

@end
