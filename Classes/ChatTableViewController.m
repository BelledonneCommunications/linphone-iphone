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

#import "linphonecore.h"
#import "PhoneMainView.h"

@implementation ChatTableViewController


#pragma mark - Lifecycle Functions

- (id)init {
    if((self = [super init]) != nil) {
        self->editMode = false;
    }
    return self;
}

- (void)dealloc {
    if(data != nil)
        [data release];
    [super dealloc];
}


#pragma mark - 

- (void)reloadData {
    if(data != nil)
        [data release];
    data = [[ChatModel listConversations] retain];
}

- (void) toggleEditMode {
    editMode = !editMode;
    [(UITableView*)[self view] reloadData];
}

- (void) enterEditMode {
    if(!editMode) {
        [self toggleEditMode];
    }
}

- (void) exitEditMode {
    if(editMode) {
        [self toggleEditMode];
    }
}

#pragma mark - UITableViewDataSource Functions

- (NSInteger)numberOfSectionsInTableView:(UITableView *)tableView
{
    return 1;
}

- (NSInteger)tableView:(UITableView *)tableView numberOfRowsInSection:(NSInteger)section
{
    [self reloadData];
    return [data count];
}

- (UITableViewCell *)tableView:(UITableView *)tableView cellForRowAtIndexPath:(NSIndexPath *)indexPath
{
    UIChatCell *cell = [tableView dequeueReusableCellWithIdentifier:@"UIChatCell"];
    if (cell == nil) {
        cell = [[UIChatCell alloc] initWithIdentifier:@"UIChatCell"];
    }
    
    [cell setChat:[data objectAtIndex:[indexPath row]]];
    if(editMode) 
        [cell enterEditMode];
    else 
        [cell exitEditMode];
    [cell update];
    
    return cell;
}


#pragma mark - UITableViewDelegate Functions

- (void)tableView:(UITableView *)tableView didSelectRowAtIndexPath:(NSIndexPath *)indexPath {
    [tableView deselectRowAtIndexPath:indexPath animated:NO];
    ChatModel *chat = [data objectAtIndex:[indexPath row]];
    
    // Go to dialer view
    NSDictionary *dict = [[[NSDictionary alloc] initWithObjectsAndKeys:
                           [[[NSArray alloc] initWithObjects: [chat remoteContact], nil] autorelease]
                           , @"setRemoteContact:",
                           nil] autorelease];
    [[PhoneMainView instance] changeView:PhoneView_ChatRoom dict:dict push:TRUE];
}

@end
