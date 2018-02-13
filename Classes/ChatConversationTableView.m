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
#import "UIChatNotifiedEventCell.h"
#import "PhoneMainView.h"

@implementation ChatConversationTableView

#pragma mark - Lifecycle Functions

- (void)dealloc {
	[self clearEventList];
}

#pragma mark - ViewController Functions

- (void)viewWillAppear:(BOOL)animated {
	[super viewWillAppear:animated];
	self.tableView.accessibilityIdentifier = @"ChatRoom list";
}

#pragma mark -

- (void)clearEventList {
	eventList = bctbx_list_free_with_data(eventList, (void (*)(void *))linphone_event_log_unref);
}

- (void)updateData {
	if (!_chatRoom)
		return;
	[self clearEventList];
	LinphoneChatRoomCapabilitiesMask capabilities = linphone_chat_room_get_capabilities(_chatRoom);
	eventList = (capabilities & LinphoneChatRoomCapabilitiesOneToOne)
		? linphone_chat_room_get_history_message_events(_chatRoom, 0)
		: linphone_chat_room_get_history_events(_chatRoom, 0);

	for (FileTransferDelegate *ftd in [LinphoneManager.instance fileTransferDelegates]) {
		const LinphoneAddress *ftd_peer =
			linphone_chat_room_get_peer_address(linphone_chat_message_get_chat_room(ftd.message));
		const LinphoneAddress *peer = linphone_chat_room_get_peer_address(_chatRoom);
		if (linphone_address_equal(ftd_peer, peer) && linphone_chat_message_is_outgoing(ftd.message)) {
			LOGI(@"Appending transient upload message %p", ftd.message);
			//TODO : eventList = bctbx_list_append(eventList, linphone_chat_message_ref(ftd.event));
		}
	}
}

- (void)reloadData {
	[self updateData];
	[self.tableView reloadData];
	[self scrollToLastUnread:false];
}

- (void)addEventEntry:(LinphoneEventLog *)event {
	eventList = bctbx_list_append(eventList, linphone_event_log_ref(event));
	int pos = (int)bctbx_list_size(eventList) - 1;

	NSIndexPath *indexPath = [NSIndexPath indexPathForRow:pos inSection:0];
	[self.tableView beginUpdates];
	[self.tableView insertRowsAtIndexPaths:@[ indexPath ] withRowAnimation:UITableViewRowAnimationFade];
	[self.tableView endUpdates];
}

- (void)updateEventEntry:(LinphoneEventLog *)event {
	NSInteger index = bctbx_list_index(eventList, event);
	if (index < 0) {
		LOGW(@"event entry doesn't exist");
		return;
	}
	[self.tableView reloadRowsAtIndexPaths:[NSArray arrayWithObject:[NSIndexPath indexPathForRow:index inSection:0]]
						  withRowAnimation:FALSE]; // just reload
	return;
}

- (void)scrollToBottom:(BOOL)animated {
	[self.tableView reloadData];
	size_t count = bctbx_list_size(eventList);
	if (count) {
		[self.tableView cellForRowAtIndexPath:[NSIndexPath indexPathForRow:(count - 1) inSection:0]];
		[self.tableView scrollToRowAtIndexPath:[NSIndexPath indexPathForRow:(count - 1) inSection:0]
							  atScrollPosition:UITableViewScrollPositionBottom
									  animated:YES];
	}
}

- (void)scrollToLastUnread:(BOOL)animated {
	if (eventList == nil || _chatRoom == nil) {
		return;
	}
	int index = -1;
	size_t count = bctbx_list_size(eventList);
	// Find first unread & set all entry read
	for (int i = 0; i < count; ++i) {
		LinphoneEventLog *event = bctbx_list_nth_data(eventList, i);
		if (!(linphone_event_log_get_type(event) == LinphoneEventLogTypeConferenceChatMessage))
			break;

		LinphoneChatMessage *chat = linphone_event_log_get_chat_message(event);
		int read = linphone_chat_message_is_read(chat);
		LinphoneChatMessageState state = linphone_chat_message_get_state(chat);
		if (read == 0 &&
			!(state == LinphoneChatMessageStateFileTransferError || state == LinphoneChatMessageStateNotDelivered)) {
			if (index == -1) {
				index = i;
				break;
			}
		}
	}
	if (index == -1 && count > 0) {
		index = (int)count - 1;
	}
	if ([UIApplication sharedApplication].applicationState == UIApplicationStateActive) {
		linphone_chat_room_mark_as_read(_chatRoom);
	}
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
	return bctbx_list_size(eventList);
}

