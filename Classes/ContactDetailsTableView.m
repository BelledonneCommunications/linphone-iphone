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

#import "ContactDetailsTableView.h"
#import "PhoneMainView.h"
#import "UIContactDetailsCell.h"
#import "UACellBackgroundView.h"
#import "Utils.h"
#import "OrderedDictionary.h"
#import "FastAddressBook.h"

@interface Entry : NSObject

@property(assign) ABMultiValueIdentifier identifier;

@end

@implementation Entry

@synthesize identifier;

#pragma mark - Lifecycle Functions

- (id)initWithData:(ABMultiValueIdentifier)aidentifier {
	self = [super init];
	if (self != NULL) {
		[self setIdentifier:aidentifier];
	}
	return self;
}

@end

@implementation ContactDetailsTableView

static const ContactSections_e contactSections[ContactSections_MAX] = {
	ContactSections_None,   ContactSections_First_Name, ContactSections_Last_Name,
	ContactSections_Number, ContactSections_Sip,		ContactSections_Email};

@synthesize contactDetailsDelegate;
@synthesize contact;

#pragma mark - Lifecycle Functions

- (void)initContactDetailsTableViewController {
	dataCache = [[NSMutableArray alloc] init];

	// pre-fill the data-cache with empty arrays
	for (int i = ContactSections_Number; i < ContactSections_MAX; i++) {
		[dataCache addObject:@[]];
	}

	labelArray = [[NSMutableArray alloc]
		initWithObjects:[[NSBundle mainBundle] objectForInfoDictionaryKey:@"CFBundleDisplayName"],
						[NSString stringWithString:(NSString *)kABPersonPhoneMobileLabel],
						[NSString stringWithString:(NSString *)kABPersonPhoneIPhoneLabel],
						[NSString stringWithString:(NSString *)kABPersonPhoneMainLabel], nil];
	editingIndexPath = nil;
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
	if (contact != nil && ABRecordGetRecordID(contact) == kABRecordInvalidID) {
		CFRelease(contact);
	}
}

#pragma mark -

- (void)updateModification {
	[contactDetailsDelegate onModification:nil];
}

- (NSMutableArray *)getSectionData:(NSInteger)section {
	if (contactSections[section] == ContactSections_Number) {
		return [dataCache objectAtIndex:0];
	} else if (contactSections[section] == ContactSections_Sip) {
		return [dataCache objectAtIndex:1];
	} else if (contactSections[section] == ContactSections_Email) {
		if ([[LinphoneManager instance] lpConfigBoolForKey:@"show_contacts_emails_preference"] == true) {
			return [dataCache objectAtIndex:2];
		} else {
			return nil;
		}
	}
	return nil;
}

- (ABPropertyID)propertyIDForSection:(ContactSections_e)section {
	switch (section) {
		case ContactSections_First_Name:
			return kABPersonFirstNameProperty;
		case ContactSections_Last_Name:
			return kABPersonLastNameProperty;
		case ContactSections_Sip:
			return kABPersonInstantMessageProperty;
		case ContactSections_Number:
			return kABPersonPhoneProperty;
		case ContactSections_Email:
			return kABPersonEmailProperty;
		default:
			return kABInvalidPropertyType;
	}
}

- (NSDictionary *)getLocalizedLabels {
	OrderedDictionary *dict = [[OrderedDictionary alloc] initWithCapacity:[labelArray count]];
	for (NSString *str in labelArray) {
		[dict setObject:[FastAddressBook localizedLabel:str] forKey:str];
	}
	return dict;
}

