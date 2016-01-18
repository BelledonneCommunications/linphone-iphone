//
//  Contact.m
//  linphone
//
//  Created by Gautier Pelloux-Prayer on 12/01/16.
//
//

#import "Contact.h"

@implementation Contact {
	ABRecordRef person;
}

- (instancetype)initWithPerson:(ABRecordRef)aperson {
	self = [super init];
	person = aperson;

	[self loadFields];

	LOGI(@"Contact %@ %@ initialized with %d phones, %d sip, %d emails", self.firstName ?: @"", self.lastName ?: @"",
		 self.phoneNumbers.count, self.sipAddresses.count, self.emails.count);
	return self;
}

- (void)dealloc {
	if (person != nil && ABRecordGetRecordID(person) == kABRecordInvalidID) {
		CFRelease(person);
	}
}

- (void)loadFields {
	// First and Last name
	{
		_firstName = (NSString *)CFBridgingRelease(ABRecordCopyValue(person, kABPersonFirstNameProperty));
		_lastName = (NSString *)CFBridgingRelease(ABRecordCopyValue(person, kABPersonLastNameProperty));
	}

	// Phone numbers
	{
		NSMutableArray *phones = [[NSMutableArray alloc] init];
		ABMultiValueRef map = ABRecordCopyValue(person, kABPersonPhoneProperty);
		if (map) {
			for (int i = 0; i < ABMultiValueGetCount(map); ++i) {
				ABMultiValueIdentifier identifier = ABMultiValueGetIdentifierAtIndex(map, i);
				NSInteger index = ABMultiValueGetIndexForIdentifier(map, identifier);
				if (index != -1) {
					NSString *valueRef = CFBridgingRelease(ABMultiValueCopyValueAtIndex(map, index));
					if (valueRef != NULL) {
						[phones addObject:[FastAddressBook localizedLabel:valueRef]];
					}
				}
			}
			CFRelease(map);
		}
		_phoneNumbers = phones;
	}

	// SIP (IM)
	{
		NSMutableArray *sip = [[NSMutableArray alloc] init];
		ABMultiValueRef map = ABRecordCopyValue(person, kABPersonInstantMessageProperty);
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
							[sip addObject:value];
						}
					}
				}
			}
			CFRelease(map);
		}
		_sipAddresses = sip;
	}

	// Email
	{
		NSMutableArray *emails = [[NSMutableArray alloc] init];
		ABMultiValueRef map = ABRecordCopyValue(person, kABPersonEmailProperty);
		if (map) {
			for (int i = 0; i < ABMultiValueGetCount(map); ++i) {
				ABMultiValueIdentifier identifier = ABMultiValueGetIdentifierAtIndex(map, i);
				NSInteger index = ABMultiValueGetIndexForIdentifier(map, identifier);
				if (index != -1) {
					NSString *valueRef = CFBridgingRelease(ABMultiValueCopyValueAtIndex(map, index));
					if (valueRef != NULL) {
						[emails addObject:valueRef];
					}
				}
			}
			CFRelease(map);
		}
		_emails = emails;
	}
}

#pragma mark - Setters
- (void)setFirstName:(NSString *)firstName {
	if ([self replaceInProperty:kABPersonFirstNameProperty value:(__bridge CFTypeRef)(firstName)]) {
		_firstName = firstName;
	}
}

- (void)setLastName:(NSString *)lastName {
	if ([self replaceInProperty:kABPersonLastNameProperty value:(__bridge CFTypeRef)(lastName)]) {
		_lastName = lastName;
	}
}

- (BOOL)replaceInProperty:(ABPropertyID)property value:(CFTypeRef)value {
	CFErrorRef error = NULL;
	if (!ABRecordSetValue(person, property, value, &error)) {
		LOGE(@"Error when saving property %d in contact %p: Fail(%@)", property, person, error);
		return NO;
	}
	return YES;
}

- (BOOL)replaceInProperty:(ABPropertyID)property value:(CFTypeRef)value atIndex:(NSInteger)index {
	ABMultiValueRef lcMap = ABRecordCopyValue(person, property);
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
	ABMultiValueRef lcMap = ABRecordCopyValue(person, property);
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
	ABMultiValueRef lcMap = ABRecordCopyValue(person, property);
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

- (BOOL)setSipAddress:(NSString *)sip atIndex:(NSInteger)index {
	NSDictionary *lDict = @{
		(NSString *) kABPersonInstantMessageUsernameKey : sip, (NSString *)
		kABPersonInstantMessageServiceKey : LinphoneManager.instance.contactSipField
	};

	BOOL ret = [self replaceInProperty:kABPersonInstantMessageProperty value:(__bridge CFTypeRef)(lDict) atIndex:index];
	if (ret) {
		_sipAddresses[index] = sip;
	}
	return ret;
}

- (BOOL)setPhoneNumber:(NSString *)phone atIndex:(NSInteger)index {
	BOOL ret = [self replaceInProperty:kABPersonPhoneProperty value:(__bridge CFTypeRef)(phone) atIndex:index];
	if (ret) {
		_phoneNumbers[index] = phone;
	}
	return ret;
}

- (BOOL)addSipAddress:(NSString *)sip {
	NSDictionary *lDict = @{
		(NSString *) kABPersonInstantMessageUsernameKey : sip, (NSString *)
		kABPersonInstantMessageServiceKey : LinphoneManager.instance.contactSipField
	};

	BOOL ret = [self addInProperty:kABPersonInstantMessageProperty value:(__bridge CFTypeRef)(lDict)];
	if (ret) {
		[_sipAddresses addObject:sip];
	}
	return ret;
}

- (BOOL)removeSipAddressAtIndex:(NSInteger)index {
	BOOL ret = [self removeInProperty:kABPersonInstantMessageProperty atIndex:index];
	if (ret) {
		[_sipAddresses removeObjectAtIndex:index];
	}
	return ret;
}
- (BOOL)addPhoneNumber:(NSString *)phone {
	BOOL ret = [self addInProperty:kABPersonPhoneProperty value:(__bridge CFTypeRef)(phone)];
	if (ret) {
		[_phoneNumbers addObject:phone];
	}
	return ret;
}

- (BOOL)removePhoneNumberAtIndex:(NSInteger)index {
	BOOL ret = [self removeInProperty:kABPersonPhoneProperty atIndex:index];
	if (ret) {
		[_phoneNumbers removeObjectAtIndex:index];
	}
	return ret;
}

- (BOOL)setEmail:(NSString *)email atIndex:(NSInteger)index {
	BOOL ret = [self replaceInProperty:kABPersonEmailProperty value:(__bridge CFTypeRef)(email) atIndex:index];
	if (ret) {
		_emails[index] = email;
	}
	return ret;
}

- (BOOL)addEmail:(NSString *)email {
	BOOL ret = [self addInProperty:kABPersonEmailProperty value:(__bridge CFTypeRef)(email)];
	if (ret) {
		[_emails addObject:email];
	}
	return ret;
}

- (BOOL)removeEmailAtIndex:(NSInteger)index {
	BOOL ret = [self removeInProperty:kABPersonEmailProperty atIndex:index];
	if (ret) {
		[_emails removeObjectAtIndex:index];
	}
	return ret;
}
@end