- (UITableViewCell *)tableView:(UITableView *)tableView cellForRowAtIndexPath:(NSIndexPath *)indexPath {
	NSString *kCellId = nil;
	LinphoneEventLog *event = bctbx_list_nth_data(eventList, (int)[indexPath row]);
	if (linphone_event_log_get_type(event) == LinphoneEventLogTypeConferenceChatMessage) {
		LinphoneChatMessage *chat = linphone_event_log_get_chat_message(event);
		if (linphone_chat_message_get_file_transfer_information(chat) || linphone_chat_message_get_external_body_url(chat))
			kCellId = NSStringFromClass(UIChatBubblePhotoCell.class);
		else
			kCellId = NSStringFromClass(UIChatBubbleTextCell.class);

		UIChatBubbleTextCell *cell = [tableView dequeueReusableCellWithIdentifier:kCellId];
		if (cell == nil)
			cell = [[NSClassFromString(kCellId) alloc] initWithIdentifier:kCellId];

		[cell setEvent:event];
		if (chat)
			[cell update];

		[cell setChatRoomDelegate:_chatRoomDelegate];
		[super accessoryForCell:cell atPath:indexPath];
		cell.selectionStyle = UITableViewCellSelectionStyleNone;
		return cell;
	} else {
		kCellId = NSStringFromClass(UIChatNotifiedEventCell.class);
		UIChatNotifiedEventCell *cell = [tableView dequeueReusableCellWithIdentifier:kCellId];
		if (cell == nil)
			cell = [[NSClassFromString(kCellId) alloc] initWithIdentifier:kCellId];

		[cell setEvent:event];
		[super accessoryForCell:cell atPath:indexPath];
		cell.selectionStyle = UITableViewCellSelectionStyleNone;
		return cell;
	}
}

#pragma mark - UITableViewDelegate Functions

- (void)scrollViewWillBeginDragging:(UIScrollView *)scrollView {
	[_chatRoomDelegate tableViewIsScrolling];
}

- (CGFloat)tableView:(UITableView *)tableView heightForRowAtIndexPath:(NSIndexPath *)indexPath {
	LinphoneEventLog *event = bctbx_list_nth_data(eventList, (int)[indexPath row]);
	if (linphone_event_log_get_type(event) == LinphoneEventLogTypeConferenceChatMessage) {
		LinphoneChatMessage *chat = linphone_event_log_get_chat_message(event);
		return [UIChatBubbleTextCell ViewHeightForMessage:chat withWidth:self.view.frame.size.width].height;
	} else {
		return [UIChatNotifiedEventCell height];
	}
}

- (void)tableView:(UITableView *)tableView
	commitEditingStyle:(UITableViewCellEditingStyle)editingStyle
	 forRowAtIndexPath:(NSIndexPath *)indexPath {
	if (editingStyle == UITableViewCellEditingStyleDelete) {
		[tableView beginUpdates];
		LinphoneEventLog *event = bctbx_list_nth_data(eventList, (int)[indexPath row]);
		linphone_event_log_delete_from_database(event);
		eventList = bctbx_list_remove(eventList, event);

		[tableView deleteRowsAtIndexPaths:[NSArray arrayWithObject:indexPath]
						 withRowAnimation:UITableViewRowAnimationBottom];
		[tableView endUpdates];
	}
}

- (void)removeSelectionUsing:(void (^)(NSIndexPath *))remover {
	[super removeSelectionUsing:^(NSIndexPath *indexPath) {
		LinphoneEventLog *event = bctbx_list_nth_data(eventList, (int)[indexPath row]);
		linphone_event_log_delete_from_database(event);
		eventList = bctbx_list_remove(eventList, event);
	}];
}

@end
