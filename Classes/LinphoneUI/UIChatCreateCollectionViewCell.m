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
	if (_controller.tableController.contactsGroup.count == 0) {
		[UIView animateWithDuration:0.2
							  delay:0
							options:UIViewAnimationOptionCurveEaseOut
						 animations:^{
							 [_controller.tableController.tableView setFrame:CGRectMake(_controller.tableController.tableView.frame.origin.x,
															_controller.tableController.searchBar.frame.origin.y + _controller.tableController.searchBar.frame.size.height,
															_controller.tableController.tableView.frame.size.width,
															_controller.tableController.tableView.frame.size.height + _controller.collectionView.frame.size.height)];
						 }
						 completion:nil];
	}
	[_controller.collectionView reloadData];
	[_controller.tableController.tableView reloadData];
	_controller.nextButton.enabled = (_controller.tableController.contactsGroup.count > 0) || _controller.isForEditing;
}
@end
