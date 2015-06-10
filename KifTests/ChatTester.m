//
//  ChatTester.m
//  linphone
//
//  Created by Guillaume on 17/01/2015.
//
//

#import "ChatTester.h"
#include "LinphoneManager.h"
#import "UIChatRoomCell.h"

@implementation ChatTester


#pragma mark - setup

- (void)beforeAll {
    [super beforeAll];
    [self switchToValidAccountIfNeeded];
}

- (void)beforeEach {
	[super beforeEach];
	if ([tester tryFindingTappableViewWithAccessibilityLabel:LOCALIZED(@"Back") error:nil]) {
		[self goBackFromChat];
	}
	[tester tapViewWithAccessibilityLabel:LOCALIZED(@"Chat")];
	[self removeAllRooms];
}

- (void)afterAll {
	// at the end of tests, go back to chat rooms to display main bar
	if ([tester tryFindingTappableViewWithAccessibilityLabel:LOCALIZED(@"Back") error:nil]) {
		[self goBackFromChat];
	}
}

#pragma mark - tools

- (void)removeAllRooms {
	[tester tapViewWithAccessibilityLabel:@"Edit" traits:UIAccessibilityTraitButton];
	while (
		[tester tryFindingTappableViewWithAccessibilityLabel:@"Delete" traits:UIAccessibilityTraitButton error:nil]) {
		[tester tapViewWithAccessibilityLabel:@"Delete" traits:UIAccessibilityTraitButton];
	}
	[tester tapViewWithAccessibilityLabel:@"Edit"
								   traits:UIAccessibilityTraitButton]; // same as the first but it is "OK" on screen
}

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

	[tester tapViewWithAccessibilityLabel:@"Edit"
								   traits:UIAccessibilityTraitButton]; // same as the first but it is "OK" on screen

	// check that the tableview is empty
	UITableView *tv = [self findTableView:@"ChatRoom list"];
	XCTAssert([tv numberOfRowsInSection:0] == 0);

	// test that there's no more chatrooms in the core
	XCTAssert(linphone_core_get_chat_rooms([LinphoneManager getLc]) == nil);
}

- (UITableView *)findTableView:(NSString *)table {
	UITableView *tv = nil;
	NSError *err = nil;
	if ([tester tryFindingAccessibilityElement:nil view:&tv withIdentifier:table tappable:false error:&err]) {
		XCTAssertNotNil(tv);
	} else {
		XCTFail(@"Error: %@", err);
	}
	return tv;
}

- (void)uploadImage {
	NSString *user = @"testios";

	[self startChatWith:user];

	[tester tapViewWithAccessibilityLabel:LOCALIZED(@"Send picture")];
	[tester tapViewWithAccessibilityLabel:LOCALIZED(@"Photo library")];
	// if popup "Linphone would access your photo" pops up, click OK.
	if ([ALAssetsLibrary authorizationStatus] == ALAuthorizationStatusNotDetermined) {
		[tester acknowledgeSystemAlert];
	}

	[tester choosePhotoInAlbum:@"Camera Roll" atRow:1 column:1];

	// TODO: do not harcode size!
	[tester tapViewWithAccessibilityLabel:LOCALIZED(@"Minimum (108.9 KB)")];

	UITableView *tv = [self findTableView:@"Chat list"];
	XCTAssertEqual([tv numberOfRowsInSection:0], 1);
	XCTAssertEqual([[[LinphoneManager instance] fileTransferDelegates] count], 1);
}

- (void)testUploadImage {
	NSString *user = @"testios";

	[self uploadImage];
	[self goBackFromChat];

	// if we go back to the same chatroom, the message should be still there
	[self startChatWith:user];
	UITableView *tv = [self findTableView:@"Chat list"];
	XCTAssertEqual([tv numberOfRowsInSection:0], 1);

	[tester waitForViewWithAccessibilityLabel:LOCALIZED(@"Download")];

	XCTAssertEqual([tv numberOfRowsInSection:0], 2);
	XCTAssertEqual([[[LinphoneManager instance] fileTransferDelegates] count], 0);
}

- (void)testCancelUploadImage {
	[self uploadImage];
	[tester tapViewWithAccessibilityLabel:LOCALIZED(@"Cancel transfer")];
	XCTAssertEqual([[[LinphoneManager instance] fileTransferDelegates] count], 0);
}

- (void)downloadImage {
	[self uploadImage];
	[tester tapViewWithAccessibilityLabel:LOCALIZED(@"Download")];
	[tester waitForTimeInterval:.5f]; // just wait a few secs to start download
	XCTAssertEqual([[[LinphoneManager instance] fileTransferDelegates] count], 1);
}

- (void)testDownloadImage {
	[self downloadImage];
	[tester waitForAbsenceOfViewWithAccessibilityLabel:@"Cancel transfer"];
	XCTAssertEqual([[[LinphoneManager instance] fileTransferDelegates] count], 0);
}

- (void)testCancelDownloadImage {
	[self downloadImage];
	[tester tapViewWithAccessibilityLabel:LOCALIZED(@"Cancel transfer")];
	XCTAssertEqual([[[LinphoneManager instance] fileTransferDelegates] count], 0);
}

@end
