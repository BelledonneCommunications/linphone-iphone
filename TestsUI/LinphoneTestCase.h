//
//  LinphoneTestCase.h
//  linphone
//
//  Created by Guillaume BIENKOWSKI on 19/01/2015.
//
//

#import <KIF/KIF.h>
#import <KIF/UIApplication-KIFAdditions.h>

#import "Utils.h"
#import "Log.h"

@interface LinphoneTestCase : KIFTestCase
@property BOOL invalidAccountSet;

- (void)switchToValidAccountIfNeeded;
- (NSString *)me;
- (NSString *)accountDomain;

- (NSString *)getUUID;
- (NSArray *)getUUIDArrayOfSize:(size_t)size;

- (UITableView *)findTableView:(NSString *)table;

- (void)waitForRegistration;

- (void)removeAllRooms;
- (void)createContact:(NSString *)firstName
			 lastName:(NSString *)lastName
		  phoneNumber:(NSString *)phone
		   SIPAddress:(NSString *)sip;

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
