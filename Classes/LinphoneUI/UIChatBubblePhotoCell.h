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

#import "UILoadingImageView.h"
#import "UITextViewNoDefine.h"
#import "FileTransferDelegate.h"
#import "ChatConversationTableView.h"
#import "UIChatBubbleTextCell.h"

@interface UIChatBubblePhotoCell : UIChatBubbleTextCell

@property(nonatomic, strong) IBOutlet UILoadingImageView *messageImageView;
@property(nonatomic, strong) IBOutlet UIButton *downloadButton;
@property(weak, nonatomic) IBOutlet UIProgressView *fileTransferProgress;
@property(weak, nonatomic) IBOutlet UIButton *cancelButton;
@property(weak, nonatomic) IBOutlet UIView *imageSubView;
@property(weak, nonatomic) IBOutlet UIView *totalView;
@property(strong, nonatomic) IBOutlet UITapGestureRecognizer *imageGestureRecognizer;

- (void)setChatMessage:(LinphoneChatMessage *)message;
- (void)connectToFileDelegate:(FileTransferDelegate *)ftd;
- (IBAction)onDownloadClick:(id)event;
- (IBAction)onImageClick:(id)event;
- (IBAction)onCancelClick:(id)sender;
- (IBAction)onResendClick:(id)event;

@end
