/* ChatTableViewController.m
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

#import "ChatsListTableView.h"
#import "UIChatCell.h"

#import "FileTransferDelegate.h"

#import "linphone/linphonecore.h"
#import "PhoneMainView.h"
#import "Utils.h"

@implementation ChatsListTableView {
	bctbx_list_t *data;
}

#pragma mark - Lifecycle Functions

- (instancetype)init {
	self = super.init;
	if (self) {
		data = nil;
	}
	return self;
}

- (void)dealloc {
	if (data != nil) {
		bctbx_list_free_with_data(data, chatTable_free_chatrooms);
	}
}

#pragma mark - ViewController Functions

- (void)viewWillAppear:(BOOL)animated {
	[super viewWillAppear:animated];
	self.tableView.accessibilityIdentifier = @"Chat list";
	[self loadData];
}

- (void)viewDidAppear:(BOOL)animated {
	[super viewDidAppear:animated];
	// we cannot do that in viewWillAppear because we will change view while previous transition
	// was not finished, leading to "[CALayer retain]: message sent to deallocated instance" error msg
	if (IPAD && [self totalNumberOfItems] > 0) {
		[PhoneMainView.instance changeCurrentView:ChatConversationView.compositeViewDescription];
	}
}

#pragma mark -

static int sorted_history_comparison(LinphoneChatRoom *to_insert, LinphoneChatRoom *elem) {
	LinphoneChatMessage *last_new_message = linphone_chat_room_get_user_data(to_insert);
	LinphoneChatMessage *last_elem_message = linphone_chat_room_get_user_data(elem);

	if (last_new_message && last_elem_message) {
		time_t new = linphone_chat_message_get_time(last_new_message);
		time_t old = linphone_chat_message_get_time(last_elem_message);
		if (new < old)
			return 1;
		else if (new > old)
			return -1;
	}
	return 0;
}

- (MSList *)sortChatRooms {
	MSList *sorted = nil;
	const MSList *unsorted = linphone_core_get_chat_rooms(LC);
	const MSList *iter = unsorted;

	while (iter) {
		// store last message in user data
		LinphoneChatRoom *chat_room = iter->data;
		bctbx_list_t *history = linphone_chat_room_get_history(iter->data, 1);
		LinphoneChatMessage *last_msg = NULL;
		if (history) {
			last_msg = linphone_chat_message_ref(history->data);
			bctbx_list_free(history);
		}
		linphone_chat_room_set_user_data(chat_room, last_msg);
		sorted = bctbx_list_insert_sorted(sorted, linphone_chat_room_ref(chat_room),
										  (bctbx_compare_func)sorted_history_comparison);
		iter = iter->next;
	}
	return sorted;
}

static void chatTable_free_chatrooms(void *data) {
	LinphoneChatMessage *lastMsg = linphone_chat_room_get_user_data(data);
	if (lastMsg) {
		linphone_chat_message_unref(lastMsg);
		linphone_chat_room_set_user_data(data, NULL);
	}
	linphone_chat_room_unref(data);
}

- (void)loadData {
	if (data != NULL) {
		bctbx_list_free_with_data(data, chatTable_free_chatrooms);
	}
	data = [self sortChatRooms];
	[super loadData];

	if (IPAD) {
		int idx = bctbx_list_index(data, VIEW(ChatConversationView).chatRoom);
		// if conversation view is using a chatroom that does not exist anymore, update it
		if (idx != -1) {
			NSIndexPath *indexPath = [NSIndexPath indexPathForRow:idx inSection:0];
			[self.tableView selectRowAtIndexPath:indexPath animated:NO scrollPosition:UITableViewScrollPositionNone];
		} else if (![self selectFirstRow]) {
			[PhoneMainView.instance changeCurrentView:ChatConversationCreateView.compositeViewDescription];
		}
	}
}

#pragma mark - UITableViewDataSource Functions

- (NSInteger)numberOfSectionsInTableView:(UITableView *)tableView {
	return 1;
}

- (NSInteger)tableView:(UITableView *)tableView numberOfRowsInSection:(NSInteger)section {
	return bctbx_list_size(data);
}

- (UITableViewCell *)tableView:(UITableView *)tableView cellForRowAtIndexPath:(NSIndexPath *)indexPath {
	static NSString *kCellId = @"UIChatCell";
	UIChatCell *cell = [tableView dequeueReusableCellWithIdentifier:kCellId];
	if (cell == nil) {
		cell = [[UIChatCell alloc] initWithIdentifier:kCellId];
	}

	[cell setChatRoom:(LinphoneChatRoom *)bctbx_list_nth_data(data, (int)[indexPath row])];
	[super accessoryForCell:cell atPath:indexPath];
	return cell;
}

#pragma mark - UITableViewDelegate Functions

- (void)tableView:(UITableView *)tableView didSelectRowAtIndexPath:(NSIndexPath *)indexPath {
	[super tableView:tableView didSelectRowAtIndexPath:indexPath];
	if (![self isEditing]) {
		LinphoneChatRoom *chatRoom = (LinphoneChatRoom *)bctbx_list_nth_data(data, (int)[indexPath row]);
		ChatConversationView *view = VIEW(ChatConversationView);
		[view setChatRoom:chatRoom];
		// on iPad, force unread bubble to disappear by reloading the cell
		if (IPAD) {
			UIChatCell *cell = (UIChatCell *)[tableView cellForRowAtIndexPath:indexPath];
			[cell updateUnreadBadge];
		}
		[PhoneMainView.instance changeCurrentView:view.compositeViewDescription];
	}
}

- (void)tableView:(UITableView *)tableView
	commitEditingStyle:(UITableViewCellEditingStyle)editingStyle
	 forRowAtIndexPath:(NSIndexPath *)indexPath {
	if (editingStyle == UITableViewCellEditingStyleDelete) {
		[tableView beginUpdates];

		LinphoneChatRoom *chatRoom = (LinphoneChatRoom *)bctbx_list_nth_data(data, (int)[indexPath row]);
		LinphoneChatMessage *last_msg = linphone_chat_room_get_user_data(chatRoom);
		if (last_msg) {
			linphone_chat_message_unref(last_msg);
			linphone_chat_room_set_user_data(chatRoom, NULL);
		}

		FileTransferDelegate *ftdToDelete = nil;
		for (FileTransferDelegate *ftd in [LinphoneManager.instance fileTransferDelegates]) {
			if (linphone_chat_message_get_chat_room(ftd.message) == chatRoom) {
				ftdToDelete = ftd;
				break;
			}
		}
		[ftdToDelete cancel];

		linphone_core_delete_chat_room(linphone_chat_room_get_core(chatRoom), chatRoom);
		data = bctbx_list_remove(data, chatRoom);

		// will force a call to [self loadData]
		[NSNotificationCenter.defaultCenter postNotificationName:kLinphoneMessageReceived object:self];

		[tableView deleteRowsAtIndexPaths:[NSArray arrayWithObject:indexPath]
						 withRowAnimation:UITableViewRowAnimationFade];
		[tableView endUpdates];
	}
}

- (void)removeSelectionUsing:(void (^)(NSIndexPath *))remover {
	[super removeSelectionUsing:^(NSIndexPath *indexPath) {
	  LinphoneChatRoom *chatRoom = (LinphoneChatRoom *)bctbx_list_nth_data(data, (int)[indexPath row]);
	  LinphoneChatMessage *last_msg = linphone_chat_room_get_user_data(chatRoom);
	  if (last_msg) {
		  linphone_chat_message_unref(last_msg);
		  linphone_chat_room_set_user_data(chatRoom, NULL);
	  }

	  FileTransferDelegate *ftdToDelete = nil;
	  for (FileTransferDelegate *ftd in [LinphoneManager.instance fileTransferDelegates]) {
		  if (linphone_chat_message_get_chat_room(ftd.message) == chatRoom) {
			  ftdToDelete = ftd;
			  break;
		  }
	  }
	  [ftdToDelete cancel];

	  linphone_core_delete_chat_room(linphone_chat_room_get_core(chatRoom), chatRoom);
	  data = bctbx_list_remove(data, chatRoom);

	  // will force a call to [self loadData]
	  [NSNotificationCenter.defaultCenter postNotificationName:kLinphoneMessageReceived object:self];
	}];
}

@end
