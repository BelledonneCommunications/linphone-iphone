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

@property(strong, nonatomic) IBOutlet UISwipeGestureRecognizer *swipeGestureRecognizer;
@property(weak, nonatomic) IBOutlet UIRoundedImageView *avatarImage;
@property(weak, nonatomic) IBOutlet UILabel *nameLabel;
@property(weak, nonatomic) IBOutlet UILabel *addressLabel;
@property(weak, nonatomic) IBOutlet UIImageView *presenceImage;
@property(strong, nonatomic) IBOutlet SideMenuTableView *sideMenuTableViewController;
@property(weak, nonatomic) IBOutlet UIView *grayBackground;
- (IBAction)onLateralSwipe:(id)sender;
- (IBAction)onHeaderClick:(id)sender;
- (IBAction)onAvatarClick:(id)sender;
- (IBAction)onBackgroundClicked:(id)sender;

@end
