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

@interface CallSideMenuView : UIViewController

@property(weak, nonatomic) IBOutlet UILabel *statsLabel;

- (IBAction)onLateralSwipe:(id)sender;

@end
