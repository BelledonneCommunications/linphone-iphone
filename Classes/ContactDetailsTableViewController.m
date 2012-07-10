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
#import "UIEditableTableViewCell.h"
#import "UIView+ModalStack.h"

@interface Entry : NSObject

@property (retain) NSString *key;
@property (retain) NSString *value;

@end

@implementation Entry

@synthesize key;
@synthesize value;


#pragma mark - Lifecycle Functions

- (id)initWithData:(NSString*)akey value:(NSString*)avalue {
    self = [super init];
    if (self != NULL) {
        [self setKey:akey];
        [self setValue:avalue];
    }
    return self;
}

- (void)dealloc {
    [key release];
    [value release];
    [super dealloc];
}

@end


@implementation ContactDetailsTableViewController

@synthesize contact;


#pragma mark - Lifecycle Functions

- (void)initContactDetailsTableViewController {
    addressBook = ABAddressBookCreate();
    dataCache = [[NSMutableArray alloc] init];
    labelArray = [[NSMutableArray alloc] initWithObjects:
                  [NSString stringWithString:(NSString*)kABPersonPhoneMobileLabel], 
                  [NSString stringWithString:(NSString*)kABPersonPhoneIPhoneLabel],
                  [NSString stringWithString:(NSString*)kABPersonPhoneMainLabel],
                  [NSString stringWithString:(NSString*)kABPersonPhoneHomeFAXLabel],
                  [NSString stringWithString:(NSString*)kABPersonPhoneWorkFAXLabel], nil];
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
    [labelArray release];
    [dataCache release];
    [super dealloc];
}


#pragma mark - ViewController Functions

- (void)viewWillAppear:(BOOL)animated {
    [super viewWillAppear:animated];
    ABAddressBookRegisterExternalChangeCallback (addressBook, sync_toc_address_book, self);
}

- (void)viewWillDisappear:(BOOL)animated {
    [super viewWillDisappear:animated];
    contact = nil; // Clear previous selection
    ABAddressBookUnregisterExternalChangeCallback(addressBook, sync_toc_address_book, self);
    if(contactDetailsLabelViewController != nil) {
        [[[self view] superview] removeModalView:[contactDetailsLabelViewController view]];
        [editingIndexPath release];
        editingIndexPath = nil;
        [contactDetailsLabelViewController release];
        contactDetailsLabelViewController = nil;
    }
}


#pragma mark - 

+ (BOOL)findAndResignFirstResponder:(UIView*)view {
    if (view.isFirstResponder) {
        [view resignFirstResponder];
        return YES;     
    }
    for (UIView *subView in view.subviews) {
        if ([ContactDetailsTableViewController findAndResignFirstResponder:subView])
            return YES;
    }
    return NO;
}

+ (NSString*)localizeLabel:(NSString*)str {
    CFStringRef lLocalizedLabel = ABAddressBookCopyLocalizedLabel((CFStringRef) str);
    NSString * retStr = [NSString stringWithString:(NSString*) lLocalizedLabel];
    CFRelease(lLocalizedLabel);
    return retStr;
}

- (NSDictionary*)getLocalizedLabels {
    NSMutableDictionary *dict = [[NSMutableDictionary alloc] initWithCapacity:[labelArray count]];
    for(NSString *str in labelArray) {
        [dict setObject:[ContactDetailsTableViewController localizeLabel:str] forKey:str];
    }
    return [dict autorelease];
}

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
            Entry *entry = [[Entry alloc] initWithData:[NSString stringWithString:(NSString*)lLabel] value:[NSString stringWithString:(NSString*)lValue]];
            [subArray addObject: entry];
            [entry release];
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
            if(CFDictionaryContainsKey(lDict, kABPersonInstantMessageServiceKey)) {
                if(CFStringCompare((CFStringRef)@"SIP", CFDictionaryGetValue(lDict, kABPersonInstantMessageServiceKey), kCFCompareCaseInsensitive) == 0) {
                    CFStringRef lValue = CFDictionaryGetValue(lDict, kABPersonInstantMessageUsernameKey);
                    CFStringRef lLabel = ABMultiValueCopyLabelAtIndex(lMap, i);
                    Entry *entry = [[Entry alloc] initWithData:[NSString stringWithString:(NSString*)lLabel] value:[NSString stringWithString:(NSString*)lValue]];
                    [subArray addObject: entry];
                    [entry release];
                    CFRelease(lLabel);
                    CFRelease(lValue);
                }
            }
            CFRelease(lDict);
        }
        [dataCache addObject:subArray];
        CFRelease(lMap);   
    }
}

