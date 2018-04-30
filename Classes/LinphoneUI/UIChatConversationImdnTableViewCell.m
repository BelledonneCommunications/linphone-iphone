//
//  UIChatConversationInfoTableViewCell.m
//  linphone
//
//  Created by REIS Benjamin on 23/10/2017.
//

#import "PhoneMainView.h"
#import "UIChatConversationImdnTableViewCell.h"

@implementation UIChatConversationImdnTableViewCell

- (void)awakeFromNib {
	[super awakeFromNib];
	// Initialization code
}

- (id)initWithIdentifier:(NSString *)identifier {
	self = [super initWithStyle:UITableViewCellStyleDefault reuseIdentifier:identifier];
	if (self != nil) {
		NSArray *arrayOfViews =
		[[NSBundle mainBundle] loadNibNamed:NSStringFromClass(self.class) owner:self options:nil];
		if ([arrayOfViews count] >= 1) {
			UIView *sub = ((UIView *)[arrayOfViews objectAtIndex:0]);
			[self setFrame:CGRectMake(0, 0, sub.frame.size.width, sub.frame.size.height)];
			[self addSubview:sub];
		}
	}
	return self;
}

@end

