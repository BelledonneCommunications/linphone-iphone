/*
 * Copyright (c) 2010-2020 Belledonne Communications SARL.
 *
 * This file is part of linphone-iphone 
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 */

#import <AddressBook/AddressBook.h>
#import <Contacts/Contacts.h>
#import <Foundation/Foundation.h>
#include <linphone/linphonecore.h>

@interface Contact : NSObject

//@property(nonatomic, readonly) ABRecordRef person;
@property(nonatomic, readonly) CNContact *person;
@property(nonatomic, readonly) LinphoneFriend *friend;

@property(nonatomic, retain) NSString *identifier;
@property(nonatomic, retain) NSString *firstName;
@property(nonatomic, retain) NSString *lastName;
@property(nonatomic, retain) NSString *displayName;
@property(nonatomic, strong) NSMutableArray *sipAddresses;
@property(nonatomic, strong) NSMutableArray *emails;
@property(nonatomic, strong) NSMutableArray *phones;
@property BOOL added;

- (void)setAvatar:(UIImage *)avatar;
- (UIImage *)avatar;
- (NSString *)displayName;

- (instancetype)initWithCNContact:(CNContact *)contact;
- (instancetype)initWithFriend:(LinphoneFriend *) friend;
- (void)reloadFriend;
- (void)clearFriend;

- (BOOL)setSipAddress:(NSString *)sip atIndex:(NSInteger)index;
- (BOOL)setEmail:(NSString *)email atIndex:(NSInteger)index;
- (BOOL)setPhoneNumber:(NSString *)phone atIndex:(NSInteger)index;

- (BOOL)addSipAddress:(NSString *)sip;
- (BOOL)addEmail:(NSString *)email;
- (BOOL)addPhoneNumber:(NSString *)phone;

- (BOOL)removeSipAddressAtIndex:(NSInteger)index;
- (BOOL)removePhoneNumberAtIndex:(NSInteger)index;
- (BOOL)removeEmailAtIndex:(NSInteger)index;
@end
