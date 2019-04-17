//
//  ChatConversationImdnView.h
//  linphone
//
//  Created by REIS Benjamin on 25/04/2018.
//

#ifndef ChatConversationImdnView_h
#define ChatConversationImdnView_h

#import <UIKit/UIKit.h>

#import "UICompositeView.h"
#import "UIRoundBorderedButton.h"

@interface ChatConversationImdnView : UIViewController <UICompositeViewDelegate, UITableViewDelegate, UITableViewDataSource>
{
  @private
    NSString *messageText;
}

@property(nonatomic) LinphoneChatMessage *msg;
@property(nonatomic) bctbx_list_t *displayedList;
@property(nonatomic) bctbx_list_t *receivedList;
@property(nonatomic) bctbx_list_t *notReceivedList;
@property(nonatomic) bctbx_list_t *errorList;

@property (weak, nonatomic) IBOutlet UIView *msgView;
@property (weak, nonatomic) IBOutlet UIImageView *msgBackgroundColorImage;
@property (weak, nonatomic) IBOutlet UIRoundedImageView *msgAvatarImage;
@property (weak, nonatomic) IBOutlet UIImageView *msgBottomBar;
@property (weak, nonatomic) IBOutlet UILabel *msgDateLabel;
@property (weak, nonatomic) IBOutlet UITextViewNoDefine *msgText;
@property (weak, nonatomic) IBOutlet UITableView *tableView;

- (IBAction)onBackClick:(id)sender;
- (void)updateImdnList;

@end

#endif /* ChatConversationImdnView_h */
