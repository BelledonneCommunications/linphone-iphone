//
//  KIFTestCase+LinphoneExtras.m
//  linphone
//
//  Created by Guillaume on 17/01/2015.
//
//

#import "KIFTestCase+LinphoneExtras.h"

@implementation KIFTestCase (LinphoneExtras)

static bool invalidAccount = true;

- (void)setInvalidAccountSet:(BOOL)invalidAccountSet {
    invalidAccount = invalidAccountSet;
}

- (BOOL)invalidAccountSet {
    return invalidAccount;
}

- (NSString *)accountUsername {
    return @"testios";
}

- (void)switchToValidAccountIfNeeded {
    [UIView setAnimationsEnabled:false];

    if( invalidAccount ){
        
        [tester tapViewWithAccessibilityLabel:@"Settings"];
        [tester tapViewWithAccessibilityLabel:@"Run assistant"];
        [tester waitForTimeInterval:0.5];
        if( [tester tryFindingViewWithAccessibilityLabel:@"Launch Wizard" error:nil]){
            [tester tapViewWithAccessibilityLabel:@"Launch Wizard"];
            [tester waitForTimeInterval:0.5];
        }

        NSLog(@"Switching to a valid account");
        
        [tester tapViewWithAccessibilityLabel:@"Start"];
        [tester tapViewWithAccessibilityLabel:@"Sign in linphone.org account"];
        
        [tester enterText:@"testios" intoViewWithAccessibilityLabel:@"Username"];
        [tester enterText:@"testtest" intoViewWithAccessibilityLabel:@"Password"];
        
        [tester tapViewWithAccessibilityLabel:@"Sign in"];
        
        invalidAccount = false;
    }
}

@end
