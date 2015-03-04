//
//  LinphoneTestCase.h
//  linphone
//
//  Created by Guillaume BIENKOWSKI on 19/01/2015.
//
//

#import <KIF/KIF.h>

#define LOCALIZED(X) (X)

@interface LinphoneTestCase : KIFTestCase
@property BOOL invalidAccountSet;

- (void)switchToValidAccountIfNeeded;
- (NSString*)accountUsername;
- (NSString*)accountDomain;

- (NSString*)getUUID;
- (NSArray*)getUUIDArrayOfSize:(size_t)size;

@end