- (void)addEntry:(UITableView*)tableview section:(NSInteger)section editing:(BOOL)editing animated:(BOOL)animated {
    NSMutableArray *sectionArray = [dataCache objectAtIndex:section];
    NSUInteger count = [sectionArray count];
    Entry *entry = [[Entry alloc] initWithData:[labelArray objectAtIndex:0] value:@""];
    [sectionArray addObject:entry];
    
    NSArray *tagInsertIndexPath = [NSArray arrayWithObject:[NSIndexPath indexPathForRow:count inSection:section]];
    
    // Add or remove the Add row as appropriate.
    UITableViewRowAnimation animationStyle = UITableViewRowAnimationNone;
    if (animated) {
        animationStyle = UITableViewRowAnimationFade;
    }
    [self.tableView insertRowsAtIndexPaths:tagInsertIndexPath withRowAnimation:animationStyle];
}

- (void)removeEntry:(UITableView*)tableview section:(NSInteger)section editing:(BOOL)editing animated:(BOOL)animated {
    NSMutableArray *sectionArray = [dataCache objectAtIndex:section];
    for (int i = 0; i < [sectionArray count]; ++i) {
        Entry *entry = [sectionArray objectAtIndex:i];
        if(![[entry value] length]) {
            [sectionArray removeObjectAtIndex:i];
            NSArray *tagInsertIndexPath = [NSArray arrayWithObject:[NSIndexPath indexPathForRow:i inSection:section]];
            UITableViewRowAnimation animationStyle = UITableViewRowAnimationNone;
            if (animated) {
                animationStyle = UITableViewRowAnimationFade;
            }
            [self.tableView deleteRowsAtIndexPaths:tagInsertIndexPath withRowAnimation:animationStyle]; 
        }
    }
}


#pragma mark - Property Functions

- (void)setContact:(ABRecordRef)acontact {
    self->contact = acontact;
    [self loadData];
    [self.tableView reloadData];
}


#pragma mark - UITableViewDataSource Functions

- (NSInteger)numberOfSectionsInTableView:(UITableView *)tableView {
    return [dataCache count];
}

- (NSInteger)tableView:(UITableView *)tableView numberOfRowsInSection:(NSInteger)section {
    return [[dataCache objectAtIndex:section] count];
}

- (UITableViewCell *)tableView:(UITableView *)tableView cellForRowAtIndexPath:(NSIndexPath *)indexPath {
    static NSString *kCellId = @"ContactDetailsCell";
    UIEditableTableViewCell *cell = [tableView dequeueReusableCellWithIdentifier:kCellId];
    if (cell == nil) {  
        cell = [[[UIEditableTableViewCell alloc] initWithStyle:UITableViewCellStyleValue2 reuseIdentifier:kCellId] autorelease];
       // [cell setEditingAccessoryType:UITableViewCellAccessoryNone];
       // [cell setSelectionStyle: UITableViewCellSelectionStyleNone];
    }
    if ([indexPath row] < [[dataCache objectAtIndex:[indexPath section]] count]) {
        NSMutableArray *sectionDict = [dataCache objectAtIndex:[indexPath section]];
        Entry *entry = [sectionDict objectAtIndex:[indexPath row]];
        [cell.textLabel setText:[ContactDetailsTableViewController localizeLabel:[entry key]]];
        NSString *value = [entry value];
        [cell.detailTextLabel setText:value];
        [cell.detailTextField setText:value];
    }
    if ([indexPath section] == 0) {
        [cell.detailTextField setKeyboardType:UIKeyboardTypeNumbersAndPunctuation];
        [cell.detailTextField setPlaceholder:@"Phone number"];
    } else {
        [cell.detailTextField setKeyboardType:UIKeyboardTypeEmailAddress];
        [cell.detailTextField setPlaceholder:@"SIP address"];
    }
    return cell;
}