- (void)loadData {
	[dataCache removeAllObjects];

	if (contact == NULL)
		return;

	LOGI(@"Load data from contact %p", contact);
	// Phone numbers
	{
		ABMultiValueRef lMap = ABRecordCopyValue(contact, kABPersonPhoneProperty);
		NSMutableArray *subArray = [NSMutableArray array];
		if (lMap) {
			for (int i = 0; i < ABMultiValueGetCount(lMap); ++i) {
				ABMultiValueIdentifier identifier = ABMultiValueGetIdentifierAtIndex(lMap, i);
				Entry *entry = [[Entry alloc] initWithData:identifier];
				[subArray addObject:entry];
			}
			CFRelease(lMap);
		}
		[dataCache addObject:subArray];
	}

	// SIP (IM)
	{
		ABMultiValueRef lMap = ABRecordCopyValue(contact, kABPersonInstantMessageProperty);
		NSMutableArray *subArray = [NSMutableArray array];
		if (lMap) {
			for (int i = 0; i < ABMultiValueGetCount(lMap); ++i) {
				ABMultiValueIdentifier identifier = ABMultiValueGetIdentifierAtIndex(lMap, i);
				CFDictionaryRef lDict = ABMultiValueCopyValueAtIndex(lMap, i);
				BOOL add = false;
				if (CFDictionaryContainsKey(lDict, kABPersonInstantMessageServiceKey)) {
					if (CFStringCompare((CFStringRef)[LinphoneManager instance].contactSipField,
										CFDictionaryGetValue(lDict, kABPersonInstantMessageServiceKey),
										kCFCompareCaseInsensitive) == 0) {
						add = true;
					}
				} else {
					// check domain
					LinphoneAddress *address = linphone_address_new(
						[(NSString *)CFDictionaryGetValue(lDict, kABPersonInstantMessageUsernameKey) UTF8String]);
					if (address) {
						if ([[ContactSelection getSipFilter] compare:@"*" options:NSCaseInsensitiveSearch] ==
							NSOrderedSame) {
							add = true;
						} else {
							NSString *domain = [NSString stringWithCString:linphone_address_get_domain(address)
																  encoding:[NSString defaultCStringEncoding]];
							add = [domain compare:[ContactSelection getSipFilter] options:NSCaseInsensitiveSearch] ==
								  NSOrderedSame;
						}
						linphone_address_destroy(address);
					} else {
						add = false;
					}
				}
				if (add) {
					Entry *entry = [[Entry alloc] initWithData:identifier];
					[subArray addObject:entry];
				}
				CFRelease(lDict);
			}
			CFRelease(lMap);
		}
		[dataCache addObject:subArray];
	}

	// Email
	if ([[LinphoneManager instance] lpConfigBoolForKey:@"show_contacts_emails_preference"] == true) {
		ABMultiValueRef lMap = ABRecordCopyValue(contact, kABPersonEmailProperty);
		NSMutableArray *subArray = [NSMutableArray array];
		if (lMap) {
			for (int i = 0; i < ABMultiValueGetCount(lMap); ++i) {
				ABMultiValueIdentifier identifier = ABMultiValueGetIdentifierAtIndex(lMap, i);
				CFDictionaryRef lDict = ABMultiValueCopyValueAtIndex(lMap, i);
				Entry *entry = [[Entry alloc] initWithData:identifier];
				[subArray addObject:entry];
				CFRelease(lDict);
			}
			CFRelease(lMap);
		}
		[dataCache addObject:subArray];
	}

	if (contactDetailsDelegate != nil) {
		[contactDetailsDelegate onModification:nil];
	}
	[self.tableView reloadData];
}

