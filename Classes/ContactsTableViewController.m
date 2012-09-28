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
#import "UACellBackgroundView.h"
#import "UILinphone.h"
#import "Utils.h"

@implementation ContactsTableViewController

static void sync_address_book (ABAddressBookRef addressBook, CFDictionaryRef info, void *context);


#pragma mark - Lifecycle Functions

- (void)initContactsTableViewController {
    addressBookMap  = [[OrderedDictionary alloc] init];
    avatarMap = [[NSMutableDictionary alloc] init];
    
    addressBook = ABAddressBookCreate();
    ABAddressBookRegisterExternalChangeCallback(addressBook, sync_address_book, self);
}

- (id)init {
    self = [super init];
    if (self) {
		[self initContactsTableViewController];
    }
    return self;
}

- (id)initWithCoder:(NSCoder *)decoder {
    self = [super initWithCoder:decoder];
    if (self) {
		[self initContactsTableViewController];
	}
    return self;
}	

- (void)dealloc {
    ABAddressBookUnregisterExternalChangeCallback(addressBook, sync_address_book, self);
    CFRelease(addressBook);
    [addressBookMap release];
    [avatarMap release];
    [super dealloc];
}


#pragma mark - 

- (void)loadData {
    [LinphoneLogger logc:LinphoneLoggerLog format:"Load contact list"];
    @synchronized (addressBookMap) {
        
        // Reset Address book
        [addressBookMap removeAllObjects];
        
        NSArray *lContacts = (NSArray *)ABAddressBookCopyArrayOfAllPeople(addressBook);
        for (id lPerson in lContacts) {
            BOOL add = true;
            if([ContactSelection getSipFilter]) {
                add = false;
                ABMultiValueRef lMap = ABRecordCopyValue((ABRecordRef)lPerson, kABPersonInstantMessageProperty);
                for(int i = 0; i < ABMultiValueGetCount(lMap); ++i) {
                    CFDictionaryRef lDict = ABMultiValueCopyValueAtIndex(lMap, i);
                    if(CFDictionaryContainsKey(lDict, kABPersonInstantMessageServiceKey)) {
                        CFStringRef serviceKey = CFDictionaryGetValue(lDict, kABPersonInstantMessageServiceKey);
                        if(CFStringCompare((CFStringRef)@"SIP", serviceKey, kCFCompareCaseInsensitive) == 0) {
                            add = true;
                        }
                    } else {
                        NSString* usernameKey = CFDictionaryGetValue(lDict, kABPersonInstantMessageUsernameKey);
                        if([usernameKey hasPrefix:@"sip:"]) {
                            add = true;
                        }
                    }
                    CFRelease(lDict);
                }
            }
            if(add) {
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
                } else {
                    
                }
                if(name != nil && [name length] > 0) {
                    // Put in correct subDic
                    NSString *firstChar = [[name substringToIndex:1] uppercaseString];
                    if([firstChar characterAtIndex:0] < 'A' || [firstChar characterAtIndex:0] > 'Z') {
                        firstChar = @"#";
                    }
                    OrderedDictionary *subDic =[addressBookMap objectForKey: firstChar];
                    if(subDic == nil) {
                        subDic = [[[OrderedDictionary alloc] init] autorelease];
                        [addressBookMap insertObject:subDic forKey:firstChar selector:@selector(caseInsensitiveCompare:)];
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
        }
        CFRelease(lContacts);
    }
    [self.tableView reloadData];
}

static void sync_address_book (ABAddressBookRef addressBook, CFDictionaryRef info, void *context) {
    ContactsTableViewController* controller = (ContactsTableViewController*)context;
    ABAddressBookRevert(addressBook);
    [controller->avatarMap removeAllObjects];
    [controller loadData];
}

#pragma mark - ViewController Functions

- (void)viewWillDisappear:(BOOL)animated {
    [super viewWillDisappear:animated];
}


#pragma mark - UITableViewDataSource Functions

- (NSArray *)sectionIndexTitlesForTableView:(UITableView *)tableView {
    return [addressBookMap allKeys];
}

- (NSInteger)numberOfSectionsInTableView:(UITableView *)tableView {
    return [addressBookMap count];
}

- (NSInteger)tableView:(UITableView *)tableView numberOfRowsInSection:(NSInteger)section {
    return [(OrderedDictionary *)[addressBookMap objectForKey: [addressBookMap keyAtIndex: section]] count];

}

- (UITableViewCell *)tableView:(UITableView *)tableView cellForRowAtIndexPath:(NSIndexPath *)indexPath {
    static NSString *kCellId = @"UIContactCell";   
    UIContactCell *cell = [tableView dequeueReusableCellWithIdentifier:kCellId];
    if (cell == nil) {
        cell = [[[UIContactCell alloc] initWithIdentifier:kCellId] autorelease];
        
        // Background View
        UACellBackgroundView *selectedBackgroundView = [[[UACellBackgroundView alloc] initWithFrame:CGRectZero] autorelease];
        cell.selectedBackgroundView = selectedBackgroundView;
        [selectedBackgroundView setBackgroundColor:LINPHONE_TABLE_CELL_BACKGROUND_COLOR];
    }
    OrderedDictionary *subDic = [addressBookMap objectForKey: [addressBookMap keyAtIndex: [indexPath section]]]; 
    
    NSString *key = [[subDic allKeys] objectAtIndex:[indexPath row]];
    ABRecordRef contact = [subDic objectForKey:key];
    
    // Cached avatar
    UIImage *image = nil;
    id data = [avatarMap objectForKey:[NSNumber numberWithInt: ABRecordGetRecordID(contact)]];
    if(data == nil) {
        image = [FastAddressBook getContactImage:contact thumbnail:true];
        if(image != nil) {
            [avatarMap setObject:image forKey:[NSNumber numberWithInt: ABRecordGetRecordID(contact)]];
        } else {
            [avatarMap setObject:[NSNull null] forKey:[NSNumber numberWithInt: ABRecordGetRecordID(contact)]];
        }
    } else if(data != [NSNull null]) {
        image = data;
    }
    if(image == nil) {
        image = [UIImage imageNamed:@"avatar_unknown_small.png"];
    }
    [[cell avatarImage] setImage:image];
    
    [cell setContact: contact];
    return cell;
}

- (NSString *)tableView:(UITableView *)tableView titleForHeaderInSection:(NSInteger)section {
        return [addressBookMap keyAtIndex: section];
}

- (void)tableView:(UITableView *)tableView didSelectRowAtIndexPath:(NSIndexPath *)indexPath {
    OrderedDictionary *subDic = [addressBookMap objectForKey: [addressBookMap keyAtIndex: [indexPath section]]]; 
    ABRecordRef lPerson = [subDic objectForKey: [subDic keyAtIndex:[indexPath row]]];
    
    // Go to Contact details view
    ContactDetailsViewController *controller = DYNAMIC_CAST([[PhoneMainView instance] changeCurrentView:[ContactDetailsViewController compositeViewDescription] push:TRUE], ContactDetailsViewController);
    if(controller != nil) {
        if([ContactSelection getSelectionMode] != ContactSelectionModeEdit) {
            [controller setContact:lPerson];
        } else {
            [controller editContact:lPerson address:[ContactSelection getAddAddress]];
        }
    }
}


#pragma mark - UITableViewDelegate Functions

- (UITableViewCellEditingStyle)tableView:(UITableView *)aTableView editingStyleForRowAtIndexPath:(NSIndexPath *)indexPath {
    // Detemine if it's in editing mode
    if (self.editing) {
        return UITableViewCellEditingStyleDelete;
    }
    return UITableViewCellEditingStyleNone;
}

@end
