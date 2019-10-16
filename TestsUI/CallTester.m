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

#import "CallTester.h"

@implementation CallTester

#pragma mark - Setup

- (void)beforeAll {
	[super beforeAll];
	[self switchToValidAccountIfNeeded];
}

- (void)beforeEach {
	[super beforeEach];
	if ([tester tryFindingTappableViewWithAccessibilityLabel:@"Back" error:nil]) {
		[tester tapViewWithAccessibilityLabel:@"Back"];
	}
	[tester tapViewWithAccessibilityLabel:@"Dialer"];
}

#pragma mark - Tests

- (void)testCallPhoneNumberEscaped {
	LinphoneProxyConfig *cfg = linphone_core_get_default_proxy_config(LC);
	linphone_proxy_config_set_dial_escape_plus(cfg, TRUE);
	NSString *num = @"+3312345-6789";
	[tester enterText:num intoViewWithAccessibilityLabel:@"Enter an address"];
	[tester tapViewWithAccessibilityLabel:@"Call" traits:UIAccessibilityTraitButton];
	[tester waitForViewWithAccessibilityLabel:@"0033123456789 is not registered."];
	[tester tapViewWithAccessibilityLabel:@"Cancel"];
	linphone_proxy_config_set_dial_escape_plus(cfg, FALSE);
}

- (void)testCallSIPNotEscaped {
	LinphoneProxyConfig *cfg = linphone_core_get_default_proxy_config(LC);
	linphone_proxy_config_set_dial_escape_plus(cfg, FALSE);
	NSString *num = @"+3312345-6789";
	[tester enterText:num intoViewWithAccessibilityLabel:@"Enter an address"];
	[tester tapViewWithAccessibilityLabel:@"Call" traits:UIAccessibilityTraitButton];
	[tester waitForViewWithAccessibilityLabel:@"+3312345-6789 is not registered."];
	[tester tapViewWithAccessibilityLabel:@"Cancel"];
}

@end
