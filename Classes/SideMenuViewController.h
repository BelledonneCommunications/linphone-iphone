//
//  SideMenuViewController.h
//  linphone
//
//  Created by Gautier Pelloux-Prayer on 28/07/15.
//
//

#import <UIKit/UIKit.h>

#import "SideMenuTableViewController.h"
#import "PhoneMainView.h"

@interface SideMenuViewController : UIViewController <ImagePickerDelegate>

@property(weak, nonatomic) IBOutlet UIRoundedImageView *avatarImage;
@property(weak, nonatomic) IBOutlet UILabel *nameLabel;
@property(weak, nonatomic) IBOutlet UILabel *addressLabel;
@property(strong, nonatomic) IBOutlet SideMenuTableViewController *sideMenuTableViewController;
- (IBAction)onLateralSwipe:(id)sender;
- (IBAction)onHeaderClick:(id)sender;
- (IBAction)onAvatarClick:(id)sender;

@end
