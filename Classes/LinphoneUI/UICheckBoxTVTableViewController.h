//
//  UICheckBoxTVTableViewController.h
//  linphone
//
//  Created by Gautier Pelloux-Prayer on 29/09/15.
//
//

#import <UIKit/UIKit.h>

@interface UICheckBoxTVTableViewController : UITableViewController

@property(nonatomic, readonly) NSMutableArray *selectedItems;
@property(weak, nonatomic) IBOutlet UIButton *deleteButton;
@property(weak, nonatomic) IBOutlet UIButton *editButton;

- (void)loadData;
- (void)accessoryForCell:(UITableViewCell *)cell atPath:(NSIndexPath *)indexPath;
- (void)removeSelection;

- (IBAction)onSelectionToggle:(id)sender;

@end
