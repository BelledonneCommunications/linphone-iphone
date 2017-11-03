//
//  UIChatNotifiedEventCell.h
//  linphone
//
//  Created by REIS Benjamin on 30/10/2017.
//

#ifndef UIChatNotifiedEventCell_h
#define UIChatNotifiedEventCell_h

#import <UIKit/UIKit.h>

#import "ChatConversationTableView.h"

@interface UIChatNotifiedEventCell : UITableViewCell

@property(readonly, nonatomic) LinphoneEventLog *event;
@property (weak, nonatomic) IBOutlet UILabel *contactDateLabel;
@property (weak, nonatomic) IBOutlet UIImageView *rightBar;
@property (weak, nonatomic) IBOutlet UIImageView *leftBar;
@property (weak, nonatomic) IBOutlet UITableViewCell *eventView;

+ (CGFloat)height;

- (void)setEvent:(LinphoneEventLog *)event;

@end

#endif /* UIChatNotifiedEventCell_h */
