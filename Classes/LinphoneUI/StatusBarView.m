/* StatusBarViewController.m
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

#import "StatusBarView.h"
#import "LinphoneManager.h"
#import "PhoneMainView.h"

@implementation StatusBarView {

	NSTimer *callQualityTimer;
	NSTimer *callSecurityTimer;
	int messagesUnreadCount;
}

@synthesize registrationState;
@synthesize callQualityImage;
@synthesize callSecurityButton;

#pragma mark - Lifecycle Functions

- (void)dealloc {
	[[NSNotificationCenter defaultCenter] removeObserver:self];
	[callQualityTimer invalidate];
}

#pragma mark - ViewController Functions

- (void)viewWillAppear:(BOOL)animated {
	[super viewWillAppear:animated];

	// Set observer
	[[NSNotificationCenter defaultCenter] addObserver:self
											 selector:@selector(registrationUpdate:)
												 name:kLinphoneRegistrationUpdate
											   object:nil];

	[[NSNotificationCenter defaultCenter] addObserver:self
											 selector:@selector(globalStateUpdate:)
												 name:kLinphoneGlobalStateUpdate
											   object:nil];

	[[NSNotificationCenter defaultCenter] addObserver:self
											 selector:@selector(notifyReceived:)
												 name:kLinphoneNotifyReceived
											   object:nil];

	[[NSNotificationCenter defaultCenter] addObserver:self
											 selector:@selector(callUpdate:)
												 name:kLinphoneCallUpdate
											   object:nil];

	// Update to default state
	LinphoneProxyConfig *config = linphone_core_get_default_proxy_config([LinphoneManager getLc]);
	messagesUnreadCount =
		lp_config_get_int(linphone_core_get_config([LinphoneManager getLc]), "app", "voice_mail_messages_count", 0);

	[self proxyConfigUpdate:config];
	[self updateUI:linphone_core_get_calls_nb([LinphoneManager getLc])];
	[self updateVoicemail];
}

- (void)viewWillDisappear:(BOOL)animated {
	[super viewWillDisappear:animated];

	// Remove observer
	[[NSNotificationCenter defaultCenter] removeObserver:self name:kLinphoneRegistrationUpdate object:nil];
	[[NSNotificationCenter defaultCenter] removeObserver:self name:kLinphoneGlobalStateUpdate object:nil];
	[[NSNotificationCenter defaultCenter] removeObserver:self name:kLinphoneNotifyReceived object:nil];
	[[NSNotificationCenter defaultCenter] removeObserver:self name:kLinphoneCallUpdate object:nil];

	if (callQualityTimer != nil) {
		[callQualityTimer invalidate];
		callQualityTimer = nil;
	}
	if (callSecurityTimer != nil) {
		[callSecurityTimer invalidate];
		callSecurityTimer = nil;
	}
}

#pragma mark - Event Functions

- (void)registrationUpdate:(NSNotification *)notif {
	LinphoneProxyConfig *config = linphone_core_get_default_proxy_config([LinphoneManager getLc]);
	[self proxyConfigUpdate:config];
}

- (void)globalStateUpdate:(NSNotification *)notif {
	[self registrationUpdate:notif];
}

- (void)notifyReceived:(NSNotification *)notif {
	const LinphoneContent *content = [[notif.userInfo objectForKey:@"content"] pointerValue];

	if ((content == NULL) || (strcmp("application", linphone_content_get_type(content)) != 0) ||
		(strcmp("simple-message-summary", linphone_content_get_subtype(content)) != 0) ||
		(linphone_content_get_buffer(content) == NULL)) {
		return;
	}
	const char *body = linphone_content_get_buffer(content);
	if ((body = strstr(body, "voice-message: ")) == NULL) {
		LOGW(@"Received new NOTIFY from voice mail but could not find 'voice-message' in BODY. Ignoring it.");
		return;
	}

	sscanf(body, "voice-message: %d", &messagesUnreadCount);

	LOGI(@"Received new NOTIFY from voice mail: there is/are now %d message(s) unread", messagesUnreadCount);

	// save in lpconfig for future
	lp_config_set_int(linphone_core_get_config([LinphoneManager getLc]), "app", "voice_mail_messages_count",
					  messagesUnreadCount);

	[self updateVoicemail];
}

- (void)updateVoicemail {
	_voicemailButton.hidden = (messagesUnreadCount <= 0);
	_voicemailButton.titleLabel.text = @(messagesUnreadCount).stringValue;
}

- (void)callUpdate:(NSNotification *)notif {
	// show voice mail only when there is no call
	[self updateUI:linphone_core_get_calls([LinphoneManager getLc]) != NULL];
	[self updateVoicemail];

	LinphoneCall *currentCall = linphone_core_get_current_call([LinphoneManager getLc]);
	if (currentCall && !linphone_call_get_authentication_token_verified(currentCall) &&
		linphone_call_get_state(currentCall) == LinphoneCallStreamsRunning) {
		[self onSecurityClick:nil];
	}
}

#pragma mark -

- (void)proxyConfigUpdate:(LinphoneProxyConfig *)config {
	LinphoneRegistrationState state = LinphoneRegistrationNone;
	NSString *message = nil;
	UIImage *image = nil;
	LinphoneCore *lc = [LinphoneManager getLc];
	LinphoneGlobalState gstate = linphone_core_get_global_state(lc);

	if (gstate == LinphoneGlobalConfiguring) {
		message = NSLocalizedString(@"Fetching remote configuration", nil);
	} else if (config == NULL) {
		state = LinphoneRegistrationNone;
		if (linphone_core_is_network_reachable([LinphoneManager getLc]))
			message = NSLocalizedString(@"No SIP account configured", nil);
		else
			message = NSLocalizedString(@"Network down", nil);
	} else {
		state = linphone_proxy_config_get_state(config);

		switch (state) {
			case LinphoneRegistrationOk:
				message = NSLocalizedString(@"Registered", nil);
				break;
			case LinphoneRegistrationNone:
			case LinphoneRegistrationCleared:
				message = NSLocalizedString(@"Not registered", nil);
				break;
			case LinphoneRegistrationFailed:
				message = NSLocalizedString(@"Registration failed", nil);
				break;
			case LinphoneRegistrationProgress:
				message = NSLocalizedString(@"Registration in progress", nil);
				break;
			default:
				break;
		}
	}

	switch (state) {
		case LinphoneRegistrationFailed:
			image = [UIImage imageNamed:@"led_error.png"];
			break;
		case LinphoneRegistrationCleared:
		case LinphoneRegistrationNone:
			image = [UIImage imageNamed:@"led_disconnected.png"];
			break;
		case LinphoneRegistrationProgress:
			image = [UIImage imageNamed:@"led_inprogress.png"];
			break;
		case LinphoneRegistrationOk:
			image = [UIImage imageNamed:@"led_connected.png"];
			break;
	}
	[registrationState setTitle:message forState:UIControlStateNormal];
	registrationState.accessibilityValue = message;
	[registrationState setImage:image forState:UIControlStateNormal];
}

#pragma mark -

- (void)updateUI:(BOOL)inCall {
	// nothing changed
	if (_outcallView.hidden == inCall)
		return;

	_outcallView.hidden = inCall;
	_incallView.hidden = !inCall;
	// always hide icons at start since they are not ready yet
	callQualityImage.hidden = callSecurityButton.hidden = YES;

	if (callQualityTimer) {
		[callQualityTimer invalidate];
		callQualityTimer = nil;
	}
	if (callSecurityTimer) {
		[callSecurityTimer invalidate];
		callSecurityTimer = nil;
	}

	// if we are in call, we have to update quality and security icons every sec
	if (inCall) {
		callQualityTimer = [NSTimer scheduledTimerWithTimeInterval:1
															target:self
														  selector:@selector(callQualityUpdate)
														  userInfo:nil
														   repeats:YES];
		callSecurityTimer = [NSTimer scheduledTimerWithTimeInterval:1
															 target:self
														   selector:@selector(callSecurityUpdate)
														   userInfo:nil
															repeats:YES];
	}
}

- (void)callSecurityUpdate {
	BOOL pending = false;
	BOOL security = true;

	const MSList *list = linphone_core_get_calls([LinphoneManager getLc]);
	if (list == NULL) {
		if (securityDialog) {
			[securityDialog dismiss];
		}
	} else {
		callSecurityButton.hidden = NO;
		while (list != NULL) {
			LinphoneCall *call = (LinphoneCall *)list->data;
			LinphoneMediaEncryption enc =
				linphone_call_params_get_media_encryption(linphone_call_get_current_params(call));
			if (enc == LinphoneMediaEncryptionNone)
				security = false;
			else if (enc == LinphoneMediaEncryptionZRTP) {
				if (!linphone_call_get_authentication_token_verified(call)) {
					pending = true;
				}
			}
			list = list->next;
		}
		NSString *imageName = security ? (pending ? @"security_pending.png" : @"security_ok.png") : @"security_ko.png";
		[callSecurityButton setImage:[UIImage imageNamed:imageName] forState:UIControlStateNormal];
	}
}

- (void)callQualityUpdate {
	LinphoneCall *call = linphone_core_get_current_call([LinphoneManager getLc]);
	if (call != NULL) {
		int quality = MIN(4, floor(linphone_call_get_average_quality(call)));
		NSString *accessibilityValue = [NSString stringWithFormat:NSLocalizedString(@"Call quality: %d", nil), quality];
		if (![accessibilityValue isEqualToString:callQualityImage.accessibilityValue]) {
			callQualityImage.accessibilityValue = accessibilityValue;
			callQualityImage.hidden = (quality == -1.f);
			callQualityImage.image =
				(quality == -1.f)
					? nil
					: [UIImage imageNamed:[NSString stringWithFormat:@"call_quality_indicator_%d.png", quality]];
		}
	}
}

#pragma mark - Action Functions

- (IBAction)onSecurityClick:(id)sender {
	if (linphone_core_get_calls_nb([LinphoneManager getLc])) {
		LinphoneCall *call = linphone_core_get_current_call([LinphoneManager getLc]);
		if (call != NULL) {
			LinphoneMediaEncryption enc =
				linphone_call_params_get_media_encryption(linphone_call_get_current_params(call));
			if (enc == LinphoneMediaEncryptionZRTP) {
				bool valid = linphone_call_get_authentication_token_verified(call);
				NSString *message =
					[NSString stringWithFormat:NSLocalizedString(@"Confirm the following SAS with peer:\n%s", nil),
											   linphone_call_get_authentication_token(call)];
				if (securityDialog == nil) {
					__block __strong StatusBarView *weakSelf = self;
					securityDialog = [UIConfirmationDialog ShowWithMessage:message
						cancelMessage:NSLocalizedString(@"DENY", nil)
						confirmMessage:NSLocalizedString(@"ACCEPT", nil)
						onCancelClick:^() {
						  linphone_call_set_authentication_token_verified(call, NO);
						  weakSelf->securityDialog = nil;
						}
						onConfirmationClick:^() {
						  linphone_call_set_authentication_token_verified(call, !valid);
						  weakSelf->securityDialog = nil;
						}];
				}
			}
		}
	}
}

- (IBAction)onSideMenuClick:(id)sender {
	UICompositeView *cvc = PhoneMainView.instance.mainViewController;
	if (cvc.sideMenuView.hidden) {
		[cvc hideSideMenu:NO];
	} else {
		[cvc hideSideMenu:cvc.sideMenuView.frame.origin.x == 0];
	}
}

- (IBAction)onRegistrationStateClick:(id)sender {
	linphone_core_refresh_registers([LinphoneManager getLc]);
}

#pragma mark - TPMultiLayoutViewController Functions

- (NSDictionary *)attributesForView:(UIView *)view {
	NSMutableDictionary *attributes = [NSMutableDictionary dictionary];

	[attributes setObject:[NSValue valueWithCGRect:view.frame] forKey:@"frame"];
	[attributes setObject:[NSValue valueWithCGRect:view.bounds] forKey:@"bounds"];
	[attributes setObject:[NSNumber numberWithInteger:view.autoresizingMask] forKey:@"autoresizingMask"];

	return attributes;
}

- (void)applyAttributes:(NSDictionary *)attributes toView:(UIView *)view {
	view.frame = [[attributes objectForKey:@"frame"] CGRectValue];
	view.bounds = [[attributes objectForKey:@"bounds"] CGRectValue];
	view.autoresizingMask = [[attributes objectForKey:@"autoresizingMask"] integerValue];
}

@end
