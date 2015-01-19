//
//  ChatTester.m
//  linphone
//
//  Created by Guillaume on 17/01/2015.
//
//

#import "ChatTester.h"
#import "KIFTestCase+LinphoneExtras.h"

@implementation ChatTester


- (void)beforeAll {
    [super beforeAll];
    [self switchToValidAccountIfNeeded];
    
    [tester tapViewWithAccessibilityLabel:@"Chat"];
}

- (void)testSendMessageToMyself {
    [tester enterText:[self accountUsername] intoViewWithAccessibilityLabel:@"Enter a address"];
    [tester tapViewWithAccessibilityLabel:@"New Discussion"];
    
    [tester enterText:@"Hello" intoViewWithAccessibilityLabel:@"Message field"];
    
    [tester tapViewWithAccessibilityLabel:@"Send"];
    
    [tester waitForViewWithAccessibilityLabel:@"Outgoing message" value:@"Hello" traits:UIAccessibilityTraitStaticText];
    [tester waitForViewWithAccessibilityLabel:@"Incoming message" value:@"Hello" traits:UIAccessibilityTraitStaticText];
    
    [tester waitForViewWithAccessibilityLabel:@"Message status" value:@"delivered" traits:UIAccessibilityTraitImage];
    
    [tester tapViewWithAccessibilityLabel:@"Back"];    
}

@end
