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
}

- (void)beforeEach {
	[super beforeEach];
	[self goBackFromChat];
	[tester tapViewWithAccessibilityLabel:@"Chat"];
	[self removeAllRooms];
}

- (void)afterAll {
	[super afterAll];
	// at the end of tests, go back to chat rooms to display main bar
	[self goBackFromChat];
	ASSERT_EQ([LinphoneManager instance].fileTransferDelegates.count, 0)
}

#pragma mark - tools

- (void)dismissKeyboard {
	[tester tapScreenAtPoint:CGPointMake(0, 0)]; // dismiss keyboard, if any
}
- (void)goBackFromChat {
	[self dismissKeyboard];
	if ([tester tryFindingTappableViewWithAccessibilityLabel:@"Back" error:nil]) {
		[tester tapViewWithAccessibilityLabel:@"Back"];
	}
}

- (void)startChatWith:(NSString *)user {
	[tester tapViewWithAccessibilityLabel:@"New discussion"];
	[tester clearTextFromFirstResponder];
	[tester enterTextIntoCurrentFirstResponder:user];
	[tester tapRowAtIndexPath:[NSIndexPath indexPathForRow:0 inSection:0]
		inTableViewWithAccessibilityIdentifier:@"Suggested addresses"];
}

- (void)sendMessage:(NSString *)message {
	[tester enterText:message.uppercaseString intoViewWithAccessibilityLabel:@"Message field"];
	[tester tapViewWithAccessibilityLabel:@"Send"];
}

- (void)uploadImageWithQuality:(NSString *)quality {
	static int ind = 0;
	[tester tapViewWithAccessibilityLabel:@"Send picture"];
	[tester tapViewWithAccessibilityLabel:@"Photo library"];
// if popup "Linphone would access your photo" pops up, click OK.
#if TARGET_IPHONE_SIMULATOR
	if ([tester acknowledgeSystemAlert]) {
		[tester waitForTimeInterval:1];
	}
#endif

	// select random photo to avoid having the same multiple times.
	// There are 9 photos by default, so lets use just 8 (2 rows, 4 columns).
	LOGI(@"Selecting photo at row %d, column %d", 1 + (ind / 4) % 2, 1 + ind % 4);
	[tester choosePhotoInAlbum:@"Camera Roll" atRow:1 + (ind / 4) % 2 column:1 + ind % 4];
	ind++;
	[[UIApplication sharedApplication] writeScreenshotForLine:__LINE__ inFile:@__FILE__ description:nil error:NULL];

	// wait for the quality popup to show up
	UIAccessibilityElement *element = nil;
	float timeout = 10;
	while (!element && timeout > 0.f) {
		[tester waitForTimeInterval:.5];
		timeout -= .5f;
		element =
			[[UIApplication sharedApplication] accessibilityElementMatchingBlock:^BOOL(UIAccessibilityElement *e) {
			  return [e.accessibilityLabel containsSubstring:quality];
			}];
	}
	XCTAssertNotNil(element);
	[tester tapViewWithAccessibilityLabel:element.accessibilityLabel];
}

- (void)downloadImageWithQuality:(NSString *)quality {
	[self startChatWith:[self me]];
	[self uploadImageWithQuality:quality];
	// wait for the upload to terminate...
	for (int i = 0; i < 180; i++) {
		[tester waitForTimeInterval:1.f];
		if (LinphoneManager.instance.fileTransferDelegates.count == 0)
			break;
	}
	[tester waitForViewWithAccessibilityLabel:@"Download"];
	[tester tapViewWithAccessibilityLabel:@"Download"];
	[tester waitForTimeInterval:.1f]; // just wait a few msecs to start download
	ASSERT_EQ(LinphoneManager.instance.fileTransferDelegates.count, 1);
}

#pragma mark - tests

