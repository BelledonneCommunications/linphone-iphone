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
	// turn off logs for chat tests because there are way to much logs in liblinphone in filetransfer and sqlite
	linphone_core_set_log_level(ORTP_WARNING);
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
	[super afterAll];
	linphone_core_set_log_level(ORTP_MESSAGE);
	// at the end of tests, go back to chat rooms to display main bar
	if ([tester tryFindingTappableViewWithAccessibilityLabel:@"Back" error:nil]) {
		[self goBackFromChat];
	}
	ASSERT_EQ([LinphoneManager instance].fileTransferDelegates.count, 0)
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

- (void)uploadImageWithQuality:(NSString *)quality {
	UITableView *tv = [self findTableView:@"ChatRoom list"];

	long messagesCount = [tv numberOfRowsInSection:0];
	[tester tapViewWithAccessibilityLabel:@"Send picture"];
	[tester tapViewWithAccessibilityLabel:@"Photo library"];
	// if popup "Linphone would access your photo" pops up, click OK.
	if ([ALAssetsLibrary authorizationStatus] == ALAuthorizationStatusNotDetermined) {
#if TARGET_IPHONE_SIMULATOR
		[tester acknowledgeSystemAlert];
		[tester waitForTimeInterval:1];
#endif
	}

	// select random photo to avoid having the same multiple times
	[tester choosePhotoInAlbum:@"Camera Roll" atRow:1 column:1 + messagesCount % 4];

	// wait for the quality popup to show up
	[tester waitForTimeInterval:1];

	UIAccessibilityElement *element =
		[[UIApplication sharedApplication] accessibilityElementMatchingBlock:^BOOL(UIAccessibilityElement *element) {
		  return [element.accessibilityLabel containsString:quality];
		}];
	[tester tapViewWithAccessibilityLabel:element.accessibilityLabel];
}

- (void)downloadImage {
	[self startChatWith:[self me]];
	[self uploadImageWithQuality:@"Minimum"];
	// wait for the upload to terminate...
	for (int i = 0; i < 45; i++) {
		[tester waitForTimeInterval:1.f];
		if ([[[LinphoneManager instance] fileTransferDelegates] count] == 0)
			break;
	}
	[tester waitForViewWithAccessibilityLabel:@"Download"];
	[tester tapViewWithAccessibilityLabel:@"Download"];
	[tester waitForTimeInterval:.5f]; // just wait a few secs to start download
	ASSERT_EQ(LinphoneManager.instance.fileTransferDelegates.count, 1);
}

#pragma mark - tests

- (void)testChatFromContactPhoneNumber {
	[tester tapViewWithAccessibilityLabel:@"New Discussion"];
	[tester tapViewWithAccessibilityLabel:@"Anna Haro"];
	[tester tapViewWithAccessibilityLabel:@"home, 555-522-8243"];
	[self goBackFromChat];
	UITableView *tv = [self findTableView:@"Chat list"];
	ASSERT_EQ([tv numberOfRowsInSection:0], 1);
	[tester waitForViewWithAccessibilityLabel:@"Contact name, Message"
										value:@"Anna Haro (0)"
									   traits:UIAccessibilityTraitStaticText];
}

- (void)testInvalidSIPAddress {

	[self startChatWith:@"sip://toto"];

	[tester waitForViewWithAccessibilityLabel:@"Invalid address" traits:UIAccessibilityTraitStaticText];
	[tester tapViewWithAccessibilityLabel:@"Cancel"];
}

- (void)testMessageRemoval {
	NSString *user = [self getUUID];

	[self startChatWith:user];
	[self sendMessage:user];
	[tester waitForViewWithAccessibilityLabel:@"Message status"
										value:@"not delivered"
									   traits:UIAccessibilityTraitImage];

	[tester tapViewWithAccessibilityLabel:@"Edit" traits:UIAccessibilityTraitButton];

	[tester tapViewWithAccessibilityLabel:@"Delete message"];

	[tester tapViewWithAccessibilityLabel:@"Edit" traits:UIAccessibilityTraitButton];

	// check that the tableview is empty
	UITableView *tv = nil;
	NSError *err = nil;
	if ([tester tryFindingAccessibilityElement:nil
										  view:&tv
								withIdentifier:@"ChatRoom list"
									  tappable:false
										 error:&err]) {
		XCTAssertNotNil(tv);
		ASSERT_EQ([tv numberOfRowsInSection:0], 0); // no more messages
	} else {
		NSLog(@"Error: %@", err);
	}

	[self goBackFromChat];
}

- (void)testPerformanceHugeChatList {
	[tester tapViewWithAccessibilityLabel:@"Dialer"];

	// create lots of chat rooms...
	LinphoneCore *lc = [LinphoneManager getLc];
	for (int i = 0; i < 100; i++) {
		linphone_core_get_chat_room_from_uri(lc, [[NSString stringWithFormat:@"%@ - %d", [self me], i] UTF8String]);
	}

	NSTimeInterval before = [[NSDate date] timeIntervalSince1970];
	[tester tapViewWithAccessibilityLabel:@"Chat"];
	NSTimeInterval after = [[NSDate date] timeIntervalSince1970];

	XCTAssertEqual([[self findTableView:@"Chat list"] numberOfRowsInSection:0], 100);
	// conversation loading MUST be less than 1 sec
	XCTAssertLessThan(after - before, 1.);
}

