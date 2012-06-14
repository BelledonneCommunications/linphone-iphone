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
#import "UIContactCell.h"

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
            CFStringRef lFirstName = ABRecordCopyValue((ABRecordRef)lPerson, kABPersonFirstNameProperty);
            CFStringRef lLocalizedFirstName = ABAddressBookCopyLocalizedLabel(lFirstName);
            CFStringRef lLastName = ABRecordCopyValue((ABRecordRef)lPerson, kABPersonLastNameProperty);
            CFStringRef lLocalizedLastName = ABAddressBookCopyLocalizedLabel(lLastName);
            NSString *name =[NSString stringWithFormat:@"%@%@", [(NSString *)lLocalizedFirstName retain], [(NSString *)lLocalizedLastName retain]];

            // Put in correct subDic
            NSString *firstChar = [[name substringToIndex:1] uppercaseString];
            OrderedDictionary *subDic =[lAddressBookMap objectForKey: firstChar];
            if(subDic == nil) {
                subDic = [[OrderedDictionary alloc] init];
                [lAddressBookMap insertObject:subDic forKey:firstChar selector:@selector(caseInsensitiveCompare:)];
            }
            [subDic insertObject:lPerson forKey:name selector:@selector(caseInsensitiveCompare:)];
              
            CFRelease(lLocalizedLastName);
            CFRelease(lLastName);
            CFRelease(lLocalizedFirstName);
            CFRelease(lFirstName);
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
    UIContactCell *cell = [tableView dequeueReusableCellWithIdentifier:@"UIContactCell"];
    if (cell == nil) {
        cell = [[UIContactCell alloc] init];
    }
    
    OrderedDictionary *subDic = [addressBookMap objectForKey: [addressBookMap keyAtIndex: [indexPath section]]]; 
    
    NSString *key = [[subDic allKeys] objectAtIndex:[indexPath row]];
    ABRecordRef record = [subDic objectForKey:key];
    
    CFStringRef lFirstName = ABRecordCopyValue(record, kABPersonFirstNameProperty);
    CFStringRef lLocalizedFirstName = ABAddressBookCopyLocalizedLabel(lFirstName);
    CFStringRef lLastName = ABRecordCopyValue(record, kABPersonLastNameProperty);
    CFStringRef lLocalizedLastName = ABAddressBookCopyLocalizedLabel(lLastName);
    
    [cell.firstName setText: [(NSString *)lLocalizedFirstName retain]];
    [cell.lastName setText: [(NSString *)lLocalizedLastName retain]];
    [cell update];
    
    CFRelease(lLocalizedLastName);
    CFRelease(lLastName);
    CFRelease(lLocalizedFirstName);
    CFRelease(lFirstName);
    return cell;
}

- (NSString *)tableView:(UITableView *)tableView titleForHeaderInSection:(NSInteger)section {
    return [addressBookMap keyAtIndex: section];
}

- (void)tableView:(UITableView *)tableView didSelectRowAtIndexPath:(NSIndexPath *)indexPath {
    OrderedDictionary *subDic = [addressBookMap objectForKey: [addressBookMap keyAtIndex: [indexPath section]]]; 
    ABRecordRef lPerson = [subDic objectForKey: [subDic keyAtIndex:[indexPath row]]];
    // TODO
}

- (void)dealloc {
    [super dealloc];
    [addressBookMap removeAllObjects];
}

@end
