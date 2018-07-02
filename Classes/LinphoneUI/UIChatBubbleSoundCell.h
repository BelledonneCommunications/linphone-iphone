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

@interface UIChatBubbleSoundCell : UIChatBubbleTextCell
- (IBAction)onPlay:(id)sender;
- (IBAction)onPause:(id)sender;
- (IBAction)onStop:(id)sender;

@end