- (void)tableView:(UITableView *)tableView didSelectRowAtIndexPath:(NSIndexPath *)indexPath {
    [tableView deselectRowAtIndexPath:indexPath animated:NO];
    NSMutableArray *sectionDict = [dataCache objectAtIndex:[indexPath section]];
    Entry *entry  = [sectionDict objectAtIndex:[indexPath row]];
    if (![self isEditing]) {
        NSString *dest = [entry value];
        if(![dest hasPrefix:@"sip:"]) 
            dest = [NSString stringWithFormat:@"sip:%@", [entry key]];
        CFStringRef lDisplayName = ABRecordCopyCompositeName(contact);
        NSString *displayName = [NSString stringWithString:(NSString*) lDisplayName];
        CFRelease(lDisplayName);
        
        // Go to dialer view
        NSDictionary *dict = [[[NSDictionary alloc] initWithObjectsAndKeys:
                               [[[NSArray alloc] initWithObjects: dest, displayName, nil] autorelease]
                               , @"call:displayName:",
                               nil] autorelease];
        [[PhoneMainView instance] changeView:PhoneView_Dialer dict:dict];
    } else {
        contactDetailsLabelViewController = [[ContactDetailsLabelViewController alloc] initWithNibName:@"ContactDetailsLabelViewController" 
                                                                                                bundle:[NSBundle mainBundle]];
        [contactDetailsLabelViewController setSelectedData:[entry key]];
        [contactDetailsLabelViewController setDataList:[self getLocalizedLabels]];
        [contactDetailsLabelViewController setModalDelegate:self];
        editingIndexPath = [indexPath copy];
        [[[self view] superview] addModalView:[contactDetailsLabelViewController view]];
    }
}


#pragma mark - UITableViewDelegate Functions

- (void)setEditing:(BOOL)editing animated:(BOOL)animated {
    // Add entries
    
    if(editing) {
        [self.tableView beginUpdates];
        for (int section = 0; section <[self numberOfSectionsInTableView:[self tableView]]; ++section) {
            [self addEntry:self.tableView section:section editing:editing animated:animated];
        }
        [self.tableView endUpdates];
    } else {
        [self.tableView beginUpdates];
        for (int section = 0; section <[self numberOfSectionsInTableView:[self tableView]]; ++section) {
            [self removeEntry:self.tableView section:section editing:editing animated:animated];
        }
        [self.tableView endUpdates];
    }

    // Set animated
    [super setEditing:editing animated:animated];

    // Resign keyboard
    if(!editing) {
        [ContactDetailsTableViewController findAndResignFirstResponder:[self tableView]];
    }
}

- (UITableViewCellEditingStyle)tableView:(UITableView *)tableView editingStyleForRowAtIndexPath:(NSIndexPath *)indexPath {
    int last_index = [[dataCache objectAtIndex:[indexPath section]] count] - 1;
	if (indexPath.row == last_index) {
		return UITableViewCellEditingStyleInsert;
	}
    return UITableViewCellEditingStyleDelete;
}

- (UIView *)tableView:(UITableView *)tableView viewForHeaderInSection:(NSInteger)section {   
    if(section == 0) {
        UIContactDetailsHeader *headerController = [[UIContactDetailsHeader alloc] init];
        UIView *headerView = [headerController view];
        [headerController setContact:contact];
        [headerController update];
        [headerController release];
        return headerView;
    } else {
        return [super tableView:tableView viewForHeaderInSection:section];
    }
}

- (NSString*)tableView:(UITableView *)tableView titleForHeaderInSection:(NSInteger)section {
    if(section == 0) {
        return [super tableView:tableView titleForHeaderInSection:section];
    } else {
        return @"SIP";
    }
}

- (CGFloat)tableView:(UITableView *)tableView heightForHeaderInSection:(NSInteger)section { 
    if(section == 0) {
        return [UIContactDetailsHeader height];
    } else {
        return 22;
    }
}


#pragma mark - UIModalViewDeletage Functions

- (void)modalViewDismiss:(UIModalViewController*)controller value:(id)value {
    [[[self view]superview] removeModalView:[contactDetailsLabelViewController view]];
    contactDetailsLabelViewController = nil;
    NSMutableArray *sectionDict = [dataCache objectAtIndex:[editingIndexPath section]];
    Entry *entry = [sectionDict objectAtIndex:[editingIndexPath row]];
    [entry setKey:value];
    [[self tableView] reloadRowsAtIndexPaths:[NSArray arrayWithObject: editingIndexPath] withRowAnimation:FALSE];
    [editingIndexPath release];
    editingIndexPath = nil;
}

@end
