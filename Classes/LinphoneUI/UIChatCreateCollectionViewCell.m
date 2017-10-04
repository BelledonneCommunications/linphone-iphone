//
//  UIChatCreateCollectionViewCell.m
//  linphone
//
//  Created by REIS Benjamin on 03/10/2017.
//

#import "UIChatCreateCollectionViewCell.h"

@implementation UIChatCreateCollectionViewCell
- (void)awakeFromNib {
    [super awakeFromNib];
}

- (id)initWithName:(NSString *)identifier {
	if (self != nil) {
		NSArray *arrayOfViews =
		[[NSBundle mainBundle] loadNibNamed:NSStringFromClass(self.class) owner:self options:nil];
		if ([arrayOfViews count] >= 1) {
			UIChatCreateCollectionViewCell *sub = ((UIChatCreateCollectionViewCell *)[arrayOfViews objectAtIndex:0]);
			[self addSubview:sub];
			_nameLabel = sub.nameLabel;
		}
	}
	[_nameLabel setText:identifier];

	UITapGestureRecognizer *tap = [[UITapGestureRecognizer alloc] initWithTarget:self action:@selector(onDelete)];
	tap.numberOfTouchesRequired = 1;
	[self addGestureRecognizer:tap];
	return self;
}

- (void) onDelete {
	[_controller.tableController.contactsGroup removeObject:_uri];
	[_controller.tableController.contactsDict removeObjectForKey:_uri];
	[_controller.collectionView reloadData];
	[_controller.tableController.tableView reloadData];
}
@end
