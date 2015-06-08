//
//  InAppProductsViewController.h
//  linphone
//
//  Created by Gautier Pelloux-Prayer on 15/04/15.
//
//

#import <UIKit/UIKit.h>
#import "UICompositeViewController.h"
#import "InAppProductsTableViewController.h"

@interface InAppProductsViewController : UIViewController<UICompositeViewDelegate> {
}

@property (nonatomic, strong) IBOutlet InAppProductsTableViewController* tableController;
@property (strong, nonatomic) IBOutlet UIView *waitView;
- (IBAction)onRestoreClicked:(UIButton *)sender;

@end
