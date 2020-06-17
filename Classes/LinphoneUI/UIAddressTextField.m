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

#import "UIAddressTextField.h"

@implementation UIAddressTextField

- (void)setText:(NSString *)text {
	[super setText:text];
	[self sendActionsForControlEvents:UIControlEventEditingChanged];
}

- (BOOL)canPerformAction:(SEL)action withSender:(id)sender {
	// disable "define" option, since it messes with the keyboard
	if ([[NSStringFromSelector(action) lowercaseString] rangeOfString:@"define"].location != NSNotFound) {
		return NO;
	} else {
		return [super canPerformAction:action withSender:sender];
	}
}

@end