- (Entry *)setOrCreateSipContactEntry:(Entry *)entry withValue:(NSString *)value {
	ABMultiValueRef lcMap = ABRecordCopyValue(contact, kABPersonInstantMessageProperty);
	ABMutableMultiValueRef lMap;
	if (lcMap != NULL) {
		lMap = ABMultiValueCreateMutableCopy(lcMap);
		CFRelease(lcMap);
	} else {
		lMap = ABMultiValueCreateMutable(kABStringPropertyType);
	}
	ABMultiValueIdentifier index;
	CFErrorRef error = NULL;

	NSDictionary *lDict = @{
		(NSString *)kABPersonInstantMessageUsernameKey : value, (NSString *)
		kABPersonInstantMessageServiceKey : [LinphoneManager instance].contactSipField
	};

	if (entry) {
		index = (int)ABMultiValueGetIndexForIdentifier(lMap, [entry identifier]);
		ABMultiValueReplaceValueAtIndex(lMap, (__bridge CFTypeRef)(lDict), index);
	} else {
		CFStringRef label = (__bridge CFStringRef)[labelArray objectAtIndex:0];
		ABMultiValueAddValueAndLabel(lMap, (__bridge CFTypeRef)lDict, label, &index);
	}

	if (!ABRecordSetValue(contact, kABPersonInstantMessageProperty, lMap, &error)) {
		LOGI(@"Can't set contact with value [%@] cause [%@]", value, [(__bridge NSError *)error localizedDescription]);
		CFRelease(lMap);
	} else {
		if (entry == nil) {
			entry = [[Entry alloc] initWithData:index];
		}
		CFRelease(lMap);

		/*check if message type is kept or not*/
		lcMap = ABRecordCopyValue(contact, kABPersonInstantMessageProperty);
		lMap = ABMultiValueCreateMutableCopy(lcMap);
		CFRelease(lcMap);
		index = (int)ABMultiValueGetIndexForIdentifier(lMap, [entry identifier]);
		lDict = CFBridgingRelease(ABMultiValueCopyValueAtIndex(lMap, index));

		if ([lDict objectForKey:(__bridge NSString *)kABPersonInstantMessageServiceKey] == nil) {
			/*too bad probably a gtalk number, storing uri*/
			NSString *username = [lDict objectForKey:(NSString *)kABPersonInstantMessageUsernameKey];
			LinphoneAddress *address = linphone_core_interpret_url([LinphoneManager getLc], [username UTF8String]);
			if (address) {
				char *uri = linphone_address_as_string_uri_only(address);
				NSDictionary *dict2 = @{
					(NSString *)kABPersonInstantMessageUsernameKey :
									[NSString stringWithCString:uri encoding:[NSString defaultCStringEncoding]],
								(NSString *)
					kABPersonInstantMessageServiceKey : [LinphoneManager instance].contactSipField
				};

				ABMultiValueReplaceValueAtIndex(lMap, (__bridge CFTypeRef)(dict2), index);

				if (!ABRecordSetValue(contact, kABPersonInstantMessageProperty, lMap, &error)) {
					LOGI(@"Can't set contact with value [%@] cause [%@]", value,
						 [(__bridge NSError *)error localizedDescription]);
				}
				linphone_address_destroy(address);
				ms_free(uri);
			}
		}
		CFRelease(lMap);
	}

	return entry;
}

- (void)setSipContactEntry:(Entry *)entry withValue:(NSString *)value {
	[self setOrCreateSipContactEntry:entry withValue:value];
}
- (void)addEntry:(UITableView *)tableview section:(NSInteger)section animated:(BOOL)animated {
	[self addEntry:tableview section:section animated:animated value:@""];
}

- (void)addEntry:(UITableView *)tableview section:(NSInteger)section animated:(BOOL)animated value:(NSString *)value {
	NSMutableArray *sectionArray = [self getSectionData:section];
	NSUInteger count = [sectionArray count];
	CFErrorRef error = NULL;
	bool added = TRUE;
	if (contactSections[section] == ContactSections_Number) {
		ABMultiValueIdentifier identifier;
		ABMultiValueRef lcMap = ABRecordCopyValue(contact, kABPersonPhoneProperty);
		ABMutableMultiValueRef lMap;
		if (lcMap != NULL) {
			lMap = ABMultiValueCreateMutableCopy(lcMap);
			CFRelease(lcMap);
		} else {
			lMap = ABMultiValueCreateMutable(kABStringPropertyType);
		}
		CFStringRef label = (__bridge CFStringRef)[labelArray objectAtIndex:0];
		if (!ABMultiValueAddValueAndLabel(lMap, (__bridge CFTypeRef)(value), label, &identifier)) {
			added = false;
		}

		if (added && ABRecordSetValue(contact, kABPersonPhoneProperty, lMap, &error)) {
			Entry *entry = [[Entry alloc] initWithData:identifier];
			[sectionArray addObject:entry];
		} else {
			added = false;
			LOGI(@"Can't add entry: %@", [(__bridge NSError *)error localizedDescription]);
		}
		CFRelease(lMap);
	} else if (contactSections[section] == ContactSections_Sip) {
		Entry *entry = [self setOrCreateSipContactEntry:nil withValue:value];
		if (entry) {
			[sectionArray addObject:entry];
			added = true;
		} else {
			added = false;
			LOGE(@"Can't add entry for value: %@", value);
		}
	} else if (contactSections[section] == ContactSections_Email) {
		ABMultiValueIdentifier identifier;
		ABMultiValueRef lcMap = ABRecordCopyValue(contact, kABPersonEmailProperty);
		ABMutableMultiValueRef lMap;
		if (lcMap != NULL) {
			lMap = ABMultiValueCreateMutableCopy(lcMap);
			CFRelease(lcMap);
		} else {
			lMap = ABMultiValueCreateMutable(kABStringPropertyType);
		}
		CFStringRef label = (__bridge CFStringRef)[labelArray objectAtIndex:0];
		if (!ABMultiValueAddValueAndLabel(lMap, (__bridge CFTypeRef)(value), label, &identifier)) {
			added = false;
		}

		if (added && ABRecordSetValue(contact, kABPersonEmailProperty, lMap, &error)) {
			Entry *entry = [[Entry alloc] initWithData:identifier];
			[sectionArray addObject:entry];
		} else {
			added = false;
			LOGI(@"Can't add entry: %@", [(__bridge NSError *)error localizedDescription]);
		}
		CFRelease(lMap);
	}

	if (added && animated) {
		// Update accessory
		if (count > 0) {
			[tableview reloadRowsAtIndexPaths:[NSArray arrayWithObject:[NSIndexPath indexPathForRow:count - 1
																						  inSection:section]]
							 withRowAnimation:FALSE];
		}
		[tableview
			insertRowsAtIndexPaths:[NSArray arrayWithObject:[NSIndexPath indexPathForRow:count inSection:section]]
				  withRowAnimation:UITableViewRowAnimationFade];
	}
	if (contactDetailsDelegate != nil) {
		[contactDetailsDelegate onModification:nil];
	}
}

