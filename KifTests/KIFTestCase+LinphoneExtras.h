//
//  KIFTestCase+LinphoneExtras.h
//  linphone
//
//  Created by Guillaume on 17/01/2015.
//
//

#import <KIF/KIF.h>

@interface KIFTestCase (LinphoneExtras)

@property BOOL invalidAccountSet;

- (void)switchToValidAccountIfNeeded;
- (NSString*)accountUsername;

@end
