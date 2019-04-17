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
#import "ChatConversationImdnView.h"
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
    _imagesInChatroom = [NSMutableDictionary dictionary];
	_currentIndex = 0;
}

#pragma mark -

- (void)clearEventList {
    for (NSValue *value in totalEventList) {
        LinphoneEventLog *event = value.pointerValue;
        linphone_event_log_unref(event);
    }
    [eventList removeAllObjects];
    [totalEventList removeAllObjects];
}

- (void)updateData {
    [self clearEventList];
	if (!_chatRoom)
		return;
    
	LinphoneChatRoomCapabilitiesMask capabilities = linphone_chat_room_get_capabilities(_chatRoom);
	bctbx_list_t *chatRoomEvents = (capabilities & LinphoneChatRoomCapabilitiesOneToOne)
		? linphone_chat_room_get_history_message_events(_chatRoom, 0)
		: linphone_chat_room_get_history_events(_chatRoom, 0);
    bctbx_list_t *head = chatRoomEvents;
    size_t listSize = bctbx_list_size(chatRoomEvents);
	totalEventList = [[NSMutableArray alloc] initWithCapacity:listSize];
    eventList = [[NSMutableArray alloc] initWithCapacity:MIN(listSize, BASIC_EVENT_LIST)];
	while (chatRoomEvents) {
        LinphoneEventLog *event = (LinphoneEventLog *)chatRoomEvents->data;
        [totalEventList addObject:[NSValue valueWithPointer:linphone_event_log_ref(event)]];
        if (listSize <= BASIC_EVENT_LIST) {            
            [eventList addObject:[NSValue valueWithPointer:linphone_event_log_ref(event)]];
        }
		chatRoomEvents = chatRoomEvents->next;
        listSize -= 1;
	}
    bctbx_list_free_with_data(head, (bctbx_list_free_func)linphone_event_log_unref);

	/*for (FileTransferDelegate *ftd in [LinphoneManager.instance fileTransferDelegates]) {
		const LinphoneAddress *ftd_peer =
			linphone_chat_room_get_peer_address(linphone_chat_message_get_chat_room(ftd.message));
		const LinphoneAddress *peer = linphone_chat_room_get_peer_address(_chatRoom);
		if (linphone_address_equal(ftd_peer, peer) && linphone_chat_message_is_outgoing(ftd.message)) {
			LOGI(@"Appending transient upload message %p", ftd.message);
			//TODO : eventList = bctbx_list_append(eventList, linphone_chat_message_ref(ftd.event));
		}
	}*/
}

- (void)refreshData {
    if (totalEventList.count <= eventList.count) {
        _currentIndex = 0;
        return;
    }
    
    NSUInteger num = MIN(totalEventList.count-eventList.count, BASIC_EVENT_LIST);
    _currentIndex = num - 1;
    while (num) {
        NSInteger index = totalEventList.count - eventList.count - 1;
        [eventList insertObject:[totalEventList objectAtIndex:index] atIndex:0];
        index -= 1;
        num -= 1;
    }
}

- (void)reloadData {
	[self updateData];
	[self.tableView reloadData];
	[self scrollToLastUnread:false];
}

- (void)addEventEntry:(LinphoneEventLog *)event {
	[eventList addObject:[NSValue valueWithPointer:linphone_event_log_ref(event)]];
    [totalEventList addObject:[NSValue valueWithPointer:linphone_event_log_ref(event)]];
	int pos = (int)eventList.count - 1;
	NSIndexPath *indexPath = [NSIndexPath indexPathForRow:pos inSection:0];
	[self.tableView beginUpdates];
	[self.tableView insertRowsAtIndexPaths:@[ indexPath ] withRowAnimation:UITableViewRowAnimationFade];
    [self.tableView reloadData];
	[self.tableView endUpdates];
}

- (void)updateEventEntry:(LinphoneEventLog *)event {
	NSInteger index = [eventList indexOfObject:[NSValue valueWithPointer:event]];
	if (index < 0) {
		LOGW(@"event entry doesn't exist");
		return;
	}
	[self.tableView reloadRowsAtIndexPaths:[NSArray arrayWithObject:[NSIndexPath indexPathForRow:index inSection:0]]
						  withRowAnimation:FALSE]; // just reload
    return;
}

- (void)scrollToBottom:(BOOL)animated {
	//[self.tableView reloadData];
	size_t count = eventList.count;
	if (!count)
		return;

	//[self.tableView cellForRowAtIndexPath:[NSIndexPath indexPathForRow:(count - 1) inSection:0]];
	[self.tableView scrollToRowAtIndexPath:[NSIndexPath indexPathForRow:(count - 1) inSection:0]
						  atScrollPosition:UITableViewScrollPositionBottom
								  animated:YES];
}