- (void)removeEmptyEntry:(UITableView *)tableview section:(NSInteger)section animated:(BOOL)animated {
	NSMutableArray *sectionDict = [self getSectionData:section];
	NSInteger row = [sectionDict count] - 1;
	if (row >= 0) {
		Entry *entry = [sectionDict objectAtIndex:row];

		ABPropertyID property = [self propertyIDForSection:contactSections[section]];
		if (property != kABInvalidPropertyType) {
			ABMultiValueRef lMap = ABRecordCopyValue(contact, property);
			NSInteger index = ABMultiValueGetIndexForIdentifier(lMap, [entry identifier]);
			CFTypeRef valueRef = ABMultiValueCopyValueAtIndex(lMap, index);
			CFTypeRef toRelease = nil;
			NSString *value = nil;
			if (property == kABPersonInstantMessageProperty) {
				// when we query the instanteMsg property we get a dictionary instead of a value
				toRelease = valueRef;
				value = CFDictionaryGetValue(valueRef, kABPersonInstantMessageUsernameKey);
			} else {
				value = CFBridgingRelease(valueRef);
			}

			if (value.length == 0) {
				[self removeEntry:tableview path:[NSIndexPath indexPathForRow:row inSection:section] animated:animated];
			}
			if (toRelease != nil) {
				CFRelease(toRelease);
			}

			CFRelease(lMap);
		}
	}
	if (contactDetailsDelegate != nil) {
		[contactDetailsDelegate onModification:nil];
	}
}

- (void)removeEntry:(UITableView *)tableview path:(NSIndexPath *)indexPath animated:(BOOL)animated {
	NSMutableArray *sectionArray = [self getSectionData:[indexPath section]];
	Entry *entry = [sectionArray objectAtIndex:[indexPath row]];
	ABPropertyID property = [self propertyIDForSection:contactSections[indexPath.section]];

	if (property != kABInvalidPropertyType) {
		ABMultiValueRef lcMap = ABRecordCopyValue(contact, property);
		ABMutableMultiValueRef lMap = ABMultiValueCreateMutableCopy(lcMap);
		CFRelease(lcMap);
		NSInteger index = ABMultiValueGetIndexForIdentifier(lMap, [entry identifier]);
		ABMultiValueRemoveValueAndLabelAtIndex(lMap, index);
		ABRecordSetValue(contact, property, lMap, nil);
		CFRelease(lMap);
	}

	[sectionArray removeObjectAtIndex:[indexPath row]];

	NSArray *tagInsertIndexPath = [NSArray arrayWithObject:indexPath];
	if (animated) {
		[tableview deleteRowsAtIndexPaths:tagInsertIndexPath withRowAnimation:UITableViewRowAnimationFade];
	}
}

