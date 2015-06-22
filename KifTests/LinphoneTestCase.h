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
- (NSArray*)getUUIDArrayOfSize:(size_t)size;

@end

#define ASSERT_EQ(actual, expected)                                                                                    \
	{                                                                                                                  \
		if ((actual) != (expected)) {                                                                                  \
			[[UIApplication sharedApplication] writeScreenshotForLine:__LINE__                                         \
															   inFile:@__FILE__                                        \
														  description:nil                                              \
																error:NULL];                                           \
		}                                                                                                              \
		XCTAssertEqual(actual, expected);                                                                              \
	}