- (void)testChatFromContactPhoneNumber {
	[tester tapViewWithAccessibilityLabel:@"Contacts"];
	NSString *name = [UIDevice.currentDevice.identifierForVendor.UUIDString
		substringFromIndex:UIDevice.currentDevice.identifierForVendor.UUIDString.length - 6];
	NSString *fullName = [NSString stringWithFormat:@"Anna %@", name];
	[self createContact:@"Anna" lastName:name phoneNumber:@"555-522-8243" SIPAddress:nil];

	[tester tapViewWithAccessibilityLabel:@"Back"];
	[tester tapViewWithAccessibilityLabel:@"All contacts filter"];
	[tester tapViewWithAccessibilityLabel:fullName];
	[tester tapViewWithAccessibilityLabel:@"Chat with 5555228243"];
	[self goBackFromChat];
	UITableView *tv = [self findTableView:@"Chat list"];
	ASSERT_EQ([tv numberOfRowsInSection:0], 1);
	[tester waitForViewWithAccessibilityLabel:@"Contact name, Message"
										value:[NSString stringWithFormat:@"%@ (0)", fullName]
									   traits:UIAccessibilityTraitStaticText];
}

- (void)testInvalidSIPAddress {
	[self startChatWith:@"sip://toto"];

	[tester waitForViewWithAccessibilityLabel:@"Invalid address" traits:UIAccessibilityTraitStaticText];
	[tester tapViewWithAccessibilityLabel:@"OK"];
}

- (void)testMessageRemoval {
	NSString *user = [self getUUID];

	[self startChatWith:user];
	[self sendMessage:user];
	[tester waitForViewWithAccessibilityLabel:@"Delivery failed" traits:UIAccessibilityTraitImage];
	[self dismissKeyboard];

	[tester tapViewWithAccessibilityLabel:@"Edit" traits:UIAccessibilityTraitButton];
	[tester waitForViewWithAccessibilityLabel:@"Checkbox" value:@"Deselected" traits:UIAccessibilityTraitButton];
	[tester tapRowAtIndexPath:[NSIndexPath indexPathForRow:0 inSection:0]
		inTableViewWithAccessibilityIdentifier:@"ChatRoom list"];
	[tester waitForViewWithAccessibilityLabel:@"Checkbox" value:@"Selected" traits:UIAccessibilityTraitButton];
	[tester tapViewWithAccessibilityLabel:@"Delete all"];
	[tester tapViewWithAccessibilityLabel:@"DELETE" traits:UIAccessibilityTraitButton];

	// check that the tableview is empty
	UITableView *tv = [self findTableView:@"ChatRoom list"];
	ASSERT_EQ([tv numberOfRowsInSection:0], 0); // no more messages

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
	size_t count = 0;
	LinphoneCore *lc = [LinphoneManager getLc];
	LinphoneChatRoom *room = linphone_core_get_chat_room_from_uri(lc, [[self me] UTF8String]);

	NSTimeInterval beforeEmpty = [[NSDate date] timeIntervalSince1970];
	[self startChatWith:[self me]];
	NSTimeInterval afterEmpty = [[NSDate date] timeIntervalSince1970];
	[self goBackFromChat];

	// generate lots of messages...
	for (; count < 50; count++) {
		LinphoneChatMessage *msg =
			linphone_chat_room_create_message(room, [[NSString stringWithFormat:@"Message %lu", count + 1] UTF8String]);
		linphone_chat_room_send_chat_message(room, msg);
	}

	for (int i = 0; i < 50; i++) {
		[tester waitForTimeInterval:.5f];

		if (bctbx_list_size(linphone_chat_room_get_history(room, 0)) == count) {
			break;
		}
	}

	[tester
		waitForViewWithAccessibilityLabel:@"Contact name, Message"
									value:[NSString stringWithFormat:@"%@, Message %lu (%lu)", self.me, count, count]
								   traits:UIAccessibilityTraitStaticText];

	NSTimeInterval before = [[NSDate date] timeIntervalSince1970];
	[tester tapRowAtIndexPath:[NSIndexPath indexPathForRow:0 inSection:0]
		inTableViewWithAccessibilityIdentifier:@"Chat list"];
	NSTimeInterval after = [[NSDate date] timeIntervalSince1970];

	// conversation loading MUST be less than 1 sec - loading messages only
	XCTAssertLessThan(after - before, afterEmpty - beforeEmpty + 1.);
}

