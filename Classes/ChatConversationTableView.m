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

#import "LinphoneManager.h"
#import "ChatConversationTableView.h"
#import "ChatConversationImdnView.h"
#import "UIChatBubblePhotoCell.h"
#import "UIChatNotifiedEventCell.h"
#import "PhoneMainView.h"
#import "linphoneapp-Swift.h"

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
	[self startEphemeralDisplayTimer];
	[NSNotificationCenter.defaultCenter addObserver:self
										   selector:@selector(ephemeralDeleted:)
											   name:kLinphoneEphemeralMessageDeletedInRoom
											 object:nil];
}

-(void) viewWillDisappear:(BOOL)animated {
	[self dismissMessagesPopups];
	[self stopEphemeralDisplayTimer];
	[NSNotificationCenter.defaultCenter removeObserver:self];
	[super viewWillDisappear:animated];
	[_floatingScrollButton setHidden:TRUE];
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

-(bool) eventTypeIsOfInterestForOneToOneRoom:(LinphoneEventLogType)type {
	return
	type == LinphoneEventLogTypeConferenceChatMessage ||
	type == LinphoneEventLogTypeConferenceEphemeralMessageEnabled ||
	type == LinphoneEventLogTypeConferenceEphemeralMessageDisabled ||
	type == LinphoneEventLogTypeConferenceEphemeralMessageLifetimeChanged;
}

- (void)updateData {
	[self clearEventList];
	if (!_chatRoom)
		return;
	
	LinphoneChatRoomCapabilitiesMask capabilities = linphone_chat_room_get_capabilities(_chatRoom);
	bool oneToOne = capabilities & LinphoneChatRoomCapabilitiesOneToOne;
	bctbx_list_t *chatRoomEvents = linphone_chat_room_get_history_events(_chatRoom, 0);
	int unread_count = 0;
	
	bctbx_list_t *head = chatRoomEvents;
	size_t listSize = bctbx_list_size(chatRoomEvents);
	totalEventList = [[NSMutableArray alloc] initWithCapacity:listSize];
	eventList = [[NSMutableArray alloc] initWithCapacity:MIN(listSize, BASIC_EVENT_LIST)];
	BOOL autoDownload = (linphone_core_get_max_size_for_auto_download_incoming_files(LC) > -1);
	while (chatRoomEvents) {
		LinphoneEventLog *event = (LinphoneEventLog *)chatRoomEvents->data;
		if (oneToOne && ![self eventTypeIsOfInterestForOneToOneRoom:linphone_event_log_get_type(event)]) {
			chatRoomEvents = chatRoomEvents->next;
		} else {
			LinphoneChatMessage *chat = linphone_event_log_get_chat_message(event);
			if (chat && !linphone_chat_message_is_read(chat)) {
				if (unread_count == 0) {
				//	[eventList addObject:[NSString stringWithString:@"Ceci est un test wesh wesh"]];
				}
			}
			// if auto_download is available and file transfer in progress, not add event now
			if (!(autoDownload && chat && linphone_chat_message_is_file_transfer_in_progress(chat))) {
				[totalEventList addObject:[NSValue valueWithPointer:linphone_event_log_ref(event)]];
				if (listSize <= BASIC_EVENT_LIST) {
					[eventList addObject:[NSValue valueWithPointer:linphone_event_log_ref(event)]];
				}
			}
			
			chatRoomEvents = chatRoomEvents->next;
			listSize -= 1;
		}
	}
	bctbx_list_free_with_data(head, (bctbx_list_free_func)linphone_event_log_unref);
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
	if ([UIApplication sharedApplication].applicationState == UIApplicationStateActive)
		[ChatConversationView markAsRead:_chatRoom];
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


- (void) scrollToMessage:(LinphoneChatMessage *)message {
	int index = [self indexOfMesssage:message];
	if (index < 0)
		return;

	[self.tableView.layer removeAllAnimations];
	[self.tableView scrollToRowAtIndexPath:[NSIndexPath indexPathForRow:index inSection:0]
						  atScrollPosition:UITableViewScrollPositionTop
								  animated:true];
}

-(int) indexOfMesssage:(LinphoneChatMessage *)message {
	if (eventList.count == 0 || _chatRoom == nil)
		return -1;
	
	const char *msgId = linphone_chat_message_get_message_id(message);

	int index = -1;
	size_t count = eventList.count;
	for (int i = (int)count - 1; i > 0; --i) {
		LinphoneEventLog *event = [[eventList objectAtIndex:i] pointerValue];
		LinphoneChatMessage *chat = linphone_event_log_get_chat_message(event);
		if (!chat)
			continue;
		if (!strcmp(msgId, linphone_chat_message_get_message_id(chat))) {
			index = i;
			break;
		}
		
	}
	return index;
}

#pragma mark - Property Functions

- (void)setChatRoom:(LinphoneChatRoom *)room {
	_chatRoom = room;
	[self reloadData];
	[self updateEphemeralTimes];
}

static const int MAX_AGGLOMERATED_TIME=300;
static const int BASIC_EVENT_LIST=15;

- (BOOL)isFirstIndexInTableView:(NSIndexPath *)indexPath chat:(LinphoneChatMessage *)chat {
    LinphoneEventLog *previousEvent = nil;
    NSInteger indexOfPreviousEvent = indexPath.row - 1;
    if (indexOfPreviousEvent > -1) {
		previousEvent = [[eventList objectAtIndex:indexOfPreviousEvent] pointerValue];
        if (linphone_event_log_get_type(previousEvent) != LinphoneEventLogTypeConferenceChatMessage) {
			return TRUE;
        }
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

- (void)tableView:(UITableView *)tableView didEndDisplayingCell:(UITableViewCell *)cell forRowAtIndexPath:(NSIndexPath*)indexPath {
	if (!_chatRoom && [[cell reuseIdentifier] isEqualToString:@"UIChatBubblePhotoCell"]) {
		[(UIChatBubbleTextCell *)cell clearEncryptedFiles];
	}
	if ([cell isKindOfClass:[UIChatBubbleTextCell class]] ||[cell isKindOfClass:[UIChatBubblePhotoCell class]])
		[(UIChatBubbleTextCell *)cell dismissPopup];
}

-(void) dismissMessagesPopups {
	for (UITableViewCell *cell in self.tableView.visibleCells) {
		if (![[cell reuseIdentifier] isEqualToString:NSStringFromClass(UIChatNotifiedEventCell.class)])
			[(UIChatBubbleTextCell *)cell dismissPopup];
	}
}

-(UIChatBubbleTextCell *)buildMessageCell:(LinphoneEventLog *) event {
	NSString *kCellId = nil;
	LinphoneChatMessage *chat = linphone_event_log_get_chat_message(event);
	BOOL isConferenceIcs = [ICSBubbleView isConferenceInvitationMessageWithCmessage:chat];
	if (!isConferenceIcs && (linphone_chat_message_get_file_transfer_information(chat) || linphone_chat_message_get_external_body_url(chat)))
					kCellId = NSStringFromClass(UIChatBubblePhotoCell.class);
	else
		kCellId = NSStringFromClass(UIChatBubbleTextCell.class);
	// To use less memory and to avoid overlapping. To be improved.
	UIChatBubbleTextCell *cell = [self.tableView dequeueReusableCellWithIdentifier:kCellId];
	cell = [[NSClassFromString(kCellId) alloc] initWithIdentifier:kCellId];
	[cell setEvent:event];
	return cell;
}

- (UITableViewCell *)tableView:(UITableView *)tableView cellForRowAtIndexPath:(NSIndexPath *)indexPath {
	NSString *kCellId = nil;
	LinphoneEventLog *event = [[eventList objectAtIndex:indexPath.row] pointerValue];
	if (linphone_event_log_get_type(event) == LinphoneEventLogTypeConferenceChatMessage) {
		UIChatBubbleTextCell *cell = [self buildMessageCell:event];
		LinphoneChatMessage *chat = linphone_event_log_get_chat_message(event);
		if (chat) {
				cell.isFirst = [self isFirstIndexInTableView:indexPath chat:chat];
				cell.isLast = [self isLastIndexInTableView:indexPath chat:chat];
				[cell update];
		}
		[cell setChatRoomDelegate:_chatRoomDelegate];
		[super accessoryForCell:cell atPath:indexPath];
		cell.selectionStyle = UITableViewCellSelectionStyleNone;
		cell.tableController = self;
		cell.popupMenuAllowed = true;
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

- (void)scrollViewDidScroll:(UIScrollView *)scrollView {
	if (scrollView.contentOffset.y >= (scrollView.contentSize.height - scrollView.frame.size.height)) {
		if ([UIApplication sharedApplication].applicationState == UIApplicationStateActive)
			[ChatConversationView markAsRead:_chatRoom];
	}
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

- (nullable UISwipeActionsConfiguration *)tableView:(UITableView *)tableView
  leadingSwipeActionsConfigurationForRowAtIndexPath:(NSIndexPath *)indexPath {
	
	LinphoneEventLog *event = [[eventList objectAtIndex:indexPath.row] pointerValue];
	
	if (linphone_event_log_get_type(event) != LinphoneEventLogTypeConferenceChatMessage) {
		return [UISwipeActionsConfiguration configurationWithActions:@[]];
	}
	
	UIContextualAction *replyAction = [UIContextualAction contextualActionWithStyle:UIContextualActionStyleNormal
																		 title:NSLocalizedString(@"Reply", nil)
																	   handler:^(UIContextualAction * _Nonnull action, __kindof UIView * _Nonnull sourceView, void (^ _Nonnull completionHandler)(BOOL)) {
		LinphoneChatMessage *msg = linphone_event_log_get_chat_message(event);
		[VIEW(ChatConversationView) initiateReplyViewForMessage:msg];
		[self scrollToBottom:TRUE];
	}];
	
		UISwipeActionsConfiguration *swipeActionConfig = [UISwipeActionsConfiguration configurationWithActions:@[replyAction]];
		swipeActionConfig.performsFirstActionWithFullSwipe = YES;
		return swipeActionConfig;
}

- (nullable UISwipeActionsConfiguration *)tableView:(UITableView *)tableView
 trailingSwipeActionsConfigurationForRowAtIndexPath:(NSIndexPath *)indexPath {
	
	UIContextualAction *delete = [UIContextualAction contextualActionWithStyle:UIContextualActionStyleDestructive
																		 title:NSLocalizedString(@"Delete", nil)
																	   handler:^(UIContextualAction * _Nonnull action, __kindof UIView * _Nonnull sourceView, void (^ _Nonnull completionHandler)(BOOL)) {
		[self tableView:tableView deleteRowAtIndex:indexPath];
	}];
	
	LinphoneEventLog *event = [[eventList objectAtIndex:indexPath.row] pointerValue];
	UIContextualAction *imdnAction = [UIContextualAction contextualActionWithStyle:UIContextualActionStyleNormal
																			 title:NSLocalizedString(@"Info", nil)
																		   handler:^(UIContextualAction * _Nonnull action, __kindof UIView * _Nonnull sourceView, void (^ _Nonnull completionHandler)(BOOL)) {
		ChatConversationImdnView *view = VIEW(ChatConversationImdnView);
		view.event = event;
		[PhoneMainView.instance changeCurrentView:view.compositeViewDescription];
	}];
	
	UISwipeActionsConfiguration *swipeActionConfig;
	
	if (linphone_event_log_get_type(event) == LinphoneEventLogTypeConferenceChatMessage &&
		!(linphone_chat_room_get_capabilities(_chatRoom) & LinphoneChatRoomCapabilitiesOneToOne)) {
		swipeActionConfig = [UISwipeActionsConfiguration configurationWithActions:@[delete, imdnAction]];
	} else {
		swipeActionConfig = [UISwipeActionsConfiguration configurationWithActions:@[delete]];
	}
	
	swipeActionConfig.performsFirstActionWithFullSwipe = YES;
	return swipeActionConfig;
}

- (void)removeSelectionUsing:(void (^)(NSIndexPath *))remover {
	[super removeSelectionUsing:^(NSIndexPath *indexPath) {
		LinphoneEventLog *event = [[eventList objectAtIndex:indexPath.row] pointerValue];
		if (linphone_event_log_get_chat_message(event)) {
			linphone_chat_room_delete_message(_chatRoom, linphone_event_log_get_chat_message(event));
		} else {
			linphone_event_log_delete_from_database(event);
		}
        NSInteger index = indexPath.row + _currentIndex + (totalEventList.count - eventList.count);
        if (index < totalEventList.count)
            [totalEventList removeObjectAtIndex:index];
		[eventList removeObjectAtIndex:indexPath.row];
	}];
}

#pragma mark ephemeral messages

-(void) startEphemeralDisplayTimer {
	_ephemeralDisplayTimer = [NSTimer scheduledTimerWithTimeInterval:1
															  target:self
															selector:@selector(updateEphemeralTimes)
															userInfo:nil
															 repeats:YES];
}

-(void) updateEphemeralTimes {
	NSDateComponentsFormatter *f= [[NSDateComponentsFormatter alloc] init];
	f.unitsStyle = NSDateComponentsFormatterUnitsStylePositional;
	f.zeroFormattingBehavior = NSDateComponentsFormatterZeroFormattingBehaviorPad;
		
	for (NSValue *v in eventList) {
		LinphoneEventLog *event = [v pointerValue];
		if (linphone_event_log_get_type(event) == LinphoneEventLogTypeConferenceChatMessage) {
			LinphoneChatMessage *msg = linphone_event_log_get_chat_message(event);
			if (linphone_chat_message_is_ephemeral(msg)) {
				UIChatBubbleTextCell *cell = (UIChatBubbleTextCell  *)[self.tableView cellForRowAtIndexPath:[NSIndexPath indexPathForRow:[eventList indexOfObject:v] inSection:0]];
				long duration = linphone_chat_message_get_ephemeral_expire_time(msg) == 0 ?
					linphone_chat_room_get_ephemeral_lifetime(linphone_chat_message_get_chat_room(msg)) :
					linphone_chat_message_get_ephemeral_expire_time(msg)-[NSDate date].timeIntervalSince1970;
				f.allowedUnits = (duration > 86400 ? kCFCalendarUnitDay : 0)|(duration > 3600 ? kCFCalendarUnitHour : 0)|kCFCalendarUnitMinute|kCFCalendarUnitSecond;
				cell.ephemeralTime.text =  [f stringFromTimeInterval:duration];
				cell.ephemeralTime.hidden = NO;
				cell.ephemeralIcon.hidden = NO;
			}
		}
	}
}

-(void) stopEphemeralDisplayTimer {
	[_ephemeralDisplayTimer invalidate];
}

- (void)ephemeralDeleted:(NSNotification *)notif {
	LinphoneChatRoom *r =[[notif.userInfo objectForKey:@"room"] pointerValue];
	if (r ==_chatRoom)
		[self reloadData];
}


@end
