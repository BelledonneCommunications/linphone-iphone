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
#import "FastAddressBook.h"
#import "ContactCell.h"

@implementation ContactTableViewController

#pragma mark Table view methods

void sync_toc_address_book (ABAddressBookRef addressBook, CFDictionaryRef info, void *context) {
    ContactTableViewController* controller = (ContactTableViewController*)context;
    OrderedDictionary* lAddressBookMap = controller->addressBookMap;
    @synchronized (lAddressBookMap) {
        
        // Reset Address book
        
        [lAddressBookMap removeAllObjects];
         
        NSArray *lContacts = (NSArray *)ABAddressBookCopyArrayOfAllPeople(addressBook);
        for (id lPerson in lContacts) {
            CFStringRef lValue = ABRecordCopyValue((ABRecordRef)lPerson, kABPersonFirstNameProperty);
            CFStringRef lLocalizedLabel = ABAddressBookCopyLocalizedLabel(lValue);
                
            // Put in correct subDic
            NSString *firstChar = [[(NSString *)lLocalizedLabel substringToIndex:1] uppercaseString];
            OrderedDictionary *subDic =[lAddressBookMap objectForKey: firstChar];
            if(subDic == nil) {
                subDic = [[OrderedDictionary alloc] init];
                [lAddressBookMap insertObject:subDic forKey:firstChar selector:@selector(caseInsensitiveCompare:)];
            }
            [subDic insertObject:lPerson forKey:[(NSString *)lLocalizedLabel retain] selector:@selector(caseInsensitiveCompare:)];
                
            if (lLocalizedLabel) CFRelease(lLocalizedLabel);
            CFRelease(lValue);
        }
        CFRelease(lContacts);
    }
    [(UITableView *)controller.view reloadData];
}

- (void) viewDidLoad {
    addressBookMap  = [[OrderedDictionary alloc] init];
    addressBook = ABAddressBookCreate();
    ABAddressBookRegisterExternalChangeCallback (addressBook, sync_toc_address_book, self);
    sync_toc_address_book(addressBook, nil, self);
}

- (NSInteger)numberOfSectionsInTableView:(UITableView *)tableView {
    return [addressBookMap count];
}

- (NSInteger)tableView:(UITableView *)tableView numberOfRowsInSection:(NSInteger)section {
    return [(OrderedDictionary *)[addressBookMap objectForKey: [addressBookMap keyAtIndex: section]] count];
}

- (UITableViewCell *)tableView:(UITableView *)tableView cellForRowAtIndexPath:(NSIndexPath *)indexPath {
    ContactCell *cell = [tableView dequeueReusableCellWithIdentifier:@"ContactCell"];
    if (cell == nil) {
        cell = [[ContactCell alloc] init];
    }
    
    OrderedDictionary *subDic = [addressBookMap objectForKey: [addressBookMap keyAtIndex: [indexPath section]]]; 
    
    [cell.label setText: (NSString *)[[subDic allKeys] objectAtIndex:[indexPath row]]];
    return cell;
}

- (NSString *)tableView:(UITableView *)tableView titleForHeaderInSection:(NSInteger)section {
    return [addressBookMap keyAtIndex: section];
}

- (void)tableView:(UITableView *)tableView didSelectRowAtIndexPath:(NSIndexPath *)indexPath {
    OrderedDictionary *subDic = [addressBookMap objectForKey: [addressBookMap keyAtIndex: [indexPath section]]]; 
    ABRecordRef lPerson = [subDic objectForKey: [subDic keyAtIndex:[indexPath row]]];
}

- (void)dealloc {
    [super dealloc];
    [addressBookMap removeAllObjects];
}

@end
