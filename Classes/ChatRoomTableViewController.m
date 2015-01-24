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
#import "ChatRoomTableViewController.h"
#import "UIChatRoomCell.h"
#import "Utils.h"
#import "PhoneMainView.h"

#import <NinePatch.h>

@implementation ChatRoomTableViewController

@synthesize chatRoomDelegate;

#pragma mark - Lifecycle Functions

- (void)dealloc {
    [chatRoomDelegate release];
    [self clearMessageList];

    [super dealloc];
}

#pragma mark - ViewController Functions

- (void)viewWillDisappear:(BOOL)animated {
    [super viewWillDisappear:animated];
    [TUNinePatchCache flushCache]; // Clear cache
//    [self clearMessageList];
//    chatRoom = NULL;
}
- (void)viewWillAppear:(BOOL)animated {
	[super viewWillAppear:animated];
    self.tableView.accessibilityIdentifier = @"Chat list";
	[self reloadData];
}

#pragma mark -

- (void)clearMessageList {
    if (messageList){
        ms_list_free_with_data(messageList, (void(*)(void*))linphone_chat_message_unref);
        messageList = nil;
    }
}

- (void)updateData {
    if( !chatRoom ) return;
    [self clearMessageList];
    self->messageList = linphone_chat_room_get_history(chatRoom, 0);
}

- (void)reloadData {
    [self updateData];
    [self.tableView reloadData];
    [self scrollToLastUnread:false];
}

- (void)addChatEntry:(LinphoneChatMessage*)chat {

    messageList = ms_list_append(messageList, linphone_chat_message_ref(chat));
    int pos = ms_list_size(messageList) - 1;

    [self.tableView beginUpdates];
    [self.tableView insertRowsAtIndexPaths:@[[NSIndexPath indexPathForRow:pos inSection:0]] withRowAnimation:UITableViewRowAnimationFade];
    [self.tableView endUpdates];
}

- (void)updateChatEntry:(LinphoneChatMessage*)chat {
    NSInteger index = ms_list_index(self->messageList, chat);
    if (index<0) {
		[LinphoneLogger logc:LinphoneLoggerWarning format:"chat entry doesn't exist"];
		return;
	}
	[self.tableView reloadRowsAtIndexPaths:[NSArray arrayWithObject:[NSIndexPath indexPathForRow:index inSection:0]] withRowAnimation:FALSE]; //just reload
	return;
}

- (void)scrollToBottom:(BOOL)animated {
    [self.tableView reloadData];
    int count = ms_list_size(messageList);
    if( count ){
        [self.tableView scrollToRowAtIndexPath:[NSIndexPath indexPathForRow:(count - 1) inSection:0]
                              atScrollPosition:UITableViewScrollPositionBottom
                                      animated:YES];
    }
}

- (void)debugMessages {
    if( !messageList ){
        NSLog(@"No data to debug");
        return;
    }
    MSList*item = self->messageList;
    int count = 0;
    while (item) {
        LinphoneChatMessage* msg = (LinphoneChatMessage*)item->data;
        NSLog(@"Message %d: %s", count++, linphone_chat_message_get_text(msg));
        item = item->next;
    }
}

- (void)scrollToLastUnread:(BOOL)animated {
    if(messageList == nil || chatRoom == nil) {
        return;
    }

    int index = -1;
    int count = ms_list_size(messageList);
    // Find first unread & set all entry read
    for(int i = 0; i <count; ++i) {
        int read = linphone_chat_message_is_read(ms_list_nth_data(messageList, i));
        if(read == 0) {
            if(index == -1)
                index = i;
        }
    }
    if(index == -1) {
        index = count - 1;
    }

    linphone_chat_room_mark_as_read(chatRoom);

    // Scroll to unread
    if(index >= 0) {
        [self.tableView.layer removeAllAnimations];
        [self.tableView scrollToRowAtIndexPath:[NSIndexPath indexPathForRow:index inSection:0]
                              atScrollPosition:UITableViewScrollPositionTop
                                      animated:animated];
    }
}

#pragma mark - Property Functions

- (void)setChatRoom:(LinphoneChatRoom*)room {
    chatRoom = room;
    [self reloadData];
}

#pragma mark - UITableViewDataSource Functions

- (NSInteger)numberOfSectionsInTableView:(UITableView *)tableView {
    return 1;
}

- (NSInteger)tableView:(UITableView *)tableView numberOfRowsInSection:(NSInteger)section {
    return ms_list_size(self->messageList);
}

- (UITableViewCell *)tableView:(UITableView *)tableView cellForRowAtIndexPath:(NSIndexPath *)indexPath {
    static NSString *kCellId = @"UIChatRoomCell";
    UIChatRoomCell *cell = [tableView dequeueReusableCellWithIdentifier:kCellId];
    if (cell == nil) {
        cell = [[[UIChatRoomCell alloc] initWithIdentifier:kCellId] autorelease];
    }

    LinphoneChatMessage* chat = ms_list_nth_data(self->messageList, (int)[indexPath row]);
    [cell setChatMessage:chat];
    [cell setChatRoomDelegate:chatRoomDelegate];
    return cell;
}


#pragma mark - UITableViewDelegate Functions

- (void)tableView:(UITableView *)tableView commitEditingStyle:(UITableViewCellEditingStyle)editingStyle forRowAtIndexPath:(NSIndexPath *)indexPath  {
    if(editingStyle == UITableViewCellEditingStyleDelete) {
        [tableView beginUpdates];
        LinphoneChatMessage *chat = ms_list_nth_data(self->messageList, (int)[indexPath row]);
        if( chat ){

            linphone_chat_room_delete_message(chatRoom, chat);
            messageList = ms_list_remove(messageList, chat);

            [tableView deleteRowsAtIndexPaths:[NSArray arrayWithObject:indexPath] withRowAnimation:UITableViewRowAnimationBottom];
        }
        [tableView endUpdates];

    }
}

- (UITableViewCellEditingStyle)tableView:(UITableView *)aTableView editingStyleForRowAtIndexPath:(NSIndexPath *)indexPath {
    // Detemine if it's in editing mode
    if (self.editing) {
        return UITableViewCellEditingStyleDelete;
    }
    return UITableViewCellEditingStyleNone;
}

- (CGFloat)tableView:(UITableView *)tableView heightForRowAtIndexPath:(NSIndexPath *)indexPath {
    LinphoneChatMessage* message = ms_list_nth_data(self->messageList, (int)[indexPath row]);
    return [UIChatRoomCell height:message width:[self.view frame].size.width];
}

@end
