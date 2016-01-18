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
														 isLeftFragment:NO
														   fragmentWith:nil];
	}
	return compositeDescription;
}

- (UICompositeViewDescription *)compositeViewDescription {
	return self.class.compositeViewDescription;
}

#pragma mark - ViewController Functions

- (void)viewWillAppear:(BOOL)animated {
	[super viewWillAppear:animated];
	// Set observer
	[NSNotificationCenter.defaultCenter addObserver:self
										   selector:@selector(registrationUpdateEvent:)
											   name:kLinphoneRegistrationUpdate
											 object:nil];

	[_usernameField setText:[LinphoneManager.instance lpConfigStringForKey:@"assistant_username"]];
	[_passwordField setText:[LinphoneManager.instance lpConfigStringForKey:@"assistant_password"]];

	// Update on show
	const MSList *list = linphone_core_get_proxy_config_list(LC);
	if (list != NULL) {
		LinphoneProxyConfig *config = (LinphoneProxyConfig *)list->data;
		if (config) {
			[self registrationUpdate:linphone_proxy_config_get_state(config)];
		}
	}

	[_usernameField showError:[AssistantView errorForStatus:LinphoneAccountCreatorUsernameInvalid]
						 when:^BOOL(NSString *inputEntry) {
						   LinphoneAccountCreatorStatus s =
							   linphone_account_creator_set_username(account_creator, inputEntry.UTF8String);
						   _usernameField.errorLabel.text = [AssistantView errorForStatus:s];
						   return s != LinphoneAccountCreatorOK;
						 }];

	[_passwordField showError:[AssistantView errorForStatus:LinphoneAccountCreatorPasswordTooShort]
						 when:^BOOL(NSString *inputEntry) {
						   LinphoneAccountCreatorStatus s =
							   linphone_account_creator_set_password(account_creator, inputEntry.UTF8String);
						   _passwordField.errorLabel.text = [AssistantView errorForStatus:s];
						   return s != LinphoneAccountCreatorOK;
						 }];

	[_domainField showError:[AssistantView errorForStatus:LinphoneAccountCreatorDomainInvalid]
					   when:^BOOL(NSString *inputEntry) {
						 LinphoneAccountCreatorStatus s =
							 linphone_account_creator_set_domain(account_creator, inputEntry.UTF8String);
						 _domainField.errorLabel.text = [AssistantView errorForStatus:s];
						 return s != LinphoneAccountCreatorOK;
					   }];
}

- (void)viewWillDisappear:(BOOL)animated {
	[super viewWillDisappear:animated];

	// Remove observer
	[NSNotificationCenter.defaultCenter removeObserver:self name:kLinphoneRegistrationUpdate object:nil];
}

- (void)viewDidLoad {
	[super viewDidLoad];

	NSString *siteUrl = [LinphoneManager.instance lpConfigStringForKey:@"first_login_view_url"];
	if (siteUrl == nil) {
		siteUrl = @"http://www.linphone.org";
	}
	[_siteButton setTitle:siteUrl forState:UIControlStateNormal];
	account_creator = linphone_account_creator_new(LC, siteUrl.UTF8String);
}

- (void)shouldEnableNextButton {
	BOOL invalidInputs = NO;
	for (UIAssistantTextField *field in @[ _usernameField, _passwordField, _domainField ]) {
		invalidInputs |= (field.isInvalid || field.lastText.length == 0);
	}
	_loginButton.enabled = !invalidInputs;
}

#pragma mark - Event Functions

- (void)registrationUpdateEvent:(NSNotification *)notif {
	[self registrationUpdate:[[notif.userInfo objectForKey:@"state"] intValue]];
}

- (void)registrationUpdate:(LinphoneRegistrationState)state {
	switch (state) {
		case LinphoneRegistrationOk: {
			[LinphoneManager.instance lpConfigSetBool:FALSE forKey:@"enable_first_login_view_preference"];
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

			// erase uername passwd
			[LinphoneManager.instance lpConfigSetString:nil forKey:@"assistant_username"];
			[LinphoneManager.instance lpConfigSetString:nil forKey:@"assistant_password"];
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
	_waitView.hidden = NO;
	[XMLRPCHelper GetProvisioningURL:_usernameField.text
							password:_passwordField.text
							  domain:_domainField.text
						   OnSuccess:^(NSString *url) {
							 if (url) {
								 linphone_core_set_provisioning_uri(LC, url.UTF8String);
								 [LinphoneManager.instance resetLinphoneCore];
							 } else {
								 _waitView.hidden = YES;
							 }
						   }];
}

#pragma mark - UITextFieldDelegate Functions

- (BOOL)textFieldShouldReturn:(UITextField *)theTextField {
	// When the user presses return, take focus away from the text field so that the keyboard is dismissed.
	[theTextField resignFirstResponder];
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