#pragma mark - Property Functions

- (void)setContact:(ABRecordRef)acontact {
	if (acontact == contact)
		return;

	if (contact != nil && ABRecordGetRecordID(contact) == kABRecordInvalidID) {
		CFRelease(contact);
	}
	contact = acontact;
	[self loadData];
}

- (void)addPhoneField:(NSString *)number {
	int i = 0;
	while (i < ContactSections_MAX && contactSections[i] != ContactSections_Number)
		++i;
	[self addEntry:[self tableView] section:i animated:FALSE value:number];
}

- (void)addSipField:(NSString *)address {
	int i = 0;
	while (i < ContactSections_MAX && contactSections[i] != ContactSections_Sip)
		++i;
	[self addEntry:[self tableView] section:i animated:FALSE value:address];
}

- (void)addEmailField:(NSString *)address {
	int i = 0;
	while (i < ContactSections_MAX && contactSections[i] != ContactSections_Email)
		++i;
	[self addEntry:[self tableView] section:i animated:FALSE value:address];
}

#pragma mark - UITableViewDataSource Functions

- (NSInteger)numberOfSectionsInTableView:(UITableView *)tableView {
	return ContactSections_MAX;
}

- (NSInteger)tableView:(UITableView *)tableView numberOfRowsInSection:(NSInteger)section {
	if (contactSections[section] == ContactSections_First_Name ||
		contactSections[section] == ContactSections_Last_Name) {
		return (self.tableView.isEditing) ? 1 : 0 /*no first and last name when not editting */;
	} else {
		return [[self getSectionData:section] count];
	}
}

