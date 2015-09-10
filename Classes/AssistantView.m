
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

#import "AssistantView.h"
#import "LinphoneManager.h"
#import "PhoneMainView.h"
#import "UITextField+DoneButton.h"

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
	ViewElement_Transport = 105,
	ViewElement_Label = 200,
	ViewElement_Error = 201,
	ViewElement_Username_Error = 404,
} ViewElement;

@implementation AssistantView

#pragma mark - Lifecycle Functions

- (id)init {
	self = [super initWithNibName:NSStringFromClass(self.class) bundle:[NSBundle mainBundle]];
	if (self != nil) {
		[[NSBundle mainBundle] loadNibNamed:@"AssistantSubviews" owner:self options:nil];
		historyViews = [[NSMutableArray alloc] init];
		currentView = nil;
	}
	return self;
}

- (void)dealloc {
	[[NSNotificationCenter defaultCenter] removeObserver:self];
}

#pragma mark - UICompositeViewDelegate Functions

static UICompositeViewDescription *compositeDescription = nil;

+ (UICompositeViewDescription *)compositeViewDescription {
	if (compositeDescription == nil) {
		compositeDescription = [[UICompositeViewDescription alloc] init:self.class
															   stateBar:StatusBarView.class
																 tabBar:nil
															 fullscreen:false
														  landscapeMode:LinphoneManager.runningOnIpad
														   portraitMode:true];
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
	[[NSNotificationCenter defaultCenter] addObserver:self
											 selector:@selector(registrationUpdateEvent:)
												 name:kLinphoneRegistrationUpdate
											   object:nil];
	[[NSNotificationCenter defaultCenter] addObserver:self
											 selector:@selector(configuringUpdate:)
												 name:kLinphoneConfiguringStateUpdate
											   object:nil];

	[self changeView:_welcomeView back:FALSE animation:FALSE];
}

- (void)viewWillDisappear:(BOOL)animated {
	[super viewWillDisappear:animated];
	[[NSNotificationCenter defaultCenter] removeObserver:self];
}

- (void)viewDidLoad {
	[super viewDidLoad];
	if (LinphoneManager.runningOnIpad) {
		[LinphoneUtils adjustFontSize:_welcomeView mult:2.22f];
		[LinphoneUtils adjustFontSize:_createAccountView mult:2.22f];
		[LinphoneUtils adjustFontSize:_linphoneLoginView mult:2.22f];
		[LinphoneUtils adjustFontSize:_loginView mult:2.22f];
		[LinphoneUtils adjustFontSize:_createAccountActivationView mult:2.22f];
		[LinphoneUtils adjustFontSize:_remoteProvisionningView mult:2.22f];
	}
}

- (void)displayUsernameAsPhoneOrUsername {
	BOOL usePhoneNumber = [LinphoneManager.instance lpConfigBoolForKey:@"use_phone_number"];

	NSString *label = usePhoneNumber ? NSLocalizedString(@"PHONE NUMBER", nil) : NSLocalizedString(@"USERNAME", nil);
	[self findLabel:ViewElement_Username].text = label;

	UITextField *text = [self findTextField:ViewElement_Username];
	if (usePhoneNumber) {
		text.keyboardType = UIKeyboardTypePhonePad;
		[text addDoneButton];
	} else {
		text.keyboardType = UIKeyboardTypeDefault;
	}
}

#pragma mark -

+ (void)cleanTextField:(UIView *)view {
	if ([view isKindOfClass:[UITextField class]]) {
		[(UITextField *)view setText:@""];
	} else {
		for (UIView *subview in view.subviews) {
			[AssistantView cleanTextField:subview];
		}
	}
}

- (void)fillDefaultValues {

	LinphoneCore *lc = [LinphoneManager getLc];
	[self resetTextFields];

	LinphoneProxyConfig *current_conf = NULL;
	linphone_core_get_default_proxy([LinphoneManager getLc], &current_conf);
	if (current_conf != NULL) {
		const char *proxy_addr = linphone_proxy_config_get_identity(current_conf);
		if (proxy_addr) {
			LinphoneAddress *addr = linphone_address_new(proxy_addr);
			if (addr) {
				const LinphoneAuthInfo *auth = linphone_core_find_auth_info(
					lc, NULL, linphone_address_get_username(addr), linphone_proxy_config_get_domain(current_conf));
				linphone_address_destroy(addr);
				if (auth) {
					LOGI(@"A proxy config was set up with the remote provisioning, skip assistant");
					[self onDialerBackClick:nil];
				}
			}
		}
	}

	LinphoneProxyConfig *default_conf = linphone_core_create_proxy_config([LinphoneManager getLc]);
	const char *identity = linphone_proxy_config_get_identity(default_conf);
	if (identity) {
		LinphoneAddress *default_addr = linphone_address_new(identity);
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

	[self changeView:_remoteProvisionningView back:FALSE animation:TRUE];

	linphone_proxy_config_destroy(default_conf);
}

- (void)resetTextFields {
	[AssistantView cleanTextField:_welcomeView];
	[AssistantView cleanTextField:_createAccountView];
	[AssistantView cleanTextField:_linphoneLoginView];
	[AssistantView cleanTextField:_loginView];
	[AssistantView cleanTextField:_createAccountActivationView];
	[AssistantView cleanTextField:_remoteProvisionningView];
}

- (void)reset {
	[[LinphoneManager instance] removeAllAccounts];
	[[LinphoneManager instance] lpConfigSetBool:FALSE forKey:@"pushnotification_preference"];

	LinphoneCore *lc = [LinphoneManager getLc];
	LCSipTransports transportValue = {5060, 5060, -1, -1};

	if (linphone_core_set_sip_transports(lc, &transportValue)) {
		LOGE(@"cannot set transport");
	}

	[[LinphoneManager instance] lpConfigSetString:@"" forKey:@"sharing_server_preference"];
	[[LinphoneManager instance] lpConfigSetBool:FALSE forKey:@"ice_preference"];
	[[LinphoneManager instance] lpConfigSetString:@"" forKey:@"stun_preference"];
	linphone_core_set_stun_server(lc, NULL);
	linphone_core_set_firewall_policy(lc, LinphonePolicyNoFirewall);
	[self resetTextFields];
	[self changeView:_welcomeView back:FALSE animation:FALSE];
	_waitView.hidden = TRUE;
}

- (UIView *)findView:(ViewElement)tag inView:view ofType:(Class)type {
	for (UIView *child in [view subviews]) {
		if (child.tag == tag) {
			return child;
		} else {
			UIView *o = [self findView:tag inView:child ofType:type];
			if (o)
				return o;
		}
	}
	return nil;
}

- (UITextField *)findTextField:(ViewElement)tag {
	return (UITextField *)[self findView:tag inView:self.contentView ofType:[UITextField class]];
}

- (UILabel *)findLabel:(ViewElement)tag {
	return (UILabel *)[self findView:tag inView:self.contentView ofType:[UILabel class]];
}

- (void)clearHistory {
	[historyViews removeAllObjects];
}

- (void)changeView:(UIView *)view back:(BOOL)back animation:(BOOL)animation {

	static BOOL placement_done = NO; // indicates if the button placement has been done in the assistant choice view

	// Change toolbar buttons following view
	_backButton.enabled = (view == _welcomeView);

	[self displayUsernameAsPhoneOrUsername];

	if (view == _welcomeView) {
		// layout is this:
		// [ Logo         ]
		// [ Create Btn   ]
		// [ Connect Btn  ]
		// [ External Btn ]
		// [ Remote Prov  ]

		BOOL show_logo =
			[[LinphoneManager instance] lpConfigBoolForKey:@"show_assistant_logo_in_choice_view_preference"];
		BOOL show_extern = ![[LinphoneManager instance] lpConfigBoolForKey:@"hide_assistant_custom_account"];
		BOOL show_new = ![[LinphoneManager instance] lpConfigBoolForKey:@"hide_assistant_create_account"];

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
	if (animation && [[LinphoneManager instance] lpConfigBoolForKey:@"animations_preference"] == true) {
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
	[_contentView insertSubview:view atIndex:0];
	[view setFrame:[_contentView bounds]];
	[_contentView setContentSize:[view bounds].size];
}

- (BOOL)addProxyConfig:(NSString *)username
			  password:(NSString *)password
				domain:(NSString *)domain
		 withTransport:(NSString *)transport {
	LinphoneCore *lc = [LinphoneManager getLc];
	LinphoneProxyConfig *proxyCfg = linphone_core_create_proxy_config(lc);
	NSString *server_address = domain;

	char normalizedUserName[256];
	linphone_proxy_config_normalize_number(proxyCfg, [username cStringUsingEncoding:[NSString defaultCStringEncoding]],
										   normalizedUserName, sizeof(normalizedUserName));

	const char *identity = linphone_proxy_config_get_identity(proxyCfg);
	if (!identity || !*identity)
		identity = "sip:user@example.com";

	LinphoneAddress *linphoneAddress = linphone_address_new(identity);
	linphone_address_set_username(linphoneAddress, normalizedUserName);

	if (domain && [domain length] != 0) {
		if (transport != nil) {
			server_address =
				[NSString stringWithFormat:@"%@;transport=%@", server_address, [transport lowercaseString]];
		}
		// when the domain is specified (for external login), take it as the server address
		linphone_proxy_config_set_server_addr(proxyCfg, [server_address UTF8String]);
		linphone_address_set_domain(linphoneAddress, [domain UTF8String]);
	}

	char *extractedAddres = linphone_address_as_string_uri_only(linphoneAddress);

	LinphoneAddress *parsedAddress = linphone_address_new(extractedAddres);
	ms_free(extractedAddres);

	if (parsedAddress == NULL || !linphone_address_is_sip(parsedAddress)) {
		if (parsedAddress)
			linphone_address_destroy(parsedAddress);
		UIAlertView *errorView =
			[[UIAlertView alloc] initWithTitle:NSLocalizedString(@"Check error(s)", nil)
									   message:NSLocalizedString(@"Please enter a valid username", nil)
									  delegate:nil
							 cancelButtonTitle:NSLocalizedString(@"Continue", nil)
							 otherButtonTitles:nil, nil];
		[errorView show];
		return FALSE;
	}

	char *c_parsedAddress = linphone_address_as_string_uri_only(parsedAddress);

	linphone_proxy_config_set_identity(proxyCfg, c_parsedAddress);

	linphone_address_destroy(parsedAddress);
	ms_free(c_parsedAddress);

	LinphoneAuthInfo *info = linphone_auth_info_new([username UTF8String], NULL, [password UTF8String], NULL, NULL,
													linphone_proxy_config_get_domain(proxyCfg));

	LinphoneManager *lm = [LinphoneManager instance];
	[lm configurePushTokenForProxyConfig:proxyCfg];

	linphone_proxy_config_enable_register(proxyCfg, true);
	linphone_core_add_auth_info(lc, info);
	linphone_core_add_proxy_config(lc, proxyCfg);
	linphone_core_set_default_proxy_config(lc, proxyCfg);
	// reload address book to prepend proxy config domain to contacts' phone number
	[[[LinphoneManager instance] fastAddressBook] reload];
	return TRUE;
}

- (void)addProvisionedProxy:(NSString *)username withPassword:(NSString *)password withDomain:(NSString *)domain {
	[[LinphoneManager instance] removeAllAccounts];
	LinphoneProxyConfig *proxyCfg = linphone_core_create_proxy_config([LinphoneManager getLc]);

	const char *addr = linphone_proxy_config_get_domain(proxyCfg);
	char normalizedUsername[256];
	LinphoneAddress *linphoneAddress = linphone_address_new(addr);

	linphone_proxy_config_normalize_number(proxyCfg, [username cStringUsingEncoding:[NSString defaultCStringEncoding]],
										   normalizedUsername, sizeof(normalizedUsername));

	linphone_address_set_username(linphoneAddress, normalizedUsername);
	linphone_address_set_domain(linphoneAddress, [domain UTF8String]);

	const char *identity = linphone_address_as_string_uri_only(linphoneAddress);
	linphone_proxy_config_set_identity(proxyCfg, identity);

	LinphoneAuthInfo *info =
		linphone_auth_info_new([username UTF8String], NULL, [password UTF8String], NULL, NULL, [domain UTF8String]);

	linphone_proxy_config_enable_register(proxyCfg, true);
	linphone_core_add_auth_info([LinphoneManager getLc], info);
	linphone_core_add_proxy_config([LinphoneManager getLc], proxyCfg);
	linphone_core_set_default_proxy_config([LinphoneManager getLc], proxyCfg);
	// reload address book to prepend proxy config domain to contacts' phone number
	[[[LinphoneManager instance] fastAddressBook] reload];
}

- (NSString *)identityFromUsername:(NSString *)username {
	char normalizedUserName[256];
	LinphoneAddress *linphoneAddress = linphone_address_new("sip:user@domain.com");
	linphone_proxy_config_normalize_number(NULL, [username cStringUsingEncoding:[NSString defaultCStringEncoding]],
										   normalizedUserName, sizeof(normalizedUserName));
	linphone_address_set_username(linphoneAddress, normalizedUserName);
	linphone_address_set_domain(
		linphoneAddress,
		[[[LinphoneManager instance] lpConfigStringForKey:@"domain" forSection:@"assistant"] UTF8String]);
	NSString *uri = [NSString stringWithUTF8String:linphone_address_as_string_uri_only(linphoneAddress)];
	NSString *scheme = [NSString stringWithUTF8String:linphone_address_get_scheme(linphoneAddress)];
	return [uri substringFromIndex:[scheme length] + 1];
}

#pragma mark - Linphone XMLRPC

- (void)checkUserExist:(NSString *)username {
	LOGI(@"XMLRPC check_account %@", username);

	NSURL *URL =
		[NSURL URLWithString:[[LinphoneManager instance] lpConfigStringForKey:@"service_url" forSection:@"assistant"]];
	XMLRPCRequest *request = [[XMLRPCRequest alloc] initWithURL:URL];
	[request setMethod:@"check_account" withParameters:[NSArray arrayWithObjects:username, nil]];

	XMLRPCConnectionManager *manager = [XMLRPCConnectionManager sharedManager];
	[manager spawnConnectionWithXMLRPCRequest:request delegate:self];

	_waitView.hidden = false;
}

- (void)createAccount:(NSString *)identity password:(NSString *)password email:(NSString *)email {
	NSString *useragent = [LinphoneManager getUserAgent];
	LOGI(@"XMLRPC create_account_with_useragent %@ %@ %@ %@", identity, password, email, useragent);

	NSURL *URL =
		[NSURL URLWithString:[[LinphoneManager instance] lpConfigStringForKey:@"service_url" forSection:@"assistant"]];
	XMLRPCRequest *request = [[XMLRPCRequest alloc] initWithURL:URL];
	[request setMethod:@"create_account_with_useragent"
		withParameters:[NSArray arrayWithObjects:identity, password, email, useragent, nil]];

	XMLRPCConnectionManager *manager = [XMLRPCConnectionManager sharedManager];
	[manager spawnConnectionWithXMLRPCRequest:request delegate:self];

	_waitView.hidden = false;
}

- (void)checkAccountValidation:(NSString *)identity {
	LOGI(@"XMLRPC check_account_validated %@", identity);

	NSURL *URL =
		[NSURL URLWithString:[[LinphoneManager instance] lpConfigStringForKey:@"service_url" forSection:@"assistant"]];
	XMLRPCRequest *request = [[XMLRPCRequest alloc] initWithURL:URL];
	[request setMethod:@"check_account_validated" withParameters:[NSArray arrayWithObjects:identity, nil]];

	XMLRPCConnectionManager *manager = [XMLRPCConnectionManager sharedManager];
	[manager spawnConnectionWithXMLRPCRequest:request delegate:self];

	_waitView.hidden = false;
}

#pragma mark -

- (void)registrationUpdate:(LinphoneRegistrationState)state
				  forProxy:(LinphoneProxyConfig *)proxy
				   message:(NSString *)message {
	// in assistant we only care about ourself
	if (proxy != linphone_core_get_default_proxy_config([LinphoneManager getLc])) {
		return;
	}

	switch (state) {
		case LinphoneRegistrationOk: {
			_waitView.hidden = true;
			[PhoneMainView.instance changeCurrentView:DialerView.compositeViewDescription];
			break;
		}
		case LinphoneRegistrationNone:
		case LinphoneRegistrationCleared: {
			_waitView.hidden = true;
			break;
		}
		case LinphoneRegistrationFailed: {
			_waitView.hidden = true;
			if ([message isEqualToString:@"Forbidden"]) {
				message = NSLocalizedString(@"Incorrect username or password.", nil);
			}
			UIAlertView *alert = [[UIAlertView alloc] initWithTitle:NSLocalizedString(@"Registration failure", nil)
															message:message
														   delegate:nil
												  cancelButtonTitle:@"OK"
												  otherButtonTitles:nil];
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

- (void)loadAssistantConfig:(NSString *)rcFilename {
	NSString *fullPath = [@"file://" stringByAppendingString:[LinphoneManager bundleFile:rcFilename]];
	linphone_core_set_provisioning_uri([LinphoneManager getLc],
									   [fullPath cStringUsingEncoding:[NSString defaultCStringEncoding]]);
	[[LinphoneManager instance] lpConfigSetInt:1 forKey:@"transient_provisioning" forSection:@"misc"];

	// For some reason, video preview hangs for 15seconds when resetting linphone core from here...
	// to avoid it, we disable it before and reenable it after core restart.
	BOOL hasPreview = linphone_core_video_preview_enabled([LinphoneManager getLc]);
	linphone_core_enable_video_preview([LinphoneManager getLc], FALSE);
	[[LinphoneManager instance] resetLinphoneCore];
	linphone_core_enable_video_preview([LinphoneManager getLc], hasPreview);
	// we will set the new default proxy config in the assistant
	linphone_core_set_default_proxy_config([LinphoneManager getLc], NULL);
}

#pragma mark - UITextFieldDelegate Functions

- (BOOL)textFieldShouldReturn:(UITextField *)textField {
	[textField resignFirstResponder];
	return YES;
}

- (void)textFieldDidBeginEditing:(UITextField *)textField {
	activeTextField = textField;
}

- (BOOL)textField:(UITextField *)textField
	shouldChangeCharactersInRange:(NSRange)range
				replacementString:(NSString *)string {
	// only validate the username when creating a new account
	if ((textField.tag == ViewElement_Username) && (currentView == _createAccountView)) {
		BOOL isValidUsername = YES;
		BOOL usePhoneNumber = [[LinphoneManager instance] lpConfigBoolForKey:@"use_phone_number"];
		if (usePhoneNumber) {
			isValidUsername = linphone_proxy_config_is_phone_number(NULL, [string UTF8String]);
		} else {
			NSRegularExpression *regex =
				[NSRegularExpression regularExpressionWithPattern:@"^[a-z0-9-_\\.]*$"
														  options:NSRegularExpressionCaseInsensitive
															error:nil];

			NSArray *matches = [regex matchesInString:string options:0 range:NSMakeRange(0, [string length])];
			isValidUsername = ([matches count] != 0);
		}

		if (!isValidUsername) {
			UILabel *error = [self findLabel:ViewElement_Username_Error];

			// show error with fade animation
			[error setText:[NSString stringWithFormat:NSLocalizedString(@"Illegal character in %@: %@", nil),
													  usePhoneNumber ? NSLocalizedString(@"phone number", nil)
																	 : NSLocalizedString(@"username", nil),
													  string]];
			error.alpha = 0;
			error.hidden = NO;
			[UIView animateWithDuration:0.3
							 animations:^{
							   error.alpha = 1;
							 }];

			// hide again in 2s
			[NSTimer scheduledTimerWithTimeInterval:2.0f
											 target:self
										   selector:@selector(hideError:)
										   userInfo:nil
											repeats:NO];
			return NO;
		}
	}
	return YES;
}
- (void)hideError:(NSTimer *)timer {
	UILabel *error_label = [self findLabel:ViewElement_Username_Error];
	if (error_label) {
		[UIView animateWithDuration:0.3
			animations:^{
			  error_label.alpha = 0;
			}
			completion:^(BOOL finished) {
			  error_label.hidden = YES;
			}];
	}
}

#pragma mark - Action Functions

- (IBAction)onBackClick:(id)sender {
	if ([historyViews count] > 0) {
		UIView *view = [historyViews lastObject];
		[historyViews removeLastObject];
		[self changeView:view back:TRUE animation:TRUE];
	}
}

- (IBAction)onDialerBackClick:(id)sender {
	[PhoneMainView.instance changeCurrentView:DialerView.compositeViewDescription];
}

- (IBAction)onGotoCreateAccountClick:(id)sender {
	nextView = _createAccountView;
	[self loadAssistantConfig:@"assistant_linphone_create.rc"];
}

- (IBAction)onGotoLinphoneLoginClick:(id)sender {
	nextView = _linphoneLoginView;
	[self loadAssistantConfig:@"assistant_linphone_existing.rc"];
}

- (IBAction)onGotoLoginClick:(id)sender {
	nextView = _loginView;
	[self loadAssistantConfig:@"assistant_external_sip.rc"];
}

- (IBAction)onCreateAccountActivationClick:(id)sender {
	NSString *username = [self findTextField:ViewElement_Username].text;
	NSString *identity = [self identityFromUsername:username];
	[self checkAccountValidation:identity];
}

- (IBAction)onGotoRemoteProvisionningClick:(id)sender {
	UIAlertView *remoteInput = [[UIAlertView alloc] initWithTitle:NSLocalizedString(@"Enter provisioning URL", @"")
														  message:@""
														 delegate:self
												cancelButtonTitle:NSLocalizedString(@"Cancel", @"")
												otherButtonTitles:NSLocalizedString(@"Fetch", @""), nil];
	remoteInput.alertViewStyle = UIAlertViewStylePlainTextInput;

	UITextField *prov_url = [remoteInput textFieldAtIndex:0];
	prov_url.keyboardType = UIKeyboardTypeURL;
	prov_url.text = [[LinphoneManager instance] lpConfigStringForKey:@"config-uri" forSection:@"misc"];
	prov_url.placeholder = @"URL";

	[remoteInput show];
}

- (BOOL)verificationWithUsername:(NSString *)username
						password:(NSString *)password
						  domain:(NSString *)domain
				   withTransport:(NSString *)transport {
	NSMutableString *errors = [NSMutableString string];
	if ([username length] == 0) {
		[errors appendString:[NSString stringWithFormat:NSLocalizedString(@"Please enter a valid username.\n", nil)]];
	}

	if (domain != nil && [domain length] == 0) {
		[errors appendString:[NSString stringWithFormat:NSLocalizedString(@"Please enter a valid domain.\n", nil)]];
	}

	if ([errors length]) {
		UIAlertView *errorView =
			[[UIAlertView alloc] initWithTitle:NSLocalizedString(@"Check error(s)", nil)
									   message:[errors substringWithRange:NSMakeRange(0, [errors length] - 1)]
									  delegate:nil
							 cancelButtonTitle:NSLocalizedString(@"Continue", nil)
							 otherButtonTitles:nil, nil];
		[errorView show];
		return FALSE;
	}
	return TRUE;
}
- (void)verificationSignInWithUsername:(NSString *)username
							  password:(NSString *)password
								domain:(NSString *)domain
						 withTransport:(NSString *)transport {
	if ([self verificationWithUsername:username password:password domain:domain withTransport:transport]) {
		_waitView.hidden = false;
		if ([LinphoneManager instance].connectivity == none) {
			DTAlertView *alert = [[DTAlertView alloc]
				initWithTitle:NSLocalizedString(@"No connectivity", nil)
					  message:NSLocalizedString(@"You can either skip verification or connect to the Internet first.",
												nil)];
			[alert addCancelButtonWithTitle:NSLocalizedString(@"Stay here", nil)
									  block:^{
										_waitView.hidden = true;
									  }];
			[alert
				addButtonWithTitle:NSLocalizedString(@"Continue", nil)
							 block:^{
							   _waitView.hidden = true;
							   [self addProxyConfig:username password:password domain:domain withTransport:transport];
							   [PhoneMainView.instance changeCurrentView:DialerView.compositeViewDescription];
							 }];
			[alert show];
		} else {
			BOOL success = [self addProxyConfig:username password:password domain:domain withTransport:transport];
			if (!success) {
				_waitView.hidden = true;
			}
		}
	}
}

- (IBAction)onLoginClick:(id)sender {
	NSString *username = [self findTextField:ViewElement_Username].text;
	NSString *password = [self findTextField:ViewElement_Password].text;
	NSString *domain = [self findTextField:ViewElement_Domain].text;
	UISegmentedControl *transportChooser =
		(UISegmentedControl *)[self findView:ViewElement_Transport inView:_contentView ofType:UISegmentedControl.class];
	NSString *transport = [transportChooser titleForSegmentAtIndex:transportChooser.selectedSegmentIndex];
	[self verificationSignInWithUsername:username password:password domain:domain withTransport:transport];
}

- (IBAction)onLinphoneLoginClick:(id)sender {
	NSString *username = [self findTextField:ViewElement_Username].text;
	NSString *password = [self findTextField:ViewElement_Password].text;

	// domain and server will be configured from the default proxy values
	[self verificationSignInWithUsername:username password:password domain:nil withTransport:nil];
}

- (BOOL)verificationRegisterWithUsername:(NSString *)username
								password:(NSString *)password
							   password2:(NSString *)password2
								   email:(NSString *)email {
	NSMutableString *errors = [NSMutableString string];
	NSInteger username_length =
		[[LinphoneManager instance] lpConfigIntForKey:@"username_length" forSection:@"assistant"];
	NSInteger password_length =
		[[LinphoneManager instance] lpConfigIntForKey:@"password_length" forSection:@"assistant"];

	if ([username length] < username_length) {
		[errors
			appendString:[NSString stringWithFormat:NSLocalizedString(
														@"The username is too short (minimum %d characters).\n", nil),
													username_length]];
	}

	if ([password length] < password_length) {
		[errors
			appendString:[NSString stringWithFormat:NSLocalizedString(
														@"The password is too short (minimum %d characters).\n", nil),
													password_length]];
	}

	if (![password2 isEqualToString:password]) {
		[errors appendString:NSLocalizedString(@"The passwords are different.\n", nil)];
	}

	NSPredicate *emailTest = [NSPredicate predicateWithFormat:@"SELF MATCHES %@", @".+@.+\\.[A-Za-z]{2}[A-Za-z]*"];
	if (![emailTest evaluateWithObject:email]) {
		[errors appendString:NSLocalizedString(@"The email is invalid.\n", nil)];
	}

	if ([errors length]) {
		UIAlertView *errorView =
			[[UIAlertView alloc] initWithTitle:NSLocalizedString(@"Check error(s)", nil)
									   message:[errors substringWithRange:NSMakeRange(0, [errors length] - 1)]
									  delegate:nil
							 cancelButtonTitle:NSLocalizedString(@"Continue", nil)
							 otherButtonTitles:nil, nil];
		[errorView show];
		return FALSE;
	}

	return TRUE;
}

- (IBAction)onCreateAccountClick:(id)sender {
	UITextField *username_tf = [self findTextField:ViewElement_Username];
	NSString *username = username_tf.text;
	NSString *password = [self findTextField:ViewElement_Password].text;
	NSString *password2 = [self findTextField:ViewElement_Password2].text;
	NSString *email = [self findTextField:ViewElement_Email].text;

	if ([self verificationRegisterWithUsername:username password:password password2:password2 email:email]) {
		username = [username lowercaseString];
		[username_tf setText:username];
		NSString *identity = [self identityFromUsername:username];
		[self checkUserExist:identity];
	}
}

- (IBAction)onRemoteProvisionningClick:(id)sender {
	NSString *username = [self findTextField:ViewElement_Username].text;
	NSString *password = [self findTextField:ViewElement_Password].text;
	NSString *domain = [self findTextField:ViewElement_Domain].text;

	NSMutableString *errors = [NSMutableString string];
	if ([username length] == 0) {

		[errors appendString:[NSString stringWithFormat:NSLocalizedString(@"Please enter a valid username.\n", nil)]];
	}

	if ([errors length]) {
		UIAlertView *errorView =
			[[UIAlertView alloc] initWithTitle:NSLocalizedString(@"Check error(s)", nil)
									   message:[errors substringWithRange:NSMakeRange(0, [errors length] - 1)]
									  delegate:nil
							 cancelButtonTitle:NSLocalizedString(@"Continue", nil)
							 otherButtonTitles:nil, nil];
		[errorView show];
	} else {
		_waitView.hidden = false;
		[self addProvisionedProxy:username withPassword:password withDomain:domain];
	}
}

- (void)didRotateFromInterfaceOrientation:(UIInterfaceOrientation)fromInterfaceOrientation {
	[_contentView contentSizeToFit];
}

#pragma mark - UIAlertViewDelegate

- (void)alertView:(UIAlertView *)alertView clickedButtonAtIndex:(NSInteger)buttonIndex {
	if (buttonIndex == 1) { /* fetch */
		NSString *url = [alertView textFieldAtIndex:0].text;
		if ([url length] > 0) {
			// missing prefix will result in http:// being used
			if ([url rangeOfString:@"://"].location == NSNotFound)
				url = [NSString stringWithFormat:@"http://%@", url];

			LOGI(@"Should use remote provisioning URL %@", url);
			linphone_core_set_provisioning_uri([LinphoneManager getLc], [url UTF8String]);

			_waitView.hidden = false;
			[[LinphoneManager instance] resetLinphoneCore];
		}
	} else {
		LOGI(@"Canceled remote provisioning");
	}
}

- (void)configuringUpdate:(NSNotification *)notif {
	LinphoneConfiguringState status = (LinphoneConfiguringState)[[notif.userInfo valueForKey:@"state"] integerValue];

	_waitView.hidden = true;

	switch (status) {
		case LinphoneConfiguringSuccessful:
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

#pragma mark - Event Functions

- (void)registrationUpdateEvent:(NSNotification *)notif {
	NSString *message = [notif.userInfo objectForKey:@"message"];
	[self registrationUpdate:[[notif.userInfo objectForKey:@"state"] intValue]
					forProxy:[[notif.userInfo objectForKeyedSubscript:@"cfg"] pointerValue]
					 message:message];
}

#pragma mark - XMLRPCConnectionDelegate Functions

- (void)request:(XMLRPCRequest *)request didReceiveResponse:(XMLRPCResponse *)response {
	LOGI(@"XMLRPC %@: %@", [request method], [response body]);
	_waitView.hidden = true;
	if ([response isFault]) {
		NSString *errorString =
			[NSString stringWithFormat:NSLocalizedString(@"Communication issue (%@)", nil), [response faultString]];
		UIAlertView *errorView = [[UIAlertView alloc] initWithTitle:NSLocalizedString(@"Communication issue", nil)
															message:errorString
														   delegate:nil
												  cancelButtonTitle:NSLocalizedString(@"Continue", nil)
												  otherButtonTitles:nil, nil];
		[errorView show];
	} else if ([response object] != nil) { // Don't handle if not object: HTTP/Communication Error
		if ([[request method] isEqualToString:@"check_account"]) {
			if ([response.object isEqualToNumber:[NSNumber numberWithInt:1]]) {
				UIAlertView *errorView =
					[[UIAlertView alloc] initWithTitle:NSLocalizedString(@"Check issue", nil)
											   message:NSLocalizedString(@"Username already exists", nil)
											  delegate:nil
									 cancelButtonTitle:NSLocalizedString(@"Continue", nil)
									 otherButtonTitles:nil, nil];
				[errorView show];
			} else {
				NSString *username = [self findTextField:ViewElement_Username].text;
				NSString *password = [self findTextField:ViewElement_Password].text;
				NSString *email = [self findTextField:ViewElement_Email].text;
				NSString *identity = [self identityFromUsername:username];
				[self createAccount:identity password:password email:email];
			}
		} else if ([[request method] isEqualToString:@"create_account_with_useragent"]) {
			if ([response.object isEqualToNumber:[NSNumber numberWithInt:0]]) {
				NSString *username = [self findTextField:ViewElement_Username].text;
				NSString *password = [self findTextField:ViewElement_Password].text;
				[self changeView:_createAccountActivationView back:FALSE animation:TRUE];
				[self findTextField:ViewElement_Username].text = username;
				[self findTextField:ViewElement_Password].text = password;
			} else {
				UIAlertView *errorView = [[UIAlertView alloc]
						initWithTitle:NSLocalizedString(@"Account creation issue", nil)
							  message:NSLocalizedString(@"Can't create the account. Please try again.", nil)
							 delegate:nil
					cancelButtonTitle:NSLocalizedString(@"Continue", nil)
					otherButtonTitles:nil, nil];
				[errorView show];
			}
		} else if ([[request method] isEqualToString:@"check_account_validated"]) {
			if ([response.object isEqualToNumber:[NSNumber numberWithInt:1]]) {
				NSString *username = [self findTextField:ViewElement_Username].text;
				NSString *password = [self findTextField:ViewElement_Password].text;
				[self addProxyConfig:username password:password domain:nil withTransport:nil];
			} else {
				UIAlertView *errorView =
					[[UIAlertView alloc] initWithTitle:NSLocalizedString(@"Account validation issue", nil)
											   message:NSLocalizedString(@"Your account is not validate yet.", nil)
											  delegate:nil
									 cancelButtonTitle:NSLocalizedString(@"Continue", nil)
									 otherButtonTitles:nil, nil];
				[errorView show];
			}
		}
	}
}

- (void)request:(XMLRPCRequest *)request didFailWithError:(NSError *)error {
	NSString *errorString =
		[NSString stringWithFormat:NSLocalizedString(@"Communication issue (%@)", nil), [error localizedDescription]];
	UIAlertView *errorView = [[UIAlertView alloc] initWithTitle:NSLocalizedString(@"Communication issue", nil)
														message:errorString
													   delegate:nil
											  cancelButtonTitle:NSLocalizedString(@"Continue", nil)
											  otherButtonTitles:nil, nil];
	[errorView show];
	_waitView.hidden = true;
}

- (BOOL)request:(XMLRPCRequest *)request canAuthenticateAgainstProtectionSpace:(NSURLProtectionSpace *)protectionSpace {
	return FALSE;
}

- (void)request:(XMLRPCRequest *)request didReceiveAuthenticationChallenge:(NSURLAuthenticationChallenge *)challenge {
}

- (void)request:(XMLRPCRequest *)request didCancelAuthenticationChallenge:(NSURLAuthenticationChallenge *)challenge {
}

#pragma mark - TPMultiLayoutViewController Functions

- (NSDictionary *)attributesForView:(UIView *)view {
	NSMutableDictionary *attributes = [NSMutableDictionary dictionary];
	[attributes setObject:[NSValue valueWithCGRect:view.frame] forKey:@"frame"];
	[attributes setObject:[NSValue valueWithCGRect:view.bounds] forKey:@"bounds"];
	if ([view isKindOfClass:[UIButton class]]) {
		UIButton *button = (UIButton *)view;
		[LinphoneUtils buttonMultiViewAddAttributes:attributes button:button];
	}
	[attributes setObject:[NSNumber numberWithInteger:view.autoresizingMask] forKey:@"autoresizingMask"];
	return attributes;
}

- (void)applyAttributes:(NSDictionary *)attributes toView:(UIView *)view {
	view.frame = [[attributes objectForKey:@"frame"] CGRectValue];
	view.bounds = [[attributes objectForKey:@"bounds"] CGRectValue];
	if ([view isKindOfClass:[UIButton class]]) {
		UIButton *button = (UIButton *)view;
		[LinphoneUtils buttonMultiViewApplyAttributes:attributes button:button];
	}
	view.autoresizingMask = [[attributes objectForKey:@"autoresizingMask"] integerValue];
}

@end
