//
//  SideMenuViewController.h
//  linphone
//
//  Created by Gautier Pelloux-Prayer on 28/07/15.
//
//

#import <UIKit/UIKit.h>

#import "SideMenuTableView.h"
#import "PhoneMainView.h"

@interface SideMenuView : UIViewController <ImagePickerDelegate>

@property(weak, nonatomic) IBOutlet UIRoundedImageView *avatarImage;
@property(weak, nonatomic) IBOutlet UILabel *nameLabel;
@property(weak, nonatomic) IBOutlet UIButton *addressButton;
@property(strong, nonatomic) IBOutlet SideMenuTableView *sideMenuTableViewController;
- (IBAction)onLateralSwipe:(id)sender;
- (IBAction)onHeaderClick:(id)sender;
- (IBAction)onAvatarClick:(id)sender;

@end
