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
	// if no call is provided, we assume that this is a conference
	if (!call) {
		[_pauseButton setType:UIPauseButtonType_Conference call:call];
		_nameLabel.text = NSLocalizedString(@"Conference", nil);
		[_avatarImage setImage:[UIImage imageNamed:@"options_start_conference_default.png"]
					  bordered:NO
			 withRoundedRadius:YES];
		_durationLabel.text = @"";
	} else {
		[_pauseButton setType:UIPauseButtonType_Call call:call];
		const LinphoneAddress *addr = linphone_call_get_remote_address(call);
		[ContactDisplay setDisplayNameLabel:_nameLabel forAddress:addr];
		[_avatarImage setImage:[FastAddressBook imageForAddress:addr thumbnail:YES] bordered:NO withRoundedRadius:YES];
		_durationLabel.text = [LinphoneUtils durationToString:linphone_call_get_duration(call)];
	}
	[_pauseButton update];
}

@end
