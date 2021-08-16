//
//  UIChatReplyBubbleView.h
//  
//
//  Created by CD on 26/07/2021.
//

#import <UIKit/UIKit.h>

NS_ASSUME_NONNULL_BEGIN

@interface UIChatReplyBubbleView : UIViewController
@property (weak, nonatomic) IBOutlet UILabel *senderNameWithTextContent;
@property (weak, nonatomic) IBOutlet UIButton *dismissButton;
@property (weak, nonatomic) IBOutlet UIView *leftBar;
@property (weak, nonatomic) IBOutlet UIView *rightBar;
@property (weak, nonatomic) IBOutlet UILabel *senderNameWithoutTextContent;
@property LinphoneChatMessage *message;
@property (weak, nonatomic) IBOutlet UILabel *textContent;
@property void (^ dismissAction)(void);
@property void (^ clickAction)(void);
@property (weak, nonatomic) IBOutlet UIImageView *dataContent;

-(void) configureForMessage:(LinphoneChatMessage *)message withDimissBlock:(void (^)(void))dismissBlock hideDismiss:(BOOL)hideDismiss withClickBlock:(void (^)(void))clickBlock;
@end

NS_ASSUME_NONNULL_END
