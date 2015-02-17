//
//  ChatTester.m
//  linphone
//
//  Created by Guillaume on 17/01/2015.
//
//

#import "ChatTester.h"
#include "LinphoneManager.h"

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

- (void)testRemoveAllChats {
    NSArray* uuids = [self getUUIDArrayOfSize:5];
    
    for( NSString* uuid in uuids ){
        [self startChatWith:uuid];
        [self sendMessage:@"Test"];
        [self goBackFromChat];
    }
    
    [tester tapViewWithAccessibilityLabel:@"Edit" traits:UIAccessibilityTraitButton];
    
    // we expect to be able to delete at least the amount of chatrooms we created
    for( int i =0; i< uuids.count; i++){
        [tester tapViewWithAccessibilityLabel:@"Delete" traits:UIAccessibilityTraitButton];
    }
    
    // then we try to delete all the rest of chatrooms
    while ( [tester tryFindingTappableViewWithAccessibilityLabel:@"Delete" traits:UIAccessibilityTraitButton error:nil] )
    {
        [tester tapViewWithAccessibilityLabel:@"Delete" traits:UIAccessibilityTraitButton];
        NSLog(@"Deleting an extra chat");
    }
    
    // check that the tableview is empty
    UITableView* tv = nil;
    NSError*    err = nil;
    if( [tester tryFindingAccessibilityElement:nil view:&tv withIdentifier:@"ChatRoom list" tappable:false error:&err] ){
        XCTAssert(tv != nil);
        XCTAssert([tv numberOfRowsInSection:0] == 0); // no more chat rooms
    } else {
        NSLog(@"Error: %@",err);
    }
    
    // test that there's no more chatrooms in the core
    XCTAssert(linphone_core_get_chat_rooms([LinphoneManager getLc]) == nil);
}


@end
