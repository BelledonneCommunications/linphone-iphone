/* FirstLoginViewController.m
 *
 * Copyright (C) 2011  Belledonne Comunications, Grenoble, France
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

#import "LinphoneManager.h"
#import "FirstLoginView.h"
#import "LinphoneManager.h"
#import "PhoneMainView.h"
#import "Utils/XMLRPCHelper.h"

@implementation FirstLoginView

#pragma mark - UICompositeViewDelegate Functions

static UICompositeViewDescription *compositeDescription = nil;

+ (UICompositeViewDescription *)compositeViewDescription {
	if (compositeDescription == nil) {
		compositeDescription = [[UICompositeViewDescription alloc] init:self.class
															  statusBar:nil
																 tabBar:nil
															   sideMenu:nil
															 fullscreen:false
														 isLeftFragment:YES
														   fragmentWith:nil];
	}
	return compositeDescription;
}

- (UICompositeViewDescription *)compositeViewDescription {
	return self.class.compositeViewDescription;
}

#pragma mark - ViewController Functions

- (void)viewDidLoad {
	[super viewDidLoad];
	NSString *siteUrl =
		[[LinphoneManager instance] lpConfigStringForKey:@"first_login_view_url"] ?: @"http://www.linphone.org";
	[_siteButton setTitle:siteUrl forState:UIControlStateNormal];
}

- (void)viewWillAppear:(BOOL)animated {
	[super viewWillAppear:animated];
	// Set observer
	[[NSNotificationCenter defaultCenter] addObserver:self
											 selector:@selector(registrationUpdateEvent:)
												 name:kLinphoneRegistrationUpdate
											   object:nil];

	[[NSNotificationCenter defaultCenter] addObserver:self
											 selector:@selector(configureStateUpdateEvent:)
												 name:kLinphoneConfiguringStateUpdate
											   object:nil];

	// Update on show
	const MSList *list = linphone_core_get_proxy_config_list([LinphoneManager getLc]);
	if (list != NULL) {
		LinphoneProxyConfig *config = (LinphoneProxyConfig *)list->data;
		if (config) {
			[self registrationUpdate:linphone_proxy_config_get_state(config)];
		}
	}

	if (account_creator) {
		linphone_account_creator_unref(account_creator);
	}
	NSString *siteUrl =
		[[LinphoneManager instance] lpConfigStringForKey:@"first_login_view_url"] ?: @"http://www.linphone.org";
	account_creator = linphone_account_creator_new([LinphoneManager getLc], siteUrl.UTF8String);

	[_usernameField showError:[self.class errorForStatus:LinphoneAccountCreatorUsernameInvalid]
						 when:^BOOL(NSString *inputEntry) {
						   LinphoneAccountCreatorStatus s =
							   linphone_account_creator_set_username(account_creator, inputEntry.UTF8String);
						   _usernameField.errorLabel.text = [self.class errorForStatus:s];
						   return s != LinphoneAccountCreatorOK;
						 }];

	[_passwordField showError:[self.class errorForStatus:LinphoneAccountCreatorPasswordTooShort]
						 when:^BOOL(NSString *inputEntry) {
						   LinphoneAccountCreatorStatus s =
							   linphone_account_creator_set_password(account_creator, inputEntry.UTF8String);
						   _passwordField.errorLabel.text = [self.class errorForStatus:s];
						   return s != LinphoneAccountCreatorOK;
						 }];

	[_domainField showError:[self.class errorForStatus:LinphoneAccountCreatorDomainInvalid]
					   when:^BOOL(NSString *inputEntry) {
						 LinphoneAccountCreatorStatus s =
							 linphone_account_creator_set_domain(account_creator, inputEntry.UTF8String);
						 _domainField.errorLabel.text = [self.class errorForStatus:s];
						 return s != LinphoneAccountCreatorOK;
					   }];
}

- (void)viewWillDisappear:(BOOL)animated {
	[super viewWillDisappear:animated];

	// Remove observer
	[[NSNotificationCenter defaultCenter] removeObserver:self name:kLinphoneRegistrationUpdate object:nil];
}

+ (NSString *)errorForStatus:(LinphoneAccountCreatorStatus)status {
	BOOL usePhoneNumber = [[LinphoneManager instance] lpConfigBoolForKey:@"use_phone_number" inSection:@"assistant"];
	switch (status) {
		case LinphoneAccountCreatorCountryCodeInvalid:
			return NSLocalizedString(@"Invalid country code.", nil);
		case LinphoneAccountCreatorEmailInvalid:
			return NSLocalizedString(@"Invalid email.", nil);
		case LinphoneAccountCreatorUsernameInvalid:
			return usePhoneNumber ? NSLocalizedString(@"Invalid phone number.", nil)
								  : NSLocalizedString(@"Invalid username.", nil);
		case LinphoneAccountCreatorUsernameTooShort:
			return usePhoneNumber ? NSLocalizedString(@"Phone number too short.", nil)
								  : NSLocalizedString(@"Username too short.", nil);
		case LinphoneAccountCreatorUsernameTooLong:
			return usePhoneNumber ? NSLocalizedString(@"Phone number too long.", nil)
								  : NSLocalizedString(@"Username too long.", nil);
		case LinphoneAccountCreatorUsernameInvalidSize:
			return usePhoneNumber ? NSLocalizedString(@"Phone number length invalid.", nil)
								  : NSLocalizedString(@"Username length invalid.", nil);
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
		case LinphoneAccountCreatorErrorServer:
			return NSLocalizedString(@"Server error", nil);
		case LinphoneAccountCreatorAccountCreated:
		case LinphoneAccountCreatorAccountExist:
		case LinphoneAccountCreatorAccountExistWithAlias:
		case LinphoneAccountCreatorAccountNotCreated:
		case LinphoneAccountCreatorAccountNotExist:
		case LinphoneAccountCreatorAccountNotActivated:
		case LinphoneAccountCreatorAccountAlreadyActivated:
		case LinphoneAccountCreatorAccountActivated:
		case LinphoneAccountCreatorAccountLinked:
		case LinphoneAccountCreatorAccountNotLinked:
		case LinphoneAccountCreatorPhoneNumberNotUsed:
		case LinphoneAccountCreatorPhoneNumberUsedAlias:
		case LinphoneAccountCreatorPhoneNumberUsedAccount:
		case LinphoneAccountCreatorOK:
			break;
	}
	return nil;
}

- (void)shouldEnableNextButton {
	BOOL invalidInputs = NO;
	for (UIAssistantTextField *field in @[ _usernameField, _passwordField, _domainField ]) {
		invalidInputs |= (field.isInvalid || field.lastText.length == 0);
	}
	_loginButton.enabled = !invalidInputs;
}

#pragma mark - Event Functions

- (void)configureStateUpdateEvent:(NSNotification *)notif {
	LinphoneConfiguringState state = [[notif.userInfo objectForKey:@"state"] intValue];
	switch (state) {
		case LinphoneConfiguringFailed: {
			[_waitView setHidden:true];
			UIAlertController *errView = [UIAlertController alertControllerWithTitle:NSLocalizedString(@"Configuration failed", nil)
																			 message:NSLocalizedString(@"Cannot retrieve your configuration. Please check credentials or try again later", nil)
																	  preferredStyle:UIAlertControllerStyleAlert];
			
			UIAlertAction* defaultAction = [UIAlertAction actionWithTitle:NSLocalizedString(@"OK", nil)
																	style:UIAlertActionStyleDefault
																  handler:^(UIAlertAction * action) {}];
			
			[errView addAction:defaultAction];
			[self presentViewController:errView animated:YES completion:nil];
			linphone_core_set_provisioning_uri([LinphoneManager getLc], NULL);
			break;
		}
		default:
			break;
	}
	if (account_creator) {
		linphone_account_creator_unref(account_creator);
	}
	NSString *siteUrl =
		[[LinphoneManager instance] lpConfigStringForKey:@"first_login_view_url"] ?: @"http://www.linphone.org";
	account_creator = linphone_account_creator_new([LinphoneManager getLc], siteUrl.UTF8String);
}

- (void)registrationUpdateEvent:(NSNotification *)notif {
	[self registrationUpdate:[[notif.userInfo objectForKey:@"state"] intValue]];
}

- (void)registrationUpdate:(LinphoneRegistrationState)state {
	switch (state) {
		case LinphoneRegistrationOk: {
			[[LinphoneManager instance] lpConfigSetBool:FALSE forKey:@"enable_first_login_view_preference"];
			[_waitView setHidden:true];
			[PhoneMainView.instance changeCurrentView:DialerView.compositeViewDescription];
			break;
		}
		case LinphoneRegistrationNone:
		case LinphoneRegistrationCleared: {
			[_waitView setHidden:true];
			break;
		}
		case LinphoneRegistrationFailed: {
			[_waitView setHidden:true];
			break;
		}
		case LinphoneRegistrationProgress: {
			[_waitView setHidden:false];
			break;
		}
		default:
			break;
	}
}

#pragma mark - Action Functions

- (void)onSiteClick:(id)sender {
	NSURL *url = [NSURL URLWithString:_siteButton.titleLabel.text];
	[[UIApplication sharedApplication] openURL:url];
	return;
}

- (void)onLoginClick:(id)sender {
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
		return;
	}

	_waitView.hidden = NO;

	void (^onSuccesssCallBack)(NSString *) = ^(NSString *url) {
	  if (url) {
		  linphone_core_set_provisioning_uri(LC, url.UTF8String);
		  [LinphoneManager.instance resetLinphoneCore];
	  } else {
		  _waitView.hidden = YES;
	  }
	};

	[XMLRPCHelper.self sendXMLRPCRequestWithParams:@"get_remote_provisioning_filename"
										withParams:@[ _usernameField.text, _passwordField.text, _domainField.text ]
										 onSuccess:onSuccesssCallBack];
}

#pragma mark - UITextFieldDelegate Functions

- (BOOL)textFieldShouldReturn:(UITextField *)textField {
	[textField resignFirstResponder];
	if (textField.returnKeyType == UIReturnKeyNext) {
		if (textField == _usernameField) {
			[_domainField becomeFirstResponder];
		} else if (textField == _domainField) {
			[_passwordField becomeFirstResponder];
		}
	} else if (textField.returnKeyType == UIReturnKeyDone) {
		[_loginButton sendActionsForControlEvents:UIControlEventTouchUpInside];
	}

	return YES;
}

- (void)textFieldDidEndEditing:(UITextField *)textField {
	UIAssistantTextField *atf = (UIAssistantTextField *)textField;
	[atf textFieldDidEndEditing:atf];
}

- (BOOL)textField:(UITextField *)textField
	shouldChangeCharactersInRange:(NSRange)range
				replacementString:(NSString *)string {
	UIAssistantTextField *atf = (UIAssistantTextField *)textField;
	[atf textField:atf shouldChangeCharactersInRange:range replacementString:string];
	[self shouldEnableNextButton];
	return YES;
}

@end
