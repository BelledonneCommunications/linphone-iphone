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
    
    [tester tapViewWithAccessibilityLabel:LOCALIZED(@"Chat")];
}

#pragma mark - tools

- (void)goBackFromChat {
    [tester tapViewWithAccessibilityLabel:LOCALIZED(@"Back")];
}

- (void)startChatWith:(NSString*)user {
    [tester enterText:user intoViewWithAccessibilityLabel:LOCALIZED(@"Enter a address")];
    [tester tapViewWithAccessibilityLabel:LOCALIZED(@"New Discussion")];
}

- (void)sendMessage:(NSString*)message {
    [tester enterText:message intoViewWithAccessibilityLabel:LOCALIZED(@"Message field")];
    [tester tapViewWithAccessibilityLabel:LOCALIZED(@"Send")];
}


#pragma mark - tests

- (void)testSendMessageToMyself {
    [self startChatWith:[self accountUsername]];
    
    [self sendMessage:@"Hello"];
    
    [tester waitForViewWithAccessibilityLabel:LOCALIZED(@"Outgoing message") value:@"Hello" traits:UIAccessibilityTraitStaticText];
    [tester waitForViewWithAccessibilityLabel:LOCALIZED(@"Incoming message") value:@"Hello" traits:UIAccessibilityTraitStaticText];
    
    [tester waitForViewWithAccessibilityLabel:LOCALIZED(@"Message status") value:@"delivered" traits:UIAccessibilityTraitImage];
    
    [self goBackFromChat];
}

- (void)testInvalidSPAddress {
    
    [self startChatWith:@"sip://toto"];
    
    [tester waitForViewWithAccessibilityLabel:LOCALIZED(@"Invalid address") traits:UIAccessibilityTraitStaticText];
    [tester tapViewWithAccessibilityLabel:LOCALIZED(@"Cancel")];
}

-(void)testSendToSIPAddress{
    NSString* sipAddr = [NSString stringWithFormat:@"sip:%@@%@", [self accountUsername], [self accountDomain]];
    
    [self startChatWith:sipAddr];
    
    [tester waitForViewWithAccessibilityLabel:LOCALIZED(@"Contact name") value:@"testios" traits:0];
    
    [self goBackFromChat];
}

- (void)testChatMessageRemoval {
    
    NSString* user = [self getUUID];
    
    [self startChatWith:user];
    [self sendMessage:user];
    
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
    
    [tester tapViewWithAccessibilityLabel:@"Edit" traits:UIAccessibilityTraitButton]; // same as the first but it is "OK" on screen
    
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
