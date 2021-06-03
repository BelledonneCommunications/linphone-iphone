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

#import "ChatsListView.h"
#import "PhoneMainView.h"

#import "ChatConversationCreateView.h"
@implementation ChatsListView

#pragma mark - ViewController Functions

- (void)viewWillAppear:(BOOL)animated {
	[super viewWillAppear:animated];
	[NSNotificationCenter.defaultCenter addObserver:self
										   selector:@selector(textReceivedEvent:)
											   name:kLinphoneMessageReceived
											 object:nil];
	[NSNotificationCenter.defaultCenter addObserver:self
										   selector:@selector(callUpdateEvent:)
											   name:kLinphoneCallUpdate
											 object:nil];
	[NSNotificationCenter.defaultCenter addObserver:self
										   selector:@selector(ephemeralDeleted:)
											   name:kLinphoneEphemeralMessageDeletedInRoom
											 object:nil];
	[_backToCallButton update];
	self.tableController.waitView = _waitView;
	[self setEditing:NO];
	LinphoneAccount *defaultAccount = linphone_core_get_default_account(LC);
	_addGroupChatButton.hidden = !(defaultAccount && linphone_account_params_get_conference_factory_uri(linphone_account_get_params(defaultAccount)));
	[_toggleSelectionButton setImage:[UIImage imageNamed:@"select_all_default.png"] forState:UIControlStateSelected];

	// For testing crashlytics
    /*UIButton* button = [UIButton buttonWithType:UIButtonTypeRoundedRect];
    button.frame = CGRectMake(20, 50, 100, 30);
    [button setTitle:@"Crash" forState:UIControlStateNormal];
    [button addTarget:self action:@selector(crashButtonTapped:)
        forControlEvents:UIControlEventTouchUpInside];
    [self.view addSubview:button];*/
}

- (void)viewWillDisappear:(BOOL)animated {
	[super viewWillDisappear:animated];

	[NSNotificationCenter.defaultCenter removeObserver:self];
	self.view = NULL;
}

#pragma mark - Event Functions

- (void)textReceivedEvent:(NSNotification *)notif {
	[_tableController loadData];
}

- (void)callUpdateEvent:(NSNotification *)notif {
	[_backToCallButton update];
}

#pragma mark - UICompositeViewDelegate Functions

static UICompositeViewDescription *compositeDescription = nil;

+ (UICompositeViewDescription *)compositeViewDescription {
	if (compositeDescription == nil) {
		compositeDescription = [[UICompositeViewDescription alloc] init:self.class
															  statusBar:StatusBarView.class
																 tabBar:TabBarView.class
															   sideMenu:SideMenuView.class
															 fullscreen:false
														 isLeftFragment:YES
														   fragmentWith:ChatConversationCreateView.class];
	}
	return compositeDescription;
}

- (UICompositeViewDescription *)compositeViewDescription {
	return self.class.compositeViewDescription;
}

#pragma mark - Action Functions

- (void)newChatCreate:(BOOL)isGroup {
    ChatConversationCreateView *view = VIEW(ChatConversationCreateView);
    view.isForEditing = false;
    view.isGroupChat = isGroup;
    view.tableController.notFirstTime = FALSE;
    [view.tableController.contactsGroup removeAllObjects];
    [PhoneMainView.instance changeCurrentView:view.compositeViewDescription];
}

- (IBAction)onAddGroupChatClick:(id)event {
    [self newChatCreate:TRUE];
    if (IPAD)
        [NSNotificationCenter.defaultCenter postNotificationName:kLinphoneChatCreateViewChange object:VIEW(ChatConversationCreateView) userInfo:nil];
}

- (IBAction)onAddClick:(id)event {
	[self newChatCreate:FALSE];
    if (IPAD)
        [NSNotificationCenter.defaultCenter postNotificationName:kLinphoneChatCreateViewChange object:VIEW(ChatConversationCreateView) userInfo:nil];
}

- (IBAction)onEditionChangeClick:(id)sender {
	_addButton.hidden = _addGroupChatButton.hidden = self.tableController.isEditing;
	[_backToCallButton update];
}

- (IBAction)onDeleteClick:(id)sender {
	BOOL group = false;
	NSArray *copy = [[NSArray alloc] initWithArray:_tableController.selectedItems];
	for (NSIndexPath *indexPath in copy) {
		LinphoneChatRoom *chatRoom = (LinphoneChatRoom *)bctbx_list_nth_data(_tableController.data, (int)[indexPath row]);
		if (LinphoneChatRoomCapabilitiesConference & linphone_chat_room_get_capabilities(chatRoom)) {
			group = true;
			break;
		}
	}
	NSString *msg = group
		? [NSString stringWithFormat:NSLocalizedString(@"Do you want to leave and delete the selected conversations?", nil)]
		: [NSString stringWithFormat:NSLocalizedString(@"Do you want to delete the selected conversations?", nil)];
	[UIConfirmationDialog ShowWithMessage:msg
		cancelMessage:nil
		confirmMessage:nil
		onCancelClick:^() {
		  [self onEditionChangeClick:nil];
		}
		onConfirmationClick:^() {
		  [_tableController removeSelectionUsing:nil];
		  [self onEditionChangeClick:nil];
		}];
}

- (IBAction)crashButtonTapped:(id)sender {
    assert(NO);
}

- (void)ephemeralDeleted:(NSNotification *)notif {
	[self.tableController loadData];
}

@end
