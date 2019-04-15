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

@implementation ChatsListTableView

#pragma mark - Lifecycle Functions

- (instancetype)init {
	self = super.init;
	if (self) {
		_data = nil;
		_nbOfChatRoomToDelete = 0;
		_waitView.hidden = TRUE;
	}
	return self;
}

#pragma mark - ViewController Functions

- (void)viewWillAppear:(BOOL)animated {
	[super viewWillAppear:animated];
	self.tableView.accessibilityIdentifier = @"Chat list";
	[self loadData];
	_chatRooms = NULL;
}

- (void)viewDidAppear:(BOOL)animated {
	[super viewDidAppear:animated];
	// we cannot do that in viewWillAppear because we will change view while previous transition
	// was not finished, leading to "[CALayer retain]: message sent to deallocated instance" error msg
	if (IPAD && [self totalNumberOfItems] > 0) {
		[PhoneMainView.instance changeCurrentView:ChatConversationView.compositeViewDescription];
	}
}

- (void)viewWillDisappear:(BOOL)animated {
	while (_chatRooms) {
		LinphoneChatRoom *chatRoom = (LinphoneChatRoom *)_chatRooms->data;
		if (!chatRoom)
			continue;

		linphone_chat_room_remove_callbacks(chatRoom, linphone_chat_room_get_current_callbacks(chatRoom));
		_chatRooms = _chatRooms->next;
	}
}

- (void)layoutSubviews {
	[self.tableView layoutSubviews];

	CGSize contentSize = self.tableView.contentSize;
	contentSize.width = self.tableView.bounds.size.width;
	self.tableView.contentSize = contentSize;
}

#pragma mark -

static int sorted_history_comparison(LinphoneChatRoom *to_insert, LinphoneChatRoom *elem) {
	time_t new = linphone_chat_room_get_last_update_time(to_insert);
	time_t old = linphone_chat_room_get_last_update_time(elem);
	if (new < old)
		return 1;
	else if (new > old)
		return -1;

	return 0;
}

- (MSList *)sortChatRooms {
	MSList *sorted = nil;
	const MSList *unsorted = linphone_core_get_chat_rooms(LC);
	const MSList *iter = unsorted;

	while (iter) {
		// store last message in user data
		LinphoneChatRoom *chat_room = iter->data;
		// hide empty one-to-one chat room
		LinphoneChatRoomCapabilitiesMask capabilities = linphone_chat_room_get_capabilities(chat_room);
		ChatConversationView *view = VIEW(ChatConversationView);
		if (linphone_chat_room_get_history_size(chat_room) > 0 || !(capabilities & LinphoneChatRoomCapabilitiesOneToOne) || (IPAD && view.chatRoom == chat_room)) {
			sorted = bctbx_list_insert_sorted(sorted, chat_room, (bctbx_compare_func)sorted_history_comparison);
		}
		iter = iter->next;
	}
	return sorted;
}

- (void)loadData {
	if (_data) bctbx_list_free(_data);
	_data = [self sortChatRooms];
	[super loadData];

	if (IPAD) {
		int idx = bctbx_list_index(_data, VIEW(ChatConversationView).chatRoom);
		// if conversation view is using a chatroom that does not exist anymore, update it
		if (idx != -1) {
			NSIndexPath *indexPath = [NSIndexPath indexPathForRow:idx inSection:0];
			[self.tableView selectRowAtIndexPath:indexPath animated:NO scrollPosition:UITableViewScrollPositionNone];
		} else if (![self selectFirstRow]) {
			ChatConversationCreateView *view = VIEW(ChatConversationCreateView);
			view.tableController.notFirstTime = FALSE;
			[PhoneMainView.instance changeCurrentView:view.compositeViewDescription];
		}
	}
}

