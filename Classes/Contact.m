//
//  Contact.m
//  linphone
//
//  Created by Gautier Pelloux-Prayer on 12/01/16.
//
//

#import "Contact.h"

@implementation Contact

- (instancetype)initWithPerson:(ABRecordRef)aperson {
	return [self initWithPerson:aperson andFriend:NULL];
}

- (instancetype)initWithFriend:(LinphoneFriend *)afriend {
	return [self initWithPerson:NULL andFriend:afriend];
}

- (instancetype)initWithPerson:(ABRecordRef)aperson andFriend:(LinphoneFriend *)afriend {
	self = [super init];
	_person = aperson;
	_friend = afriend ? linphone_friend_ref(afriend) : NULL;

	if (_person) {
		[self loadProperties];

		const char* key = [NSString stringWithFormat:@"ab%d", ABRecordGetRecordID(aperson)].UTF8String;
		// try to find friend associated with that person
		_friend = linphone_friend_list_find_friend_by_ref_key(linphone_core_get_default_friend_list(LC), key);
		if (!_friend) {
			_friend = linphone_friend_ref(linphone_core_create_friend(LC));
			linphone_friend_set_ref_key(_friend, key);
			linphone_friend_set_name(_friend, [NSString stringWithFormat:@"%@ %@", _firstName, _lastName].UTF8String);
			for (NSString* sipAddr in _sipAddresses) {
				LinphoneAddress* addr = linphone_core_interpret_url(LC, sipAddr.UTF8String);
				if (addr) {
					linphone_address_set_display_name(addr, [self displayName].UTF8String);
					linphone_friend_add_address(_friend, addr);
					linphone_address_destroy(addr);
				}
			}
			for (NSString* phone in _phoneNumbers) {
#if 0
				char* normalized_phone = linphone_proxy_config_normalize_phone_number(linphone_core_get_default_proxy_config(LC), phone.UTF8String);
				if (normalized_phone) {
					LinphoneAddress* addr = linphone_core_interpret_url(LC, normalized_phone);
					if (addr) {
						linphone_address_set_display_name(addr, [self displayName].UTF8String);
						linphone_friend_add_address(_friend, addr);
						linphone_address_destroy(addr);
					}
					ms_free(normalized_phone);
				}
#else
				linphone_friend_add_phone_number(_friend, phone.UTF8String);
#endif
			}
			if (_friend) {
				linphone_friend_enable_subscribes(_friend, FALSE);
				linphone_friend_set_inc_subscribe_policy(_friend, LinphoneSPDeny);
				linphone_core_add_friend(LC, _friend);
			}
		}
		linphone_friend_ref(_friend);
	} else if (_friend) {
		[self loadFriend];
	} else {
		LOGE(@"Contact cannot be initialized");
		return nil;
	}

	LOGI(@"Contact %@ %@ initialized with %d phones, %d sip, %d emails", self.firstName ?: @"", self.lastName ?: @"",
		 self.phoneNumbers.count, self.sipAddresses.count, self.emails.count);
	return self;
}

- (void)dealloc {
	if (_person != nil && ABRecordGetRecordID(_person) == kABRecordInvalidID) {
		CFRelease(_person);
	}
	if (_friend) {
		linphone_friend_unref(_friend);
	}
	_person = nil;
	_friend = NULL;
}

#pragma mark - Getters
- (UIImage *)avatar:(BOOL)thumbnail {
	if (_person && ABPersonHasImageData(_person)) {
		NSData *imgData = CFBridgingRelease(ABPersonCopyImageDataWithFormat(
			_person, thumbnail ? kABPersonImageFormatThumbnail : kABPersonImageFormatOriginalSize));
		return [UIImage imageWithData:imgData];
	}
	return nil;
}

