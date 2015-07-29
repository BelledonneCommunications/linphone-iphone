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

#import "FastAddressBook.h"
#import "LinphoneManager.h"
#import "ContactsViewController.h"
#import "UILabel+Boldify.h"

@implementation FastAddressBook

static void sync_address_book(ABAddressBookRef addressBook, CFDictionaryRef info, void *context);

+ (NSString *)getContactDisplayName:(ABRecordRef)contact {
	NSString *retString = nil;
	if (contact) {
		retString = CFBridgingRelease(ABRecordCopyCompositeName(contact));
	}
	return retString;
}

+ (UIImage *)squareImageCrop:(UIImage *)image {
	UIImage *ret = nil;

	// This calculates the crop area.

	float originalWidth = image.size.width;
	float originalHeight = image.size.height;

	float edge = fminf(originalWidth, originalHeight);

	float posX = (originalWidth - edge) / 2.0f;
	float posY = (originalHeight - edge) / 2.0f;

	CGRect cropSquare = CGRectMake(posX, posY, edge, edge);

	CGImageRef imageRef = CGImageCreateWithImageInRect([image CGImage], cropSquare);
	ret = [UIImage imageWithCGImage:imageRef scale:image.scale orientation:image.imageOrientation];

	CGImageRelease(imageRef);

	return ret;
}

+ (UIImage *)getContactImage:(ABRecordRef)contact thumbnail:(BOOL)thumbnail {
	UIImage *retImage = nil;
	if (contact && ABPersonHasImageData(contact)) {
		NSData *imgData = CFBridgingRelease(ABPersonCopyImageDataWithFormat(
			contact, thumbnail ? kABPersonImageFormatThumbnail : kABPersonImageFormatOriginalSize));

		retImage = [UIImage imageWithData:imgData];
	}
	if (retImage == nil) {
		retImage = [UIImage imageNamed:@"avatar"];
	}
	if (retImage.size.width != retImage.size.height) {
		LOGW(@"Image is not square (%@): cropping it.", retImage.size);
		retImage = [self squareImageCrop:retImage];
	}
	return retImage;
}

+ (ABRecordRef)getContact:(NSString *)address {
	if (LinphoneManager.instance.fastAddressBook != nil) {
		@synchronized(LinphoneManager.instance.fastAddressBook->addressBookMap) {
			return (
				__bridge ABRecordRef)[LinphoneManager.instance.fastAddressBook->addressBookMap objectForKey:address];
		}
	}
	return nil;
}

+ (ABRecordRef)getContactWithLinphoneAddress:(const LinphoneAddress *)address {
	ABRecordRef contact = nil;
	if (address) {
		char *uri = linphone_address_as_string_uri_only(address);
		NSString *normalizedSipAddress = [FastAddressBook normalizeSipURI:[NSString stringWithUTF8String:uri]];
		contact = [FastAddressBook getContact:normalizedSipAddress];
		ms_free(uri);
	}
	return contact;
}

+ (BOOL)isSipURI:(NSString *)address {
	return [address hasPrefix:@"sip:"] || [address hasPrefix:@"sips:"];
}

+ (NSString *)appendCountryCodeIfPossible:(NSString *)number {
	if (![number hasPrefix:@"+"] && ![number hasPrefix:@"00"]) {
		NSString *lCountryCode = [[LinphoneManager instance] lpConfigStringForKey:@"countrycode_preference"];
		if (lCountryCode && [lCountryCode length] > 0) {
			// append country code
			return [lCountryCode stringByAppendingString:number];
		}
	}
	return number;
}

+ (NSString *)normalizeSipURI:(NSString *)address {
	// replace all whitespaces (non-breakable, utf8 nbsp etc.) by the "classical" whitespace
	address = [[address componentsSeparatedByCharactersInSet:[NSCharacterSet whitespaceCharacterSet]]
		componentsJoinedByString:@" "];
	NSString *normalizedSipAddress = nil;
	LinphoneAddress *linphoneAddress = linphone_core_interpret_url([LinphoneManager getLc], [address UTF8String]);
	if (linphoneAddress != NULL) {
		char *tmp = linphone_address_as_string_uri_only(linphoneAddress);
		if (tmp != NULL) {
			normalizedSipAddress = [NSString stringWithUTF8String:tmp];
			// remove transport, if any
			NSRange pos = [normalizedSipAddress rangeOfString:@";"];
			if (pos.location != NSNotFound) {
				normalizedSipAddress = [normalizedSipAddress substringToIndex:pos.location];
			}
			ms_free(tmp);
		}
		linphone_address_destroy(linphoneAddress);
	}
	return normalizedSipAddress;
}

