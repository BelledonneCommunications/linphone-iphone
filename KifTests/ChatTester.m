//
//  ChatTester.m
//  linphone
//
//  Created by Guillaume on 17/01/2015.
//
//

#import "ChatTester.h"

@implementation ChatTester


#pragma mark - setup

- (void)beforeAll {
    [super beforeAll];
    [self switchToValidAccountIfNeeded];
    
    [tester tapViewWithAccessibilityLabel:@"Chat"];
}

#pragma mark - tools

- (void)goBackFromChat {
    [tester tapViewWithAccessibilityLabel:@"Back"];
}

- (void)startChatWith:(NSString*)user {
    [tester enterText:user intoViewWithAccessibilityLabel:@"Enter a address"];
    [tester tapViewWithAccessibilityLabel:@"New Discussion"];
}

- (void)sendMessage:(NSString*)message {
    [tester enterText:message intoViewWithAccessibilityLabel:@"Message field"];
    [tester tapViewWithAccessibilityLabel:@"Send"];
}


#pragma mark - tests

- (void)testSendMessageToMyself {
    [self startChatWith:[self accountUsername]];
    
    [self sendMessage:@"Hello"];
    
    [tester waitForViewWithAccessibilityLabel:@"Outgoing message" value:@"Hello" traits:UIAccessibilityTraitStaticText];
    [tester waitForViewWithAccessibilityLabel:@"Incoming message" value:@"Hello" traits:UIAccessibilityTraitStaticText];
    
    [tester waitForViewWithAccessibilityLabel:@"Message status" value:@"delivered" traits:UIAccessibilityTraitImage];
    
    [self goBackFromChat];
}

- (void)testInvalidSPAddress {
    
    [self startChatWith:@"sip://toto"];
    
    [tester waitForViewWithAccessibilityLabel:@"Invalid address" traits:UIAccessibilityTraitStaticText];
    [tester tapViewWithAccessibilityLabel:@"Cancel"];
}

-(void)testSendToSIPAddress{
    NSString* sipAddr = [NSString stringWithFormat:@"sip:%@@%@", [self accountUsername], [self accountDomain]];
    
    [self startChatWith:sipAddr];
    
    [tester waitForViewWithAccessibilityLabel:@"Contact name" value:@"testios" traits:0];
    
    [self goBackFromChat];
}

- (void)testChatMessageRemoval {
    
    NSString* user = [self getUUID];
    
    [self startChatWith:user];
    [self sendMessage:@"Hello Bro"];
    
    [tester tapViewWithAccessibilityLabel:@"Edit" traits:UIAccessibilityTraitButton];
    
    [tester tapViewWithAccessibilityLabel:@"Delete message"];
    
    [tester tapViewWithAccessibilityLabel:@"Edit" traits:UIAccessibilityTraitButton];
    

   
    // check that the tableview is empty
    UITableView* tv = nil;
    NSError*    err = nil;
    if( [tester tryFindingAccessibilityElement:nil view:&tv withIdentifier:@"Chat list" tappable:false error:&err] ){
        XCTAssert(tv != nil);
        XCTAssert([tv numberOfRowsInSection:0] == 0); // no more messages
    } else {
        NSLog(@"Error: %@",err);
    }
    
    [self goBackFromChat];
}


@end
