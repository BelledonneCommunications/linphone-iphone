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
#import "PhoneMainView.h"

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

- (void)setParticipant:(LinphoneParticipant *)p {
	_participant = p;
	if (!p) {
		return;
	}
	const LinphoneAddress *addr = linphone_participant_get_address(p);
	[ContactDisplay setDisplayNameLabel:_nameLabel forAddress:addr];
	_durationLabel.text = [LinphoneUtils durationToString:[NSDate date].timeIntervalSince1970 - linphone_participant_get_creation_time(p)];
	_kickButton.hidden = CallManager.instance.isInConferenceAsGuest;
}


- (IBAction)onKickClick:(id)sender {
	if (!_participant) {
		return;
	}

	if ([CallManager callKitEnabled]) {
		LinphoneCall *call = [CallManager.instance getCallForParticipant:_participant];
		if (call) {
			[CallManager.instance setHeldWithCall:call hold:true];
		}
	}
	linphone_conference_remove_participant_2([CallManager.instance getConference], _participant);


}
@end
