//
//  UIChatCreateCollectionViewCell.h
//  linphone
//
//  Created by REIS Benjamin on 03/10/2017.
//

#import <UIKit/UIKit.h>
#import "ChatConversationCreateView.h"

@interface UIChatCreateCollectionViewCell : UICollectionViewCell
@property (weak, nonatomic) IBOutlet UILabel *nameLabel;
@property (strong, nonatomic) ChatConversationCreateView *controller;
@property (strong, nonatomic) NSString *uri;
- (id)initWithName:(NSString *)identifier;
- (void)onDelete;
@end
