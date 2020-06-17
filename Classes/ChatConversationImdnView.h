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

#ifndef ChatConversationImdnView_h
#define ChatConversationImdnView_h

#import <UIKit/UIKit.h>

#import "UICompositeView.h"
#import "UIRoundBorderedButton.h"

@interface ChatConversationImdnView : UIViewController <UICompositeViewDelegate, UITableViewDelegate, UITableViewDataSource>
{
  @private
    NSString *messageText;
}

@property(nonatomic) LinphoneChatMessage *msg;
@property(nonatomic) bctbx_list_t *displayedList;
@property(nonatomic) bctbx_list_t *receivedList;
@property(nonatomic) bctbx_list_t *notReceivedList;
@property(nonatomic) bctbx_list_t *errorList;

@property (weak, nonatomic) IBOutlet UIView *msgView;
@property (weak, nonatomic) IBOutlet UIImageView *msgBackgroundColorImage;
@property (weak, nonatomic) IBOutlet UIRoundedImageView *msgAvatarImage;
@property (weak, nonatomic) IBOutlet UIImageView *msgBottomBar;
@property (weak, nonatomic) IBOutlet UILabel *msgDateLabel;
@property (weak, nonatomic) IBOutlet UITextViewNoDefine *msgText;
@property (weak, nonatomic) IBOutlet UITableView *tableView;

- (IBAction)onBackClick:(id)sender;
- (void)updateImdnList;

@end

#endif /* ChatConversationImdnView_h */