- (void)scrollToLastUnread:(BOOL)animated {
	if (eventList.count == 0 || _chatRoom == nil)
		return;

	int index = -1;
	size_t count = eventList.count;
	// Find first unread & set all entry read
	for (int i = (int)count - 1; i > 0; --i) {
		LinphoneEventLog *event = [[eventList objectAtIndex:i] pointerValue];
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
	if (index == -1 && count > 0)
		index = (int)count - 1;

	if ([UIApplication sharedApplication].applicationState == UIApplicationStateActive)
		[ChatConversationView markAsRead:_chatRoom];

	// Scroll to unread
	if (index < 0)
		return;

	[self.tableView.layer removeAllAnimations];
	[self.tableView scrollToRowAtIndexPath:[NSIndexPath indexPathForRow:index inSection:0]
						  atScrollPosition:UITableViewScrollPositionTop
								  animated:animated];
}

#pragma mark - Property Functions

- (void)setChatRoom:(LinphoneChatRoom *)room {
	_chatRoom = room;
	[self reloadData];
}

static const int MAX_AGGLOMERATED_TIME=300;
static const int BASIC_EVENT_LIST=15;

- (BOOL)isFirstIndexInTableView:(NSIndexPath *)indexPath chat:(LinphoneChatMessage *)chat {
    LinphoneEventLog *previousEvent = nil;
    NSInteger indexOfPreviousEvent = indexPath.row - 1;
    while (!previousEvent && indexOfPreviousEvent > -1) {
        LinphoneEventLog *tmp = [[eventList objectAtIndex:indexOfPreviousEvent] pointerValue];
        if (linphone_event_log_get_type(tmp) == LinphoneEventLogTypeConferenceChatMessage) {
            previousEvent = tmp;
        }
        --indexOfPreviousEvent;
    }
    if (!previousEvent)
        return TRUE;

    LinphoneChatMessage *previousChat = linphone_event_log_get_chat_message(previousEvent);
    if (!linphone_address_equal(linphone_chat_message_get_from_address(previousChat), linphone_chat_message_get_from_address(chat))) {
        return TRUE;
    }
    // the maximum interval between 2 agglomerated chats at 5mn
    if ((linphone_chat_message_get_time(chat)-linphone_chat_message_get_time(previousChat)) > MAX_AGGLOMERATED_TIME) {
        return TRUE;
    }
        
    return FALSE;
}

- (BOOL)isLastIndexInTableView:(NSIndexPath *)indexPath chat:(LinphoneChatMessage *)chat {
    LinphoneEventLog *nextEvent = nil;
    NSInteger indexOfNextEvent = indexPath.row + 1;
    while (!nextEvent && indexOfNextEvent < [eventList count]) {
        LinphoneEventLog *tmp = [[eventList objectAtIndex:indexOfNextEvent] pointerValue];
        if (linphone_event_log_get_type(tmp) == LinphoneEventLogTypeConferenceChatMessage) {
            nextEvent = tmp;
        }
        ++indexOfNextEvent;
    }
    
    if (!nextEvent)
        return TRUE;

    LinphoneChatMessage *nextChat = linphone_event_log_get_chat_message(nextEvent);
    if (!linphone_address_equal(linphone_chat_message_get_from_address(nextChat), linphone_chat_message_get_from_address(chat))) {
        return TRUE;
    }
    
    return FALSE;
}

#pragma mark - UITableViewDataSource Functions

- (NSInteger)numberOfSectionsInTableView:(UITableView *)tableView {
	return 1;
}

- (NSInteger)tableView:(UITableView *)tableView numberOfRowsInSection:(NSInteger)section {
	return eventList.count;
}

- (UITableViewCell *)tableView:(UITableView *)tableView cellForRowAtIndexPath:(NSIndexPath *)indexPath {
	NSString *kCellId = nil;
	LinphoneEventLog *event = [[eventList objectAtIndex:indexPath.row] pointerValue];
	if (linphone_event_log_get_type(event) == LinphoneEventLogTypeConferenceChatMessage) {
		LinphoneChatMessage *chat = linphone_event_log_get_chat_message(event);
        if (linphone_chat_message_get_file_transfer_information(chat) || linphone_chat_message_get_external_body_url(chat))
            kCellId = NSStringFromClass(UIChatBubblePhotoCell.class);
		else
			kCellId = NSStringFromClass(UIChatBubbleTextCell.class);
        
		UIChatBubbleTextCell *cell = [tableView dequeueReusableCellWithIdentifier:kCellId];
        if (!cell) {
			cell = [[NSClassFromString(kCellId) alloc] initWithIdentifier:kCellId];
        }
		[cell setEvent:event];
        if (chat) {
            cell.isFirst = [self isFirstIndexInTableView:indexPath chat:chat];
            cell.isLast = [self isLastIndexInTableView:indexPath chat:chat];
            [cell update];
        }

		[cell setChatRoomDelegate:_chatRoomDelegate];
		[super accessoryForCell:cell atPath:indexPath];
		cell.selectionStyle = UITableViewCellSelectionStyleNone;
		return cell;
	} else {
		kCellId = NSStringFromClass(UIChatNotifiedEventCell.class);
		UIChatNotifiedEventCell *cell = [tableView dequeueReusableCellWithIdentifier:kCellId];
		if (!cell)
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

static const CGFloat MESSAGE_SPACING_PERCENTAGE = 1.f;

- (CGFloat)tableView:(UITableView *)tableView heightForRowAtIndexPath:(NSIndexPath *)indexPath {
	LinphoneEventLog *event = [[eventList objectAtIndex:indexPath.row] pointerValue];
	if (linphone_event_log_get_type(event) == LinphoneEventLogTypeConferenceChatMessage) {
        LinphoneChatMessage *chat = linphone_event_log_get_chat_message(event);
        
        //If the message is followed by another one that is not from the same address, we add a little space under it
        CGFloat height = 0;
        if ([self isLastIndexInTableView:indexPath chat:chat])
            height += tableView.frame.size.height * MESSAGE_SPACING_PERCENTAGE / 100;
        if (![self isFirstIndexInTableView:indexPath chat:chat])
            height -= 20;

		return [UIChatBubbleTextCell ViewHeightForMessage:chat withWidth:self.view.frame.size.width].height + height;
	}
    return [UIChatNotifiedEventCell height];
}

- (void) tableView:(UITableView *)tableView deleteRowAtIndex:(NSIndexPath *)indexPath {
	[tableView beginUpdates];
	LinphoneEventLog *event = [[eventList objectAtIndex:indexPath.row] pointerValue];
	linphone_event_log_delete_from_database(event);
	NSInteger index = indexPath.row + _currentIndex + (totalEventList.count - eventList.count);
	if (index < totalEventList.count)
		[totalEventList removeObjectAtIndex:index];
	[eventList removeObjectAtIndex:indexPath.row];

	[tableView deleteRowsAtIndexPaths:[NSArray arrayWithObject:indexPath]
					 withRowAnimation:UITableViewRowAnimationBottom];
	[tableView endUpdates];
    [self loadData];
}

- (NSArray<UITableViewRowAction *> *)tableView:(UITableView *)tableView
				  editActionsForRowAtIndexPath:(NSIndexPath *)indexPath {
	LinphoneEventLog *event = [[eventList objectAtIndex:indexPath.row] pointerValue];
	UITableViewRowAction *deleteAction = [UITableViewRowAction rowActionWithStyle:UITableViewRowActionStyleDestructive
																			title:NSLocalizedString(@"Delete", nil)
																		  handler:^(UITableViewRowAction *action, NSIndexPath *indexPath){
																			  [self tableView:tableView deleteRowAtIndex:indexPath];
																		  }];

	UITableViewRowAction *imdnAction = [UITableViewRowAction rowActionWithStyle:UITableViewRowActionStyleNormal
																			title:NSLocalizedString(@"Info", nil)
																		  handler:^(UITableViewRowAction *action, NSIndexPath *indexPath){
																			  LinphoneChatMessage *msg = linphone_event_log_get_chat_message(event);
																			  ChatConversationImdnView *view = VIEW(ChatConversationImdnView);
																			  view.msg = msg;
																			  [PhoneMainView.instance changeCurrentView:view.compositeViewDescription];
																		  }];

	if (linphone_event_log_get_type(event) == LinphoneEventLogTypeConferenceChatMessage &&
		!(linphone_chat_room_get_capabilities(_chatRoom) & LinphoneChatRoomCapabilitiesOneToOne))
		return @[deleteAction, imdnAction];
	else
		return @[deleteAction];
}

- (void)tableView:(UITableView *)tableView
	commitEditingStyle:(UITableViewCellEditingStyle)editingStyle
	 forRowAtIndexPath:(NSIndexPath *)indexPath {
	if (editingStyle == UITableViewCellEditingStyleDelete) {
		[self tableView:tableView deleteRowAtIndex:indexPath];
	}
}

- (void)removeSelectionUsing:(void (^)(NSIndexPath *))remover {
	[super removeSelectionUsing:^(NSIndexPath *indexPath) {
		LinphoneEventLog *event = [[eventList objectAtIndex:indexPath.row] pointerValue];
		linphone_event_log_delete_from_database(event);
        NSInteger index = indexPath.row + _currentIndex + (totalEventList.count - eventList.count);
        if (index < totalEventList.count)
            [totalEventList removeObjectAtIndex:index];
		[eventList removeObjectAtIndex:indexPath.row];
	}];
}

@end
