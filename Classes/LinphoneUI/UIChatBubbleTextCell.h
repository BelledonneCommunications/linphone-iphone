/*
 * Copyright (c) 2010-2020 Belledonne Communications SARL.
 *
 * This file is part of linphone-iphone 
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 */

#import <UIKit/UIKit.h>

#import "UITextViewNoDefine.h"
#import "ChatConversationTableView.h"
#import "UIRoundedImageView.h"
#import "UIChatReplyBubbleView.h"

#define CELL_IMAGE_X_MARGIN 100
#define IMAGE_DEFAULT_WIDTH 120
#define IMAGE_DEFAULT_MARGIN 5
#define VOICE_RECORDING_PLAYER_HEIGHT 60
#define VOICE_RECORDING_PLAYER_WIDTH 300
#define CONFERENCE_INVITATION_HEIGHT 210
#define CONFERENCE_INVITATION_WIDTH 300



@interface UIChatBubbleTextCell : UITableViewCell <UIDocumentPickerDelegate, UITableViewDataSource,UITableViewDelegate>

@property(readonly, nonatomic) LinphoneEventLog *event;
@property(readonly, nonatomic) LinphoneChatMessage *message;
@property(nonatomic, weak) IBOutlet UIImageView *backgroundColorImage;
@property(nonatomic, weak) IBOutlet UIRoundedImageView *avatarImage;
@property(nonatomic, weak) IBOutlet UILabel *contactDateLabel;
//@property(weak, nonatomic) IBOutlet UIActivityIndicatorView *statusInProgressSpinner;
@property(nonatomic, weak) IBOutlet UITextViewNoDefine *messageText;
//@property(weak, nonatomic) IBOutlet UIImageView *bottomBarColor;
@property(nonatomic, strong) id<ChatConversationDelegate> chatRoomDelegate;
@property(strong, nonatomic) IBOutlet UIView *bubbleView;
@property(strong, nonatomic) IBOutlet UITapGestureRecognizer *resendRecognizer;
//@property(weak, nonatomic) IBOutlet UIImageView *LIMEKO;
@property(weak, nonatomic) IBOutlet UIImageView *imdmIcon;
//@property(weak, nonatomic) IBOutlet UILabel *imdmLabel;
@property (nonatomic, strong) UIDocumentPickerViewController *documentPicker;
@property (weak, nonatomic) IBOutlet UIView *innerView;
@property (weak, nonatomic) IBOutlet UILabel *ephemeralTime;
@property (weak, nonatomic) IBOutlet UIImageView *ephemeralIcon;
@property ChatConversationTableView *tableController;
@property BOOL popupMenuAllowed;

// Message popup menu
@property UITableView *popupMenu;
@property NSMutableArray *messageActionsTitles;
@property NSMutableArray *messageActionsIcons;
@property NSMutableArray *messageActionsBlocks;

// Message reply/transfer
@property UIChatReplyBubbleView *replyView;
@property UILabel *replyOrForward;
@property (weak, nonatomic) IBOutlet UIImageView *replyTransferIcon;
@property (weak, nonatomic) IBOutlet UILabel *replyTransferLabel;
@property (weak, nonatomic) IBOutlet UIView *photoCellContentView;
@property UIView *icsBubbleView;


@property(nonatomic) BOOL isFirst;
@property(nonatomic) BOOL isLast;
@property(nonatomic) BOOL notDelivered;

+ (CGSize)ViewSizeForMessage:(LinphoneChatMessage *)chat withWidth:(int)width;
+ (CGSize)ViewHeightForMessageText:(LinphoneChatMessage *)chat withWidth:(int)width textForImdn:(NSString *)imdnText;
+ (CGSize)getMediaMessageSizefromOriginalSize:(CGSize)originalSize withWidth:(int)width;
+ (UIImage *)getImageFromVideoUrl:(NSURL *)url;
+ (UIImage *)getImageFromContent:(LinphoneContent *)content filePath:(NSString *)filePath forReplyBubble:(BOOL)forReplyBubble;
+ (UIImage *)getImageFromFileName:(NSString *)fileName forReplyBubble:(BOOL)forReplyBubbble;


- (void)setEvent:(LinphoneEventLog *)event;
- (void)setChatMessageForCbs:(LinphoneChatMessage *)message;
- (void)clearEncryptedFiles;

- (void)onDelete;
- (void)update;

- (void)displayImdmStatus:(LinphoneChatMessageState)state;
+ (CGSize)ViewHeightForMessage:(LinphoneChatMessage *)chat withWidth:(int)width;
+ (NSString *)TextMessageForChat:(LinphoneChatMessage *)message;
+ (CGSize)computeBoundingBox:(NSString *)text size:(CGSize)size font:(UIFont *)font;
+ (NSString *)ContactDateForChat:(LinphoneChatMessage *)message;
+(LinphoneContent *) voiceContent:(LinphoneChatMessage *)message;
-(void) onPopupMenuPressed;
-(void) dismissPopup;
@end
