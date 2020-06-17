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

#import "UIToggleButton.h"

#import "ChatsListTableView.h"
#import "UICompositeView.h"
#import "UIBackToCallButton.h"

@interface ChatsListView : UIViewController <UICompositeViewDelegate> {
}

@property(nonatomic, strong) IBOutlet ChatsListTableView *tableController;
@property(weak, nonatomic) IBOutlet UIButton *addButton;
@property (weak, nonatomic) IBOutlet UIButton *addGroupChatButton;
@property(weak, nonatomic) IBOutlet UIBackToCallButton *backToCallButton;
@property (weak, nonatomic) IBOutlet UIView *waitView;
@property (weak, nonatomic) IBOutlet UIInterfaceStyleButton *toggleSelectionButton;

- (IBAction)onAddGroupChatClick:(id)event;
- (IBAction)onAddClick:(id)event;
- (IBAction)onEditionChangeClick:(id)sender;
- (IBAction)onDeleteClick:(id)sender;

@end