- (UITableViewCell *)tableView:(UITableView *)tableView cellForRowAtIndexPath:(NSIndexPath *)indexPath {
	static NSString *kCellId = @"UIContactDetailsCell";
	UIContactDetailsCell *cell = [tableView dequeueReusableCellWithIdentifier:kCellId];
	if (cell == nil) {
		cell = [[UIContactDetailsCell alloc] initWithIdentifier:kCellId];
		[cell.editTextfield setDelegate:self];
	}

	NSMutableArray *sectionDict = [self getSectionData:[indexPath section]];
	Entry *entry = [sectionDict objectAtIndex:[indexPath row]];

	NSString *value = @"";
	// default label is our app name
	NSString *label = [FastAddressBook localizedLabel:[labelArray objectAtIndex:0]];

	if (contactSections[indexPath.section] == ContactSections_First_Name) {
		value =
			(__bridge NSString *)(ABRecordCopyValue(contact, [self propertyIDForSection:ContactSections_First_Name]));
		label = nil;
		[cell hideDeleteButton];
	} else if (contactSections[indexPath.section] == ContactSections_Last_Name) {
		value =
			(__bridge NSString *)(ABRecordCopyValue(contact, [self propertyIDForSection:ContactSections_Last_Name]));
		label = nil;
		[cell hideDeleteButton];
	} else if (contactSections[[indexPath section]] == ContactSections_Number) {
		ABMultiValueRef lMap = ABRecordCopyValue(contact, kABPersonPhoneProperty);
		NSInteger index = ABMultiValueGetIndexForIdentifier(lMap, [entry identifier]);
		NSString *labelRef = CFBridgingRelease(ABMultiValueCopyLabelAtIndex(lMap, index));
		if (labelRef != NULL) {
			label = [FastAddressBook localizedLabel:labelRef];
		}
		NSString *valueRef = CFBridgingRelease(ABMultiValueCopyValueAtIndex(lMap, index));
		if (valueRef != NULL) {
			value = [FastAddressBook localizedLabel:valueRef];
		}
		CFRelease(lMap);
	} else if (contactSections[[indexPath section]] == ContactSections_Sip) {
		ABMultiValueRef lMap = ABRecordCopyValue(contact, kABPersonInstantMessageProperty);
		NSInteger index = ABMultiValueGetIndexForIdentifier(lMap, [entry identifier]);

		NSString *labelRef = CFBridgingRelease(ABMultiValueCopyLabelAtIndex(lMap, index));
		if (labelRef != NULL) {
			label = [FastAddressBook localizedLabel:labelRef];
		}
		CFDictionaryRef lDict = ABMultiValueCopyValueAtIndex(lMap, index);
		value = (__bridge NSString *)(CFDictionaryGetValue(lDict, kABPersonInstantMessageUsernameKey));
		if (value != NULL) {
			LinphoneAddress *addr = NULL;
			if ([[LinphoneManager instance] lpConfigBoolForKey:@"contact_display_username_only"] &&
				(addr = linphone_address_new([value UTF8String]))) {
				if (linphone_address_get_username(addr)) {
					value = [NSString stringWithCString:linphone_address_get_username(addr)
											   encoding:[NSString defaultCStringEncoding]];
				}
			}
			if (addr)
				linphone_address_destroy(addr);
		}
		CFRelease(lDict);
		CFRelease(lMap);
	} else if (contactSections[[indexPath section]] == ContactSections_Email) {
		ABMultiValueRef lMap = ABRecordCopyValue(contact, kABPersonEmailProperty);
		NSInteger index = ABMultiValueGetIndexForIdentifier(lMap, [entry identifier]);
		NSString *labelRef = CFBridgingRelease(ABMultiValueCopyLabelAtIndex(lMap, index));
		if (labelRef != NULL) {
			label = [FastAddressBook localizedLabel:labelRef];
		}
		NSString *valueRef = CFBridgingRelease(ABMultiValueCopyValueAtIndex(lMap, index));
		if (valueRef != NULL) {
			value = [FastAddressBook localizedLabel:valueRef];
		}
		CFRelease(lMap);
	}
	[cell.editTextfield setText:value];
	cell.addressLabel.text = value;
	if (contactSections[[indexPath section]] == ContactSections_Number) {
		[cell.editTextfield setKeyboardType:UIKeyboardTypePhonePad];
		[cell.editTextfield setPlaceholder:NSLocalizedString(@"Phone number", nil)];
	} else if (contactSections[[indexPath section]] == ContactSections_Sip) {
		[cell.editTextfield setKeyboardType:UIKeyboardTypeASCIICapable];
		[cell.editTextfield setPlaceholder:NSLocalizedString(@"SIP address", nil)];
	} else if (contactSections[[indexPath section]] == ContactSections_Email) {
		[cell.editTextfield setKeyboardType:UIKeyboardTypeASCIICapable];
		[cell.editTextfield setPlaceholder:NSLocalizedString(@"Email address", nil)];
	}
	return cell;
}

- (void)tableView:(UITableView *)tableView didSelectRowAtIndexPath:(NSIndexPath *)indexPath {
	[tableView deselectRowAtIndexPath:indexPath animated:NO];
	NSMutableArray *sectionDict = [self getSectionData:[indexPath section]];
	Entry *entry = [sectionDict objectAtIndex:[indexPath row]];
	if ([self isEditing]) {
		NSString *key = nil;
		ABPropertyID property = [self propertyIDForSection:contactSections[indexPath.section]];

		if (property != kABInvalidPropertyType) {
			ABMultiValueRef lMap = ABRecordCopyValue(contact, property);
			NSInteger index = ABMultiValueGetIndexForIdentifier(lMap, [entry identifier]);
			NSString *labelRef = CFBridgingRelease(ABMultiValueCopyLabelAtIndex(lMap, index));
			if (labelRef != NULL) {
				key = (NSString *)(labelRef);
			}
			CFRelease(lMap);
		}
		if (key != nil) {
			editingIndexPath = indexPath;
		}
	}
}

- (void)tableView:(UITableView *)tableView
	commitEditingStyle:(UITableViewCellEditingStyle)editingStyle
	 forRowAtIndexPath:(NSIndexPath *)indexPath {
	[LinphoneUtils findAndResignFirstResponder:[self tableView]];
	if (editingStyle == UITableViewCellEditingStyleInsert) {
		[tableView beginUpdates];
		[self addEntry:tableView section:[indexPath section] animated:TRUE];
		[tableView endUpdates];
	} else if (editingStyle == UITableViewCellEditingStyleDelete) {
		[tableView beginUpdates];
		[self removeEntry:tableView path:indexPath animated:TRUE];
		[tableView endUpdates];
	}
}

