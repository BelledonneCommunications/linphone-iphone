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
#import "UIChatCell.h"

@implementation ChatRoomTableViewController

@synthesize data;


#pragma mark - 

- (void)setData:(NSArray *)adata {
    if(self->data != nil)
        [self->data release];
    self->data = [adata retain];
    [[self tableView] reloadData];
}


#pragma mark - UITableViewDataSource Functions

- (NSInteger)numberOfSectionsInTableView:(UITableView *)tableView {
    return 1;
}

- (NSInteger)tableView:(UITableView *)tableView numberOfRowsInSection:(NSInteger)section {
    return [data count];
}

- (UITableViewCell *)tableView:(UITableView *)tableView cellForRowAtIndexPath:(NSIndexPath *)indexPath {
    UIChatCell *cell = [tableView dequeueReusableCellWithIdentifier:@"UIChatCell"];
    if (cell == nil) {
        cell = [[UIChatCell alloc] init];
    }
    
    [cell setChat:[data objectAtIndex:[indexPath row]]];
    [cell update];
    
    return cell;
}

@end
