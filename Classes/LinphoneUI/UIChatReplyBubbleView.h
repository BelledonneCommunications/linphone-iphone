//
//  UIChatReplyBubbleView.h
//  
//
//  Created by CD on 26/07/2021.
//

#import <UIKit/UIKit.h>

NS_ASSUME_NONNULL_BEGIN

@interface UIChatReplyBubbleView : UIViewController <UICollectionViewDataSource>
@property (weak, nonatomic) IBOutlet UILabel *senderName;
@property (weak, nonatomic) IBOutlet UIButton *dismissButton;
@property (weak, nonatomic) IBOutlet UIView *leftBar;
@property (weak, nonatomic) IBOutlet UIView *rightBar;
@property LinphoneChatMessage *message;
@property (weak, nonatomic) IBOutlet UILabel *textContent;
@property void (^ dismissAction)(void);
@property void (^ clickAction)(void);
@property (weak, nonatomic) IBOutlet UICollectionView *contentCollection;
@property NSArray *dataContent;

-(void) configureForMessage:(LinphoneChatMessage *)message withDimissBlock:(void (^)(void))dismissBlock hideDismiss:(BOOL)hideDismiss withClickBlock:(void (^)(void))clickBlock;
@end

NS_ASSUME_NONNULL_END
