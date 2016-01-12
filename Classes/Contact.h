//
//  Contact.h
//  linphone
//
//  Created by Gautier Pelloux-Prayer on 12/01/16.
//
//

#import <Foundation/Foundation.h>
#import <AddressBook/AddressBook.h>

@interface Contact : NSObject

@property(nonatomic, assign) NSString *firstName;
@property(nonatomic, assign) NSString *lastName;
@property(nonatomic, strong) NSMutableArray *sipAddresses;
@property(nonatomic, strong) NSMutableArray *emails;
@property(nonatomic, strong) NSMutableArray *phoneNumbers;

- (instancetype)initWithPerson:(ABRecordRef)person;

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
