//
//  UIBackToCallButton.h
//  linphone
//
//  Created by Gautier Pelloux-Prayer on 26/10/15.
//
//

#import "UIIconButton.h"

@interface UIBackToCallButton : UIIconButton

- (IBAction)onBackToCallClick:(id)sender;
- (void)update;

@property(assign, nonatomic) IBOutlet UITableView *tableView;
@end
