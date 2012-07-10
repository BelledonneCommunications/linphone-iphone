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

#import "ChatRoomTableViewController.h"
#import "UIChatRoomCell.h"
#import "UIChatRoomHeader.h"

@implementation ChatRoomTableViewController

@synthesize remoteContact;

#pragma mark - 

- (void)reloadData {
    if(data != nil)
        [data release];
    data = [[ChatModel listMessages:remoteContact] retain];
}


#pragma mark - Property Functions

- (void)setRemoteContact:(NSString *)aremoteContact {
    self->remoteContact = aremoteContact;
    [[self tableView] reloadData];
}


#pragma mark - UITableViewDataSource Functions

- (NSInteger)numberOfSectionsInTableView:(UITableView *)tableView {
    return 1;
}

- (NSInteger)tableView:(UITableView *)tableView numberOfRowsInSection:(NSInteger)section {
    [self reloadData];
    return [data count];
}

- (UITableViewCell *)tableView:(UITableView *)tableView cellForRowAtIndexPath:(NSIndexPath *)indexPath {
    static NSString *kCellId = @"UIChatRoomCell";
    UIChatRoomCell *cell = [tableView dequeueReusableCellWithIdentifier:kCellId];
    if (cell == nil) {
        cell = [[[UIChatRoomCell alloc] initWithIdentifier:kCellId] autorelease];
    }
    
    [cell setChat:[data objectAtIndex:[indexPath row]]];
    [cell update];
    
    return cell;
}


#pragma mark - UITableViewDelegate Functions

- (UIView *)tableView:(UITableView *)tableView viewForHeaderInSection:(NSInteger)section {    
    UIChatRoomHeader *headerController = [[UIChatRoomHeader alloc] init];
    UIView *headerView = [headerController view];
    [headerController setContact:remoteContact];
    [headerController update];
    [headerController release];
    return headerView;
}

- (CGFloat)tableView:(UITableView *)tableView heightForHeaderInSection:(NSInteger)section { 
    return [UIChatRoomHeader height];
}

- (CGFloat)tableView:(UITableView *)tableView heightForRowAtIndexPath:(NSIndexPath *)indexPath {
    ChatModel *chat = [data objectAtIndex:[indexPath row]];
    return [UIChatRoomCell height:chat];
}

@end
