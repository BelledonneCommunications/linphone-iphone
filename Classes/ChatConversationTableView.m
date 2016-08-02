/* ChatRoomTableViewController.m
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

#import "LinphoneManager.h"
#import "ChatConversationTableView.h"
#import "UIChatBubbleTextCell.h"
#import "UIChatBubblePhotoCell.h"
#import "PhoneMainView.h"

@implementation ChatConversationTableView

#pragma mark - Lifecycle Functions

- (void)dealloc {
	[self clearMessageList];
}

#pragma mark - ViewController Functions

- (void)viewWillAppear:(BOOL)animated {
	[super viewWillAppear:animated];
	self.tableView.accessibilityIdentifier = @"ChatRoom list";
}

#pragma mark -

- (void)clearMessageList {
	messageList = bctbx_list_free_with_data(messageList, (void (*)(void *))linphone_chat_message_unref);
}

- (void)updateData {
	if (!_chatRoom)
		return;
	[self clearMessageList];
	messageList = linphone_chat_room_get_history(_chatRoom, 0);

	// also append transient upload messages because they are not in history yet!
	for (FileTransferDelegate *ftd in [LinphoneManager.instance fileTransferDelegates]) {
		const LinphoneAddress *ftd_peer =
			linphone_chat_room_get_peer_address(linphone_chat_message_get_chat_room(ftd.message));
		const LinphoneAddress *peer = linphone_chat_room_get_peer_address(_chatRoom);
		if (linphone_address_equal(ftd_peer, peer) && linphone_chat_message_is_outgoing(ftd.message)) {
			LOGI(@"Appending transient upload message %p", ftd.message);
			messageList = bctbx_list_append(messageList, linphone_chat_message_ref(ftd.message));
		}
	}
}

- (void)reloadData {
	[self updateData];
	[self.tableView reloadData];
	[self scrollToLastUnread:false];
}

- (void)addChatEntry:(LinphoneChatMessage *)chat {
	// do not add the same message multiple times. It can happen on iPad since in fragment
	// mode, "message received" notification will reload tabledata, retrieving all history
	// and THEN addChatEntry will be called, requesting the newest message to be added again
	if (messageList &&
		(linphone_chat_message_get_storage_id(chat) == linphone_chat_message_get_storage_id(bctbx_list_nth_data(
														   messageList, (int)bctbx_list_size(messageList) - 1)))) {
		return;
	}

	messageList = bctbx_list_append(messageList, linphone_chat_message_ref(chat));
	int pos = (int)bctbx_list_size(messageList) - 1;

	NSIndexPath *indexPath = [NSIndexPath indexPathForRow:pos inSection:0];
	[self.tableView beginUpdates];
	[self.tableView insertRowsAtIndexPaths:@[ indexPath ] withRowAnimation:UITableViewRowAnimationFade];
	[self.tableView endUpdates];
}

- (void)updateChatEntry:(LinphoneChatMessage *)chat {
	NSInteger index = bctbx_list_index(messageList, chat);
	if (index < 0) {
		LOGW(@"chat entry doesn't exist");
		return;
	}
	[self.tableView reloadRowsAtIndexPaths:[NSArray arrayWithObject:[NSIndexPath indexPathForRow:index inSection:0]]
						  withRowAnimation:FALSE]; // just reload
	return;
}

- (void)scrollToBottom:(BOOL)animated {
	[self.tableView reloadData];
	size_t count = bctbx_list_size(messageList);
	if (count) {
		[self.tableView scrollToRowAtIndexPath:[NSIndexPath indexPathForRow:(count - 1) inSection:0]
							  atScrollPosition:UITableViewScrollPositionBottom
									  animated:YES];
	}
}

- (void)scrollToLastUnread:(BOOL)animated {
	if (messageList == nil || _chatRoom == nil) {
		return;
	}

	int index = -1;
	size_t count = bctbx_list_size(messageList);
	// Find first unread & set all entry read
	for (int i = 0; i < count; ++i) {
		int read = linphone_chat_message_is_read(bctbx_list_nth_data(messageList, i));
		if (read == 0) {
			if (index == -1)
				index = i;
		}
	}
	if (index == -1 && count > 0) {
		index = (int)count - 1;
	}

	linphone_chat_room_mark_as_read(_chatRoom);
	TabBarView *tab = (TabBarView *)[PhoneMainView.instance.mainViewController
		getCachedController:NSStringFromClass(TabBarView.class)];
	[tab update:YES];

	// Scroll to unread
	if (index >= 0) {
		[self.tableView.layer removeAllAnimations];
		[self.tableView scrollToRowAtIndexPath:[NSIndexPath indexPathForRow:index inSection:0]
							  atScrollPosition:UITableViewScrollPositionTop
									  animated:animated];
	}
}

#pragma mark - Property Functions

- (void)setChatRoom:(LinphoneChatRoom *)room {
	_chatRoom = room;
	[self reloadData];
}

#pragma mark - UITableViewDataSource Functions

- (NSInteger)numberOfSectionsInTableView:(UITableView *)tableView {
	return 1;
}

- (NSInteger)tableView:(UITableView *)tableView numberOfRowsInSection:(NSInteger)section {
	return bctbx_list_size(messageList);
}

- (UITableViewCell *)tableView:(UITableView *)tableView cellForRowAtIndexPath:(NSIndexPath *)indexPath {
	NSString *kCellId = nil;
	LinphoneChatMessage *chat = bctbx_list_nth_data(messageList, (int)[indexPath row]);
	if (linphone_chat_message_get_file_transfer_information(chat) ||
		linphone_chat_message_get_external_body_url(chat)) {
		kCellId = NSStringFromClass(UIChatBubblePhotoCell.class);
	} else {
		kCellId = NSStringFromClass(UIChatBubbleTextCell.class);
	}
	UIChatBubbleTextCell *cell = [tableView dequeueReusableCellWithIdentifier:kCellId];
	if (cell == nil) {
		cell = [[NSClassFromString(kCellId) alloc] initWithIdentifier:kCellId];
	}
	[cell setChatMessage:chat];
	if (chat) {
		[cell update];
	}
	[cell setChatRoomDelegate:_chatRoomDelegate];
	[super accessoryForCell:cell atPath:indexPath];
	cell.selectionStyle = UITableViewCellSelectionStyleNone;
	return cell;
}

#pragma mark - UITableViewDelegate Functions

- (void)scrollViewWillBeginDragging:(UIScrollView *)scrollView {
	[_chatRoomDelegate tableViewIsScrolling];
}

- (CGFloat)tableView:(UITableView *)tableView heightForRowAtIndexPath:(NSIndexPath *)indexPath {
	LinphoneChatMessage *chat = bctbx_list_nth_data(messageList, (int)[indexPath row]);
	return [UIChatBubbleTextCell ViewHeightForMessage:chat withWidth:self.view.frame.size.width].height;
}

- (void)tableView:(UITableView *)tableView
	commitEditingStyle:(UITableViewCellEditingStyle)editingStyle
	 forRowAtIndexPath:(NSIndexPath *)indexPath {
	if (editingStyle == UITableViewCellEditingStyleDelete) {
		[tableView beginUpdates];
		LinphoneChatMessage *chat = bctbx_list_nth_data(messageList, (int)[indexPath row]);
		linphone_chat_room_delete_message(_chatRoom, chat);
		messageList = bctbx_list_remove(messageList, chat);

		[tableView deleteRowsAtIndexPaths:[NSArray arrayWithObject:indexPath]
						 withRowAnimation:UITableViewRowAnimationBottom];
		[tableView endUpdates];
	}
}

- (void)removeSelectionUsing:(void (^)(NSIndexPath *))remover {
	[super removeSelectionUsing:^(NSIndexPath *indexPath) {
	  LinphoneChatMessage *chat = bctbx_list_nth_data(messageList, (int)[indexPath row]);
	  linphone_chat_room_delete_message(_chatRoom, chat);
	  messageList = bctbx_list_remove(messageList, chat);
	}];
}

@end
