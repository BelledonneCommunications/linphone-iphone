//
//  UIChatCreateConfirmCollectionViewCell.h
//  linphone
//
//  Created by REIS Benjamin on 05/10/2017.
//

#import <UIKit/UIKit.h>
#import "UIChatCreateCollectionViewCell.h"
#import "ChatConversationCreateConfirmView.h"

@interface UIChatCreateConfirmCollectionViewCell : UICollectionViewCell
@property (weak, nonatomic) IBOutlet UILabel *displayNameLabel;
@property (strong, nonatomic) ChatConversationCreateConfirmView *confirmController;
@property (strong, nonatomic) NSString *uri;
- (id)initWithName:(NSString *)identifier;
@end
