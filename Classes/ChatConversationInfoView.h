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

#import "UICompositeView.h"
#import "UIRoundBorderedButton.h"

@interface ChatConversationInfoView : UIViewController <UICompositeViewDelegate, UIGestureRecognizerDelegate, UITextFieldDelegate, UITableViewDelegate, UITableViewDataSource>

@property(nonatomic) BOOL create;
@property(nonatomic) BOOL imAdmin;
@property(nonatomic) BOOL encrypted;
@property(nonatomic, strong) NSMutableArray *contacts;
@property(nonatomic, strong) NSMutableArray *admins;
@property(nonatomic, strong) NSMutableArray *oldContacts;
@property(nonatomic, strong) NSMutableArray *oldAdmins;
@property(nonatomic) NSString *oldSubject;
@property(nonatomic) LinphoneChatRoom *room;
@property(nonatomic) LinphoneChatRoomCbs *chatRoomCbs;

@property (weak, nonatomic) IBOutlet UIIconButton *nextButton;
@property (weak, nonatomic) IBOutlet UIRoundBorderedButton *quitButton;
@property (weak, nonatomic) IBOutlet UIIconButton *addButton;
@property (weak, nonatomic) IBOutlet UITextField *nameLabel;
@property (weak, nonatomic) IBOutlet UITableView *tableView;
@property (weak, nonatomic) IBOutlet UIView *waitView;
@property (weak, nonatomic) IBOutlet UIView *participantsBar;

+ (void)displayCreationError;

- (IBAction)onNextClick:(id)sender;
- (IBAction)onBackClick:(id)sender;
- (IBAction)onQuitClick:(id)sender;

- (void)removeCallbacks;

@end