+ (void) saveDataToUserDefaults {
    NSUserDefaults *defaults = [[NSUserDefaults alloc] initWithSuiteName:@"group.belledonne-communications.linphone.widget"];
    MSList *sorted = nil;
    const MSList *unsorted = linphone_core_get_chat_rooms(LC);
    const MSList *iter = unsorted;
    
    while (iter) {
        // store last message in user data
        LinphoneChatRoom *chat_room = iter->data;
        sorted = bctbx_list_insert_sorted(sorted, chat_room, (bctbx_compare_func)sorted_history_comparison);
        iter = iter->next;
    }
    
    NSMutableArray *addresses = [NSMutableArray array];
    
    while (sorted) {
        NSMutableDictionary *dict = [NSMutableDictionary dictionary];
        LinphoneChatRoom *cr = sorted->data;
        if (!cr) {
            sorted = sorted->next;
            continue;
        }
        const LinphoneAddress *peer_address = linphone_chat_room_get_peer_address(cr);
        const LinphoneAddress *local_address = linphone_chat_room_get_local_address(cr);
        NSString *display;
        [dict setObject:[NSString stringWithUTF8String:linphone_address_as_string_uri_only(peer_address)]
                 forKey:@"peer"];
        [dict setObject:[NSString stringWithUTF8String:linphone_address_as_string_uri_only(local_address)]
                 forKey:@"local"];
        LinphoneChatRoomCapabilitiesMask capabilities = linphone_chat_room_get_capabilities(cr);
        if (!(capabilities & LinphoneChatRoomCapabilitiesOneToOne)) {
            if (!linphone_chat_room_get_subject(cr)) {
                sorted = sorted->next;
                continue;
            }
            display = [NSString stringWithUTF8String:linphone_chat_room_get_subject(cr)];
        } else {
            bctbx_list_t *participants = linphone_chat_room_get_participants(cr);
            LinphoneParticipant *firstParticipant = participants ? (LinphoneParticipant *)participants->data : NULL;
            const LinphoneAddress *addr = firstParticipant ? linphone_participant_get_address(firstParticipant) : peer_address;
            if (!linphone_address_get_username(addr)) {
                sorted = sorted->next;
                continue;
            }
            display = [NSString stringWithUTF8String:linphone_address_get_display_name(addr)?:linphone_address_get_username(addr)];
            if ([FastAddressBook imageForAddress:addr])
                [dict setObject:UIImageJPEGRepresentation([UIImage resizeImage:[FastAddressBook imageForAddress:peer_address]
                                                                  withMaxWidth:200
                                                                  andMaxHeight:200],
                                                          1)
                         forKey:@"img"];
        }
        [dict setObject:display
                 forKey:@"display"];
        BOOL isGroupChat = linphone_chat_room_get_capabilities(cr) & LinphoneChatRoomCapabilitiesConference;
        [dict setObject:[NSNumber numberWithBool:isGroupChat]
                 forKey:@"nbParticipants"];
        [addresses addObject:dict];
        if (addresses.count >= 4) //send no more data than needed
            break;
        sorted = sorted->next;
	}
	
    [defaults setObject:addresses forKey:@"chatrooms"];
}

- (void)markCellAsRead:(LinphoneChatRoom *)chatRoom {
	int idx = bctbx_list_index(_data, VIEW(ChatConversationView).chatRoom);
	NSIndexPath *indexPath = [NSIndexPath indexPathForRow:idx inSection:0];
	if (IPAD) {
		UIChatCell *cell = (UIChatCell *)[self.tableView cellForRowAtIndexPath:indexPath];
		[cell updateUnreadBadge];
	}
}

#pragma mark - UITableViewDataSource Functions

- (NSInteger)numberOfSectionsInTableView:(UITableView *)tableView {
	return 1;
}

- (NSInteger)tableView:(UITableView *)tableView numberOfRowsInSection:(NSInteger)section {
	return bctbx_list_size(_data);
}

- (UITableViewCell *)tableView:(UITableView *)tableView cellForRowAtIndexPath:(NSIndexPath *)indexPath {
	static NSString *kCellId = @"UIChatCell";
	UIChatCell *cell = [tableView dequeueReusableCellWithIdentifier:kCellId];
	if (cell == nil)
		cell = [[UIChatCell alloc] initWithIdentifier:kCellId];


	[cell setChatRoom:(LinphoneChatRoom *)bctbx_list_nth_data(_data, (int)[indexPath row])];
	[super accessoryForCell:cell atPath:indexPath];
	return cell;
}

- (CGFloat)tableView:(UITableView *)tableView heightForRowAtIndexPath:(nonnull NSIndexPath *)indexPath {
    return 86.0;
}

#pragma mark - UITableViewDelegate Functions

