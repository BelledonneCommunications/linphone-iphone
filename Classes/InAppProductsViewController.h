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

@property (nonatomic, retain) IBOutlet InAppProductsTableViewController* tableController;
- (IBAction)onRestoreClicked:(UIButton *)sender;

@end
