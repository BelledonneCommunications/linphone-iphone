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
