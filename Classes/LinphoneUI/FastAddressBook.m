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

@synthesize addressBook;

 -(Contact*) getMatchingRecord:(NSString*) number {
     @synchronized (mAddressBookMap){
      return (Contact*) [mAddressBookMap objectForKey:number];   
     } 
}

+(NSString*) appendCountryCodeIfPossible:(NSString*) number {
    if (![number hasPrefix:@"+"] && ![number hasPrefix:@"00"]) {
        NSString* lCountryCode = [[LinphoneManager instance].settingsStore objectForKey:@"countrycode_preference"];
        if (lCountryCode && [lCountryCode length]>0) {
            //append country code
            return [lCountryCode stringByAppendingString:number];
        }
    }
    return number;
}
+(NSString*) normalizePhoneNumber:(NSString*) number  {
    NSString* lNormalizedNumber =  [(NSString*)number stringByReplacingOccurrencesOfString:@" " withString:@""];
    lNormalizedNumber = [lNormalizedNumber stringByReplacingOccurrencesOfString:@"(" withString:@""];
    lNormalizedNumber = [lNormalizedNumber stringByReplacingOccurrencesOfString:@")" withString:@""];
    lNormalizedNumber = [lNormalizedNumber stringByReplacingOccurrencesOfString:@"-" withString:@""];
    lNormalizedNumber = [FastAddressBook appendCountryCodeIfPossible:lNormalizedNumber];
    return lNormalizedNumber;
}
void sync_address_book (ABAddressBookRef addressBook, CFDictionaryRef info, void *context) {
    NSMutableDictionary* lAddressBookMap = (NSMutableDictionary*)context;
    @synchronized (lAddressBookMap) {
        [lAddressBookMap removeAllObjects];
        
        NSArray *lContacts = (NSArray *)ABAddressBookCopyArrayOfAllPeople(addressBook);
        for (id lPerson in lContacts) {
            ABMutableMultiValueRef lPhoneNumbers = ABRecordCopyValue((ABRecordRef)lPerson, kABPersonPhoneProperty);
            for ( int i=0; i<ABMultiValueGetCount(lPhoneNumbers); i++) {
                CFStringRef lValue = ABMultiValueCopyValueAtIndex(lPhoneNumbers,i);
                CFStringRef lLabel = ABMultiValueCopyLabelAtIndex(lPhoneNumbers,i);
                CFStringRef lLocalizedLabel = ABAddressBookCopyLocalizedLabel(lLabel);
                NSString* lNormalizedKey = [FastAddressBook normalizePhoneNumber:(NSString*)lValue];
                Contact* lContact = [[Contact alloc] initWithRecord:lPerson ofType:(NSString *)lLocalizedLabel];
                [lAddressBookMap setObject:lContact forKey:lNormalizedKey];
                CFRelease(lValue);
                [lContact release];
                if (lLabel) CFRelease(lLabel);
                if (lLocalizedLabel) CFRelease(lLocalizedLabel);
            }
            CFRelease(lPhoneNumbers);
        }
        CFRelease(lContacts);
    }
}
-(FastAddressBook*) init {
    if ((self = [super init])) {
        mAddressBookMap  = [[NSMutableDictionary alloc] init];
        addressBook = ABAddressBookCreate();
        ABAddressBookRegisterExternalChangeCallback (addressBook,sync_address_book,mAddressBookMap);
        sync_address_book(addressBook,nil,mAddressBookMap);
    }
    return self;
}

@end
@implementation Contact 
@synthesize record;
@synthesize numberType;

-(id) initWithRecord:(ABRecordRef) aRecord ofType:(NSString*) type {
     if ((self = [super init])) {
         record=CFRetain(aRecord);
         numberType= [type?type:@"unknown" retain];
     }
    return self;
}
- (void)dealloc {
    CFRelease(record);
    [numberType release];
    [super dealloc];
}
@end
