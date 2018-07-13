//
//  UIChatBubbleSoundCell.h
//  linphone
//
//  Created by David Idmansour on 02/07/2018.
//

#import "UIChatBubbleTextCell.h"

#import "UILoadingImageView.h"
#import "UITextViewNoDefine.h"
#import "FileTransferDelegate.h"
#import "ChatConversationTableView.h"
#import "UIChatBubbleTextCell.h"
#import "UILinphoneAudioPlayer.h"

@interface UIChatBubbleSoundCell : UIChatBubbleTextCell
@property (weak, nonatomic) IBOutlet UIView *playerView;
@property (weak, nonatomic) IBOutlet UIButton *loadButton;
@property (weak, nonatomic) IBOutlet UIView *content;

- (IBAction)onLoad:(id)sender;

@end
