//
//  UIPausedCallCell.m
//  linphone
//
//  Created by Gautier Pelloux-Prayer on 11/09/15.
//
//

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
	if (!call || !linphone_call_is_in_conference(call)) {
		LOGF(@"Invalid call: either NULL or not in conference.");
		return;
	}

	const LinphoneAddress *addr = linphone_call_get_remote_address(call);
	[ContactDisplay setDisplayNameLabel:_nameLabel forAddress:addr];

	_avatarImage.image = [FastAddressBook imageForAddress:addr thumbnail:YES];

	_durationLabel.text = [LinphoneUtils durationForCall:linphone_core_get_current_call([LinphoneManager getLc])];
}

- (IBAction)onKickClick:(id)sender {
	linphone_core_remove_from_conference([LinphoneManager getLc], _call);
}
@end
