//
//  Contact.h
//  linphone
//
//  Created by Gautier Pelloux-Prayer on 12/01/16.
//
//

#import <Foundation/Foundation.h>
#import <AddressBook/AddressBook.h>

#include <linphone/linphonecore.h>

@interface Contact : NSObject

@property(nonatomic, readonly) ABRecordRef person;
@property(nonatomic, readonly) LinphoneFriend *friend;

@property(nonatomic, retain) NSString *firstName;
@property(nonatomic, retain) NSString *lastName;
@property(nonatomic, strong) NSMutableArray *sipAddresses;
@property(nonatomic, strong) NSMutableArray *emails;
@property(nonatomic, strong) NSMutableArray *phoneNumbers;

- (void)setAvatar:(UIImage *)avatar;
- (UIImage *)avatar:(BOOL)thumbnail;
- (NSString *)displayName;

- (instancetype)initWithPerson:(ABRecordRef)person;
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
