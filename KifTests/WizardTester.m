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
    [UIView setAnimationsEnabled:false];
    
    [tester tapViewWithAccessibilityLabel:@"Settings"];
    [tester tapViewWithAccessibilityLabel:@"Run assistant"];
    [tester waitForTimeInterval:0.5];
    if( [tester tryFindingViewWithAccessibilityLabel:@"Launch Wizard" error:nil]){
        [tester tapViewWithAccessibilityLabel:@"Launch Wizard"];
        [tester waitForTimeInterval:0.5];
    }
}

- (void)afterEach{
    [tester tapViewWithAccessibilityLabel:@"Dialer"];
}

#pragma mark - State 

+ (void)switchToValidAccountWithTester:(KIFTestCase*)testCase {
}

#pragma mark - Utilities

- (void)_linphoneLogin:(NSString*)username withPW:(NSString*)pw {
    [tester tapViewWithAccessibilityLabel:@"Start"];
    [tester tapViewWithAccessibilityLabel:@"Sign in linphone.org account"];
    
    [tester enterText:username intoViewWithAccessibilityLabel:@"Username"];
    [tester enterText:pw intoViewWithAccessibilityLabel:@"Password"];
    
    [tester tapViewWithAccessibilityLabel:@"Sign in"];
}



- (void)_externalLoginWithProtocol:(NSString*)protocol {
    
    [self setInvalidAccountSet:true];
    [tester tapViewWithAccessibilityLabel:@"Start"];
    [tester tapViewWithAccessibilityLabel:@"Sign in SIP account"];
    
    [tester enterText:@"testios" intoViewWithAccessibilityLabel:@"Username"];
    [tester enterText:@"testtest" intoViewWithAccessibilityLabel:@"Password"];
    [tester enterText:@"sip.linphone.org" intoViewWithAccessibilityLabel:@"Domain"];
    [tester tapViewWithAccessibilityLabel:protocol];
    
    [tester tapViewWithAccessibilityLabel:@"Sign in"];
    
    // check the registration state
    UIView* regState = [tester waitForViewWithAccessibilityLabel:@"Registration state"];
    [tester waitForTimeInterval:1];
    [tester expectView:regState toContainText:@"Registered"];
}

#pragma mark - Tests

- (void)testLinphoneLogin {

    [self _linphoneLogin:@"testios" withPW:@"testtest"];
    
    // check the registration state
    UIView* regState = [tester waitForViewWithAccessibilityLabel:@"Registration state"];
    [tester waitForTimeInterval:1];
    [tester expectView:regState toContainText:@"Registered"];
    
}

- (void)testLinphoneLoginWithBadPassword {
    [self _linphoneLogin:@"testios" withPW:@"badPass"];
    
    [self setInvalidAccountSet:true];
    
    UIView* alertViewText = [tester waitForViewWithAccessibilityLabel:@"Registration failure" traits:UIAccessibilityTraitStaticText];
    if( alertViewText ){
        UIView *reason = [tester waitForViewWithAccessibilityLabel:@"Forbidden" traits:UIAccessibilityTraitStaticText];
        if( reason == nil ){ [tester fail];
        } else {
            [tester tapViewWithAccessibilityLabel:@"OK"]; // alertview
            [tester tapViewWithAccessibilityLabel:@"Cancel"]; // cancel wizard
        }
    } else {
        [tester fail];
    }
}

- (void)testExternalLoginWithUDP {
    [self _externalLoginWithProtocol:@"UDP"];
}

- (void)testExternalLoginWithTCP {
    [self _externalLoginWithProtocol:@"TCP"];
}

- (void)testExternalLoginWithTLS {
    [self _externalLoginWithProtocol:@"TLS"];
}





@end
