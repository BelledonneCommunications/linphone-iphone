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

#import "ChatTableViewController.h"
#import "UIChatCell.h"

#import "FileTransferDelegate.h"

#import "linphone/linphonecore.h"
#import "PhoneMainView.h"
#import "UACellBackgroundView.h"
#import "UILinphone.h"
#import "Utils.h"

@implementation ChatTableViewController {
	MSList *data;
}

#pragma mark - Lifecycle Functions

- (instancetype)init {
	self = super.init;
	if (self) {
		self->data = nil;
	}
	return self;
}

- (void)dealloc {
	if (data != nil) {
		ms_list_free_with_data(data, chatTable_free_chatrooms);
	}
}

#pragma mark - ViewController Functions

- (void)viewDidAppear:(BOOL)animated {
	[super viewDidAppear:animated];
	self.tableView.accessibilityIdentifier = @"ChatRoom list";
	[self loadData];
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
	MSList *unsorted = linphone_core_get_chat_rooms([LinphoneManager getLc]);
	MSList *iter = unsorted;

	while (iter) {
		// store last message in user data
		LinphoneChatRoom *chat_room = iter->data;
		MSList *history = linphone_chat_room_get_history(iter->data, 1);
		LinphoneChatMessage *last_msg = history ? history->data : NULL;
		linphone_chat_room_set_user_data(chat_room, last_msg);
		sorted = ms_list_insert_sorted(sorted, chat_room, (MSCompareFunc)sorted_history_comparison);

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
}

- (void)loadData {
	if (data != NULL) {
		ms_list_free_with_data(data, chatTable_free_chatrooms);
	}
	data = [self sortChatRooms];
	[[self tableView] reloadData];
}

#pragma mark - UITableViewDataSource Functions

- (NSInteger)numberOfSectionsInTableView:(UITableView *)tableView {
	return 1;
}

- (NSInteger)tableView:(UITableView *)tableView numberOfRowsInSection:(NSInteger)section {
	return ms_list_size(data);
}

- (UITableViewCell *)tableView:(UITableView *)tableView cellForRowAtIndexPath:(NSIndexPath *)indexPath {
	static NSString *kCellId = @"UIChatCell";
	UIChatCell *cell = [tableView dequeueReusableCellWithIdentifier:kCellId];
	if (cell == nil) {
		cell = [[UIChatCell alloc] initWithIdentifier:kCellId];

		// Background View
		UACellBackgroundView *selectedBackgroundView = [[UACellBackgroundView alloc] initWithFrame:CGRectZero];
		cell.selectedBackgroundView = selectedBackgroundView;
		[selectedBackgroundView setBackgroundColor:LINPHONE_TABLE_CELL_BACKGROUND_COLOR];
	}

	[cell setChatRoom:(LinphoneChatRoom *)ms_list_nth_data(data, (int)[indexPath row])];

	return cell;
}

#pragma mark - UITableViewDelegate Functions

- (void)tableView:(UITableView *)tableView didSelectRowAtIndexPath:(NSIndexPath *)indexPath {
	[tableView deselectRowAtIndexPath:indexPath animated:NO];
	LinphoneChatRoom *chatRoom = (LinphoneChatRoom *)ms_list_nth_data(data, (int)[indexPath row]);

	// Go to ChatRoom view
	ChatRoomViewController *controller = DYNAMIC_CAST(
		[[PhoneMainView instance] changeCurrentView:[ChatRoomViewController compositeViewDescription] push:TRUE],
		ChatRoomViewController);
	if (controller != nil) {
		[controller setChatRoom:chatRoom];
	}
}

- (UITableViewCellEditingStyle)tableView:(UITableView *)aTableView
		   editingStyleForRowAtIndexPath:(NSIndexPath *)indexPath {
	// Detemine if it's in editing mode
	if (self.editing) {
		return UITableViewCellEditingStyleDelete;
	}
	return UITableViewCellEditingStyleNone;
}

- (void)tableView:(UITableView *)tableView
	commitEditingStyle:(UITableViewCellEditingStyle)editingStyle
	 forRowAtIndexPath:(NSIndexPath *)indexPath {
	if (editingStyle == UITableViewCellEditingStyleDelete) {
		[tableView beginUpdates];

		LinphoneChatRoom *chatRoom = (LinphoneChatRoom *)ms_list_nth_data(data, (int)[indexPath row]);
		LinphoneChatMessage *last_msg = linphone_chat_room_get_user_data(chatRoom);
		if (last_msg) {
			linphone_chat_message_unref(last_msg);
			linphone_chat_room_set_user_data(chatRoom, NULL);
		}

		for (FileTransferDelegate *ftd in [[LinphoneManager instance] fileTransferDelegates]) {
			if (linphone_chat_message_get_chat_room(ftd.message) == chatRoom) {
				[ftd cancel];
			}
		}
		linphone_chat_room_delete_history(chatRoom);
		linphone_chat_room_unref(chatRoom);
		data = ms_list_remove(data, chatRoom);

		// will force a call to [self loadData]
		[[NSNotificationCenter defaultCenter] postNotificationName:kLinphoneTextReceived object:self];

		[tableView deleteRowsAtIndexPaths:[NSArray arrayWithObject:indexPath]
						 withRowAnimation:UITableViewRowAnimationFade];
		[tableView endUpdates];
	}
}

@end