+ (NSString *)normalizePhoneNumber:(NSString *)address {
	NSMutableString *lNormalizedAddress = [NSMutableString stringWithString:address];
	[lNormalizedAddress replaceOccurrencesOfString:@" "
										withString:@""
										   options:0
											 range:NSMakeRange(0, [lNormalizedAddress length])];
	[lNormalizedAddress replaceOccurrencesOfString:@"("
										withString:@""
										   options:0
											 range:NSMakeRange(0, [lNormalizedAddress length])];
	[lNormalizedAddress replaceOccurrencesOfString:@")"
										withString:@""
										   options:0
											 range:NSMakeRange(0, [lNormalizedAddress length])];
	[lNormalizedAddress replaceOccurrencesOfString:@"-"
										withString:@""
										   options:0
											 range:NSMakeRange(0, [lNormalizedAddress length])];
	return [FastAddressBook appendCountryCodeIfPossible:lNormalizedAddress];
}

+ (BOOL)isAuthorized {
	return !ABAddressBookGetAuthorizationStatus ||
		   ABAddressBookGetAuthorizationStatus() == kABAuthorizationStatusAuthorized;
}

- (FastAddressBook *)init {
	if ((self = [super init]) != nil) {
		addressBookMap = [NSMutableDictionary dictionary];
		addressBook = nil;
		[self reload];
	}
	return self;
}

- (void)saveAddressBook {
	if (addressBook != nil) {
		if (!ABAddressBookSave(addressBook, nil)) {
			LOGW(@"Couldn't save Address Book");
		}
	}
}

- (void)reload {
	CFErrorRef error;

	// create if it doesn't exist
	if (addressBook == nil) {
		addressBook = ABAddressBookCreateWithOptions(NULL, &error);
	}

	if (addressBook != nil) {
		__weak FastAddressBook *weakSelf = self;
		ABAddressBookRequestAccessWithCompletion(addressBook, ^(bool granted, CFErrorRef error) {
		  if (!granted) {
			  LOGE(@"Permission for address book acces was denied: %@", [(__bridge NSError *)error description]);
			  return;
		  }

		  ABAddressBookRegisterExternalChangeCallback(addressBook, sync_address_book, (__bridge void *)(weakSelf));
		  [weakSelf loadData];

		});
	} else {
		LOGE(@"Create AddressBook failed, reason: %@", [(__bridge NSError *)error localizedDescription]);
	}
}

- (void)loadData {
	ABAddressBookRevert(addressBook);
	@synchronized(addressBookMap) {
		[addressBookMap removeAllObjects];

		CFArrayRef lContacts = ABAddressBookCopyArrayOfAllPeople(addressBook);
		CFIndex count = CFArrayGetCount(lContacts);
		for (CFIndex idx = 0; idx < count; idx++) {
			ABRecordRef lPerson = CFArrayGetValueAtIndex(lContacts, idx);
			// Phone
			{
				ABMultiValueRef lMap = ABRecordCopyValue(lPerson, kABPersonPhoneProperty);
				if (lMap) {
					for (int i = 0; i < ABMultiValueGetCount(lMap); i++) {
						CFStringRef lValue = ABMultiValueCopyValueAtIndex(lMap, i);

						NSString *lNormalizedKey = [FastAddressBook normalizePhoneNumber:(__bridge NSString *)(lValue)];
						NSString *lNormalizedSipKey = [FastAddressBook normalizeSipURI:lNormalizedKey];
						if (lNormalizedSipKey != NULL)
							lNormalizedKey = lNormalizedSipKey;

						[addressBookMap setObject:(__bridge id)(lPerson)forKey:lNormalizedKey];

						CFRelease(lValue);
					}
					CFRelease(lMap);
				}
			}

			// SIP
			{
				ABMultiValueRef lMap = ABRecordCopyValue(lPerson, kABPersonInstantMessageProperty);
				if (lMap) {
					for (int i = 0; i < ABMultiValueGetCount(lMap); ++i) {
						CFDictionaryRef lDict = ABMultiValueCopyValueAtIndex(lMap, i);
						BOOL add = false;
						if (CFDictionaryContainsKey(lDict, kABPersonInstantMessageServiceKey)) {
							if (CFStringCompare((CFStringRef)[LinphoneManager instance].contactSipField,
												CFDictionaryGetValue(lDict, kABPersonInstantMessageServiceKey),
												kCFCompareCaseInsensitive) == 0) {
								add = true;
							}
						} else {
							add = true;
						}
						if (add) {
							NSString *lValue =
								(__bridge NSString *)CFDictionaryGetValue(lDict, kABPersonInstantMessageUsernameKey);
							NSString *lNormalizedKey = [FastAddressBook normalizeSipURI:lValue];
							if (lNormalizedKey != NULL) {
								[addressBookMap setObject:(__bridge id)(lPerson)forKey:lNormalizedKey];
							} else {
								[addressBookMap setObject:(__bridge id)(lPerson)forKey:lValue];
							}
						}
						CFRelease(lDict);
					}
					CFRelease(lMap);
				}
			}
		}
		CFRelease(lContacts);
	}
	[[NSNotificationCenter defaultCenter] postNotificationName:kLinphoneAddressBookUpdate object:self];
}

