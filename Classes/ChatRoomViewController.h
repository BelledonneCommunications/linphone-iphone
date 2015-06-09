/* ChatRoomViewController.h
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
 *  GNU General Public License for more details.                
 *                                                                      
 *  You should have received a copy of the GNU General Public License   
 *  along with this program; if not, write to the Free Software         
 *  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */ 

#import <UIKit/UIKit.h>

#import "UIToggleButton.h"
#import "UICompositeViewController.h"
#import "ChatRoomTableViewController.h"
#import "HPGrowingTextView.h"
#import "ImagePickerViewController.h"
#import "ImageSharing.h"
#import "OrderedDictionary.h"

#include "linphone/linphonecore.h"

@interface ChatRoomViewController : UIViewController<HPGrowingTextViewDelegate, UICompositeViewDelegate, ImagePickerDelegate, ChatRoomDelegate, LinphoneChatContentTransferDelegate> {
    LinphoneChatRoom *chatRoom;
    ImageSharing *imageSharing;
    OrderedDictionary *imageQualities;
    BOOL scrollOnGrowingEnabled;
    BOOL composingVisible;
}

@property (nonatomic, strong) IBOutlet ChatRoomTableViewController* tableController;
@property (nonatomic, strong) IBOutlet UIToggleButton *editButton;
@property (nonatomic, strong) IBOutlet HPGrowingTextView* messageField;
@property (nonatomic, strong) IBOutlet UIButton* sendButton;
@property (nonatomic, strong) IBOutlet UILabel *addressLabel;
@property (nonatomic, strong) IBOutlet UIImageView *avatarImage;
@property (nonatomic, strong) IBOutlet UIView *headerView;
@property (nonatomic, strong) IBOutlet UIView *chatView;
@property (nonatomic, strong) IBOutlet UIView *messageView;
@property (nonatomic, strong) IBOutlet UIImageView *messageBackgroundImage;
@property (nonatomic, strong) IBOutlet UIImageView *transferBackgroundImage;
@property (nonatomic, strong) IBOutlet UITapGestureRecognizer *listTapGestureRecognizer;
@property (nonatomic, strong) IBOutlet UISwipeGestureRecognizer *listSwipeGestureRecognizer;
@property (strong, nonatomic) IBOutlet UILabel *composeLabel;
@property (strong, nonatomic) IBOutlet UIView *composeIndicatorView;

@property (nonatomic, strong) IBOutlet UIButton* pictureButton;
@property (nonatomic, strong) IBOutlet UIButton* cancelTransferButton;
@property (nonatomic, strong) IBOutlet UIProgressView* imageTransferProgressBar;
@property (nonatomic, strong) IBOutlet UIView* transferView;
@property (nonatomic, strong) IBOutlet UIView* waitView;

- (IBAction)onBackClick:(id)event;
- (IBAction)onEditClick:(id)event;
- (IBAction)onMessageChange:(id)sender;
- (IBAction)onSendClick:(id)event;
- (IBAction)onPictureClick:(id)event;
- (IBAction)onTransferCancelClick:(id)event;
- (IBAction)onListTap:(id)sender;

- (void)setChatRoom:(LinphoneChatRoom*)room;

@end
