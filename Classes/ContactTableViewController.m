/* ContactTableViewController.m
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

#include "ContactTableViewController.h"
#import "LinphoneManager.h"
#import "FastAddressBook.h"
#import "ContactCell.h"

@implementation ContactTableViewController

#pragma mark Table view methods
NSString *contactTOC = @"ABCDEFGHIJKLMNOPQRSTUVWXYZ";

void sync_toc_address_book (ABAddressBookRef addressBook, CFDictionaryRef info, void *context) {
    ContactTableViewController* controller = (ContactTableViewController*)context;
    NSMutableDictionary* lAddressBookMap = controller->addressBookMap;
    @synchronized (lAddressBookMap) {
        
        // Reset Address book
        [lAddressBookMap removeAllObjects];

        for(int i = 0; i < [contactTOC length]; i++) {
            [lAddressBookMap setObject: [[NSMutableDictionary alloc] init] forKey:[contactTOC substringWithRange:NSMakeRange(i, 1)]];
        }
         
        NSArray *lContacts = (NSArray *)ABAddressBookCopyArrayOfAllPeople(addressBook);
        for (id lPerson in lContacts) {
            CFStringRef lValue = ABRecordCopyValue((ABRecordRef)lPerson, kABPersonFirstNameProperty);
            CFStringRef lLocalizedLabel = ABAddressBookCopyLocalizedLabel(lValue);
                
            // Put in correct subDic
            NSString *firstChar = [[(NSString *)lLocalizedLabel substringToIndex:1] uppercaseString];
            NSMutableDictionary *subDic =[lAddressBookMap objectForKey: firstChar];
            if(subDic == nil) {
                subDic = [[NSMutableDictionary alloc] init];
                [lAddressBookMap setObject: subDic forKey:firstChar];
            }
            [subDic setObject:lPerson forKey:[(NSString *)lLocalizedLabel retain]];
                
            if (lLocalizedLabel) CFRelease(lLocalizedLabel);
            CFRelease(lValue);
        }
        CFRelease(lContacts);
    }
    [controller.view reloadData];
}

- (void) viewDidLoad {
    addressBookMap  = [[NSMutableDictionary alloc] init];
    addressBook = ABAddressBookCreate();
    ABAddressBookRegisterExternalChangeCallback (addressBook, sync_toc_address_book, self);
    sync_toc_address_book(addressBook, nil, self);
}

- (NSInteger)numberOfSectionsInTableView:(UITableView *)tableView {
    return [contactTOC length];
}

- (NSInteger)tableView:(UITableView *)tableView numberOfRowsInSection:(NSInteger)section {
    return [(NSMutableDictionary *)[addressBookMap objectForKey: [contactTOC substringWithRange:NSMakeRange(section, 1)]] count];
}

- (UITableViewCell *)tableView:(UITableView *)tableView cellForRowAtIndexPath:(NSIndexPath *)indexPath {
    ContactCell *cell = [tableView dequeueReusableCellWithIdentifier:@"ContactCell"];
    if (cell == nil) {
        cell = [[ContactCell alloc] init];
    }
    
    NSMutableDictionary *subDic = [addressBookMap objectForKey: [contactTOC substringWithRange:NSMakeRange([indexPath section], 1)]]; 
    
    [cell.label setText: (NSString *)[[subDic allKeys] objectAtIndex:[indexPath row]]];
    return cell;
}

- (NSString *)tableView:(UITableView *)tableView titleForHeaderInSection:(NSInteger)section {
    return [contactTOC substringWithRange:NSMakeRange(section, 1)];
}

- (void)tableView:(UITableView *)tableView didSelectRowAtIndexPath:(NSIndexPath *)indexPath {
}

- (void)dealloc {
    [super dealloc];
    [addressBookMap removeAllObjects];
}

@end
