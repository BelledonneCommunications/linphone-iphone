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

@interface ChatConversationCreateView : UIViewController <UICompositeViewDelegate>

@property(strong, nonatomic) IBOutlet ChatConversationCreateTableView *tableController;

- (IBAction)onBackClick:(id)sender;

@end
