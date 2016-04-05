//
//  CallTester.m
//  linphone
//
//  Created by Gautier Pelloux-Prayer on 04/04/16.
//
//

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
