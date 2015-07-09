//
//  LinphoneTestCase.h
//  linphone
//
//  Created by Guillaume BIENKOWSKI on 19/01/2015.
//
//

#import <KIF/KIF.h>
#import <KIF/UIApplication-KIFAdditions.h>

@interface LinphoneTestCase : KIFTestCase
@property BOOL invalidAccountSet;

- (void)switchToValidAccountIfNeeded;
- (NSString *)me;
- (NSString *)accountDomain;

- (NSString *)getUUID;
- (NSArray *)getUUIDArrayOfSize:(size_t)size;

- (UITableView *)findTableView:(NSString *)table;

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
