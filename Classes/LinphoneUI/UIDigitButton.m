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

#import "UIDigitButton.h"
#include "linphone/linphonecore.h"
#import "LinphoneManager.h"

@implementation UIDigitButton

@synthesize dtmf;
@synthesize digit;
@synthesize addressField;

#pragma mark - Lifecycle Functions

- (void)initUIDigitButton {
	dtmf = FALSE;
	[self addTarget:self action:@selector(touchDown:) forControlEvents:UIControlEventTouchDown];
	[self addTarget:self
				  action:@selector(touchUp:)
		forControlEvents:UIControlEventTouchUpInside | UIControlEventTouchUpOutside];
}

- (id)init {
	self = [super init];
	if (self) {
		[self initUIDigitButton];
	}
	return self;
}

- (id)initWithFrame:(CGRect)frame {
	self = [super initWithFrame:frame];
	if (self) {
		[self initUIDigitButton];
	}
	return self;
}

- (id)initWithCoder:(NSCoder *)decoder {
	self = [super initWithCoder:decoder];
	if (self) {
		[self initUIDigitButton];
	}
	return self;
}

#pragma mark - Actions Functions

- (void)touchDown:(id)sender {
	if (addressField && (!dtmf || !linphone_core_in_call(LC))) {
		NSString *newAddress = [NSString stringWithFormat:@"%@%c", addressField.text, digit];
		[addressField setText:newAddress];
		linphone_core_play_dtmf(LC, digit, -1);
	} else {
		linphone_call_send_dtmf(linphone_core_get_current_call(LC), digit);
		linphone_core_play_dtmf(LC, digit, 100);
	}
}

- (void)touchUp:(id)sender {
	linphone_core_stop_dtmf(LC);
}

@end
