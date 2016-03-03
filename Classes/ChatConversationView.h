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
#import "UICompositeView.h"
#import "ChatConversationTableView.h"
#import "ImagePickerView.h"
#import "OrderedDictionary.h"
#import "UIRoundedImageView.h"
#import "UIBackToCallButton.h"
#import "Utils/HPGrowingTextView/HPGrowingTextView.h"

#include "linphone/linphonecore.h"

@interface ChatConversationView
	: TPMultiLayoutViewController <HPGrowingTextViewDelegate, UICompositeViewDelegate, ImagePickerDelegate,
								   ChatConversationDelegate, UISearchBarDelegate> {
	OrderedDictionary *imageQualities;
	BOOL scrollOnGrowingEnabled;
	BOOL composingVisible;
}

@property(nonatomic) LinphoneChatRoom *chatRoom;

@property(weak, nonatomic) IBOutlet UIIconButton *backButton;
@property(nonatomic, strong) IBOutlet ChatConversationTableView *tableController;
@property(weak, nonatomic) IBOutlet HPGrowingTextView *messageField;
@property(weak, nonatomic) IBOutlet UIView *topBar;
@property(nonatomic, strong) IBOutlet UIButton *sendButton;
@property(nonatomic, strong) IBOutlet UILabel *addressLabel;
@property(nonatomic, strong) IBOutlet UIView *chatView;
@property(nonatomic, strong) IBOutlet UIView *messageView;
@property(nonatomic, strong) IBOutlet UITapGestureRecognizer *listTapGestureRecognizer;
@property(nonatomic, strong) IBOutlet UISwipeGestureRecognizer *listSwipeGestureRecognizer;
@property(strong, nonatomic) IBOutlet UILabel *composeLabel;
@property(strong, nonatomic) IBOutlet UIView *composeIndicatorView;
@property(nonatomic, strong) IBOutlet UIButton *pictureButton;
@property(weak, nonatomic) IBOutlet UIIconButton *callButton;
@property(weak, nonatomic) IBOutlet UIBackToCallButton *backToCallButton;

- (IBAction)onBackClick:(id)event;
- (IBAction)onEditClick:(id)event;
- (IBAction)onMessageChange:(id)sender;
- (IBAction)onSendClick:(id)event;
- (IBAction)onPictureClick:(id)event;
- (IBAction)onListTap:(id)sender;
- (IBAction)onCallClick:(id)sender;
- (IBAction)onDeleteClick:(id)sender;
- (IBAction)onEditionChangeClick:(id)sender;

@end