- (void)testRemoveAllChats {
	NSArray *uuids = [self getUUIDArrayOfSize:3];

	for (NSString *uuid in uuids) {
		[self startChatWith:uuid];
		[self sendMessage:@"Test"];
		[self goBackFromChat];
	}

	UITableView *tv = [self findTableView:@"Chat list"];

	ASSERT_EQ([tv numberOfRowsInSection:0], uuids.count);

	[self removeAllRooms];

	// check that the tableview is empty
	ASSERT_EQ([tv numberOfRowsInSection:0], 0);

	// test that there's no more chatrooms in the core
	ASSERT_EQ(linphone_core_get_chat_rooms([LinphoneManager getLc]), NULL);
}

- (void)testSendMessageToMyself {
	[self startChatWith:[self me]];

	[self sendMessage:@"HELLO"];
	[tester waitForViewWithAccessibilityLabel:@"Outgoing message" value:@"HELLO" traits:UIAccessibilityTraitStaticText];
	[tester waitForViewWithAccessibilityLabel:@"Incoming message" value:@"HELLO" traits:UIAccessibilityTraitStaticText];
	[tester waitForAbsenceOfViewWithAccessibilityLabel:@"Message status"];

	[self goBackFromChat];
}

- (void)testSendToSIPAddress {
	NSString *sipAddr = [NSString stringWithFormat:@"sip:%@@%@", [self me], [self accountDomain]];

	[self startChatWith:sipAddr];

	[tester waitForViewWithAccessibilityLabel:@"Contact name" value:[self me] traits:0];

	[self goBackFromChat];
}

- (void)testTransferCancelDownloadImage {
	[self downloadImageWithQuality:@"Maximum"];
	[tester tapViewWithAccessibilityLabel:@"Cancel"];
	ASSERT_EQ([[[LinphoneManager instance] fileTransferDelegates] count], 0);
}

- (void)testTransferCancelUploadImage {
	[self startChatWith:[self me]];
	[self uploadImageWithQuality:@"Minimum"];
	[tester tapViewWithAccessibilityLabel:@"Cancel"];
	ASSERT_EQ([[[LinphoneManager instance] fileTransferDelegates] count], 0);
}

- (void)testTransferDestroyRoomWhileUploading {
	[self startChatWith:[self me]];
	[self uploadImageWithQuality:@"Maximum"];
	[self goBackFromChat];
	[self removeAllRooms];
}

- (void)testTransferDownloadImage {
	[self downloadImageWithQuality:@"Minimum"];
	[tester waitForAbsenceOfViewWithAccessibilityLabel:@"Cancel"];
	ASSERT_EQ([[[LinphoneManager instance] fileTransferDelegates] count], 0);
}

- (void)testTransferSimultanouslyDownload {
// wait for bugfix
#if 0
	[self startChatWith:[self me]];
	[self uploadImageWithQuality:@"Minimum"];
	[self uploadImageWithQuality:@"Minimum"];
	UITableView *tv = [self findTableView:@"ChatRoom list"];
	int timeout = 3;
	// wait for ALL uploads to terminate...
	for (int i = 0; i < 90; i++) {
		[tester waitForTimeInterval:1.f];
		if ([tv numberOfRowsInSection:0] == 4)
			break;
	}
	[tester waitForTimeInterval:.5f];
	ASSERT_EQ([[LinphoneManager instance] fileTransferDelegates].count, 0);
	[tester scrollViewWithAccessibilityIdentifier:@"ChatRoom list" byFractionOfSizeHorizontal:0.f vertical:1.f];
	for (int i = 0; i < 2; i++) {
		// messages order is not known: if upload bitrate is huge, first image can be uploaded before last started
		timeout = 3;
		while (![tester tryFindingTappableViewWithAccessibilityLabel:@"Download" error:nil] && timeout) {
			[tester scrollViewWithAccessibilityIdentifier:@"ChatRoom list"
							   byFractionOfSizeHorizontal:0.f
												 vertical:-.1f];
			timeout--;
		}
		[tester waitForViewWithAccessibilityLabel:@"Download"];
		[tester tapViewWithAccessibilityLabel:@"Download"];
		[tester waitForTimeInterval:.2f]; // just wait a few secs to start download
	}
	timeout = 30;
	while ([LinphoneManager instance].fileTransferDelegates.count > 0 && timeout) {
		[tester waitForTimeInterval:.5];
		timeout--;
	}
	[self goBackFromChat];
#endif
}

@end
