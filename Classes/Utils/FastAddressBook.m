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

@implementation FastAddressBook

static void sync_address_book (ABAddressBookRef addressBook, CFDictionaryRef info, void *context);

+ (NSString*)getContactDisplayName:(ABRecordRef)contact {
    NSString *retString = nil;
    if (contact) {
        CFStringRef lDisplayName = ABRecordCopyCompositeName(contact);
        if(lDisplayName != NULL) {
            retString = [NSString stringWithString:(NSString*)lDisplayName];
            CFRelease(lDisplayName);
        }
    }
    return retString;
}

+ (UIImage*)squareImageCrop:(UIImage*)image
{
	UIImage *ret = nil;

	// This calculates the crop area.

	float originalWidth  = image.size.width;
	float originalHeight = image.size.height;

	float edge = fminf(originalWidth, originalHeight);

	float posX = (originalWidth - edge) / 2.0f;
	float posY = (originalHeight - edge) / 2.0f;


	CGRect cropSquare = CGRectMake(posX, posY,
								   edge, edge);


	CGImageRef imageRef = CGImageCreateWithImageInRect([image CGImage], cropSquare);
	ret = [UIImage imageWithCGImage:imageRef
							  scale:image.scale
						orientation:image.imageOrientation];

	CGImageRelease(imageRef);

	return ret;
}

+ (UIImage*)getContactImage:(ABRecordRef)contact thumbnail:(BOOL)thumbnail {
    UIImage* retImage = nil;
    if (contact && ABPersonHasImageData(contact)) {
        CFDataRef imgData = ABPersonCopyImageDataWithFormat(contact, thumbnail?
                                                            kABPersonImageFormatThumbnail: kABPersonImageFormatOriginalSize);

        retImage = [UIImage imageWithData:(NSData *)imgData];
        if(imgData != NULL) {
            CFRelease(imgData);
        }

		if (retImage != nil && retImage.size.width != retImage.size.height) {
			[LinphoneLogger log:LinphoneLoggerLog format:@"Image is not square : cropping it."];
			return [self squareImageCrop:retImage];
		}
    }

    return retImage;
}

- (ABRecordRef)getContact:(NSString*)address {
    @synchronized (addressBookMap){
        return (ABRecordRef)[addressBookMap objectForKey:address];
    }
}

+ (BOOL)isSipURI:(NSString*)address {
    return [address hasPrefix:@"sip:"] || [address hasPrefix:@"sips:"];
}

+ (NSString*)appendCountryCodeIfPossible:(NSString*)number {
    if (![number hasPrefix:@"+"] && ![number hasPrefix:@"00"]) {
        NSString* lCountryCode = [[LinphoneManager instance] lpConfigStringForKey:@"countrycode_preference"];
        if (lCountryCode && [lCountryCode length]>0) {
            //append country code
            return [lCountryCode stringByAppendingString:number];
        }
    }
    return number;
}

