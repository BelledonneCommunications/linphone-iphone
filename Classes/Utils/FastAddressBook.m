/* FastAddressBook.h
 *
 * Copyright (C) 2011  Belledonne Comunications, Grenoble, France
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
#ifdef __IPHONE_9_0
#import <Contacts/Contacts.h>
#endif
#import "FastAddressBook.h"
#import "LinphoneManager.h"
#import "ContactsListView.h"
#import "Utils.h"

@implementation FastAddressBook {
	CNContactStore* store;
}

+ (UIImage *)imageForContact:(Contact *)contact {
	@synchronized(LinphoneManager.instance.fastAddressBook.addressBookMap) {
		UIImage *retImage = [contact avatar];
		if (retImage == nil) {
			retImage = [UIImage imageNamed:@"avatar.png"];
		}
		if (retImage.size.width != retImage.size.height) {
			retImage = [retImage squareCrop];
		}
		return retImage;
	}
}

+ (UIImage *)imageForAddress:(const LinphoneAddress *)addr {
	if ([LinphoneManager isMyself:addr] && [LinphoneUtils hasSelfAvatar]) {
		return [LinphoneUtils selfAvatar];
	}
	return [FastAddressBook imageForContact:[FastAddressBook getContactWithAddress:addr]];
}

+ (UIImage *)imageForSecurityLevel:(LinphoneChatRoomSecurityLevel)level {
    switch (level) {
        case LinphoneChatRoomSecurityLevelUnsafe:
            return [UIImage imageNamed:@"security_alert_indicator.png"];
        case LinphoneChatRoomSecurityLevelEncrypted:
            return [UIImage imageNamed:@"security_1_indicator.png.png"];
        case LinphoneChatRoomSecurityLevelSafe:
            return [UIImage imageNamed:@"security_2_indicator.png.png"];
            
        default:
            return nil;
    }
}

+ (Contact *)getContact:(NSString *)address {
	if (LinphoneManager.instance.fastAddressBook != nil) {
		@synchronized(LinphoneManager.instance.fastAddressBook.addressBookMap) {
			return [LinphoneManager.instance.fastAddressBook.addressBookMap objectForKey:address];
		}
	}
  	return nil;
}

+ (Contact *)getContactWithAddress:(const LinphoneAddress *)address {
	Contact *contact = nil;
	if (address) {
		char *uri = linphone_address_as_string_uri_only(address);
		NSString *normalizedSipAddress = [FastAddressBook normalizeSipURI:[NSString stringWithUTF8String:uri]];
		contact = [FastAddressBook getContact:normalizedSipAddress];
		ms_free(uri);

		if (!contact) {
			LinphoneFriend *friend = linphone_core_find_friend(LC, address);
			MSList *numbers = linphone_friend_get_phone_numbers(friend);
			while (numbers) {
				NSString *phone = [NSString stringWithUTF8String:numbers->data];
				LinphoneProxyConfig *cfg = linphone_core_get_default_proxy_config(LC);
				const char *normvalue = linphone_proxy_config_normalize_phone_number(cfg, phone.UTF8String);
				LinphoneAddress *addr = linphone_proxy_config_normalize_sip_uri(cfg, normvalue);
				const char *phone_addr = linphone_address_as_string_uri_only(addr);
				contact = [FastAddressBook getContact:[NSString stringWithUTF8String:phone_addr]];
				if (contact) {
					break;
				}
				numbers = numbers->next;
			}
		}
	}
	return contact;
}

+ (BOOL)isSipURI:(NSString *)address {
	return [address hasPrefix:@"sip:"] || [address hasPrefix:@"sips:"];
}

+ (BOOL)isSipAddress:(CNLabeledValue<CNInstantMessageAddress *> *)sipAddr {
	NSString *username = sipAddr.value.username;
	NSString *service = sipAddr.value.service;
	LOGI(@"Parsing contact with username : %@ and service : %@", username, service);
	if (!username || [username isEqualToString:@""])
		return FALSE;

	if (!service || [service isEqualToString:@""])
		return [FastAddressBook isSipURI:username];

	if ([service isEqualToString:LinphoneManager.instance.contactSipField])
		return TRUE;

	return FALSE;
}

+ (NSString *)normalizeSipURI:(NSString *)address {
	// replace all whitespaces (non-breakable, utf8 nbsp etc.) by the "classical" whitespace
	NSString *normalizedSipAddress = nil;
	LinphoneAddress *addr = linphone_core_interpret_url(LC, [address UTF8String]);
	if (addr != NULL) {
		linphone_address_clean(addr);
		char *tmp = linphone_address_as_string(addr);
		normalizedSipAddress = [NSString stringWithUTF8String:tmp];
		ms_free(tmp);
		linphone_address_destroy(addr);
		return normalizedSipAddress;
	}else {
		normalizedSipAddress = [[address componentsSeparatedByCharactersInSet:[NSCharacterSet whitespaceCharacterSet]] componentsJoinedByString:@" "];
		return normalizedSipAddress;
	}
}

+ (BOOL)isAuthorized {
  return [CNContactStore authorizationStatusForEntityType:CNEntityTypeContacts];
}

- (FastAddressBook *)init {
	if ((self = [super init]) != nil) {
		store = [[CNContactStore alloc] init];
		_addressBookMap = [NSMutableDictionary dictionary];
	}
	self.needToUpdate = FALSE;
	if (floor(NSFoundationVersionNumber) >= NSFoundationVersionNumber_iOS_9_x_Max) {
		if ([CNContactStore class]) {
			// ios9 or later
			if (store == NULL)
				store = [[CNContactStore alloc] init];
			[self fetchContactsInBackGroundThread];
			[[NSNotificationCenter defaultCenter] addObserver:self selector:@selector(updateAddressBook:) name:CNContactStoreDidChangeNotification object:nil];
		}
	}
	return self;
}

- (void) fetchContactsInBackGroundThread{
	[_addressBookMap removeAllObjects];
	_addressBookMap = [NSMutableDictionary dictionary];
	CNEntityType entityType = CNEntityTypeContacts;
	[store requestAccessForEntityType:entityType completionHandler:^(BOOL granted, NSError *_Nullable error) {
		BOOL success = FALSE;
		if(granted){
			LOGD(@"CNContactStore authorization granted");
			
			NSError *contactError;
			CNContactStore* store = [[CNContactStore alloc] init];
			[store containersMatchingPredicate:[CNContainer predicateForContainersWithIdentifiers:@[ store.defaultContainerIdentifier]] error:&contactError];
			NSArray *keysToFetch = @[
									 CNContactEmailAddressesKey, CNContactPhoneNumbersKey,
									 CNContactFamilyNameKey, CNContactGivenNameKey, CNContactNicknameKey,
									 CNContactPostalAddressesKey, CNContactIdentifierKey,
									 CNInstantMessageAddressUsernameKey, CNContactInstantMessageAddressesKey,
									 CNInstantMessageAddressUsernameKey, CNContactImageDataKey, CNContactOrganizationNameKey
									 ];
			CNContactFetchRequest *request = [[CNContactFetchRequest alloc] initWithKeysToFetch:keysToFetch];
			
			success = [store enumerateContactsWithFetchRequest:request error:&contactError usingBlock:^(CNContact *__nonnull contact, BOOL *__nonnull stop) {
				if (contactError) {
				  NSLog(@"error fetching contacts %@",
						contactError);
				} else {
					Contact *newContact = [[Contact alloc] initWithCNContact:contact];
					[self registerAddrsFor:newContact];
				}
			}];
		}

	}];
	// load Linphone friends
	const MSList *lists = linphone_core_get_friends_lists(LC);
	while (lists) {
		LinphoneFriendList *fl = lists->data;
		const MSList *friends = linphone_friend_list_get_friends(fl);
		while (friends) {
			LinphoneFriend *f = friends->data;
			// only append friends that are not native contacts (already added
			// above)
			if (linphone_friend_get_ref_key(f) == NULL) {
				Contact *contact = [[Contact alloc] initWithFriend:f];
				[self registerAddrsFor:contact];
			}
			friends = friends->next;
		}
		linphone_friend_list_update_subscriptions(fl);
		lists = lists->next;
	}
	[NSNotificationCenter.defaultCenter
	 postNotificationName:kLinphoneAddressBookUpdate
	 object:self];
}

-(void) updateAddressBook:(NSNotification*) notif {
	LOGD(@"address book has changed");
	self.needToUpdate = TRUE;
}

- (void)registerAddrsFor:(Contact *)contact {
	if (!contact)
		return;

	Contact* mContact = contact;
	if (!_addressBookMap)
		return;

	for (NSString *phone in mContact.phones) {
		char *normalizedPhone = linphone_proxy_config_normalize_phone_number(linphone_core_get_default_proxy_config(LC), phone.UTF8String);
		NSString *name = [FastAddressBook normalizeSipURI:normalizedPhone ? [NSString stringWithUTF8String:normalizedPhone] : phone];
		if (phone != NULL)
			[_addressBookMap setObject:mContact forKey:(name ?: [FastAddressBook localizedLabel:phone])];

		if (normalizedPhone)
			ms_free(normalizedPhone);
	}

	for (NSString *sip in mContact.sipAddresses)
		[_addressBookMap setObject:mContact forKey:([FastAddressBook normalizeSipURI:sip] ?: sip)];
}

#pragma mark - Tools

+ (NSString *)localizedLabel:(NSString *)label {
	if (label)
		return [CNLabeledValue localizedStringForLabel:label];

	return @"";
}

+ (BOOL)contactHasValidSipDomain:(Contact *)contact {
	if (!contact)
		return NO;
	
	// Check if one of the contact' sip URI matches the expected SIP filter
	NSString *domain = LinphoneManager.instance.contactFilter;

	for (NSString *sip in contact.sipAddresses) {
		// check domain
		LinphoneAddress *address = linphone_core_interpret_url(LC, sip.UTF8String);
		if (address) {
			const char *dom = linphone_address_get_domain(address);
			BOOL match = false;
			if (dom != NULL) {
				NSString *contactDomain = [NSString stringWithCString:dom encoding:[NSString defaultCStringEncoding]];
				match = (([domain compare:@"*" options:NSCaseInsensitiveSearch] == NSOrderedSame) ||
						 ([domain compare:contactDomain options:NSCaseInsensitiveSearch] == NSOrderedSame));
			}
			linphone_address_destroy(address);
			if (match)
				return YES;
		}
	}
	return NO;
}

+ (BOOL) isSipURIValid:(NSString*)addr {
	NSString *domain = LinphoneManager.instance.contactFilter;
	LinphoneAddress* address = linphone_core_interpret_url(LC, addr.UTF8String);
	if (address) {
		const char *dom = linphone_address_get_domain(address);
		BOOL match = false;
		if (dom != NULL) {
			NSString *contactDomain = [NSString stringWithCString:dom encoding:[NSString defaultCStringEncoding]];
			match = (([domain compare:@"*" options:NSCaseInsensitiveSearch] == NSOrderedSame) ||
					 ([domain compare:contactDomain options:NSCaseInsensitiveSearch] == NSOrderedSame));
		}
		linphone_address_destroy(address);
		if (match) {
			return YES;
		}
	}
	return NO;
}

+ (NSString *)displayNameForContact:(Contact *)contact {
	return contact.displayName;
}

+ (NSString *)displayNameForAddress:(const LinphoneAddress *)addr {
	Contact *contact = [FastAddressBook getContactWithAddress:addr];
	if (contact)
		return [FastAddressBook displayNameForContact:contact];

	LinphoneFriend *friend = linphone_core_find_friend(LC, addr);
	if (friend)
		return [NSString stringWithUTF8String:linphone_friend_get_name(friend)];

	const char *displayName = linphone_address_get_display_name(addr);
	if (displayName)
		return [NSString stringWithUTF8String:displayName];

	const char *userName = linphone_address_get_username(addr);
	if (userName)
			return [NSString stringWithUTF8String:userName];

	return NSLocalizedString(@"Unknown", nil);
}


- (CNContact *)getCNContactFromContact:(Contact *)acontact {
  NSArray *keysToFetch = @[
    CNContactEmailAddressesKey, CNContactPhoneNumbersKey,
    CNContactFamilyNameKey, CNContactGivenNameKey, CNContactPostalAddressesKey,
    CNContactIdentifierKey, CNContactInstantMessageAddressesKey,
    CNInstantMessageAddressUsernameKey, CNContactImageDataKey
  ];
  CNMutableContact *mCNContact =
      [[store unifiedContactWithIdentifier:acontact.identifier
                               keysToFetch:keysToFetch
                                     error:nil] mutableCopy];
  return mCNContact;
}

- (BOOL)deleteCNContact:(CNContact *)contact {
		return TRUE;//[self deleteContact:] ;
}

- (BOOL)deleteContact:(Contact *)contact {
	CNSaveRequest *saveRequest = [[CNSaveRequest alloc] init];
	NSArray *keysToFetch = @[
							 CNContactEmailAddressesKey, CNContactPhoneNumbersKey,
							 CNContactInstantMessageAddressesKey, CNInstantMessageAddressUsernameKey,
							 CNContactFamilyNameKey, CNContactGivenNameKey, CNContactPostalAddressesKey,
							 CNContactIdentifierKey, CNContactImageDataKey, CNContactNicknameKey
							 ];
	CNMutableContact *mCNContact =
	[[store unifiedContactWithIdentifier:contact.identifier
							 keysToFetch:keysToFetch
								   error:nil] mutableCopy];
	if(mCNContact != nil){
		[saveRequest deleteContact:mCNContact];
		@try {
			[self removeFriend:contact ];
			[LinphoneManager.instance setContactsUpdated:TRUE];
			if([contact.sipAddresses count] > 0){
				for (NSString *sip in contact.sipAddresses) {
					[_addressBookMap removeObjectForKey:([FastAddressBook normalizeSipURI:sip] ?: sip)];
				}
			}
			if([contact.phones count] > 0){
				for (NSString *phone in contact.phones) {
					[_addressBookMap removeObjectForKey:([FastAddressBook normalizeSipURI:phone] ?: phone)];
				}
			}
			BOOL success = [store executeSaveRequest:saveRequest error:nil];
			NSLog(@"Success %d", success);
		} @catch (NSException *exception) {
			NSLog(@"description = %@", [exception description]);
			return FALSE;
		}
	}
	return TRUE;
}


- (BOOL)deleteAllContacts {
  NSArray *keys = @[ CNContactPhoneNumbersKey ];
  NSString *containerId = store.defaultContainerIdentifier;
  NSPredicate *predicate = [CNContact predicateForContactsInContainerWithIdentifier:containerId];
  NSError *error;
  NSArray *cnContacts = [store unifiedContactsMatchingPredicate:predicate
                                                    keysToFetch:keys
														  error:&error];
	if (error) {
		NSLog(@"error fetching contacts %@", error);
		return FALSE;
	} else {
		CNSaveRequest *saveRequest = [[CNSaveRequest alloc] init];
		for (CNContact *contact in cnContacts) {
		  [saveRequest deleteContact:[contact mutableCopy]];
		}
		@try {
			NSLog(@"Success %d", [store executeSaveRequest:saveRequest error:nil]);
		} @catch (NSException *exception) {
			NSLog(@"description = %@", [exception description]);
			return FALSE;
		}
	  NSLog(@"Deleted contacts %lu", (unsigned long)cnContacts.count);
	}
	return TRUE;
}

- (BOOL)saveContact:(Contact *)contact {
  return [self saveCNContact:contact.person contact:contact];
}

- (BOOL)saveCNContact:(CNContact *)cNContact contact:(Contact *)contact {
  CNSaveRequest *saveRequest = [[CNSaveRequest alloc] init];
  NSArray *keysToFetch = @[
    CNContactEmailAddressesKey, CNContactPhoneNumbersKey,
    CNContactInstantMessageAddressesKey, CNInstantMessageAddressUsernameKey,
    CNContactFamilyNameKey, CNContactGivenNameKey, CNContactPostalAddressesKey,
    CNContactIdentifierKey, CNContactImageDataKey, CNContactNicknameKey
  ];
  CNMutableContact *mCNContact =
      [[store unifiedContactWithIdentifier:contact.identifier
                               keysToFetch:keysToFetch
                                     error:nil] mutableCopy];
	if(mCNContact == NULL){
		[saveRequest addContact:[cNContact mutableCopy] toContainerWithIdentifier:nil];
	}else{
	  [mCNContact setGivenName:contact.firstName];
	  [mCNContact setFamilyName:contact.lastName];
	  [mCNContact setNickname:contact.displayName];
	  [mCNContact setPhoneNumbers:contact.person.phoneNumbers];
	  [mCNContact setEmailAddresses:contact.person.emailAddresses];
	  [mCNContact
		  setInstantMessageAddresses:contact.person.instantMessageAddresses];
	  [mCNContact setImageData:UIImageJPEGRepresentation(contact.avatar, 0.9f)];

	  [saveRequest updateContact:mCNContact];
	}
  NSError *saveError;
  @try {
	  [self updateFriend:contact];
	  [LinphoneManager.instance setContactsUpdated:TRUE];
	  NSLog(@"Success %d", [store executeSaveRequest:saveRequest error:&saveError]);
  } @catch (NSException *exception) {
	  NSLog(@"=====>>>>> CNContact SaveRequest failed : description = %@", [exception description]);
	  return FALSE;
  }
	[self fetchContactsInBackGroundThread];
  return TRUE;
}

-(void)updateFriend:(Contact*) contact{
	bctbx_list_t *phonesList = linphone_friend_get_phone_numbers(contact.friend);
	for (NSString *phone in contact.phones) {
		if(!(bctbx_list_find(phonesList, [phone UTF8String]))){
			linphone_friend_edit(contact.friend);
			linphone_friend_add_phone_number(contact.friend, [phone UTF8String]);
			linphone_friend_enable_subscribes(contact.friend, TRUE);
			linphone_friend_done(contact.friend);
		}
	}
	
	BOOL enabled = [LinphoneManager.instance lpConfigBoolForKey:@"use_rls_presence"];
	const MSList *lists = linphone_core_get_friends_lists(LC);
	while (lists) {
		linphone_friend_list_enable_subscriptions(lists->data, FALSE);
		linphone_friend_list_enable_subscriptions(lists->data, enabled);
		linphone_friend_list_update_subscriptions(lists->data);
		lists = lists->next;
	}
}

-(void)removeFriend:(Contact*) contact{
	BOOL enabled = [LinphoneManager.instance lpConfigBoolForKey:@"use_rls_presence"];
	const MSList *lists = linphone_core_get_friends_lists(LC);
	while (lists) {
		linphone_friend_list_remove_friend(lists->data, contact.friend);
		linphone_friend_list_enable_subscriptions(lists->data, FALSE);
		linphone_friend_list_enable_subscriptions(lists->data, enabled);
		linphone_friend_list_update_subscriptions(lists->data);
		lists = lists->next;
	}
}
@end
