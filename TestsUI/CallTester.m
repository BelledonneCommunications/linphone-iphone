//
//  CallTester.m
//  linphone
//
//  Created by Gautier Pelloux-Prayer on 24/08/15.
//
//

#import "CallTester.h"
#include "LinphoneManager.h"

@implementation CallTester

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

- (void)afterEach {
	if ([tester tryFindingTappableViewWithAccessibilityLabel:@"Hangup" error:nil]) {
		[tester tapViewWithAccessibilityLabel:@"Hangup"];
	}
	[super afterEach];
}
#pragma mark - Tools

- (void)callURI:(NSString *)address {
	[tester enterText:address intoViewWithAccessibilityLabel:@"Enter an address"];
	[tester tapViewWithAccessibilityLabel:@"Call" traits:UIAccessibilityTraitButton];
}

#pragma mark - Tests

- (void)testCallMeBusy {
	[self callURI:[self me]];
	[tester waitForViewWithAccessibilityLabel:[NSString stringWithFormat:@"%@ is busy.", [self me]]];
	[tester tapViewWithAccessibilityLabel:@"Cancel" traits:UIAccessibilityTraitButton];
}

- (void)testCallUnregisteredUser {
	NSString *unregisteredUser = [self getUUID];
	[self callURI:unregisteredUser];
	[tester waitForViewWithAccessibilityLabel:[NSString stringWithFormat:@"%@ is not registered.", unregisteredUser]];
	[tester tapViewWithAccessibilityLabel:@"Cancel" traits:UIAccessibilityTraitButton];
}

- (void)testDialInvalidSIPURI {
	NSString *user = @"123 😀";
	[self callURI:user];
	[tester waitForViewWithAccessibilityLabel:[NSString
												  stringWithFormat:@"Cannot call %@.\nReason was: Call failed", user]];
	[tester tapViewWithAccessibilityLabel:@"Cancel" traits:UIAccessibilityTraitButton];
}

@end
