/* UIChatRoomCell.h
 *
 * Copyright (C) 2012  Belledonne Comunications, Grenoble, France
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU Library General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */

#import <UIKit/UIKit.h>

#import "UITextViewNoDefine.h"
#import "ChatConversationTableView.h"
#import "UIRoundedImageView.h"

@interface UIChatBubbleTextCell : UITableViewCell

@property(readonly, nonatomic) LinphoneChatMessage *message;
@property(nonatomic, weak) IBOutlet UIImageView *backgroundColorImage;
@property(nonatomic, weak) IBOutlet UIRoundedImageView *avatarImage;
@property(nonatomic, weak) IBOutlet UILabel *contactDateLabel;
@property(nonatomic, weak) IBOutlet UIImageView *statusErrorImage;
@property(weak, nonatomic) IBOutlet UIActivityIndicatorView *statusInProgressSpinner;
@property(nonatomic, weak) IBOutlet UITextViewNoDefine *messageText;
@property(weak, nonatomic) IBOutlet UIImageView *bottomBarColor;
@property(nonatomic, strong) id<ChatConversationDelegate> chatRoomDelegate;
@property(strong, nonatomic) IBOutlet UIView *bubbleView;
@property(strong, nonatomic) IBOutlet UITapGestureRecognizer *resendRecognizer;

+ (CGSize)ViewSizeForMessage:(LinphoneChatMessage *)chat withWidth:(int)width;

- (void)setChatMessage:(LinphoneChatMessage *)message;

- (IBAction)onDeleteClick:(id)event;
- (IBAction)onResendClick:(id)event;
- (void)update;

+ (CGSize)ViewHeightForMessage:(LinphoneChatMessage *)chat withWidth:(int)width;
+ (NSString *)TextMessageForChat:(LinphoneChatMessage *)message;
+ (CGSize)computeBoundingBox:(NSString *)text size:(CGSize)size font:(UIFont *)font;

@end
