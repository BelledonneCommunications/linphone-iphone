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

#import "UILoadingImageView.h"
#import "UITextViewNoDefine.h"
#import "FileTransferDelegate.h"
#import "ChatConversationTableView.h"
#import "UIChatBubbleTextCell.h"
#import "UIChatContentView.h"

@interface UIChatBubblePhotoCell : UIChatBubbleTextCell

@property(nonatomic, strong) IBOutlet UILoadingImageView *messageImageView;
@property(nonatomic, strong) IBOutlet UIButton *downloadButton;
@property (weak, nonatomic) IBOutlet UILabel *fileName;
@property(nonatomic, strong) IBOutlet UIButton *playButton;
@property(weak, nonatomic) IBOutlet UIProgressView *fileTransferProgress;
@property(weak, nonatomic) IBOutlet UIButton *cancelButton;
@property(weak, nonatomic) IBOutlet UIView *imageSubView;
@property(weak, nonatomic) IBOutlet UIView *totalView;
@property (weak, nonatomic) IBOutlet UIView *finalAssetView;
@property (weak, nonatomic) IBOutlet UIImageView *finalImage;
@property(strong, nonatomic) IBOutlet UITapGestureRecognizer *imageGestureRecognizer;
@property (weak, nonatomic) IBOutlet UIButton *fileButton;
@property (weak, nonatomic) IBOutlet UIView *fileView;
@property (strong, nonatomic) IBOutlet UILongPressGestureRecognizer *plusLongGestureRecognizer;
@property(strong, nonatomic) NSMutableArray<UIChatContentView *> *contentViews;


- (void)setEvent:(LinphoneEventLog *)event vfsEnabled:(BOOL)enabled;
- (void)setChatMessage:(LinphoneChatMessage *)message;
- (void)connectToFileDelegate:(FileTransferDelegate *)ftd;
- (IBAction)onDownloadClick:(id)event;
- (IBAction)onImageClick:(id)event;
- (IBAction)onCancelClick:(id)sender;
- (IBAction)onResendClick:(id)event;
- (IBAction)onPlayClick:(id)sender;
- (IBAction)onFileClick:(id)sender;
- (IBAction)onPlusClick:(id)sender;

@end


