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
#import "linphoneapp-Swift.h"

@implementation UIChatCreateCollectionViewCell

- (id)initWithFrame:(CGRect)frame {
	self = [super initWithFrame:frame];
	self.contentView.translatesAutoresizingMaskIntoConstraints = false;
	[SnapkitBridge matchParentDimensionsWithView:self.contentView topInset:10];
	
	self.nameLabel = [[UILabel alloc] initWithFrame:CGRectZero];
	self.nameLabel.numberOfLines = 1;
	[self.contentView addSubview:self.nameLabel];
	[SnapkitBridge matchParentDimensionsWithView:self.nameLabel leftInset:20];
	[SnapkitBridge heightWithView:self heiht:50];
	
	UIImageView *image = [[UIImageView alloc] initWithImage:[UIImage imageNamed:@"conference_delete"]];
	image.contentMode = UIViewContentModeScaleAspectFit;
	[self.contentView addSubview:image];
	[SnapkitBridge squareWithView:image size:15];
	[SnapkitBridge alignParentLeftWithView:image];
	[SnapkitBridge centerYWithView:image];
	UITapGestureRecognizer *tap = [[UITapGestureRecognizer alloc] initWithTarget:self action:@selector(onDelete)];
	tap.numberOfTouchesRequired = 1;
	[image addGestureRecognizer:tap];
	image.userInteractionEnabled = true;
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
