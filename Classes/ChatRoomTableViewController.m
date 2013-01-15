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

@synthesize remoteAddress;
@synthesize chatRoomDelegate;

#pragma mark - Lifecycle Functions

- (void)dealloc {
    [remoteAddress release];
    [chatRoomDelegate release];
    
    [super dealloc];
}

#pragma mark - ViewController Functions

- (void)viewWillDisappear:(BOOL)animated {
    [super viewWillDisappear:animated];
    [TUNinePatchCache flushCache]; // Clear cache
    if(data != nil) {
        [data removeAllObjects];
        [data release];
        data = nil;
    }
}
- (void)viewWillAppear:(BOOL)animated {
	[self loadData];
}

#pragma mark -

- (void)loadData {
    if(data != nil) {
        [data removeAllObjects];
        [data release];
    }
    data = [[ChatModel listMessages:remoteAddress] retain];
    [[self tableView] reloadData];
    [self scrollToLastUnread:false];
}

- (void)addChatEntry:(ChatModel*)chat {
    if(data == nil) {
        [LinphoneLogger logc:LinphoneLoggerWarning format:"Cannot add entry: null data"];
        return;
    }
    [self.tableView beginUpdates];
    int pos = [data count];
    [data insertObject:chat atIndex:pos];
    [self.tableView insertRowsAtIndexPaths:[NSArray arrayWithObject:[NSIndexPath indexPathForRow:pos inSection:0]] withRowAnimation:UITableViewRowAnimationFade];
    [self.tableView endUpdates];
}

- (void)updateChatEntry:(ChatModel*)chat {
    if(data == nil) {
        [LinphoneLogger logc:LinphoneLoggerWarning format:"Cannot update entry: null data"];
        return;
    }
	NSInteger index = [data indexOfObject:chat];
    if (index<0) {
		[LinphoneLogger logc:LinphoneLoggerWarning format:"chat entries diesn not exixt"];
		return;
	}
	[self.tableView reloadRowsAtIndexPaths:[NSArray arrayWithObject:[NSIndexPath indexPathForRow:index inSection:0]] withRowAnimation:FALSE];; //just reload
	return;
}

- (void)scrollToBottom:(BOOL)animated {
    CGSize size = [self.tableView contentSize];
    CGRect bounds = [self.tableView bounds];
    bounds.origin.y = size.height - bounds.size.height;
    
    [self.tableView.layer removeAllAnimations];
    [self.tableView scrollRectToVisible:bounds animated:animated];
}

- (void)scrollToLastUnread:(BOOL)animated {
    if(data == nil) {
        [LinphoneLogger logc:LinphoneLoggerWarning format:"Cannot add entry: null data"];
        return;
    }
    
    int index = -1;
    // Find first unread & set all entry read
    for(int i = 0; i <[data count]; ++i) {
        ChatModel *chat = [data objectAtIndex:i];
        if([[chat read] intValue] == 0) {
            [chat setRead:[NSNumber numberWithInt:1]];
            if(index == -1)
                index = i;
        }
    }
    if(index == -1) {
        index = [data count] - 1;
    }
    
    // Scroll to unread
    if(index >= 0) {
        [self.tableView.layer removeAllAnimations];
        [self.tableView scrollToRowAtIndexPath:[NSIndexPath indexPathForRow:index inSection:0] 
                              atScrollPosition:UITableViewScrollPositionTop
                                      animated:animated];
    }
}

#pragma mark - Property Functions

- (void)setRemoteAddress:(NSString *)aremoteAddress {
    if(remoteAddress != nil) {
        [remoteAddress release]; 
    }
    self->remoteAddress = [aremoteAddress copy];
    [self loadData];
}


#pragma mark - UITableViewDataSource Functions

- (NSInteger)numberOfSectionsInTableView:(UITableView *)tableView {
    return 1;
}

- (NSInteger)tableView:(UITableView *)tableView numberOfRowsInSection:(NSInteger)section {
    return [data count];
}

- (UITableViewCell *)tableView:(UITableView *)tableView cellForRowAtIndexPath:(NSIndexPath *)indexPath {
    static NSString *kCellId = @"UIChatRoomCell";
    UIChatRoomCell *cell = [tableView dequeueReusableCellWithIdentifier:kCellId];
    if (cell == nil) {
        cell = [[[UIChatRoomCell alloc] initWithIdentifier:kCellId] autorelease];
    }
    
    [cell setChat:[data objectAtIndex:[indexPath row]]];
    [cell setChatRoomDelegate:chatRoomDelegate];
    return cell;
}


#pragma mark - UITableViewDelegate Functions

- (void)tableView:(UITableView *)tableView commitEditingStyle:(UITableViewCellEditingStyle)editingStyle forRowAtIndexPath:(NSIndexPath *)indexPath  {
    if(editingStyle == UITableViewCellEditingStyleDelete) {
        [tableView beginUpdates];
        ChatModel *chat = [data objectAtIndex:[indexPath row]];
        [data removeObjectAtIndex:[indexPath row]];
        [chat delete];
        [tableView deleteRowsAtIndexPaths:[NSArray arrayWithObject:indexPath] withRowAnimation:UITableViewRowAnimationFade];
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
    ChatModel *chat = [data objectAtIndex:[indexPath row]];
    return [UIChatRoomCell height:chat width:[self.view frame].size.width];
}

@end
