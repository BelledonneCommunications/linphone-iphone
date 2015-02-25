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

	addressBook = ABAddressBookCreateWithOptions(nil, nil);

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

- (BOOL)contactHasValidSipDomain:(ABRecordRef)person {
	// Check if one of the contact' sip URI matches the expected SIP filter
	ABMultiValueRef personSipAddresses = ABRecordCopyValue(person, kABPersonInstantMessageProperty);
	BOOL match = false;
	NSString * filter = [ContactSelection getSipFilter];

	for(int i = 0; i < ABMultiValueGetCount(personSipAddresses) && !match; ++i) {
		CFDictionaryRef lDict = ABMultiValueCopyValueAtIndex(personSipAddresses, i);
		if(CFDictionaryContainsKey(lDict, kABPersonInstantMessageServiceKey)) {
			CFStringRef serviceKey = CFDictionaryGetValue(lDict, kABPersonInstantMessageServiceKey);

			if (CFStringCompare((CFStringRef)[LinphoneManager instance].contactSipField, serviceKey, kCFCompareCaseInsensitive) == 0) {
				match = true;
			}
		}  else {
			//check domain
			LinphoneAddress* address = linphone_address_new([(NSString*)CFDictionaryGetValue(lDict,kABPersonInstantMessageUsernameKey) UTF8String]);

			if (address) {
				const char* dom =linphone_address_get_domain(address);
				if( dom != NULL ){
					NSString* domain = [NSString stringWithCString:dom
														  encoding:[NSString defaultCStringEncoding]];

					if (([filter compare:@"*" options:NSCaseInsensitiveSearch] == NSOrderedSame)
						|| ([filter compare:domain options:NSCaseInsensitiveSearch] == NSOrderedSame)) {
						match = true;
					}
				}
				linphone_address_destroy(address);
			}
		}
		CFRelease(lDict);
	}
	CFRelease(personSipAddresses);
	return match;
}

static int ms_strcmpfuz(const char * fuzzy_word, const char * sentence) {
	if (! fuzzy_word || !sentence) {
		return fuzzy_word == sentence;
	}
	const char * c = fuzzy_word;
	const char * within_sentence = sentence;
	for (; c != NULL && *c != '\0' && within_sentence != NULL; ++c) {
		within_sentence = strchr(within_sentence, *c);
		// Could not find c character in sentence. Abort.
		if (within_sentence == NULL) {
			break;
		}
		// since strchr returns the index of the matched char, move forward
		within_sentence++;
	}

	// If the whole fuzzy was found, returns 0. Otherwise returns number of characters left.
	return (int)(within_sentence != NULL ? 0 : fuzzy_word + strlen(fuzzy_word) - c);
}

- (void)loadData {
	[LinphoneLogger logc:LinphoneLoggerLog format:"Load contact list"];
	@synchronized (addressBookMap) {

		// Reset Address book
		[addressBookMap removeAllObjects];

		NSArray *lContacts = (NSArray *)ABAddressBookCopyArrayOfAllPeople(addressBook);
		for (id lPerson in lContacts) {
			BOOL add = true;
			ABRecordRef person = (ABRecordRef)lPerson;

			// Do not add the contact directly if we set some filter
			if([ContactSelection getSipFilter] || [ContactSelection emailFilterEnabled]) {
				add = false;
			}
			if([ContactSelection getSipFilter] && [self contactHasValidSipDomain:person]) {
				add = true;
			}
			if (!add && [ContactSelection emailFilterEnabled]) {
				ABMultiValueRef personEmailAddresses = ABRecordCopyValue(person, kABPersonEmailProperty);
				// Add this contact if it has an email
				add = (ABMultiValueGetCount(personEmailAddresses) > 0);

				CFRelease(personEmailAddresses);
			}

			if(add) {
				CFStringRef lFirstName = ABRecordCopyValue(person, kABPersonFirstNameProperty);
				CFStringRef lLocalizedFirstName = (lFirstName != nil)? ABAddressBookCopyLocalizedLabel(lFirstName): nil;
				CFStringRef lLastName = ABRecordCopyValue(person, kABPersonLastNameProperty);
				CFStringRef lLocalizedLastName = (lLastName != nil)? ABAddressBookCopyLocalizedLabel(lLastName): nil;
				CFStringRef lOrganization = ABRecordCopyValue(person, kABPersonOrganizationProperty);
				CFStringRef lLocalizedlOrganization = (lOrganization != nil)? ABAddressBookCopyLocalizedLabel(lOrganization): nil;
				NSString *name = nil;
				if(lLocalizedFirstName != nil && lLocalizedLastName != nil) {
					name=[NSString stringWithFormat:@"%@ %@", [(NSString *)lLocalizedFirstName retain], [(NSString *)lLocalizedLastName retain]];
				} else if(lLocalizedLastName != nil) {
					name=[NSString stringWithFormat:@"%@",[(NSString *)lLocalizedLastName retain]];
				} else if(lLocalizedFirstName != nil) {
					name=[NSString stringWithFormat:@"%@",[(NSString *)lLocalizedFirstName retain]];
				} else if(lLocalizedlOrganization != nil) {
					name=[NSString stringWithFormat:@"%@",[(NSString *)lLocalizedlOrganization retain]];
				}

				if(name != nil && [name length] > 0) {
					// Add the contact only if it fuzzy match filter too (if any)
					if ([ContactSelection getNameOrEmailFilter] == nil ||
						(ms_strcmpfuz([[[ContactSelection getNameOrEmailFilter] lowercaseString] UTF8String], [[name lowercaseString] UTF8String]) == 0)) {

						//Sort contacts by first letter. We need to translate the name to ASCII first, because of UTF-8 issues. For instance
						// we expect order:  Alberta(A tilde) before ASylvano.
						NSData *name2ASCIIdata = [name dataUsingEncoding:NSASCIIStringEncoding allowLossyConversion:YES];
						NSString *name2ASCII = [[[NSString alloc] initWithData:name2ASCIIdata encoding:NSASCIIStringEncoding] autorelease];
						NSString *firstChar = [[name2ASCII substringToIndex:1] uppercaseString];

						// Put in correct subDic
						if([firstChar characterAtIndex:0] < 'A' || [firstChar characterAtIndex:0] > 'Z') {
							firstChar = @"#";
						}
						OrderedDictionary *subDic =[addressBookMap objectForKey: firstChar];
						if(subDic == nil) {
							subDic = [[[OrderedDictionary alloc] init] autorelease];
							[addressBookMap insertObject:subDic forKey:firstChar selector:@selector(caseInsensitiveCompare:)];
						}
						[subDic insertObject:lPerson forKey:name2ASCII selector:@selector(caseInsensitiveCompare:)];
					}
				}
				if(lLocalizedlOrganization != nil)
					CFRelease(lLocalizedlOrganization);
				if(lOrganization != nil)
					CFRelease(lOrganization);
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
		if (lContacts)
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