- (NSString *)displayName {
	if (_friend) {
	//	const char *dp = linphone_address_get_display_name(linphone_friend_get_address(_friend));
		//if (dp)
		//	return [NSString stringWithUTF8String:dp];
	}

	if (_person != nil) {
		NSString *lFirstName = CFBridgingRelease(ABRecordCopyValue(_person, kABPersonFirstNameProperty));
		NSString *lLocalizedFirstName = [FastAddressBook localizedLabel:lFirstName];
		NSString *compositeName = CFBridgingRelease(ABRecordCopyCompositeName(_person));

		NSString *lLastName = CFBridgingRelease(ABRecordCopyValue(_person, kABPersonLastNameProperty));
		NSString *lLocalizedLastName = [FastAddressBook localizedLabel:lLastName];

		NSString *lOrganization = CFBridgingRelease(ABRecordCopyValue(_person, kABPersonOrganizationProperty));
		NSString *lLocalizedOrganization = [FastAddressBook localizedLabel:lOrganization];

		if (compositeName) {
			return compositeName;
		} else if (lLocalizedFirstName || lLocalizedLastName) {
			return [NSString stringWithFormat:@"%@ %@", lLocalizedFirstName, lLocalizedLastName];
		} else {
			return (NSString *)lLocalizedOrganization;
		}
	}

	if (_lastName || _firstName) {
		NSMutableString *str;
		if (_firstName)
			[str appendString:_firstName];
		if (_firstName && _lastName)
			[str appendString:@" "];
		if (_lastName)
			[str appendString:_lastName];
	}

	return NSLocalizedString(@"Unknown", nil);
}

#pragma mark - Setters

- (void)setAvatar:(UIImage *)avatar {
	if (_person) {
		CFErrorRef error = NULL;
		if (!ABPersonRemoveImageData(_person, &error)) {
			LOGW(@"Can't remove entry: %@", [(__bridge NSError *)error localizedDescription]);
		}
		NSData *dataRef = UIImageJPEGRepresentation(avatar, 0.9f);
		CFDataRef cfdata = CFDataCreate(NULL, [dataRef bytes], [dataRef length]);

		if (!ABPersonSetImageData(_person, cfdata, &error)) {
			LOGW(@"Can't add entry: %@", [(__bridge NSError *)error localizedDescription]);
		}

		CFRelease(cfdata);
	} else {
		LOGW(@"%s: Cannot do it when using LinphoneFriend, skipping", __FUNCTION__);
	}
}

- (void)setFirstName:(NSString *)firstName {
	BOOL ret = FALSE;
	if (_person) {
		ret = ([self replaceInProperty:kABPersonFirstNameProperty value:(__bridge CFTypeRef)(firstName)]);
	} else {
		ret = (linphone_friend_set_name(_friend, firstName.UTF8String) == 0);
	}

	if (ret) {
		_firstName = firstName;
	}
}

- (void)setLastName:(NSString *)lastName {
	BOOL ret = FALSE;
	if (_person) {
		ret = ([self replaceInProperty:kABPersonLastNameProperty value:(__bridge CFTypeRef)(lastName)]);
	} else {
		LOGW(@"%s: Cannot do it when using LinphoneFriend, skipping", __FUNCTION__);
	}

	if (ret) {
		_lastName = lastName;
	}
}

- (BOOL)setSipAddress:(NSString *)sip atIndex:(NSInteger)index {
	BOOL ret = FALSE;
	if (_person) {
		NSDictionary *lDict = @{
			(NSString *) kABPersonInstantMessageUsernameKey : sip, (NSString *)
			kABPersonInstantMessageServiceKey : LinphoneManager.instance.contactSipField
		};

		ret = [self replaceInProperty:kABPersonInstantMessageProperty value:(__bridge CFTypeRef)(lDict) atIndex:index];
	} else {
		LOGW(@"%s: Cannot do it when using LinphoneFriend, skipping", __FUNCTION__);
	}

	if (ret) {
		_sipAddresses[index] = sip;
	}
	return ret;
}

- (BOOL)setPhoneNumber:(NSString *)phone atIndex:(NSInteger)index {
	BOOL ret = FALSE;
	if (_person) {
		ret = [self replaceInProperty:kABPersonPhoneProperty value:(__bridge CFTypeRef)(phone) atIndex:index];
	} else {
		LOGW(@"%s: Cannot do it when using LinphoneFriend, skipping", __FUNCTION__);
	}
	if (ret) {
		_phoneNumbers[index] = phone;
	}
	return ret;
}

