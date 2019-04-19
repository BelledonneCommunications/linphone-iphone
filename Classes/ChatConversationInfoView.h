//
//  ChatConversationInfoView.h
//  linphone
//
//  Created by REIS Benjamin on 23/10/2017.
//

#import <UIKit/UIKit.h>

#import "UICompositeView.h"
#import "UIRoundBorderedButton.h"

@interface ChatConversationInfoView : UIViewController <UICompositeViewDelegate, UIGestureRecognizerDelegate, UITextFieldDelegate, UITableViewDelegate, UITableViewDataSource>

@property(nonatomic) BOOL create;
@property(nonatomic) BOOL imAdmin;
@property(nonatomic) BOOL encrypted;
@property(nonatomic, strong) NSMutableArray *contacts;
@property(nonatomic, strong) NSMutableArray *admins;
@property(nonatomic, strong) NSMutableArray *oldContacts;
@property(nonatomic, strong) NSMutableArray *oldAdmins;
@property(nonatomic) NSString *oldSubject;
@property(nonatomic) LinphoneChatRoom *room;
@property(nonatomic) LinphoneChatRoomCbs *chatRoomCbs;

@property (weak, nonatomic) IBOutlet UIIconButton *nextButton;
@property (weak, nonatomic) IBOutlet UIRoundBorderedButton *quitButton;
@property (weak, nonatomic) IBOutlet UIIconButton *addButton;
@property (weak, nonatomic) IBOutlet UITextField *nameLabel;
@property (weak, nonatomic) IBOutlet UITableView *tableView;
@property (weak, nonatomic) IBOutlet UIView *waitView;
@property (weak, nonatomic) IBOutlet UIView *participantsBar;

+ (void)displayCreationError;

- (IBAction)onNextClick:(id)sender;
- (IBAction)onBackClick:(id)sender;
- (IBAction)onQuitClick:(id)sender;

@end
