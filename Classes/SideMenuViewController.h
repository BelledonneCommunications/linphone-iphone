//
//  SideMenuViewController.h
//  linphone
//
//  Created by Gautier Pelloux-Prayer on 28/07/15.
//
//

#import <UIKit/UIKit.h>

#import "SideMenuTableViewController.h"

@interface SideMenuViewController : UIViewController
@property(weak, nonatomic) IBOutlet UIImageView *avatarImage;
@property(weak, nonatomic) IBOutlet UILabel *nameLabel;
@property(weak, nonatomic) IBOutlet UILabel *addressLabel;
@property(strong, nonatomic) IBOutlet SideMenuTableViewController *sideMenuTableViewController;
- (IBAction)onLateralSwipe:(id)sender;
- (IBAction)onHeaderClick:(id)sender;

@end
