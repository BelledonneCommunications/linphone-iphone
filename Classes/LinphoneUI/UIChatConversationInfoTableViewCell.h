//
//  UIChatConversationInfoTableViewCell.h
//  linphone
//
//  Created by REIS Benjamin on 23/10/2017.
//

#import <UIKit/UIKit.h>
#import "ChatConversationInfoView.h"

@interface UIChatConversationInfoTableViewCell : UITableViewCell <UIGestureRecognizerDelegate>

@property (weak, nonatomic) IBOutlet UIRoundedImageView *avatarImage;
@property (weak, nonatomic) IBOutlet UIIconButton *removeButton;
@property (weak, nonatomic) IBOutlet UIView *adminButton;
@property (weak, nonatomic) IBOutlet UILabel *adminLabel;
@property (weak, nonatomic) IBOutlet UIImageView *adminImage;
@property (weak, nonatomic) IBOutlet UILabel *nameLabel;
@property (weak, nonatomic) ChatConversationInfoView *controllerView;
@property (strong) NSString *uri;

- (id)initWithIdentifier:(NSString *)identifier;
@end