- (BOOL)setEmail:(NSString *)email atIndex:(NSInteger)index {
	BOOL ret = FALSE;
	if (_person) {
		ret = [self replaceInProperty:kABPersonEmailProperty value:(__bridge CFTypeRef)(email) atIndex:index];
	} else {
	}
	if (ret) {
		_emails[index] = email;
	}
	return ret;
}

- (BOOL)addSipAddress:(NSString *)sip {
	BOOL ret = FALSE;
	if (_person) {
		NSDictionary *lDict = @{
			(NSString *) kABPersonInstantMessageUsernameKey : sip, (NSString *)
			kABPersonInstantMessageServiceKey : LinphoneManager.instance.contactSipField
		};

		ret = [self addInProperty:kABPersonInstantMessageProperty value:(__bridge CFTypeRef)(lDict)];
	} else {
		LinphoneAddress *addr = linphone_core_interpret_url(LC, sip.UTF8String) ?: linphone_address_new(sip.UTF8String);
		if (addr) {
			ret = TRUE;
			linphone_friend_add_address(_friend, addr);
			linphone_address_destroy(addr);
			// ensure that it was added by checking list size
			ret = (bctbx_list_size(linphone_friend_get_addresses(_friend)) == _sipAddresses.count + 1);
		}
	}
	if (ret) {
		[_sipAddresses addObject:sip];
	}
	return ret;
}

- (BOOL)addPhoneNumber:(NSString *)phone {
	BOOL ret = FALSE;
	if (_person) {
		ret = [self addInProperty:kABPersonPhoneProperty value:(__bridge CFTypeRef)(phone)];
	} else {
		char *cphone = ms_strdup(phone.UTF8String);
		// linphone_proxy_config_normalize_phone_number(NULL, phone.UTF8String) ?: ms_strdup(phone.UTF8String);
		if (cphone) {
			linphone_friend_add_phone_number(_friend, cphone);
			phone = [NSString stringWithUTF8String:cphone];
			ms_free(cphone);
			// ensure that it was added by checking list size
			ret = (bctbx_list_size(linphone_friend_get_phone_numbers(_friend)) == _phoneNumbers.count + 1);
		}
	}
	if (ret) {
		[_phoneNumbers addObject:phone];
	}
	return ret;
}

- (BOOL)addEmail:(NSString *)email {
	BOOL ret = FALSE;
	if (_person) {
		ret = [self addInProperty:kABPersonEmailProperty value:(__bridge CFTypeRef)(email)];
	} else {
		LOGW(@"%s: Cannot do it when using LinphoneFriend, skipping", __FUNCTION__);
	}
	if (ret) {
		[_emails addObject:email];
	}
	return ret;
}

- (BOOL)removeSipAddressAtIndex:(NSInteger)index {
	BOOL ret = FALSE;
	if (_person) {
		ret = [self removeInProperty:kABPersonInstantMessageProperty atIndex:index];
	} else {
		LinphoneAddress *addr = linphone_core_interpret_url(LC, ((NSString *)_sipAddresses[index]).UTF8String);
		if (addr) {
			linphone_friend_remove_address(_friend, addr);
			linphone_address_destroy(addr);
			// ensure that it was destroyed by checking list size
			ret = (bctbx_list_size(linphone_friend_get_addresses(_friend)) + 1 == _sipAddresses.count);
		}
	}
	if (ret) {
		[_sipAddresses removeObjectAtIndex:index];
	}
	return ret;
}

