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
#import "Utils.h"

@implementation UICheckBoxTVTableViewController

- (instancetype)initWithCoder:(NSCoder *)aDecoder {
	self = [super initWithCoder:aDecoder];
	_selectedItems = [[NSMutableArray alloc] init];
	return self;
}

- (instancetype)init {
	self = [super init];
	_selectedItems = [[NSMutableArray alloc] init];
	return self;
}

#pragma mark - UITableViewDelegate Functions

- (void)viewDidAppear:(BOOL)animated {
	[super viewDidAppear:animated];
	_editButton.enabled = [self tableView:self.tableView numberOfRowsInSection:0] != 0;
}

- (void)tableView:(UITableView *)tableView didSelectRowAtIndexPath:(NSIndexPath *)indexPath {
	UITableViewCell *cell = [tableView cellForRowAtIndexPath:indexPath];
	[tableView deselectRowAtIndexPath:[tableView indexPathForSelectedRow] animated:NO];
	cell.accessoryType = (cell.accessoryType == UITableViewCellAccessoryCheckmark) ? UITableViewCellAccessoryNone
																				   : UITableViewCellAccessoryCheckmark;
	[self accessoryForCell:cell atPath:indexPath];
}

- (UITableViewCellEditingStyle)tableView:(UITableView *)aTableView
		   editingStyleForRowAtIndexPath:(NSIndexPath *)indexPath {
	// Detemine if it's in editing mode
	if (self.editing) {
		return UITableViewCellEditingStyleDelete;
	}
	return UITableViewCellEditingStyleNone;
}

#pragma mark -

- (void)accessoryForCell:(UITableViewCell *)cell atPath:(NSIndexPath *)indexPath {
	cell.selectionStyle = UITableViewCellSelectionStyleNone;
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
		checkBoxButton.userInteractionEnabled = NO;
		cell.accessoryView = checkBoxButton;
	} else {
		cell.accessoryView = nil;
		cell.accessoryType = UITableViewCellAccessoryNone;
	}
	_deleteButton.enabled = (_selectedItems.count != 0);
	_editButton.enabled = [self tableView:self.tableView numberOfRowsInSection:0] != 0;
}

- (void)setEditing:(BOOL)editing animated:(BOOL)animated {
	[super setEditing:editing animated:animated];

	_editButton.hidden = editing;
	_deleteButton.hidden = _cancelButton.hidden = _toggleSelectionButton.hidden = !editing;

	// when switching editing mode, we must reload all cells to remove/add checkboxes
	[self loadData];
}

- (void)loadData {
	[_selectedItems removeAllObjects];
	[self.tableView reloadData];
}

- (void)removeSelection {
	// we must iterate through selected items in reverse order
	[_selectedItems sortUsingComparator:^(NSIndexPath *obj1, NSIndexPath *obj2) {
	  return [obj2 compare:obj1];
	}];
	for (NSIndexPath *indexPath in _selectedItems) {
		[self tableView:self.tableView
			commitEditingStyle:UITableViewCellEditingStyleDelete
			 forRowAtIndexPath:indexPath];
	}
	[_selectedItems removeAllObjects];
}

- (void)onSelectionToggle:(id)sender {
	[_selectedItems removeAllObjects];
	UIButton *button = (UIButton *)sender;
	for (int i = 0; i < [self numberOfSectionsInTableView:self.tableView]; i++) {
		for (int j = 0; j < [self tableView:self.tableView numberOfRowsInSection:i]; j++) {
			NSIndexPath *idx = [NSIndexPath indexPathForRow:j inSection:i];
			UITableViewCell *cell = [self tableView:self.tableView cellForRowAtIndexPath:idx];
			cell.accessoryType = (button.state == UIControlStateSelected) ? UITableViewRowAnimationNone
																		  : UITableViewCellAccessoryCheckmark;
			[self accessoryForCell:cell atPath:idx];
		}
	}
}

- (IBAction)onEditClick:(id)sender {
	[self setEditing:YES animated:YES];
}

- (IBAction)onCancelClick:(id)sender {
	[self setEditing:NO animated:YES];
}

@end