- (void)testPerformanceHugeConversation {
	int count = 0;
	LinphoneCore *lc = [LinphoneManager getLc];
	LinphoneChatRoom *room = linphone_core_get_chat_room_from_uri(lc, [[self me] UTF8String]);
	// generate lots of messages...
	for (; count < 50; count++) {
		linphone_chat_room_send_message(room, [[NSString stringWithFormat:@"Message %d", count + 1] UTF8String]);
	}

	for (int i = 0; i < 25; i++) {
		[tester waitForTimeInterval:1.f];
		if (linphone_chat_room_get_history_size(room) == count * 2) {
			break;
		}
	}

	[tester waitForViewWithAccessibilityLabel:@"Contact name, Message, Unread message number"
										value:[NSString stringWithFormat:@"%@ - Message %d (%d)", self.me, count, count]
									   traits:UIAccessibilityTraitStaticText];

	NSTimeInterval before = [[NSDate date] timeIntervalSince1970];
	[self startChatWith:[self me]];
	NSTimeInterval after = [[NSDate date] timeIntervalSince1970];

	// conversation loading MUST be less than 1 sec - opening an empty conversation is around 2.15 sec
	XCTAssertLessThan(after - before, 2.15 + 1.);
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
	UITableView *tv = [self findTableView:@"Chat list"];
	ASSERT_EQ([tv numberOfRowsInSection:0], 0);

	// test that there's no more chatrooms in the core
	ASSERT_EQ(linphone_core_get_chat_rooms([LinphoneManager getLc]), NULL);
}

- (void)testSendMessageToMyself {
	[self startChatWith:[self me]];

	[self sendMessage:@"Hello"];

	[tester waitForViewWithAccessibilityLabel:@"Outgoing message" value:@"Hello" traits:UIAccessibilityTraitStaticText];
	[tester waitForViewWithAccessibilityLabel:@"Incoming message" value:@"Hello" traits:UIAccessibilityTraitStaticText];

	[tester waitForViewWithAccessibilityLabel:@"Message status" value:@"delivered" traits:UIAccessibilityTraitImage];

	[self goBackFromChat];
}

- (void)testSendToSIPAddress {
	NSString *sipAddr = [NSString stringWithFormat:@"sip:%@@%@", [self me], [self accountDomain]];

	[self startChatWith:sipAddr];

	[tester waitForViewWithAccessibilityLabel:@"Contact name" value:[self me] traits:0];

	[self goBackFromChat];
}

- (void)testTransfer2TransfersSimultanously {
	[self startChatWith:[self me]];
	[self uploadImageWithQuality:@"Minimum"];
	[self uploadImageWithQuality:@"Minimum"];
	UITableView *tv = [self findTableView:@"ChatRoom list"];
	// wait for ALL uploads to terminate...
	for (int i = 0; i < 45; i++) {
		[tester waitForTimeInterval:1.f];
		if ([tv numberOfRowsInSection:0] == 4)
			break;
	}
	[tester waitForTimeInterval:.5f];
	ASSERT_EQ([[LinphoneManager instance] fileTransferDelegates].count, 0);
	[tester scrollViewWithAccessibilityIdentifier:@"ChatRoom list" byFractionOfSizeHorizontal:0.f vertical:1.f];
	for (int i = 0; i < 2; i++) {
		// messages order is not known: if upload bitrate is huge, first image can be uploaded before last started
		while (![tester tryFindingTappableViewWithAccessibilityLabel:@"Download" error:nil]) {
			[tester scrollViewWithAccessibilityIdentifier:@"ChatRoom list"
							   byFractionOfSizeHorizontal:0.f
												 vertical:-.1f];
		}
		[tester waitForViewWithAccessibilityLabel:@"Download"];
		[tester tapViewWithAccessibilityLabel:@"Download"];
		[tester waitForTimeInterval:.2f]; // just wait a few secs to start download
	}
	while ([LinphoneManager instance].fileTransferDelegates.count > 0) {
		[tester waitForTimeInterval:.5];
	}
	[self goBackFromChat];
}

- (void)testTransferCancelDownloadImage {
	[self downloadImage];
	[tester tapViewWithAccessibilityLabel:@"Cancel transfer"];
	ASSERT_EQ([[[LinphoneManager instance] fileTransferDelegates] count], 0);
}

- (void)testTransferCancelUploadImage {
	[self startChatWith:[self me]];
	[self uploadImageWithQuality:@"Minimum"];
	[tester tapViewWithAccessibilityLabel:@"Cancel transfer"];
	ASSERT_EQ([[[LinphoneManager instance] fileTransferDelegates] count], 0);
}

- (void)testTransferDestroyRoomWhileUploading {
	[self startChatWith:[self me]];
	[self uploadImageWithQuality:@"Maximum"];
	[self goBackFromChat];
	[self removeAllRooms];
}

- (void)testTransferDownloadImage {
	[self downloadImage];
	[tester waitForAbsenceOfViewWithAccessibilityLabel:@"Cancel transfer"];
	ASSERT_EQ([[[LinphoneManager instance] fileTransferDelegates] count], 0);
}

@end
