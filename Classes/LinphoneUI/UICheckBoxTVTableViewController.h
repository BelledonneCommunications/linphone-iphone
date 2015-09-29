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

- (void)loadData;
- (void)accessoryForCell:(UITableViewCell *)cell atPath:(NSIndexPath *)indexPath;

@end
