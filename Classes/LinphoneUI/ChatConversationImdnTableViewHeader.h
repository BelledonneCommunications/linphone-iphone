//
//  ChatConversationImdnTableViewHeader.h
//  linphone
//
//  Created by REIS Benjamin on 26/04/2018.
//

#ifndef ChatConversationImdnTableViewHeader_h
#define ChatConversationImdnTableViewHeader_h

@interface ChatConversationImdnTableViewHeader : UITableViewHeaderFooterView

@property (weak, nonatomic) IBOutlet UILabel *label;
@property (weak, nonatomic) IBOutlet UIIconButton *icon;

- (id)initWithIdentifier:(NSString *)identifier;

@end
#endif /* ChatConversationImdnTableViewHeader_h */
