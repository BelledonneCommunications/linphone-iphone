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

#import "UICheckBoxTableView.h"
#import "Utils.h"

@implementation UICheckBoxTableView

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

- (BOOL)selectFirstRow {
	// reset details view in fragment mode
	if ([self totalNumberOfItems] > 0) {
		NSIndexPath *indexPath = [NSIndexPath indexPathForRow:0 inSection:0];
		[self.tableView selectRowAtIndexPath:indexPath animated:NO scrollPosition:UITableViewScrollPositionNone];
		[self tableView:self.tableView didSelectRowAtIndexPath:indexPath];
		_emptyView.hidden = YES;
	} else {
		_emptyView.hidden = NO;
	}
	return _emptyView.hidden;
}

- (void)viewDidAppear:(BOOL)animated {
	[super viewDidAppear:animated];
	_emptyView.hidden = _editButton.enabled = ([self totalNumberOfItems] > 0);
}

- (void)toggleRowSelectionForRowAtIndexPath:(NSIndexPath *)indexPath {
	UITableViewCell *cell = [self.tableView cellForRowAtIndexPath:indexPath];
	if ([_selectedItems containsObject:indexPath]) {
		[_selectedItems removeObject:indexPath];
	} else {
		[_selectedItems addObject:indexPath];
	}
	[self accessoryForCell:cell atPath:indexPath];
	[self selectToggleButton:(_selectedItems.count != [self totalNumberOfItems])];
}

- (void)tableView:(UITableView *)tableView didDeselectRowAtIndexPath:(NSIndexPath *)indexPath {
	[self toggleRowSelectionForRowAtIndexPath:indexPath];
}

- (void)tableView:(UITableView *)tableView didSelectRowAtIndexPath:(NSIndexPath *)indexPath {
	[self toggleRowSelectionForRowAtIndexPath:indexPath];
}

/* Empty methods allow to not freeze UI...*/
- (void)tableView:(UITableView *)tableView willBeginEditingRowAtIndexPath:(nonnull NSIndexPath *)indexPath {
}

- (void)tableView:(UITableView *)tableView didEndEditingRowAtIndexPath:(NSIndexPath *)indexPath {
}

- (void)selectToggleButton:(BOOL)select {
	_toggleSelectionButton.selected = select;
	if (select) {
		_toggleSelectionButton.accessibilityLabel = NSLocalizedString(@"Select all", nil);
	} else {
		_toggleSelectionButton.accessibilityLabel = NSLocalizedString(@"Deselect all", nil);
	}
}
#pragma mark -

- (void)accessoryForCell:(UITableViewCell *)cell atPath:(NSIndexPath *)indexPath {
	cell.selectionStyle = UITableViewCellSelectionStyleGray;
	if ([self isEditing]) {
		UIButton *checkBoxButton = [UIButton buttonWithType:UIButtonTypeCustom];
		UIImage *image = nil;
		if ([_selectedItems containsObject:indexPath]) {
			image = [UIImage imageNamed:@"checkbox_checked.png"];
			checkBoxButton.accessibilityValue = NSLocalizedString(@"Selected", nil);
		} else {
			image = [UIImage imageNamed:@"checkbox_unchecked.png"];
			checkBoxButton.accessibilityValue = NSLocalizedString(@"Deselected", nil);
		}
		[checkBoxButton setImage:image forState:UIControlStateNormal];
		[checkBoxButton setFrame:CGRectMake(0, 0, 19, 19)];
		[checkBoxButton setBackgroundColor:[UIColor clearColor]];
		checkBoxButton.accessibilityLabel = NSLocalizedString(@"Checkbox", nil);
		checkBoxButton.userInteractionEnabled = NO;
		cell.accessoryView = checkBoxButton;
	} else {
		cell.accessoryView = nil;
		cell.accessoryType = UITableViewCellAccessoryNone;
	}
	_deleteButton.enabled = (_selectedItems.count != 0);
	_editButton.enabled = YES;
}

- (void)setEditing:(BOOL)editing animated:(BOOL)animated {
	[super setEditing:editing animated:animated];

	_editButton.hidden = editing;
	_deleteButton.hidden = _cancelButton.hidden = _toggleSelectionButton.hidden = !editing;
	[self selectToggleButton:YES];

	// when switching editing mode, we must reload all cells to remove/add checkboxes
	[self loadData];
}

- (void)loadData {
	[_selectedItems removeAllObjects];
	[self.tableView reloadData];
	
	_emptyView.hidden = _editButton.enabled = ([self totalNumberOfItems] > 0);
}

- (void)removeSelectionUsing:(void (^)(NSIndexPath *indexPath))remover {
	// we must iterate through selected items in reverse order
	[_selectedItems sortUsingComparator:^(NSIndexPath *obj1, NSIndexPath *obj2) {
	  return [obj2 compare:obj1];
	}];
	NSArray *copy = [[NSArray alloc] initWithArray:_selectedItems];
	for (NSIndexPath *indexPath in copy) {
		if (remover) {
			remover(indexPath);
		} else {
			[self tableView:self.tableView
				commitEditingStyle:UITableViewCellEditingStyleDelete
				 forRowAtIndexPath:indexPath];
		}
	}
	[_selectedItems removeAllObjects];
	[self setEditing:NO animated:YES];
}

- (void)onSelectionToggle:(id)sender {
	[_selectedItems removeAllObjects];

	[self selectToggleButton:!_toggleSelectionButton.selected]; // TODO: why do we need that?
	LOGI(@"onSelectionToggle: select %@", _toggleSelectionButton.selected ? @"NONE" : @"ALL");
	for (int i = 0; i < [self numberOfSectionsInTableView:self.tableView]; i++) {
		for (int j = 0; j < [self tableView:self.tableView numberOfRowsInSection:i]; j++) {
			NSIndexPath *indexPath = [NSIndexPath indexPathForRow:j inSection:i];
			UITableViewCell *cell = [self.tableView cellForRowAtIndexPath:indexPath];
			if (!_toggleSelectionButton.selected) {
				[_selectedItems addObject:indexPath];
				[self.tableView selectRowAtIndexPath:indexPath
											animated:NO
									  scrollPosition:UITableViewScrollPositionNone];
			} else {
				[self.tableView deselectRowAtIndexPath:indexPath animated:NO];
			}

			[self accessoryForCell:cell atPath:indexPath];
		}
	}
}

- (IBAction)onEditClick:(id)sender {
	[self setEditing:YES animated:YES];
}

- (IBAction)onCancelClick:(id)sender {
	[self setEditing:NO animated:YES];
}

- (NSInteger)totalNumberOfItems {
	NSInteger total = 0;
	for (int i = 0; i < [self numberOfSectionsInTableView:self.tableView]; i++) {
		total += [self tableView:self.tableView numberOfRowsInSection:i];
	}
	return total;
}
@end
