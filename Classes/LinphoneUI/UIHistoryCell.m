/* UIHistoryCell.m
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

#import "UIHistoryCell.h"
#import "LinphoneManager.h"
#import "PhoneMainView.h"
#import "Utils.h"

@implementation UIHistoryCell

@synthesize callLog;
@synthesize displayNameLabel;

#pragma mark - Lifecycle Functions

- (id)initWithIdentifier:(NSString *)identifier {
	if ((self = [super initWithStyle:UITableViewCellStyleDefault reuseIdentifier:identifier]) != nil) {
		NSArray *arrayOfViews =
			[[NSBundle mainBundle] loadNibNamed:NSStringFromClass(self.class) owner:self options:nil];

		// resize cell to match .nib size. It is needed when resized the cell to
		// correctly adapt its height too
		UIView *sub = ((UIView *)[arrayOfViews objectAtIndex:0]);
		[self setFrame:CGRectMake(0, 0, sub.frame.size.width, sub.frame.size.height)];
		[self addSubview:sub];
		_detailsButton.hidden = IPAD;
		callLog = NULL;
	}
	return self;
}

#pragma mark - Action Functions

- (void)setCallLog:(LinphoneCallLog *)acallLog {
	callLog = acallLog;

	[self update];
}

#pragma mark - Action Functions

- (IBAction)onDetails:(id)event {
	if (callLog != NULL && linphone_call_log_get_call_id(callLog) != NULL) {
		// Go to History details view
		HistoryDetailsView *view = VIEW(HistoryDetailsView);
		[view setCallLogId:[NSString stringWithUTF8String:linphone_call_log_get_call_id(callLog)]];
		[PhoneMainView.instance changeCurrentView:view.compositeViewDescription];
	} else {
		LOGE(@"Cannot open selected call log, it is NULL or corrupted");
	}
}

#pragma mark -

- (NSString *)accessibilityValue {
	BOOL incoming = linphone_call_log_get_dir(callLog) == LinphoneCallIncoming;
	BOOL missed = linphone_call_log_get_status(callLog) == LinphoneCallMissed;
	NSString *call_type = incoming ? (missed ? @"Missed" : @"Incoming") : @"Outgoing";
	return [NSString stringWithFormat:@"%@ call from %@", call_type, displayNameLabel.text];
}

- (void)update {
	if (callLog == NULL) {
		LOGW(@"Cannot update history cell: null callLog");
		return;
	}

	// Set up the cell...
	const LinphoneAddress *addr;
	UIImage *image;
	if (linphone_call_log_get_dir(callLog) == LinphoneCallIncoming) {
		if (linphone_call_log_get_status(callLog) != LinphoneCallMissed) {
			image = [UIImage imageNamed:@"call_status_incoming.png"];
		} else {
			image = [UIImage imageNamed:@"call_status_missed.png"];
		}
		addr = linphone_call_log_get_from_address(callLog);
	} else {
		image = [UIImage imageNamed:@"call_status_outgoing.png"];
		addr = linphone_call_log_get_to_address(callLog);
	}
	_stateImage.image = image;

	[ContactDisplay setDisplayNameLabel:displayNameLabel forAddress:addr];

	size_t count = bctbx_list_size(linphone_call_log_get_user_data(callLog)) + 1;
	if (count > 1) {
		displayNameLabel.text =
			[displayNameLabel.text stringByAppendingString:[NSString stringWithFormat:@" (%lu)", count]];
	}

	[_avatarImage setImage:[FastAddressBook imageForAddress:addr thumbnail:YES] bordered:NO withRoundedRadius:YES];
}

- (void)setEditing:(BOOL)editing {
	[self setEditing:editing animated:FALSE];
}

- (void)setEditing:(BOOL)editing animated:(BOOL)animated {
	if (animated) {
		[UIView beginAnimations:nil context:nil];
		[UIView setAnimationDuration:0.3];
	}
	if (editing) {
		[_detailsButton setAlpha:0.0f];
	} else {
		[_detailsButton setAlpha:1.0f];
	}
	if (animated) {
		[UIView commitAnimations];
	}
}

@end
