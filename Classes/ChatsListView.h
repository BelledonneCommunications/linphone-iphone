/* ChatViewController.h
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

- (IBAction)onAddGroupChatClick:(id)event;
- (IBAction)onAddClick:(id)event;
- (IBAction)onEditionChangeClick:(id)sender;
- (IBAction)onDeleteClick:(id)sender;

@end
