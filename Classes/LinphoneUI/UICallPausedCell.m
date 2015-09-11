//
//  UIPausedCallCell.m
//  linphone
//
//  Created by Gautier Pelloux-Prayer on 11/09/15.
//
//

#import "UICallPausedCell.h"
#import "Utils.h"

@implementation UICallPausedCell

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
	if (!call) {
		LOGW(@"Cannot update call cell: null call or data");
		return;
	}

	const LinphoneAddress *addr = linphone_call_get_remote_address(call);
	[ContactDisplay setDisplayNameLabel:_nameLabel forAddress:addr];

	_avatarImage.image =
		[FastAddressBook getContactImage:[FastAddressBook getContactWithLinphoneAddress:addr] thumbnail:NO];

	int duration = linphone_call_get_duration(call);
	[_durationLabel setText:[NSString stringWithFormat:@"%02i:%02i", (duration / 60), (duration % 60), nil]];
}

@end