- (void)tableView:(UITableView *)tableView didSelectRowAtIndexPath:(NSIndexPath *)indexPath {
	[super tableView:tableView didSelectRowAtIndexPath:indexPath];
	if ([self isEditing])
		return;

	LinphoneChatRoom *chatRoom = (LinphoneChatRoom *)bctbx_list_nth_data(_data, (int)[indexPath row]);
	[PhoneMainView.instance goToChatRoom:chatRoom];
}

void deletion_chat_room_state_changed(LinphoneChatRoom *cr, LinphoneChatRoomState newState) {
	LinphoneChatRoomCbs *cbs = linphone_chat_room_get_current_callbacks(cr);
	ChatsListTableView *view = (__bridge ChatsListTableView *)linphone_chat_room_cbs_get_user_data(cbs) ?: NULL;
	if (!view)
		return;
	
	if (newState == LinphoneChatRoomStateDeleted || newState == LinphoneChatRoomStateTerminationFailed) {
		linphone_chat_room_remove_callbacks(cr, cbs);
		view.chatRooms = bctbx_list_remove(view.chatRooms, cr);
		view.nbOfChatRoomToDelete--;
	}

	if (view.nbOfChatRoomToDelete == 0) {
		// will force a call to [self loadData]
		[NSNotificationCenter.defaultCenter postNotificationName:kLinphoneMessageReceived object:view];
		view.waitView.hidden = TRUE;
	}
}

- (void) deleteChatRooms {
	_waitView.hidden = FALSE;
	bctbx_list_t *chatRooms = bctbx_list_copy(_chatRooms);
	while (chatRooms) {
		LinphoneChatRoom *chatRoom = (LinphoneChatRoom *)chatRooms->data;
		if (!chatRoom)
			continue;

		_nbOfChatRoomToDelete++;
		LinphoneChatRoomCbs *cbs = linphone_factory_create_chat_room_cbs(linphone_factory_get());
		linphone_chat_room_cbs_set_state_changed(cbs, deletion_chat_room_state_changed);
		linphone_chat_room_cbs_set_user_data(cbs, (__bridge void*)self);
		linphone_chat_room_add_callbacks(chatRoom, cbs);

		FileTransferDelegate *ftdToDelete = nil;
		for (FileTransferDelegate *ftd in [LinphoneManager.instance fileTransferDelegates]) {
			if (linphone_chat_message_get_chat_room(ftd.message) == chatRoom) {
				ftdToDelete = ftd;
				break;
			}
		}
		[ftdToDelete cancel];

		linphone_core_delete_chat_room(LC, chatRoom);
		chatRooms = chatRooms->next;
	}
}

- (void)tableView:(UITableView *)tableView
	commitEditingStyle:(UITableViewCellEditingStyle)editingStyle
	 forRowAtIndexPath:(NSIndexPath *)indexPath {
	if (editingStyle == UITableViewCellEditingStyleDelete) {
		LinphoneChatRoom *chatRoom = (LinphoneChatRoom *)bctbx_list_nth_data(_data, (int)[indexPath row]);
		NSString *msg = (LinphoneChatRoomCapabilitiesOneToOne & linphone_chat_room_get_capabilities(chatRoom))
			? [NSString stringWithFormat:NSLocalizedString(@"Do you want to delete this conversation?", nil)]
			: [NSString stringWithFormat:NSLocalizedString(@"Do you want to leave and delete this conversation?", nil)];
		[UIConfirmationDialog ShowWithMessage:msg
								cancelMessage:nil
							   confirmMessage:nil
								onCancelClick:^() {}
						  onConfirmationClick:^() {
							  _chatRooms = bctbx_list_new((void *)chatRoom);
							  [self deleteChatRooms];
						  }];
	}
}

- (void)removeSelectionUsing:(void (^)(NSIndexPath *))remover {
	_chatRooms = NULL;
	// we must iterate through selected items in reverse order
	[self.selectedItems sortUsingComparator:^(NSIndexPath *obj1, NSIndexPath *obj2) {
		return [obj2 compare:obj1];
	}];
	NSArray *copy = [[NSArray alloc] initWithArray:self.selectedItems];
	for (NSIndexPath *indexPath in copy) {
		LinphoneChatRoom *chatRoom = (LinphoneChatRoom *)bctbx_list_nth_data(_data, (int)[indexPath row]);
		_chatRooms = bctbx_list_append(_chatRooms, chatRoom);
	}
	[self deleteChatRooms];
	[self.selectedItems removeAllObjects];
	[self setEditing:NO animated:YES];
}

@end
