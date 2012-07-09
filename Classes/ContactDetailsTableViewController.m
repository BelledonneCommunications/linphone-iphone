/* ContactDetailsTableViewController.m
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

#import "ContactDetailsTableViewController.h"
#import "UIContactDetailsHeader.h"
#import "PhoneMainView.h"

@implementation ContactDetailsTableViewController

@synthesize contact;


#pragma mark - Lifecycle Functions

- (void)initContactDetailsTableViewController {
    addressBook = ABAddressBookCreate();
    dataCache = [[NSMutableArray alloc] init];
}

- (id)init {
    self = [super init];
    if (self) {
		[self initContactDetailsTableViewController];
    }
    return self;
}

- (id)initWithCoder:(NSCoder *)decoder {
    self = [super initWithCoder:decoder];
    if (self) {
		[self initContactDetailsTableViewController];
	}
    return self;
}	

- (void)dealloc {
    [dataCache dealloc];
    [super dealloc];
}


#pragma mark - ViewController Functions

- (void)viewWillAppear:(BOOL)animated {
    [super viewWillAppear:animated];
    ABAddressBookRegisterExternalChangeCallback (addressBook, sync_toc_address_book, self);
}

- (void)viewWillDisappear:(BOOL)animated {
    [super viewWillDisappear:animated];
    ABAddressBookUnregisterExternalChangeCallback(addressBook, sync_toc_address_book, self);
}


#pragma mark - 

static void sync_toc_address_book (ABAddressBookRef addressBook, CFDictionaryRef info, void *context) {
    ContactDetailsTableViewController* controller = (ContactDetailsTableViewController*)context;
    ABRecordID recordID = ABRecordGetRecordID([controller contact]);
    ABRecordRef newRecord = ABAddressBookGetPersonWithRecordID(addressBook, recordID);
    if(newRecord) {
        [controller setContact:newRecord];
    }
}

- (void)loadData {
    [dataCache removeAllObjects];
    
    if(contact == NULL) 
        return;
    
    // Phone numbers 
    {
        ABMultiValueRef lMap = ABRecordCopyValue(contact, kABPersonPhoneProperty);
        NSMutableArray *subArray = [[NSMutableArray alloc] init];
        for(int i = 0; i < ABMultiValueGetCount(lMap); ++i) {
            CFStringRef lValue = ABMultiValueCopyValueAtIndex(lMap, i);
            CFStringRef lLabel = ABMultiValueCopyLabelAtIndex(lMap, i);
            CFStringRef lLocalizedLabel =  ABAddressBookCopyLocalizedLabel(lLabel);
            [subArray addObject:[NSArray arrayWithObjects:[NSString stringWithString:(NSString*)lValue], [NSString stringWithString:(NSString*)lLocalizedLabel], nil]];
            if(lLocalizedLabel)
                CFRelease(lLocalizedLabel);
            CFRelease(lLabel);
            CFRelease(lValue);
        }
        [dataCache addObject:subArray];
        CFRelease(lMap);
    }
    
    // SIP (IM)
    {
        ABMultiValueRef lMap = ABRecordCopyValue(contact, kABPersonInstantMessageProperty);
        NSMutableArray *subArray = [[NSMutableArray alloc] init];
        for(int i = 0; i < ABMultiValueGetCount(lMap); ++i) {
            CFDictionaryRef lDict = ABMultiValueCopyValueAtIndex(lMap, i);
            if(CFDictionaryContainsKey(lDict, @"service")) {
                if(CFStringCompare((CFStringRef)@"SIP", CFDictionaryGetValue(lDict, @"service"), kCFCompareCaseInsensitive) == 0) {
                    CFStringRef lValue = CFDictionaryGetValue(lDict, @"username");
                    CFStringRef lLabel = ABMultiValueCopyLabelAtIndex(lMap, i);
                    CFStringRef lLocalizedLabel =  ABAddressBookCopyLocalizedLabel(lLabel);
                    [subArray addObject:[NSArray arrayWithObjects:[NSString stringWithString:(NSString*)lValue], [NSString stringWithString:(NSString*)lLocalizedLabel], nil]];
                    if(lLocalizedLabel)
                        CFRelease(lLocalizedLabel);
                    CFRelease(lLabel);

                }
            }
            CFRelease(lDict);
        }
        [dataCache addObject:subArray];
        CFRelease(lMap);   
    }
}


#pragma mark - Property Functions

- (void)setContact:(ABRecordRef)acontact {
    self->contact = acontact;
    [[self tableView] reloadData];
}


#pragma mark - UITableViewDataSource Functions

- (NSInteger)numberOfSectionsInTableView:(UITableView *)tableView {
    [self loadData];
    return [dataCache count];
}

- (NSInteger)tableView:(UITableView *)tableView numberOfRowsInSection:(NSInteger)section {
    NSMutableDictionary *dict = [dataCache objectAtIndex:section];
    return [dict count];
}

- (UITableViewCell *)tableView:(UITableView *)tableView cellForRowAtIndexPath:(NSIndexPath *)indexPath {
    UITableViewCell *cell = [tableView dequeueReusableCellWithIdentifier:@"ContactDetailsCell"];
    if (cell == nil) {
        cell = [[UITableViewCell alloc] initWithStyle:UITableViewCellStyleValue2 reuseIdentifier:@"ContactDetailsCell"];
        //[cell setSelectedBackgroundView:[[UIImageView alloc] initWithImage:[UIImage imageNamed:@"list_hightlight.png"]]];
    }
    NSMutableArray *sectionDict = [dataCache objectAtIndex:[indexPath section]];
    NSArray *tuple = [sectionDict objectAtIndex:[indexPath row]];
    [cell.textLabel setText:[tuple objectAtIndex:1]];
    [cell.detailTextLabel setText:[tuple objectAtIndex:0]];
    return cell;
}

- (void)tableView:(UITableView *)tableView didSelectRowAtIndexPath:(NSIndexPath *)indexPath {
    NSMutableArray *sectionDict = [dataCache objectAtIndex:[indexPath section]];
    NSArray *tuple = [sectionDict objectAtIndex:[indexPath row]];
    
    NSString *dest = [tuple objectAtIndex:0];
    if(![dest hasPrefix:@"sip:"]) 
        dest = [NSString stringWithFormat:@"sip:%@", [tuple objectAtIndex:0]];
    CFStringRef lDisplayName = ABRecordCopyCompositeName(contact);
    NSString *displayName = [NSString stringWithString:(NSString*) lDisplayName];
    CFRelease(lDisplayName);
    
    // Go to dialer view
    NSDictionary *dict = [[[NSDictionary alloc] initWithObjectsAndKeys:
                           [[[NSArray alloc] initWithObjects: dest, displayName, nil] autorelease]
                           , @"call:displayName:",
                           nil] autorelease];
    [[PhoneMainView instance] changeView:PhoneView_Dialer dict:dict];
}


#pragma mark - UITableViewDelegate Functions

- (UIView *)tableView:(UITableView *)tableView viewForHeaderInSection:(NSInteger)section {   
    if(section == 0) {
        UIContactDetailsHeader *headerController = [[UIContactDetailsHeader alloc] init];
        UIView *headerView = [headerController view];
        [headerController setContact:contact];
        [headerController update];
        [headerController release];
        return headerView;
    } else {
         return [[UIView alloc] initWithFrame:CGRectZero];
    }
}

- (CGFloat)tableView:(UITableView *)tableView heightForHeaderInSection:(NSInteger)section { 
    if(section == 0) 
        return [UIContactDetailsHeader height];
    else {
        return 0.000001f; // Hack UITableView = 0
    }
}

@end
