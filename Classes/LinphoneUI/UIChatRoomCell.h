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

#import "ChatRoomTableViewController.h"
#import "UILoadingImageView.h"
#import "UITransparentTVCell.h"
#import "UITextViewNoDefine.h"
#include "linphone/linphonecore.h"


@interface UIChatRoomCell : UITransparentTVCell {
    LinphoneChatMessage* chat;
}

@property (nonatomic, strong) IBOutlet UIView *innerView;
@property (nonatomic, strong) IBOutlet UIView *bubbleView;
@property (nonatomic, strong) IBOutlet UIImageView* backgroundImage;
@property (nonatomic, strong) IBOutlet UITextViewNoDefine *messageText;
@property (nonatomic, strong) IBOutlet UILoadingImageView *messageImageView;
@property (nonatomic, strong) IBOutlet UIButton *deleteButton;
@property (nonatomic, strong) IBOutlet UILabel *dateLabel;
@property (nonatomic, strong) IBOutlet UIImageView* statusImage;
@property (nonatomic, strong) IBOutlet UIButton* downloadButton;
@property (nonatomic, strong) IBOutlet UITapGestureRecognizer* imageTapGestureRecognizer;
@property (nonatomic, strong) IBOutlet UITapGestureRecognizer* resendTapGestureRecognizer;

- (id)initWithIdentifier:(NSString*)identifier;
+ (CGFloat)height:(LinphoneChatMessage*)chatMessage width:(int)width;

@property (nonatomic, strong) id<ChatRoomDelegate> chatRoomDelegate;

- (IBAction)onDeleteClick:(id)event;
- (IBAction)onDownloadClick:(id)event;
- (IBAction)onImageClick:(id)event;

- (void)setChatMessage:(LinphoneChatMessage*)message;

@end
