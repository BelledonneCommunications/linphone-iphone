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

#import <Foundation/Foundation.h>
#import <AddressBook/AddressBook.h>

#include "linphone/linphonecore.h"

@interface FastAddressBook :  NSObject {
	NSMutableDictionary *addressBookMap;
	ABAddressBookRef addressBook;
}

- (void)reload;
- (void)saveAddressBook;

+ (BOOL)isAuthorized;

// TOOLS

+ (ABRecordRef)getContact:(NSString *)address;
+ (ABRecordRef)getContactWithLinphoneAddress:(const LinphoneAddress *)address;

+ (NSString *)getContactDisplayName:(ABRecordRef)contact;
+ (UIImage *)getContactImage:(ABRecordRef)contact thumbnail:(BOOL)thumbnail;
+ (BOOL)contactHasValidSipDomain:(ABRecordRef)person;

+ (NSString *)displayNameForContact:(ABRecordRef)person;
+ (NSString *)displayNameForAddress:(const LinphoneAddress *)addr;

+ (BOOL)isSipURI:(NSString *)address;						  // should be removed
+ (NSString *)appendCountryCodeIfPossible:(NSString *)number; // should be removed
+ (NSString *)normalizePhoneNumber:(NSString *)number;		  // should be removed
+ (NSString *)normalizeSipURI:(NSString *)address;			  // should be removed

+ (NSString *)localizedLabel:(NSString *)label;

@end
