//
//  LinphoneTestCase.h
//  linphone
//
//  Created by Guillaume BIENKOWSKI on 19/01/2015.
//
//

#import <KIF/KIF.h>

@interface LinphoneTestCase : KIFTestCase
@property BOOL invalidAccountSet;

- (void)switchToValidAccountIfNeeded;
- (NSString*)accountUsername;
- (NSString*)accountDomain;

- (NSString*)getUUID;

@end
