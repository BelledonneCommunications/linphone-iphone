//
//  UIChatCreateConfirmCollectionViewCell.m
//  linphone
//
//  Created by REIS Benjamin on 05/10/2017.
//

#import "UIChatCreateConfirmCollectionViewCell.h"

@implementation UIChatCreateConfirmCollectionViewCell

- (id)initWithName:(NSString *)identifier {
	if (self != nil) {
		NSArray *arrayOfViews =
		[[NSBundle mainBundle] loadNibNamed:NSStringFromClass(self.class) owner:self options:nil];
		if ([arrayOfViews count] >= 1) {
			UIChatCreateCollectionViewCell *sub = ((UIChatCreateCollectionViewCell *)[arrayOfViews objectAtIndex:0]);
			[self addSubview:sub];
			_displayNameLabel = sub.nameLabel;
		}
	}
	[_displayNameLabel setText:identifier];

	UITapGestureRecognizer *tap = [[UITapGestureRecognizer alloc] initWithTarget:self action:@selector(onDelete)];
	tap.numberOfTouchesRequired = 1;
	[self addGestureRecognizer:tap];
	return self;
}

- (void)onDelete {
	[_confirmController deleteContact:_uri];
}
@end
