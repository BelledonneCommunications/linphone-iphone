//
//  WizardTester.m
//  linphone
//
//  Created by Guillaume on 17/01/2015.
//
//

#import <UIKit/UIKit.h>
#import <XCTest/XCTest.h>
#import <KIF/KIF.h>

@interface WizardTester : KIFTestCase

@end

@implementation WizardTester

- (void)beforeEach {
    [UIView setAnimationsEnabled:false];
    
    [tester tapViewWithAccessibilityLabel:@"Settings"];
    [tester tapViewWithAccessibilityLabel:@"Run assistant"];
    [tester waitForTimeInterval:2];
    if( [tester tryFindingViewWithAccessibilityLabel:@"Launch Wizard" error:nil]){
        [tester tapViewWithAccessibilityLabel:@"Launch Wizard"];
        [tester waitForTimeInterval:1];
    }
    
    
    // TODO: goto Wizard
}

- (void)afterEach{
    // TODO: goto Dialer
    [tester tapViewWithAccessibilityLabel:@"Dialer"];
}

- (void)testLinphoneLogin {

    [tester tapViewWithAccessibilityLabel:@"Start"];
    [tester tapViewWithAccessibilityLabel:@"Sign in linphone.org account"];
    
    [tester enterText:@"testios" intoViewWithAccessibilityLabel:@"Username"];
    [tester enterText:@"testtest" intoViewWithAccessibilityLabel:@"Password"];
    
    [tester tapViewWithAccessibilityLabel:@"Sign in"];
    
    // check the registration state
    UIView* regState = [tester waitForViewWithAccessibilityLabel:@"Registration state"];
    [tester waitForTimeInterval:1];
    [tester expectView:regState toContainText:@"Registered"];
    
}


@end
