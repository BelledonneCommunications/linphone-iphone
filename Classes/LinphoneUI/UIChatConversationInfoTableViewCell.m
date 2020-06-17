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

#import "PhoneMainView.h"
#import "UIChatConversationInfoTableViewCell.h"

@implementation UIChatConversationInfoTableViewCell

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
			UIChatConversationInfoTableViewCell *sub = ((UIChatConversationInfoTableViewCell *)[arrayOfViews objectAtIndex:0]);
			self = sub;
		}
	}

	UITapGestureRecognizer *adminTap = [[UITapGestureRecognizer alloc]
										initWithTarget:self
										action:@selector(onAdmin)];
	adminTap.delegate = self;
	adminTap.numberOfTapsRequired = 1;
	[_adminButton addGestureRecognizer:adminTap];
	return self;
}

- (IBAction)onDelete:(id)sender {
	[_controllerView.contacts removeObject:_uri];
	if ([_controllerView.admins containsObject:_uri])
	   [_controllerView.admins removeObject:_uri];

	[_controllerView.tableView reloadData];
	_controllerView.nextButton.enabled = _controllerView.nameLabel.text.length > 0 && _controllerView.contacts.count > 0;
}

- (void)onAdmin {
	_adminLabel.enabled = !_adminLabel.enabled;
	NSString *content = _adminLabel.enabled
						? @"check_selected.png"
						: @"check_unselected.png";
	
	_adminImage.image = [UIImage imageNamed:content];

	if (_adminLabel.enabled)
		[_controllerView.admins addObject:_uri];
	else
		[_controllerView.admins removeObject:_uri];
}

@end
