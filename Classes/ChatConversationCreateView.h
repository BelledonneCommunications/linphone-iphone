//
//  ChatConversationCreateViewViewController.h
//  linphone
//
//  Created by Gautier Pelloux-Prayer on 12/10/15.
//
//

#import <UIKit/UIKit.h>
#import "ChatConversationCreateTableView.h"
#import "UICompositeView.h"

@interface ChatConversationCreateView : UIViewController <UICompositeViewDelegate, UIGestureRecognizerDelegate>

@property(strong, nonatomic) IBOutlet ChatConversationCreateTableView *tableController;
@property(weak, nonatomic) IBOutlet UIIconButton *backButton;

- (IBAction)onBackClick:(id)sender;

@end
