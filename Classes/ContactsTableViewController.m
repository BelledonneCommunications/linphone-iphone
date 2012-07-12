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

@synthesize sipFilter;


#pragma mark - Lifecycle Functions

- (void)initContactsTableViewController {
    addressBookMap  = [[OrderedDictionary alloc] init];
    
    addressBook = ABAddressBookCreate();
    ABAddressBookRegisterExternalChangeCallback(addressBook, sync_toc_address_book, self);
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
    ABAddressBookUnregisterExternalChangeCallback(addressBook, sync_toc_address_book, self);
    CFRelease(addressBook);
    [addressBookMap removeAllObjects];
    [super dealloc];
}


#pragma mark - Property Functions

- (void)setSipFilter:(BOOL)asipFilter {
    self->sipFilter = asipFilter;
    [self reloadData];
}


#pragma mark - 

static void sync_toc_address_book (ABAddressBookRef addressBook, CFDictionaryRef info, void *context) {
    ContactsTableViewController* controller = (ContactsTableViewController*)context;
    ABAddressBookRevert(addressBook);
    [(UITableView *)controller.view reloadData];
}

#pragma mark - ViewController Functions

- (void)viewWillAppear:(BOOL)animated {
    [super viewWillAppear:animated];
    [self reloadData];
}

- (void)viewWillDisappear:(BOOL)animated {
    [super viewWillDisappear:animated];
}


#pragma mark - UITableViewDataSource Functions

- (void)reloadData {
    NSLog(@"Load contact list");
    @synchronized (addressBookMap) {
        
        // Reset Address book
        [addressBookMap removeAllObjects];
        
        NSArray *lContacts = (NSArray *)ABAddressBookCopyArrayOfAllPeople(addressBook);
        for (id lPerson in lContacts) {
            BOOL add = true;
            if(sipFilter) {
                add = false;
                ABMultiValueRef lMap = ABRecordCopyValue((ABRecordRef)lPerson, kABPersonInstantMessageProperty);
                for(int i = 0; i < ABMultiValueGetCount(lMap); ++i) {
                    CFDictionaryRef lDict = ABMultiValueCopyValueAtIndex(lMap, i);
                    if(CFDictionaryContainsKey(lDict, @"service")) {
                        if(CFStringCompare((CFStringRef)@"SIP", CFDictionaryGetValue(lDict, @"service"), kCFCompareCaseInsensitive) == 0) {     
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
                if(name != nil) {
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

- (NSInteger)numberOfSectionsInTableView:(UITableView *)tableView {
    return [addressBookMap count] + 1;
}

- (NSInteger)tableView:(UITableView *)tableView numberOfRowsInSection:(NSInteger)section {
    if(section == 0) {
        return 1;
    } else {
        return [(OrderedDictionary *)[addressBookMap objectForKey: [addressBookMap keyAtIndex: section - 1]] count];
    }
}

- (UITableViewCell *)tableView:(UITableView *)tableView cellForRowAtIndexPath:(NSIndexPath *)indexPath {
    static NSString *kCellId = @"UIContactCell";
    static NSString *kNewContactCellId = @"NewContactCell";
    
    if([indexPath section] != 0) {
        UIContactCell *cell = [tableView dequeueReusableCellWithIdentifier:kCellId];
        if (cell == nil) {
            cell = [[[UIContactCell alloc] initWithIdentifier:kCellId] autorelease];
        }
        OrderedDictionary *subDic = [addressBookMap objectForKey: [addressBookMap keyAtIndex: [indexPath section] - 1]]; 
        
        NSString *key = [[subDic allKeys] objectAtIndex:[indexPath row]];
        ABRecordRef contact = [subDic objectForKey:key];
        [cell setContact: contact];
        return cell;
    } else {
        UITableViewCell *cell = [tableView dequeueReusableCellWithIdentifier:kNewContactCellId];
        if (cell == nil) {  
            cell = [[[UITableViewCell alloc] initWithStyle:UITableViewCellStyleValue1 reuseIdentifier:kNewContactCellId] autorelease];
            //TODO
            /*
            [cell setSelectedBackgroundView:[[[UIImageView alloc] initWithImage:[UIImage imageNamed:@"list_highlight.png"]]autorelease]];*/
        }
        [cell.textLabel setText:@"Add new contact"];
        return cell;
    }
}

- (NSString *)tableView:(UITableView *)tableView titleForHeaderInSection:(NSInteger)section {
    if (section == 0) {
        return @"";
    } else {
        return [addressBookMap keyAtIndex: section - 1];
    }
}

- (void)tableView:(UITableView *)tableView didSelectRowAtIndexPath:(NSIndexPath *)indexPath {
    if([indexPath section] == 0) {
        // Go to Contact details view
        NSDictionary *dict = [[[NSDictionary alloc] initWithObjectsAndKeys:
                               [[[NSArray alloc] initWithObjects: nil] autorelease]
                               , @"newContact",
                               nil] autorelease];
        [[PhoneMainView instance] changeView:PhoneView_ContactDetails dict:dict push:TRUE];
    } else {
        OrderedDictionary *subDic = [addressBookMap objectForKey: [addressBookMap keyAtIndex: [indexPath section] - 1]]; 
        ABRecordRef lPerson = [subDic objectForKey: [subDic keyAtIndex:[indexPath row]]];
        
        // Go to Contact details view
        NSDictionary *dict = [[[NSDictionary alloc] initWithObjectsAndKeys:
                               [[[NSArray alloc] initWithObjects: lPerson, nil] autorelease]
                               , @"setContact:",
                               nil] autorelease];
        [[PhoneMainView instance] changeView:PhoneView_ContactDetails dict:dict push:TRUE];
    }
}

@end
