//
//  UIChatConversationImdnTableViewCell.h
//  linphone
//
//  Created by REIS Benjamin on 25/04/2018.
//

#ifndef UIChatConversationImdnTableViewCell_h
#define UIChatConversationImdnTableViewCell_h

@interface UIChatConversationImdnTableViewCell : UITableViewCell

@property (weak, nonatomic) IBOutlet UIRoundedImageView *avatar;
@property (weak, nonatomic) IBOutlet UILabel *displayName;
@property (weak, nonatomic) IBOutlet UILabel *dateLabel;
- (id)initWithIdentifier:(NSString *)identifier;
@end

#endif /* UIChatConversationImdnTableViewCell_h */
