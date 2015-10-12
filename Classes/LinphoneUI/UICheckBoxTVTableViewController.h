//
//  UICheckBoxTVTableViewController.h
//  linphone
//
//  Created by Gautier Pelloux-Prayer on 29/09/15.
//
//

#pragma once

#import <UIKit/UIKit.h>

@interface UICheckBoxTVTableViewController : UITableViewController

@property(nonatomic, readonly) NSMutableArray *selectedItems;
@property(weak, nonatomic) IBOutlet UIButton *deleteButton;
@property(weak, nonatomic) IBOutlet UIButton *editButton;
@property(weak, nonatomic) IBOutlet UIButton *cancelButton;
@property(weak, nonatomic) IBOutlet UIButton *toggleSelectionButton;

- (void)loadData;
- (void)accessoryForCell:(UITableViewCell *)cell atPath:(NSIndexPath *)indexPath;
- (void)removeSelection;

- (IBAction)onSelectionToggle:(id)sender;
- (IBAction)onEditClick:(id)sender;
- (IBAction)onCancelClick:(id)sender;

@end
