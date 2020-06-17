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

#pragma once

#import <UIKit/UIKit.h>

@interface UICheckBoxTableView : UITableViewController

@property(nonatomic, readonly) NSMutableArray *selectedItems;
@property(weak, nonatomic) IBOutlet UIButton *deleteButton;
@property(weak, nonatomic) IBOutlet UIButton *editButton;
@property(weak, nonatomic) IBOutlet UIButton *cancelButton;
@property(weak, nonatomic) IBOutlet UIButton *toggleSelectionButton;
@property(weak, nonatomic) IBOutlet UIView *emptyView;

- (void)loadData;
- (void)accessoryForCell:(UITableViewCell *)cell atPath:(NSIndexPath *)indexPath;
- (void)removeSelectionUsing:(void (^)(NSIndexPath *indexPath))remover;

- (BOOL)selectFirstRow;

- (IBAction)onSelectionToggle:(id)sender;
- (IBAction)onEditClick:(id)sender;
- (IBAction)onCancelClick:(id)sender;

- (NSInteger)totalNumberOfItems;

@end