void sync_address_book(ABAddressBookRef addressBook, CFDictionaryRef info, void *context) {
	FastAddressBook *fastAddressBook = (__bridge FastAddressBook *)context;
	[fastAddressBook loadData];
}

- (void)dealloc {
	ABAddressBookUnregisterExternalChangeCallback(addressBook, sync_address_book, (__bridge void *)(self));
	CFRelease(addressBook);
}

#pragma mark - Tools

+ (NSString *)localizedLabel:(NSString *)label {
	if (label != nil) {
		return CFBridgingRelease(ABAddressBookCopyLocalizedLabel((__bridge CFStringRef)(label)));
	}
	return @"";
}

+ (BOOL)contactHasValidSipDomain:(ABRecordRef)person {
	if (person == nil)
		return NO;

	// Check if one of the contact' sip URI matches the expected SIP filter
	ABMultiValueRef personSipAddresses = ABRecordCopyValue(person, kABPersonInstantMessageProperty);
	BOOL match = false;
	NSString *domain = [ContactSelection getSipFilter];

	for (int i = 0; i < ABMultiValueGetCount(personSipAddresses) && !match; ++i) {
		CFDictionaryRef lDict = ABMultiValueCopyValueAtIndex(personSipAddresses, i);
		if (CFDictionaryContainsKey(lDict, kABPersonInstantMessageServiceKey)) {
			CFStringRef serviceKey = CFDictionaryGetValue(lDict, kABPersonInstantMessageServiceKey);

			if (CFStringCompare((CFStringRef)[LinphoneManager instance].contactSipField, serviceKey,
								kCFCompareCaseInsensitive) == 0) {
				match = true;
			}
		} else if (domain != nil) {
			// check domain
			LinphoneAddress *address = linphone_address_new(
				[(NSString *)CFDictionaryGetValue(lDict, kABPersonInstantMessageUsernameKey) UTF8String]);

			if (address) {
				const char *dom = linphone_address_get_domain(address);
				if (dom != NULL) {
					NSString *contactDomain =
						[NSString stringWithCString:dom encoding:[NSString defaultCStringEncoding]];

					match = (([domain compare:@"*" options:NSCaseInsensitiveSearch] == NSOrderedSame) ||
							 ([domain compare:contactDomain options:NSCaseInsensitiveSearch] == NSOrderedSame));
				}
				linphone_address_destroy(address);
			}
		}
		CFRelease(lDict);
	}
	CFRelease(personSipAddresses);
	return match;
}

+ (void)setDisplayNameLabel:(UILabel *)label forContact:(ABRecordRef)contact {
	label.text = [FastAddressBook displayNameForContact:contact];
	NSString *lLastName = CFBridgingRelease(ABRecordCopyValue(contact, kABPersonLastNameProperty));
	NSString *lLocalizedLastName = [FastAddressBook localizedLabel:lLastName];
	[label boldSubstring:lLocalizedLastName];
}

+ (NSString *)displayNameForContact:(ABRecordRef)contact {
	NSString *ret = nil;
	if (contact != nil) {
		NSString *lFirstName = CFBridgingRelease(ABRecordCopyValue(contact, kABPersonFirstNameProperty));
		NSString *lLocalizedFirstName = [FastAddressBook localizedLabel:lFirstName];

		NSString *lLastName = CFBridgingRelease(ABRecordCopyValue(contact, kABPersonLastNameProperty));
		NSString *lLocalizedLastName = [FastAddressBook localizedLabel:lLastName];

		NSString *lOrganization = CFBridgingRelease(ABRecordCopyValue(contact, kABPersonOrganizationProperty));
		NSString *lLocalizedOrganization = [FastAddressBook localizedLabel:lOrganization];

		if (lLocalizedFirstName == nil && lLocalizedLastName == nil) {
			ret = (NSString *)lLocalizedOrganization;
		} else {
			ret = [NSString stringWithFormat:@"%@ %@", lLocalizedFirstName, lLocalizedLastName];
		}
	}
	return ret;
}

+ (NSString *)displayNameForAddress:(const LinphoneAddress *)addr {
	NSString *ret = NSLocalizedString(@"Unknown", nil);
	ABRecordRef contact = [FastAddressBook getContactWithLinphoneAddress:addr];
	if (contact) {
		ret = [FastAddressBook displayNameForContact:contact];
	} else {
		const char *lDisplayName = linphone_address_get_display_name(addr);
		const char *lUserName = linphone_address_get_username(addr);
		if (lDisplayName) {
			ret = [NSString stringWithUTF8String:lDisplayName];
		} else if (lUserName) {
			ret = [NSString stringWithUTF8String:lUserName];
		}
	}
	return ret;
}

+ (void)setDisplayNameLabel:(UILabel *)label forAddress:(const LinphoneAddress *)addr {
	label.text = [FastAddressBook displayNameForAddress:addr];
}

@end
