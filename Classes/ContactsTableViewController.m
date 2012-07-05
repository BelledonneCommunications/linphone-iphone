/* ContactsTableViewController.m
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

#import "ContactsTableViewController.h"
#import "UIContactCell.h"
#import "LinphoneManager.h"
#import "PhoneMainView.h"

@implementation ContactsTableViewController

void sync_toc_address_book (ABAddressBookRef addressBook, CFDictionaryRef info, void *context) {
    ContactsTableViewController* controller = (ContactsTableViewController*)context;
    OrderedDictionary* lAddressBookMap = controller->addressBookMap;
    @synchronized (lAddressBookMap) {
        
        // Reset Address book
        
        [lAddressBookMap removeAllObjects];
         
        NSArray *lContacts = (NSArray *)ABAddressBookCopyArrayOfAllPeople(addressBook);
        for (id lPerson in lContacts) {
            CFStringRef lFirstName = ABRecordCopyValue((ABRecordRef)lPerson, kABPersonFirstNameProperty);
            CFStringRef lLocalizedFirstName = (lFirstName != nil)? ABAddressBookCopyLocalizedLabel(lFirstName): nil;
            CFStringRef lLastName = ABRecordCopyValue((ABRecordRef)lPerson, kABPersonLastNameProperty);
            CFStringRef lLocalizedLastName = (lLastName != nil)? ABAddressBookCopyLocalizedLabel(lLastName): nil;
            NSString *name = nil;
            if(lLocalizedFirstName != nil && lLocalizedLastName != nil) {
                 name=[NSString stringWithFormat:@"%@%@", [(NSString *)lLocalizedFirstName retain], [(NSString *)lLocalizedLastName retain]];
            } else if(lLocalizedLastName != nil) {
                name=[NSString stringWithFormat:@"%@",[(NSString *)lLocalizedLastName retain]];
            } else if(lLocalizedFirstName != nil) {
                name=[NSString stringWithFormat:@"%@",[(NSString *)lLocalizedFirstName retain]];
            }
            if(name != nil) {
                // Put in correct subDic
                NSString *firstChar = [[name substringToIndex:1] uppercaseString];
                OrderedDictionary *subDic =[lAddressBookMap objectForKey: firstChar];
                if(subDic == nil) {
                    subDic = [[OrderedDictionary alloc] init];
                    [lAddressBookMap insertObject:subDic forKey:firstChar selector:@selector(caseInsensitiveCompare:)];
                }
                [subDic insertObject:lPerson forKey:name selector:@selector(caseInsensitiveCompare:)];
            }
            if(lLocalizedLastName != nil)
                CFRelease(lLocalizedLastName);
            if(lLastName != nil)
                CFRelease(lLastName);
            if(lLocalizedFirstName != nil)
                CFRelease(lLocalizedFirstName);
            if(lFirstName != nil)
                CFRelease(lFirstName);
        }
        CFRelease(lContacts);
    }
    [(UITableView *)controller.view reloadData];
}


#pragma mark - ViewController Functions

- (void) viewDidLoad {
    addressBookMap  = [[OrderedDictionary alloc] init];
    addressBook = ABAddressBookCreate();
    ABAddressBookRegisterExternalChangeCallback (addressBook, sync_toc_address_book, self);
    sync_toc_address_book(addressBook, nil, self);
}


#pragma mark - UITableViewDataSource Functions

- (NSInteger)numberOfSectionsInTableView:(UITableView *)tableView {
    return [addressBookMap count];
}

- (NSInteger)tableView:(UITableView *)tableView numberOfRowsInSection:(NSInteger)section {
    return [(OrderedDictionary *)[addressBookMap objectForKey: [addressBookMap keyAtIndex: section]] count];
}

- (UITableViewCell *)tableView:(UITableView *)tableView cellForRowAtIndexPath:(NSIndexPath *)indexPath {
    UIContactCell *cell = [tableView dequeueReusableCellWithIdentifier:@"UIContactCell"];
    if (cell == nil) {
        cell = [[UIContactCell alloc] initWithIdentifier:@"UIContactCell"];
    }
    
    OrderedDictionary *subDic = [addressBookMap objectForKey: [addressBookMap keyAtIndex: [indexPath section]]]; 
    
    NSString *key = [[subDic allKeys] objectAtIndex:[indexPath row]];
    ABRecordRef record = [subDic objectForKey:key];

    [cell update: record];
    
    return cell;
}

- (NSString *)tableView:(UITableView *)tableView titleForHeaderInSection:(NSInteger)section {
    return [addressBookMap keyAtIndex: section];
}

- (void)tableView:(UITableView *)tableView didSelectRowAtIndexPath:(NSIndexPath *)indexPath {
    OrderedDictionary *subDic = [addressBookMap objectForKey: [addressBookMap keyAtIndex: [indexPath section]]]; 
    ABRecordRef lPerson = [subDic objectForKey: [subDic keyAtIndex:[indexPath row]]];
    // TODO
    ABMultiValueRef lPhoneNumbers = ABRecordCopyValue((ABRecordRef)lPerson, kABPersonPhoneProperty);
    for(CFIndex i = 0; i < ABMultiValueGetCount(lPhoneNumbers); i++) {
        CFStringRef lLabel = ABMultiValueCopyLabelAtIndex(lPhoneNumbers, i);
        if ([(NSString*)lLabel isEqualToString:(NSString*)kABPersonPhoneMainLabel]) {
            CFStringRef lNumber = ABMultiValueCopyValueAtIndex(lPhoneNumbers,i);
            NSString *number = [(NSString *)lNumber retain];
            
            // Go to dialer view
            NSDictionary *dict = [[[NSDictionary alloc] initWithObjectsAndKeys:
                                  [[[NSArray alloc] initWithObjects: number, nil] autorelease]
                                  , @"setAddress:",
                                  nil] autorelease];
            [[PhoneMainView instance] changeView:PhoneView_Dialer dict:dict];
            
            CFRelease(lNumber);
            break;
        }
        CFRelease(lLabel);
    }
    CFRelease(lPhoneNumbers);
}


#pragma mark - Lifecycle Functions

- (void)dealloc {
    [super dealloc];
    [addressBookMap removeAllObjects];
}

@end