- (BOOL)removePhoneNumberAtIndex:(NSInteger)index {
	BOOL ret = FALSE;
	if (_person) {
		ret = [self removeInProperty:kABPersonPhoneProperty atIndex:index];
	} else {
		const char *phone = ((NSString *)_phoneNumbers[index]).UTF8String;
		linphone_friend_remove_phone_number(_friend, phone);
		// ensure that it was destroyed by checking list size
		ret = (bctbx_list_size(linphone_friend_get_phone_numbers(_friend)) + 1 == _phoneNumbers.count);
	}
	if (ret) {
		[_phoneNumbers removeObjectAtIndex:index];
	}
	return ret;
}

- (BOOL)removeEmailAtIndex:(NSInteger)index {
	BOOL ret = FALSE;
	if (_person) {
		ret = [self removeInProperty:kABPersonEmailProperty atIndex:index];
	} else {
		LOGW(@"%s: Cannot do it when using LinphoneFriend, skipping", __FUNCTION__);
	}
	if (ret) {
		[_emails removeObjectAtIndex:index];
	}
	return ret;
}

#pragma mark - ABPerson utils

- (void)loadProperties {
	// First and Last name
	{
		_firstName = (NSString *)CFBridgingRelease(ABRecordCopyValue(_person, kABPersonFirstNameProperty));
		_lastName = (NSString *)CFBridgingRelease(ABRecordCopyValue(_person, kABPersonLastNameProperty));
	}

	// Phone numbers
	{
		_phoneNumbers = [[NSMutableArray alloc] init];
		ABMultiValueRef map = ABRecordCopyValue(_person, kABPersonPhoneProperty);
		if (map) {
			for (int i = 0; i < ABMultiValueGetCount(map); ++i) {
				ABMultiValueIdentifier identifier = ABMultiValueGetIdentifierAtIndex(map, i);
				NSInteger index = ABMultiValueGetIndexForIdentifier(map, identifier);
				if (index != -1) {
					NSString *valueRef = CFBridgingRelease(ABMultiValueCopyValueAtIndex(map, index));
					// char *normalizedPhone = linphone_proxy_config_normalize_phone_number(
					//	linphone_core_get_default_proxy_config(LC), valueRef.UTF8String);
					// if (normalizedPhone) {
					//	valueRef = [NSString stringWithUTF8String:normalizedPhone];
					//	ms_free(normalizedPhone);
					//}

					[_phoneNumbers addObject:valueRef];
				}
			}
			CFRelease(map);
		}
	}

	// SIP (IM)
	{
		_sipAddresses = [[NSMutableArray alloc] init];
		ABMultiValueRef map = ABRecordCopyValue(_person, kABPersonInstantMessageProperty);
		if (map) {
			for (int i = 0; i < ABMultiValueGetCount(map); ++i) {
				CFDictionaryRef lDict = ABMultiValueCopyValueAtIndex(map, i);
				if (CFDictionaryContainsKey(lDict, kABPersonInstantMessageServiceKey)) {
					if (CFStringCompare((CFStringRef)LinphoneManager.instance.contactSipField,
										CFDictionaryGetValue(lDict, kABPersonInstantMessageServiceKey),
										kCFCompareCaseInsensitive) == 0) {
						NSString *value = (NSString *)(CFDictionaryGetValue(lDict, kABPersonInstantMessageUsernameKey));
						CFRelease(lDict);
						if (value != NULL) {
							[_sipAddresses addObject:value];
						}
					}
				}
			}
			CFRelease(map);
		}
	}

	// Email
	{
		_emails = [[NSMutableArray alloc] init];
		ABMultiValueRef map = ABRecordCopyValue(_person, kABPersonEmailProperty);
		if (map) {
			for (int i = 0; i < ABMultiValueGetCount(map); ++i) {
				ABMultiValueIdentifier identifier = ABMultiValueGetIdentifierAtIndex(map, i);
				NSInteger index = ABMultiValueGetIndexForIdentifier(map, identifier);
				if (index != -1) {
					NSString *valueRef = CFBridgingRelease(ABMultiValueCopyValueAtIndex(map, index));
					if (valueRef != NULL) {
						[_emails addObject:valueRef];
					}
				}
			}
			CFRelease(map);
		}
	}
}

