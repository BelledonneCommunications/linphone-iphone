/* HistoryTableViewController.m
 *
 * Copyright (C) 2009  Belledonne Comunications, Grenoble, France
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */

#import "UICheckBoxTVTableViewController.h"

@implementation UICheckBoxTVTableViewController

- (instancetype)init {
	self = [super init];
	_selectedItems = [[NSMutableArray alloc] init];
	return self;
}

#pragma mark - UITableViewDelegate Functions

- (void)tableView:(UITableView *)tableView didSelectRowAtIndexPath:(NSIndexPath *)indexPath {
	UITableViewCell *cell = [tableView cellForRowAtIndexPath:indexPath];
	[tableView deselectRowAtIndexPath:[tableView indexPathForSelectedRow] animated:NO];
	cell.accessoryType = (cell.accessoryType == UITableViewCellAccessoryCheckmark) ? UITableViewCellAccessoryNone
																				   : UITableViewCellAccessoryCheckmark;
	[self accessoryForCell:cell atPath:indexPath];
}

- (void)accessoryForCell:(UITableViewCell *)cell atPath:(NSIndexPath *)indexPath {
	if ([self isEditing]) {
		UIImage *image = nil;
		if (cell.accessoryType == UITableViewCellAccessoryCheckmark) {
			[_selectedItems addObject:indexPath];
			image = [UIImage imageNamed:@"checkbox_checked.png"];
		} else if (cell.accessoryType == UITableViewCellAccessoryNone) {
			[_selectedItems removeObject:indexPath];
			image = [UIImage imageNamed:@"checkbox_unchecked.png"];
		}
		UIButton *checkBoxButton = [UIButton buttonWithType:UIButtonTypeCustom];
		[checkBoxButton setImage:image forState:UIControlStateNormal];
		[checkBoxButton setFrame:CGRectMake(0, 0, 19, 19)];
		[checkBoxButton setBackgroundColor:[UIColor clearColor]];

		cell.accessoryView = checkBoxButton;
	} else {
		cell.accessoryView = nil;
		cell.accessoryType = UITableViewCellAccessoryNone;
	}
}

- (void)setEditing:(BOOL)editing animated:(BOOL)animated {
	[super setEditing:editing animated:animated];
	[self loadData];
}

- (void)loadData {
	[_selectedItems removeAllObjects];
	[self.tableView reloadData];
}

@end
