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

@property(nonatomic) LinphoneChatMessage *msg;
@property(nonatomic) bctbx_list_t *displayedList;
@property(nonatomic) bctbx_list_t *receivedList;
@property(nonatomic) bctbx_list_t *notReceivedList;
@property(nonatomic) CGFloat height;

@property (weak, nonatomic) IBOutlet UIView *whiteView;
@property (weak, nonatomic) IBOutlet UIView *msgView;
@property (weak, nonatomic) IBOutlet UIImageView *msgBackgroundColorImage;
@property (weak, nonatomic) IBOutlet UIRoundedImageView *msgAvatarImage;
@property (weak, nonatomic) IBOutlet UIImageView *msgBottomBar;
@property (weak, nonatomic) IBOutlet UILabel *msgDateLabel;
@property (weak, nonatomic) IBOutlet UITextViewNoDefine *msgText;
@property (weak, nonatomic) IBOutlet UIView *readHeader;
@property (weak, nonatomic) IBOutlet UIView *deliveredHeader;
@property (weak, nonatomic) IBOutlet UIView *undeliveredHeader;
@property (weak, nonatomic) IBOutlet UIScrollView *scrollView;

- (IBAction)onBackClick:(id)sender;

@end

#endif /* ChatConversationImdnView_h */
