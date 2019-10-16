/*
 * Copyright (c) 2010-2019 Belledonne Communications SARL.
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
