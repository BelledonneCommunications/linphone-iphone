//
//  Contact.h
//  linphone
//
//  Created by Gautier Pelloux-Prayer on 12/01/16.
//
//

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
