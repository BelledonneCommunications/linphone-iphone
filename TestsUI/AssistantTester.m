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

#import "AssistantTester.h"
#import <KIF/KIF.h>
#import "LinphoneManager.h"

@implementation AssistantTester

- (void)beforeEach {
	[super beforeEach];
	[UIView setAnimationsEnabled:false];

	[tester tapViewWithAccessibilityLabel:@"Side menu button"];
	[tester tapViewWithAccessibilityLabel:@"Assistant"];
}

- (void)afterEach {
	[super afterEach];
	[LinphoneManager.instance removeAllAccounts];
	if ([tester tryFindingTappableViewWithAccessibilityLabel:@"Cancel" error:nil]) {
		[tester tapViewWithAccessibilityLabel:@"Cancel"];
	}
	[tester tapViewWithAccessibilityLabel:@"Dialer"];
}

#pragma mark - Utilities

- (void)_linphoneLogin:(NSString *)username withPW:(NSString *)pw {
	[tester tapViewWithAccessibilityLabel:@"Use Linphone account"];

	[tester enterText:username intoViewWithAccessibilityLabel:@"Username"];
	[tester enterText:pw intoViewWithAccessibilityLabel:@"Password"];

	[tester tapViewWithAccessibilityLabel:@"Login"];
}

- (void)_externalLoginWithProtocol:(NSString *)protocol {
	[tester tapViewWithAccessibilityLabel:@"Use SIP account"];

	[tester enterText:[self me] intoViewWithAccessibilityLabel:@"Username"];
	[tester enterText:[self me] intoViewWithAccessibilityLabel:@"Password"];
	[tester clearTextFromViewWithAccessibilityLabel:@"Domain"];
	[tester enterText:[self accountDomain] intoViewWithAccessibilityLabel:@"Domain"];
	[tester tapViewWithAccessibilityLabel:protocol];

	[tester tapViewWithAccessibilityLabel:@"Login"];
}

#pragma mark - Tests

- (void)testAccountCreation {
	NSString *username = [NSString stringWithFormat:@"%@-%.2f", [self getUUID], [[NSDate date] timeIntervalSince1970]];
	[tester tapViewWithAccessibilityLabel:@"Create account" traits:UIAccessibilityTraitButton];

	[tester enterText:username intoViewWithAccessibilityLabel:@"Username"];
	[tester enterText:username intoViewWithAccessibilityLabel:@"Password "];
	[tester enterText:username intoViewWithAccessibilityLabel:@"Password confirmation"];
	[tester enterText:@"testios@.dev.null" intoViewWithAccessibilityLabel:@"Email"];

	[tester tapViewWithAccessibilityLabel:@"Create account" traits:UIAccessibilityTraitButton];

	[tester waitForViewWithAccessibilityLabel:@"Finish configuration" traits:UIAccessibilityTraitButton];
	[tester tapViewWithAccessibilityLabel:@"Finish configuration"];

	[tester waitForViewWithAccessibilityLabel:@"Account validation failed"];
	[tester tapViewWithAccessibilityLabel:@"Skip verification"];
}

- (void)testExternalLoginWithTCP {
	[self _externalLoginWithProtocol:@"TCP"];
	[self waitForRegistration];
}

- (void)testExternalLoginWithTLS {
	[self _externalLoginWithProtocol:@"TLS"];
	[self waitForRegistration];
}

- (void)testExternalLoginWithUDP {
	[self _externalLoginWithProtocol:@"UDP"];
	[self waitForRegistration];
}

- (void)testLinphoneLogin {
	[self _linphoneLogin:@"testios" withPW:@"testtest"];
	[self waitForRegistration];
}

- (void)testLinphoneLoginWithBadPassword {
	[self _linphoneLogin:@"testios" withPW:@"badPass"];

	[self setInvalidAccountSet:true];

	UIView *alertViewText =
		[tester waitForViewWithAccessibilityLabel:@"Connection failure" traits:UIAccessibilityTraitStaticText];
	if (alertViewText) {
		UIView *reason = [tester waitForViewWithAccessibilityLabel:@"Bad credentials, check your account settings"
															traits:UIAccessibilityTraitStaticText];
		if (reason == nil) {
			[tester fail];
		} else {
			[tester tapViewWithAccessibilityLabel:@"Continue"];
		}
	} else {
		[tester fail];
	}
}

- (void)testRemoteProvisioning {
	[tester tapViewWithAccessibilityLabel:@"Fetch remote configuration"];
	[tester enterText:@"smtp.linphone.org/testios_xml" intoViewWithAccessibilityLabel:@"URL"];
	[tester tapViewWithAccessibilityLabel:@"Fetch and apply"];
	[self waitForRegistration];
}
@end