#pragma mark - UITableViewDelegate Functions

- (void)setEditing:(BOOL)editing animated:(BOOL)animated {
	[super setEditing:editing animated:animated];
	if (!editing) {
		[LinphoneUtils findAndResignFirstResponder:[self tableView]];
	}
	[self loadData];
	if (contactDetailsDelegate != nil) {
		[contactDetailsDelegate onModification:nil];
	}
}

- (UITableViewCellEditingStyle)tableView:(UITableView *)tableView
		   editingStyleForRowAtIndexPath:(NSIndexPath *)indexPath {
	return UITableViewCellEditingStyleNone;
}

- (UIView *)tableView:(UITableView *)tableView viewForHeaderInSection:(NSInteger)section {
	NSString *text = nil;
	BOOL canAddEntry = self.tableView.isEditing;
	if (contactSections[section] == ContactSections_First_Name && self.tableView.isEditing) {
		text = NSLocalizedString(@"First name", nil);
		canAddEntry = NO;
	} else if (contactSections[section] == ContactSections_Last_Name && self.tableView.isEditing) {
		text = NSLocalizedString(@"Last name", nil);
		canAddEntry = NO;
	} else if ([self getSectionData:section].count > 0 || self.tableView.isEditing) {
		if (contactSections[section] == ContactSections_Number) {
			text = NSLocalizedString(@"Phone numbers", nil);
		} else if (contactSections[section] == ContactSections_Sip) {
			text = NSLocalizedString(@"SIP addresses", nil);
		} else if (contactSections[section] == ContactSections_Email &&
				   [LinphoneManager.instance lpConfigBoolForKey:@"show_contacts_emails_preference"]) {
			text = NSLocalizedString(@"Email addresses", nil);
		}
	}

	if (!text) {
		return nil;
	}

	UIView *tempView = [[UIView alloc] initWithFrame:CGRectMake(0, 0, 300, 35)];
	tempView.backgroundColor = [UIColor whiteColor];

	UILabel *tempLabel = [[UILabel alloc] initWithFrame:CGRectMake(15, 5, 300, 35)];
	tempLabel.backgroundColor = [UIColor clearColor];
	tempLabel.textColor = [UIColor colorWithPatternImage:[UIImage imageNamed:@"color_E"]];
	tempLabel.text = text.uppercaseString;
	tempLabel.textAlignment = NSTextAlignmentLeft;
	tempLabel.font = [UIFont systemFontOfSize:12];
	[tempView addSubview:tempLabel];

	if (canAddEntry) {
		CGRect frame = CGRectMake(255, 5, 30, 30);
		frame.origin.x = tableView.frame.size.width - 35;
		UIIconButton *tempAddButton = [[UIIconButton alloc] initWithFrame:frame];
		[tempAddButton setImage:[UIImage imageNamed:@"add_field_default"] forState:UIControlStateNormal];
		[tempAddButton setImage:[UIImage imageNamed:@"add_field_over"] forState:UIControlStateHighlighted];
		[tempAddButton setImage:[UIImage imageNamed:@"add_field_over"] forState:UIControlStateSelected];
		[tempAddButton addTarget:self action:@selector(onAddClick:) forControlEvents:UIControlEventTouchUpInside];
		tempAddButton.tag = section;
		[tempView addSubview:tempAddButton];
	}

	return tempView;
}

- (void)onAddClick:(id)sender {
	NSInteger section = ((UIButton *)sender).tag;
	UITableView *tableView = VIEW(ContactDetailsView).tableController.tableView;
	NSInteger count = [self.tableView numberOfRowsInSection:section];
	NSIndexPath *indexPath = [NSIndexPath indexPathForRow:count inSection:section];
	[tableView.dataSource tableView:tableView
				 commitEditingStyle:UITableViewCellEditingStyleInsert
				  forRowAtIndexPath:indexPath];
}

#pragma mark - ContactDetailsLabelDelegate Functions