+ (NSString*)normalizeSipURI:(NSString*)address {
    // replace all whitespaces (non-breakable, utf8 nbsp etc.) by the "classical" whitespace 
    address = [[address componentsSeparatedByCharactersInSet:[NSCharacterSet whitespaceCharacterSet]] componentsJoinedByString:@" "];
    NSString *normalizedSipAddress = nil;
	LinphoneAddress* linphoneAddress = linphone_core_interpret_url([LinphoneManager getLc], [address UTF8String]);
    if(linphoneAddress != NULL) {
        char *tmp = linphone_address_as_string_uri_only(linphoneAddress);
        if(tmp != NULL) {
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

+ (NSString*)normalizePhoneNumber:(NSString*)address {
    NSMutableString* lNormalizedAddress = [NSMutableString stringWithString:address];
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
    return !ABAddressBookGetAuthorizationStatus || ABAddressBookGetAuthorizationStatus() ==  kABAuthorizationStatusAuthorized;
}

- (FastAddressBook*)init {
    if ((self = [super init]) != nil) {
        addressBookMap  = [[NSMutableDictionary alloc] init];
        addressBook = nil;
        [self reload];
    }
    return self;
}

- (void)saveAddressBook {
	if( addressBook != nil ){
		NSError* err = nil;
		if( !ABAddressBookSave(addressBook, (CFErrorRef*)err) ){
			Linphone_warn(@"Couldn't save Address Book");
		}
	}
}

- (void)reload {
    if(addressBook != nil) {
        ABAddressBookUnregisterExternalChangeCallback(addressBook, sync_address_book, self);
        CFRelease(addressBook);
        addressBook = nil;
    }
    NSError *error = nil;

    addressBook = ABAddressBookCreateWithOptions(NULL, NULL);
    if(addressBook != NULL) {
        ABAddressBookRequestAccessWithCompletion(addressBook, ^(bool granted, CFErrorRef error) {
            ABAddressBookRegisterExternalChangeCallback (addressBook, sync_address_book, self);
            [self loadData];
        });
       } else {
        [LinphoneLogger log:LinphoneLoggerError format:@"Create AddressBook: Fail(%@)", [error localizedDescription]];
    }
}

- (void)loadData {
    ABAddressBookRevert(addressBook);
    @synchronized (addressBookMap) {
        [addressBookMap removeAllObjects];

        NSArray *lContacts = (NSArray *)ABAddressBookCopyArrayOfAllPeople(addressBook);
        for (id lPerson in lContacts) {
            // Phone
            {
                ABMultiValueRef lMap = ABRecordCopyValue((ABRecordRef)lPerson, kABPersonPhoneProperty);
                if(lMap) {
                    for (int i=0; i<ABMultiValueGetCount(lMap); i++) {
                        CFStringRef lValue = ABMultiValueCopyValueAtIndex(lMap, i);
                        CFStringRef lLabel = ABMultiValueCopyLabelAtIndex(lMap, i);
                        CFStringRef lLocalizedLabel = ABAddressBookCopyLocalizedLabel(lLabel);
                        NSString* lNormalizedKey = [FastAddressBook normalizePhoneNumber:(NSString*)lValue];
                        NSString* lNormalizedSipKey = [FastAddressBook normalizeSipURI:lNormalizedKey];
                        if (lNormalizedSipKey != NULL) lNormalizedKey = lNormalizedSipKey;
                        [addressBookMap setObject:lPerson forKey:lNormalizedKey];
                        CFRelease(lValue);
                        if (lLabel) CFRelease(lLabel);
                        if (lLocalizedLabel) CFRelease(lLocalizedLabel);                    }
                    CFRelease(lMap);
                }
            }

            // SIP
            {
                ABMultiValueRef lMap = ABRecordCopyValue((ABRecordRef)lPerson, kABPersonInstantMessageProperty);
                if(lMap) {
                    for(int i = 0; i < ABMultiValueGetCount(lMap); ++i) {
                        CFDictionaryRef lDict = ABMultiValueCopyValueAtIndex(lMap, i);
                        BOOL add = false;
                        if(CFDictionaryContainsKey(lDict, kABPersonInstantMessageServiceKey)) {
                            if(CFStringCompare((CFStringRef)[LinphoneManager instance].contactSipField, CFDictionaryGetValue(lDict, kABPersonInstantMessageServiceKey), kCFCompareCaseInsensitive) == 0) {
                                add = true;
                            }
                        } else {
                            add = true;
                        }
                        if(add) {
                            CFStringRef lValue = CFDictionaryGetValue(lDict, kABPersonInstantMessageUsernameKey);
                            NSString* lNormalizedKey = [FastAddressBook normalizeSipURI:(NSString*)lValue];
                            if(lNormalizedKey != NULL) {
                                [addressBookMap setObject:lPerson forKey:lNormalizedKey];
                            } else {
                                [addressBookMap setObject:lPerson forKey:(NSString*)lValue];
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

void sync_address_book (ABAddressBookRef addressBook, CFDictionaryRef info, void *context) {
    FastAddressBook* fastAddressBook = (FastAddressBook*)context;
    [fastAddressBook loadData];
}

- (void)dealloc {
    ABAddressBookUnregisterExternalChangeCallback(addressBook, sync_address_book, self);
    CFRelease(addressBook);
    [addressBookMap release];
    [super dealloc];
}

@end
