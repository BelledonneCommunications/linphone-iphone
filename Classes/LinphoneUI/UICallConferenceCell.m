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

#import "UICallConferenceCell.h"
#import "Utils.h"

@implementation UICallConferenceCell

- (id)initWithIdentifier:(NSString *)identifier {
	self = [super initWithStyle:UITableViewCellStyleDefault reuseIdentifier:identifier];
	if (self != nil) {
		NSArray *arrayOfViews =
			[[NSBundle mainBundle] loadNibNamed:NSStringFromClass(self.class) owner:self options:nil];
		if ([arrayOfViews count] >= 1) {
			// resize cell to match .nib size. It is needed when resized the cell to
			// correctly adapt its height too
			UIView *sub = ((UIView *)[arrayOfViews objectAtIndex:0]);
			[self setFrame:CGRectMake(0, 0, sub.frame.size.width, sub.frame.size.height)];
			[self addSubview:sub];
		}
	}
	return self;
}

- (void)setCall:(LinphoneCall *)call {
	_call = call;
	if (!call || !linphone_call_params_get_local_conference_mode(linphone_call_get_current_params(call))) {
		LOGF(@"Invalid call: either NULL or not in conference.");
		return;
	}

	const LinphoneAddress *addr = linphone_call_get_remote_address(call);
	[ContactDisplay setDisplayNameLabel:_nameLabel forAddress:addr];

	[_avatarImage setImage:[FastAddressBook imageForAddress:addr] bordered:NO withRoundedRadius:YES];

	_durationLabel.text = [LinphoneUtils durationToString:linphone_call_get_duration(call)];
}

- (IBAction)onKickClick:(id)sender {
	linphone_core_remove_from_conference(LC, _call);
}
@end