- (void)changeContactDetailsLabel:(NSString *)value {
	if (value != nil) {
		NSInteger section = editingIndexPath.section;
		NSMutableArray *sectionDict = [self getSectionData:section];
		ABPropertyID property = [self propertyIDForSection:(int)section];
		Entry *entry = [sectionDict objectAtIndex:editingIndexPath.row];

		if (property != kABInvalidPropertyType) {
			ABMultiValueRef lcMap = ABRecordCopyValue(contact, kABPersonPhoneProperty);
			ABMutableMultiValueRef lMap = ABMultiValueCreateMutableCopy(lcMap);
			CFRelease(lcMap);
			NSInteger index = ABMultiValueGetIndexForIdentifier(lMap, [entry identifier]);
			ABMultiValueReplaceLabelAtIndex(lMap, (__bridge CFStringRef)(value), index);
			ABRecordSetValue(contact, kABPersonPhoneProperty, lMap, nil);
			CFRelease(lMap);
		}

		[self.tableView beginUpdates];
		[self.tableView reloadRowsAtIndexPaths:[NSArray arrayWithObject:editingIndexPath] withRowAnimation:FALSE];
		[self.tableView reloadSectionIndexTitles];
		[self.tableView endUpdates];
	}
	editingIndexPath = nil;
}

#pragma mark - UITextFieldDelegate Functions

- (BOOL)textField:(UITextField *)textField
	shouldChangeCharactersInRange:(NSRange)range
				replacementString:(NSString *)string {
	if (contactDetailsDelegate != nil) {
		[self performSelector:@selector(updateModification) withObject:nil afterDelay:0];
	}
	return YES;
}

- (BOOL)textFieldShouldReturn:(UITextField *)textField {
	[textField resignFirstResponder];
	return YES;
}

- (BOOL)textFieldShouldEndEditing:(UITextField *)textField {
	UIView *view = [textField superview];
	// Find TableViewCell
	while (view != nil && ![view isKindOfClass:[UIContactDetailsCell class]])
		view = [view superview];
	if (view != nil) {
		UIContactDetailsCell *cell = (UIContactDetailsCell *)view;
		NSIndexPath *path = [self.tableView indexPathForCell:cell];
		NSMutableArray *sectionDict = [self getSectionData:[path section]];
		Entry *entry = [sectionDict objectAtIndex:[path row]];
		ContactSections_e sect = contactSections[[path section]];

		ABPropertyID property = [self propertyIDForSection:sect];
		NSString *value = [textField text];

		if (sect == ContactSections_Sip) {
			[self setSipContactEntry:entry withValue:value];
		} else if (property != kABInvalidPropertyType) {
			ABMultiValueRef lcMap = ABRecordCopyValue(contact, property);
			ABMutableMultiValueRef lMap = ABMultiValueCreateMutableCopy(lcMap);
			CFRelease(lcMap);
			NSInteger index = ABMultiValueGetIndexForIdentifier(lMap, [entry identifier]);
			ABMultiValueReplaceValueAtIndex(lMap, (__bridge CFStringRef)value, index);
			ABRecordSetValue(contact, property, lMap, nil);
			CFRelease(lMap);
		}

		cell.editTextfield.text = value;
	} else {
		LOGE(@"Not valid UIEditableTableViewCell");
	}
	if (contactDetailsDelegate != nil) {
		[self performSelector:@selector(updateModification) withObject:nil afterDelay:0];
	}
	return TRUE;
}
- (BOOL)isValid {
	return true;
}

- (CGFloat)tableView:(UITableView *)tableView heightForRowAtIndexPath:(NSIndexPath *)indexPath {
	if (tableView.isEditing) {
		return 44;
	} else {
		return 88;
	}
}
- (CGFloat)tableView:(UITableView *)tableView heightForFooterInSection:(NSInteger)section {
	return 1e-5;
}

- (CGFloat)tableView:(UITableView *)tableView heightForHeaderInSection:(NSInteger)section {
	if (section == 0 || (!self.tableView.isEditing && (contactSections[section] == ContactSections_First_Name ||
													   contactSections[section] == ContactSections_Last_Name))) {
		return 1e-5;
	}
	return [self tableView:tableView viewForHeaderInSection:section].frame.size.height;
}

@end