- (BOOL)replaceInProperty:(ABPropertyID)property value:(CFTypeRef)value {
	CFErrorRef error = NULL;
	if (!ABRecordSetValue(_person, property, value, &error)) {
		LOGE(@"Error when saving property %d in contact %p: Fail(%@)", property, _person, error);
		return NO;
	}
	return YES;
}

- (BOOL)replaceInProperty:(ABPropertyID)property value:(CFTypeRef)value atIndex:(NSInteger)index {
	ABMultiValueRef lcMap = ABRecordCopyValue(_person, property);
	ABMutableMultiValueRef lMap;
	if (lcMap != NULL) {
		lMap = ABMultiValueCreateMutableCopy(lcMap);
		CFRelease(lcMap);
	} else {
		lMap = ABMultiValueCreateMutable(kABStringPropertyType);
	}

	BOOL ret = ABMultiValueReplaceValueAtIndex(lMap, value, index);
	if (ret) {
		ret = [self replaceInProperty:property value:lMap];
	} else {
		LOGW(@"Could not replace %@ at index %d from property %d", value, index, property);
	}

	CFRelease(lMap);
	return ret;
}

- (BOOL)addInProperty:(ABPropertyID)property value:(CFTypeRef)value {
	ABMultiValueRef lcMap = ABRecordCopyValue(_person, property);
	ABMutableMultiValueRef lMap;
	if (lcMap != NULL) {
		lMap = ABMultiValueCreateMutableCopy(lcMap);
		CFRelease(lcMap);
	} else {
		lMap = ABMultiValueCreateMutable(kABStringPropertyType);
	}

	// will display this field with our application name
	CFStringRef label = (__bridge CFStringRef)[[NSBundle mainBundle] objectForInfoDictionaryKey:@"CFBundleDisplayName"];
	BOOL ret = ABMultiValueAddValueAndLabel(lMap, value, label, nil);
	if (ret) {
		ret = [self replaceInProperty:property value:lMap];
	} else {
		LOGW(@"Could not add %@ to property %d", value, property);
	}
	CFRelease(lMap);
	return ret;
}

- (BOOL)removeInProperty:(ABPropertyID)property atIndex:(NSInteger)index {
	ABMultiValueRef lcMap = ABRecordCopyValue(_person, property);
	ABMutableMultiValueRef lMap;
	if (lcMap != NULL) {
		lMap = ABMultiValueCreateMutableCopy(lcMap);
		CFRelease(lcMap);
	} else {
		lMap = ABMultiValueCreateMutable(kABStringPropertyType);
	}

	BOOL ret = ABMultiValueRemoveValueAndLabelAtIndex(lMap, index);
	if (ret) {
		ret = [self replaceInProperty:property value:lMap];
	} else {
		LOGW(@"Could not remove at index %d from property %d", index, property);
	}

	CFRelease(lMap);
	return ret;
}

#pragma mark - LinphoneFriend utils

- (void)loadFriend {
	// First and Last name
	{
		_firstName = [NSString stringWithUTF8String:linphone_friend_get_name(_friend) ?: ""];
		_lastName = nil;
	}

	// Phone numbers
	{
		_phoneNumbers = [[NSMutableArray alloc] init];
		MSList *numbers = linphone_friend_get_phone_numbers(_friend);
		while (numbers) {
			NSString *phone = [NSString stringWithUTF8String:numbers->data];
			[_phoneNumbers addObject:phone];
			numbers = numbers->next;
		}
	}

	// SIP (IM)
	{
		_sipAddresses = [[NSMutableArray alloc] init];
		const MSList *sips = linphone_friend_get_addresses(_friend);
		while (sips) {
			LinphoneAddress *addr = sips->data;
			char *uri = linphone_address_as_string_uri_only(addr);
			NSString *sipaddr = [NSString stringWithUTF8String:uri];
			[_sipAddresses addObject:sipaddr];
			ms_free(uri);

			sips = sips->next;
		}
	}

	// Email - no support for LinphoneFriend
	{ _emails = [[NSMutableArray alloc] init]; }
}

@end
