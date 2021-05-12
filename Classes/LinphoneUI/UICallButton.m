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

#import "UICallButton.h"
#import "LinphoneManager.h"

#import <CoreTelephony/CTCallCenter.h>

@implementation UICallButton

@synthesize addressField;

#pragma mark - Lifecycle Functions

- (void)initUICallButton {
	[self addTarget:self action:@selector(touchUp:) forControlEvents:UIControlEventTouchUpInside];
}

- (id)init {
	self = [super init];
	if (self) {
		[self initUICallButton];
	}
	return self;
}

- (id)initWithFrame:(CGRect)frame {
	self = [super initWithFrame:frame];
	if (self) {
		[self initUICallButton];
	}
	return self;
}

- (id)initWithCoder:(NSCoder *)decoder {
	self = [super initWithCoder:decoder];
	if (self) {
		[self initUICallButton];
	}
	return self;
}

#pragma mark -

- (void)touchUp:(id)sender {
	NSString *address = addressField.text;
	if (address.length == 0) {
		LinphoneCallLog *log = linphone_core_get_last_outgoing_call_log(LC);
		if (log) {
			const LinphoneAddress *to = linphone_call_log_get_to_address(log);
			const char *domain = linphone_address_get_domain(to);
			char *bis_address = NULL;
			LinphoneAccount *def_account = linphone_core_get_default_account(LC);

			// if the 'to' address is on the default proxy, only present the username
			if (def_account) {
				const char *def_domain = linphone_account_params_get_domain(linphone_account_get_params(def_account));
				if (def_domain && domain && !strcmp(domain, def_domain)) {
					bis_address = ms_strdup(linphone_address_get_username(to));
				}
			}
			if (bis_address == NULL) {
				bis_address = linphone_address_as_string_uri_only(to);
			}
			[addressField setText:[NSString stringWithUTF8String:bis_address]];
			ms_free(bis_address);
			// return after filling the address, let the user confirm the call by pressing again
			return;
		}
	}

	if ([address length] > 0) {
		LinphoneAddress *addr = [LinphoneUtils normalizeSipOrPhoneAddress:address];
		[LinphoneManager.instance call:addr];
		if (addr)
			linphone_address_unref(addr);
	}
}

- (void)updateIcon {
	if (linphone_core_video_capture_enabled(LC) && linphone_core_get_video_policy(LC)->automatically_initiate) {
		[self setImage:[UIImage imageNamed:@"call_video_start_default.png"] forState:UIControlStateNormal];
		[self setImage:[UIImage imageNamed:@"call_video_start_disabled.png"] forState:UIControlStateDisabled];
	} else {
		[self setImage:[UIImage imageNamed:@"call_audio_start_default.png"] forState:UIControlStateNormal];
		[self setImage:[UIImage imageNamed:@"call_audio_start_disabled.png"] forState:UIControlStateDisabled];
	}

	if (CallManager.instance.nextCallIsTransfer) {
		[self setImage:[UIImage imageNamed:@"call_transfer_default.png"] forState:UIControlStateNormal];
		[self setImage:[UIImage imageNamed:@"call_transfer_disabled.png"] forState:UIControlStateDisabled];
	} else if (linphone_core_get_calls_nb(LC) > 0) {
		[self setImage:[UIImage imageNamed:@"call_add_default.png"] forState:UIControlStateNormal];
		[self setImage:[UIImage imageNamed:@"call_add_disabled.png"] forState:UIControlStateDisabled];
	}
}
@end
