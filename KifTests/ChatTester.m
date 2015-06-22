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

#import "KIF/UIApplication-KIFAdditions.h"

@implementation ChatTester

#pragma mark - setup

- (void)beforeAll {
	[super beforeAll];
	[self switchToValidAccountIfNeeded];
}

- (void)beforeEach {
	[super beforeEach];
	if ([tester tryFindingTappableViewWithAccessibilityLabel:@"Back" error:nil]) {
		[self goBackFromChat];
	}
	[tester tapViewWithAccessibilityLabel:@"Chat"];
	[self removeAllRooms];
}

- (void)afterAll {
	// at the end of tests, go back to chat rooms to display main bar
	if ([tester tryFindingTappableViewWithAccessibilityLabel:@"Back" error:nil]) {
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
	[tester tapViewWithAccessibilityLabel:@"Back"];
}

- (void)startChatWith:(NSString *)user {
	[tester enterText:user intoViewWithAccessibilityLabel:@"Enter a address"];
	[tester tapViewWithAccessibilityLabel:@"New Discussion"];
}

- (void)sendMessage:(NSString *)message {
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

- (void)testInvalidSIPAddress {

	[self startChatWith:@"sip://toto"];

	[tester waitForViewWithAccessibilityLabel:@"Invalid address" traits:UIAccessibilityTraitStaticText];
	[tester tapViewWithAccessibilityLabel:@"Cancel"];
}

- (void)testSendToSIPAddress {
	NSString *sipAddr = [NSString stringWithFormat:@"sip:%@@%@", [self accountUsername], [self accountDomain]];

	[self startChatWith:sipAddr];

	[tester waitForViewWithAccessibilityLabel:@"Contact name" value:@"testios" traits:0];

	[self goBackFromChat];
}

- (void)testChatMessageRemoval {

	NSString *user = [self getUUID];

	[self startChatWith:user];
	[self sendMessage:user];

	[tester tapViewWithAccessibilityLabel:@"Edit" traits:UIAccessibilityTraitButton];

	[tester tapViewWithAccessibilityLabel:@"Delete message"];

	[tester tapViewWithAccessibilityLabel:@"Edit" traits:UIAccessibilityTraitButton];

	// check that the tableview is empty
	UITableView *tv = nil;
	NSError *err = nil;
	if ([tester tryFindingAccessibilityElement:nil view:&tv withIdentifier:@"Chat list" tappable:false error:&err]) {
		XCTAssert(tv != nil);
		XCTAssert([tv numberOfRowsInSection:0] == 0); // no more messages
	} else {
		NSLog(@"Error: %@", err);
	}

	[self goBackFromChat];
}

- (void)testRemoveAllChats {
	NSArray *uuids = [self getUUIDArrayOfSize:5];

	for (NSString *uuid in uuids) {
		[self startChatWith:uuid];
		[self sendMessage:@"Test"];
		[self goBackFromChat];
	}

	[tester tapViewWithAccessibilityLabel:@"Edit" traits:UIAccessibilityTraitButton];

	// we expect to be able to delete at least the amount of chatrooms we created
	for (int i = 0; i < uuids.count; i++) {
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

	[tester tapViewWithAccessibilityLabel:@"Send picture"];
	[tester tapViewWithAccessibilityLabel:@"Photo library"];
	// if popup "Linphone would access your photo" pops up, click OK.
	if ([ALAssetsLibrary authorizationStatus] == ALAuthorizationStatusNotDetermined) {
#if TARGET_IPHONE_SIMULATOR
		[tester acknowledgeSystemAlert];
#endif
	}

	[tester choosePhotoInAlbum:@"Camera Roll" atRow:1 column:1];

	// wait for the quality popup to show up
	[tester waitForTimeInterval:1];

	UIAccessibilityElement *element =
		[[UIApplication sharedApplication] accessibilityElementMatchingBlock:^BOOL(UIAccessibilityElement *element) {
		  return [element.accessibilityLabel containsString:@"Minimum ("];
		}];
	[tester tapViewWithAccessibilityLabel:element.accessibilityLabel];

	UITableView *tv = [self findTableView:@"Chat list"];
	XCTAssertEqual([tv numberOfRowsInSection:0], 1);
	XCTAssertEqual([[[LinphoneManager instance] fileTransferDelegates] count], 1);
}

- (void)testUploadImage {
	NSString *user = @"testios";

	XCTAssertEqual([[LinphoneManager instance] fileTransferDelegates].count, 0);
	[self uploadImage];
	XCTAssertEqual([[LinphoneManager instance] fileTransferDelegates].count, 1);
	[self goBackFromChat];

	// if we go back to the same chatroom, the message should be still there
	[self startChatWith:user];
	UITableView *tv = [self findTableView:@"Chat list"];
	XCTAssertEqual([tv numberOfRowsInSection:0], 1);

	// wait for the upload to terminate...
	for (int i = 0; i < 15; i++) {
		[tester waitForTimeInterval:1.f];
		if ([[[LinphoneManager instance] fileTransferDelegates] count] == 0)
			break;
	}
	[tester waitForViewWithAccessibilityLabel:@"Download"];

	XCTAssertEqual([tv numberOfRowsInSection:0], 2);
	XCTAssertEqual([[[LinphoneManager instance] fileTransferDelegates] count], 0);
}

- (void)testCancelUploadImage {
	[self uploadImage];
	[tester tapViewWithAccessibilityLabel:@"Cancel transfer"];
	XCTAssertEqual([[[LinphoneManager instance] fileTransferDelegates] count], 0);
}

- (void)downloadImage {
	[self uploadImage];
	// wait for the upload to terminate...
	for (int i = 0; i < 15; i++) {
		[tester waitForTimeInterval:1.f];
		if ([[[LinphoneManager instance] fileTransferDelegates] count] == 0)
			break;
	}
	[tester waitForViewWithAccessibilityLabel:@"Download"];
	[tester tapViewWithAccessibilityLabel:@"Download"];
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
	[tester tapViewWithAccessibilityLabel:@"Cancel transfer"];
	XCTAssertEqual([[[LinphoneManager instance] fileTransferDelegates] count], 0);
}

@end
