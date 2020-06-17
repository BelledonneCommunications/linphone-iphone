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

#import "UITextField+DoneButton.h"

#import "LinphoneManager.h"

@implementation UITextField (DoneButton)

- (void)addDoneButton {
	// actually on iPad there is a done button
	if (!IPAD) {
		UIToolbar *numberToolbar = [[UIToolbar alloc] initWithFrame:CGRectMake(0, 0, 320, 50)];
		numberToolbar.items = [NSArray
			arrayWithObjects:[[UIBarButtonItem alloc] initWithTitle:NSLocalizedString(@"Cancel", nil)
															  style:UIBarButtonItemStyleBordered
															 target:self
															 action:@selector(cancelNumberPad)],
							 [[UIBarButtonItem alloc] initWithBarButtonSystemItem:UIBarButtonSystemItemFlexibleSpace
																		   target:nil
																		   action:nil],
							 [[UIBarButtonItem alloc] initWithTitle:NSLocalizedString(@"Done", nil)
															  style:UIBarButtonItemStyleDone
															 target:self
															 action:@selector(doneWithNumberPad)],
							 nil];
		[numberToolbar sizeToFit];

		self.inputAccessoryView = numberToolbar;
	}
}

- (void)cancelNumberPad {
	[self resignFirstResponder];
	self.text = @"";
}

- (void)doneWithNumberPad {
	[self resignFirstResponder];
}
@end
