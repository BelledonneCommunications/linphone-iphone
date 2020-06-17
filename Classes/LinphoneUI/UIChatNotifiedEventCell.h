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

#ifndef UIChatNotifiedEventCell_h
#define UIChatNotifiedEventCell_h

#import <UIKit/UIKit.h>

#import "ChatConversationTableView.h"

@interface UIChatNotifiedEventCell : UITableViewCell

@property(readonly, nonatomic) LinphoneEventLog *event;
@property (weak, nonatomic) IBOutlet UILabel *contactDateLabel;
@property (weak, nonatomic) IBOutlet UIImageView *rightBar;
@property (weak, nonatomic) IBOutlet UIImageView *leftBar;
@property (weak, nonatomic) IBOutlet UITableViewCell *eventView;

+ (CGFloat)height;

- (void)setEvent:(LinphoneEventLog *)event;

@end

#endif /* UIChatNotifiedEventCell_h */
