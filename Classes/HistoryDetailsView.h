/* HistoryDetailsViewController.h
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
#include "linphone/linphonecore.h"

#import <AddressBook/AddressBook.h>
#import "UICompositeView.h"
#import "HistoryDetailsTableView.h"
#import "UIRoundedImageView.h"

@interface HistoryDetailsView : TPMultiLayoutViewController <UICompositeViewDelegate> {
  @private
	LinphoneCallLog *callLog;
}
@property(weak, nonatomic) IBOutlet UIButton *backButton;
@property(weak, nonatomic) IBOutlet UILabel *contactLabel;
@property(nonatomic, strong) IBOutlet UIRoundedImageView *avatarImage;
@property(nonatomic, strong) IBOutlet UILabel *addressLabel;
@property(nonatomic, strong) IBOutlet UIButton *addContactButton;
@property(nonatomic, copy, setter=setCallLogId:) NSString *callLogId;
@property(weak, nonatomic) IBOutlet UIView *headerView;
@property(strong, nonatomic) IBOutlet HistoryDetailsTableView *tableView;
@property(weak, nonatomic) IBOutlet UILabel *emptyLabel;

- (IBAction)onBackClick:(id)event;
- (IBAction)onAddContactClick:(id)event;
- (IBAction)onCallClick:(id)event;
- (IBAction)onChatClick:(id)event;
- (void)setCallLogId:(NSString *)acallLogId;

@end
