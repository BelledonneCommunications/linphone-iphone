/* ContactDetailsLabelViewController.m
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

#import "ContactDetailsLabelViewController.h"

@implementation ContactDetailsLabelViewController

@synthesize dataList;
@synthesize tableView;
@synthesize selectedData;


#pragma mark - Property Functions

- (void)setDataList:(NSDictionary *)adatalist {
    if (self->dataList != nil) {
        [self->dataList release];
    }
    self->dataList = [adatalist retain];
    [tableView reloadData];
}


- (void)setSelectedData:(NSString *)aselectedData {
    if (selectedData != nil) {
        [selectedData release];
    }
    selectedData = [[NSString alloc] initWithString:aselectedData];
    [tableView reloadData];
}

#pragma mark - UITableViewDataSource Functions

- (NSInteger)numberOfSectionsInTableView:(UITableView *)tableView {
    return 1;
}

- (NSInteger)tableView:(UITableView *)tableView numberOfRowsInSection:(NSInteger)section {
    return [dataList count];
}

- (UITableViewCell *)tableView:(UITableView *)atableView cellForRowAtIndexPath:(NSIndexPath *)indexPath {
    static NSString *kCellId = @"ContactDetailsLabelCell";
    UITableViewCell *cell = [atableView dequeueReusableCellWithIdentifier:kCellId];
    if (cell == nil) {  
        cell = [[[UITableViewCell alloc] initWithStyle:UITableViewCellStyleValue1 reuseIdentifier:kCellId] autorelease];
    }
    NSString* key = [[dataList allKeys] objectAtIndex:[indexPath row]];
    [cell.textLabel setText:[dataList objectForKey:key]];
    if ([key compare:selectedData]==0) {
        [cell setAccessoryType:UITableViewCellAccessoryCheckmark];
    } else {
        [cell setAccessoryType:UITableViewCellAccessoryNone];
    }
    return cell;
}

- (void)tableView:(UITableView *)tableView didSelectRowAtIndexPath:(NSIndexPath *)indexPath { 
    NSString* key = [[dataList allKeys] objectAtIndex:[indexPath row]];
    [self setSelectedData:key];
    [self dismiss:key];
}

#pragma mark - Action Functions

- (IBAction)onBackClick:(id)event {
    [self dismiss];
}

@end
