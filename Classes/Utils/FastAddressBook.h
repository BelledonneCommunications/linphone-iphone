/* FastAddressBook.h
 *
 * Copyright (C) 2011  Belledonne Comunications, Grenoble, France
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed |in the hope that it will be useful,
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
#include "Contact.h"

@interface FastAddressBook : NSObject

@property(readonly, nonatomic) NSMutableDictionary *addressBookMap;
@property BOOL needToUpdate;

- (void) fetchContactsInBackGroundThread;
- (BOOL)deleteContact:(Contact *)contact;
- (BOOL)deleteCNContact:(CNContact *)CNContact;
- (BOOL)deleteAllContacts;
- (BOOL)saveContact:(Contact *)contact;
- (BOOL)saveCNContact:(CNContact *)CNContact contact:(Contact *)Contact;

+ (BOOL)isAuthorized;

// TOOLS

+ (Contact *)getContactWithAddress:(const LinphoneAddress *)address;
- (CNContact *)getCNContactFromContact:(Contact *)acontact;

+ (UIImage *)imageForContact:(Contact *)contact;
+ (UIImage *)imageForAddress:(const LinphoneAddress *)addr;
+ (UIImage *)imageForSecurityLevel:(LinphoneChatRoomSecurityLevel)level;

+ (BOOL)contactHasValidSipDomain:(Contact *)person;
+ (BOOL)isSipURIValid:(NSString*)addr;

+ (NSString *)displayNameForContact:(Contact *)person;
+ (NSString *)displayNameForAddress:(const LinphoneAddress *)addr;

+ (BOOL)isSipURI:(NSString *)address;
+ (BOOL)isSipAddress:(CNLabeledValue<CNInstantMessageAddress *> *)sipAddr;
+ (NSString *)normalizeSipURI:(NSString *)address;

+ (NSString *)localizedLabel:(NSString *)label;
- (void)registerAddrsFor:(Contact *)contact;

@end
