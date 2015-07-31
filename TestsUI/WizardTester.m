//
//  WizardTester.m
//  linphone
//
//  Created by Guillaume on 17/01/2015.
//
//

#import "WizardTester.h"
#import <KIF/KIF.h>

@implementation WizardTester

- (void)beforeEach {
	[super beforeEach];
	[UIView setAnimationsEnabled:false];

	[tester tapViewWithAccessibilityLabel:@"Settings"];
	[tester tapViewWithAccessibilityLabel:@"Run assistant"];
	[tester waitForTimeInterval:0.5];
	if ([tester tryFindingViewWithAccessibilityLabel:@"Launch Wizard" error:nil]) {
		[tester tapViewWithAccessibilityLabel:@"Launch Wizard"];
		[tester waitForTimeInterval:0.5];
	}
}

- (void)afterEach {
	[super afterEach];
	[tester tapViewWithAccessibilityLabel:@"Dialer"];
}

#pragma mark - State

+ (void)switchToValidAccountWithTester:(KIFTestCase *)testCase {
}

#pragma mark - Utilities

- (void)_linphoneLogin:(NSString *)username withPW:(NSString *)pw {
	[tester tapViewWithAccessibilityLabel:@"Start"];
	[tester tapViewWithAccessibilityLabel:@"Sign in linphone.org account"];

	[tester enterText:username intoViewWithAccessibilityLabel:@"Username"];
	[tester enterText:pw intoViewWithAccessibilityLabel:@"Password"];

	[tester tapViewWithAccessibilityLabel:@"Sign in"];
}

- (void)_externalLoginWithProtocol:(NSString *)protocol {

	[self setInvalidAccountSet:true];
	[tester tapViewWithAccessibilityLabel:@"Start"];
	[tester tapViewWithAccessibilityLabel:@"Sign in SIP account"];

	[tester enterText:[self me] intoViewWithAccessibilityLabel:@"Username"];
	[tester enterText:@"testtest" intoViewWithAccessibilityLabel:@"Password"];
	[tester enterText:[self accountDomain] intoViewWithAccessibilityLabel:@"Domain"];
	[tester tapViewWithAccessibilityLabel:protocol];

	[tester tapViewWithAccessibilityLabel:@"Sign in"];

	// check the registration state
	UIView *regState = [tester waitForViewWithAccessibilityLabel:@"Registration state"];
	[tester waitForTimeInterval:1];
	[tester expectView:regState toContainText:@"Registered"];
}

#pragma mark - Tests

- (void)testAccountCreation {
	NSString *username = [NSString stringWithFormat:@"%@-%.2f", [self getUUID], [[NSDate date] timeIntervalSince1970]];
	[tester tapViewWithAccessibilityLabel:@"Start"];
	[tester tapViewWithAccessibilityLabel:@"Create linphone.org account" traits:UIAccessibilityTraitButton];

	[tester enterText:username intoViewWithAccessibilityLabel:@"Username"];
	[tester enterText:username intoViewWithAccessibilityLabel:@"Password "];
	[tester enterText:username intoViewWithAccessibilityLabel:@"Password again"];
	[tester enterText:@"testios@.dev.null" intoViewWithAccessibilityLabel:@"Email"];

	[tester tapViewWithAccessibilityLabel:@"Register" traits:UIAccessibilityTraitButton];

	[tester waitForViewWithAccessibilityLabel:@"Check validation" traits:UIAccessibilityTraitButton];
	[tester tapViewWithAccessibilityLabel:@"Check validation"];

	[tester waitForViewWithAccessibilityLabel:@"Account validation issue"];
	[tester tapViewWithAccessibilityLabel:@"Continue"];
	[tester tapViewWithAccessibilityLabel:@"Cancel"];
}

- (void)testExternalLoginWithTCP {
	[self _externalLoginWithProtocol:@"TCP"];
}

- (void)testExternalLoginWithTLS {
	[self _externalLoginWithProtocol:@"TLS"];
}

- (void)testExternalLoginWithUDP {
	[self _externalLoginWithProtocol:@"UDP"];
}

- (void)testLinphoneLogin {

	[self _linphoneLogin:[self me] withPW:@"testtest"];

	// check the registration state
	UIView *regState = [tester waitForViewWithAccessibilityLabel:@"Registration state"];
	[tester waitForTimeInterval:1];
	[tester expectView:regState toContainText:@"Registered"];
}

- (void)testLinphoneLoginWithBadPassword {
	[self _linphoneLogin:[self me] withPW:@"badPass"];

	[self setInvalidAccountSet:true];

	UIView *alertViewText =
		[tester waitForViewWithAccessibilityLabel:@"Registration failure" traits:UIAccessibilityTraitStaticText];
	if (alertViewText) {
		UIView *reason = [tester waitForViewWithAccessibilityLabel:@"Forbidden" traits:UIAccessibilityTraitStaticText];
		if (reason == nil) {
			[tester fail];
		} else {
			[tester tapViewWithAccessibilityLabel:@"OK"];	 // alertview
			[tester tapViewWithAccessibilityLabel:@"Cancel"]; // cancel wizard
		}
	} else {
		[tester fail];
	}
}

@end
