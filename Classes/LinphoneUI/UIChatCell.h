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

#import "UIRoundedImageView.h"
#import "UIIconButton.h"
#import "UIBouncingView.h"

#include "linphone/linphonecore.h"

@interface UIChatCell : UITableViewCell {
	LinphoneChatRoom *chatRoom;
}

@property(nonatomic, strong) IBOutlet UIRoundedImageView *avatarImage;
@property (weak, nonatomic) IBOutlet UIImageView *securityImage;
@property(nonatomic, strong) IBOutlet UILabel *addressLabel;
@property(nonatomic, strong) IBOutlet UILabel *chatContentLabel;
@property(weak, nonatomic) IBOutlet UILabel *chatLatestTimeLabel;
@property(weak, nonatomic) IBOutlet UIBouncingView *unreadCountView;
@property(weak, nonatomic) IBOutlet UILabel *unreadCountLabel;
@property (weak, nonatomic) IBOutlet UIImageView *imdmIcon;

- (id)initWithIdentifier:(NSString*)identifier;

- (IBAction)onDeleteClick:(id)event;
- (void)updateUnreadBadge;
- (void)setChatRoom:(LinphoneChatRoom *)achat;
@end
