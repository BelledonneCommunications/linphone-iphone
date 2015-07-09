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
	if ([tester tryFindingTappableViewWithAccessibilityLabel:@"Back" error:nil]) {
		[self goBackFromChat];
	}
	[tester tapViewWithAccessibilityLabel:@"Chat"];
	[self removeAllRooms];
}

- (void)afterAll {
	[super afterAll];
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
	UITableView *tv = [self findTableView:@"Chat list"];

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
	for (int i = 0; i < 15; i++) {
		[tester waitForTimeInterval:1.f];
		if ([[[LinphoneManager instance] fileTransferDelegates] count] == 0)
			break;
	}
	[tester waitForViewWithAccessibilityLabel:@"Download"];
	[tester tapViewWithAccessibilityLabel:@"Download"];
	[tester waitForTimeInterval:.5f]; // just wait a few secs to start download
	ASSERT_EQ([[[LinphoneManager instance] fileTransferDelegates] count], 1);
}

#pragma mark - tests

- (void)test3DownloadsSimultanously {
	[self startChatWith:[self me]];
	[self uploadImageWithQuality:@"Maximum"];
	[self uploadImageWithQuality:@"Average"];
	[self uploadImageWithQuality:@"Minimum"];
	UITableView *tv = [self findTableView:@"Chat list"];
	// wait for ALL uploads to terminate...
	for (int i = 0; i < 45; i++) {
		[tester waitForTimeInterval:1.f];
		if ([tv numberOfRowsInSection:0] == 6)
			break;
	}
	[tester waitForTimeInterval:.5f];
	ASSERT_EQ([[LinphoneManager instance] fileTransferDelegates].count, 0);
	[tester scrollViewWithAccessibilityIdentifier:@"Chat list" byFractionOfSizeHorizontal:0.f vertical:1.f];
	for (int i = 0; i < 3; i++) {
		// messages order is not known: if upload bitrate is huge, first image can be uploaded before last started
		while (![tester tryFindingTappableViewWithAccessibilityLabel:@"Download" error:nil]) {
			[tester scrollViewWithAccessibilityIdentifier:@"Chat list" byFractionOfSizeHorizontal:0.f vertical:-.1f];
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

- (void)test3UploadsSimultanously {
	[self startChatWith:[self me]];
	// use Maximum quality to be sure that first transfer is not terminated when the third begins
	[self uploadImageWithQuality:@"Maximum"];
	[self uploadImageWithQuality:@"Average"];
	[self uploadImageWithQuality:@"Minimum"];
	UITableView *tv = [self findTableView:@"Chat list"];
	// wait for ALL uploads to terminate...
	for (int i = 0; i < 45; i++) {
		[tester waitForTimeInterval:1.f];
		if ([tv numberOfRowsInSection:0] == 6)
			break;
	}
	[tester waitForTimeInterval:.5f];
	ASSERT_EQ([[LinphoneManager instance] fileTransferDelegates].count, 0);
	ASSERT_EQ([tv numberOfRowsInSection:0], 6);
	[self goBackFromChat];
}

- (void)testCancelDownloadImage {
	[self downloadImage];
	[tester tapViewWithAccessibilityLabel:@"Cancel transfer"];
	ASSERT_EQ([[[LinphoneManager instance] fileTransferDelegates] count], 0);
}

- (void)testCancelUploadImage {
	[self startChatWith:[self me]];
	[self uploadImageWithQuality:@"Minimum"];
	[tester tapViewWithAccessibilityLabel:@"Cancel transfer"];
	if ([[[LinphoneManager instance] fileTransferDelegates] count] != 0) {
		[[UIApplication sharedApplication] writeScreenshotForLine:__LINE__ inFile:@__FILE__ description:nil error:NULL];
		;
	}
	ASSERT_EQ([[[LinphoneManager instance] fileTransferDelegates] count], 0);
}

- (void)testChatFromContactPhoneNumber {
	[tester tapViewWithAccessibilityLabel:@"New Discussion"];
	[tester tapViewWithAccessibilityLabel:@"Anna Haro"];
	[tester tapViewWithAccessibilityLabel:@"home, 555-522-8243"];
	[self goBackFromChat];
	UITableView *tv = [self findTableView:@"ChatRoom list"];
	ASSERT_EQ([tv numberOfRowsInSection:0], 1);
	[tester waitForViewWithAccessibilityLabel:@"Contact name, Message"
										value:@"Anna Haro (0)"
									   traits:UIAccessibilityTraitStaticText];
}

- (void)testDownloadImage {
	[self downloadImage];
	[tester waitForAbsenceOfViewWithAccessibilityLabel:@"Cancel transfer"];
	ASSERT_EQ([[[LinphoneManager instance] fileTransferDelegates] count], 0);
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
	if ([tester tryFindingAccessibilityElement:nil view:&tv withIdentifier:@"Chat list" tappable:false error:&err]) {
		XCTAssertNotNil(tv);
		ASSERT_EQ([tv numberOfRowsInSection:0], 0); // no more messages
	} else {
		NSLog(@"Error: %@", err);
	}

	[self goBackFromChat];
}

- (void)testInvalidSIPAddress {

	[self startChatWith:@"sip://toto"];

	[tester waitForViewWithAccessibilityLabel:@"Invalid address" traits:UIAccessibilityTraitStaticText];
	[tester tapViewWithAccessibilityLabel:@"Cancel"];
}

- (void)testRemoveAllChats {
	NSArray *uuids = [self getUUIDArrayOfSize:5];

	[self removeAllRooms];

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

- (void)testUploadImage {
	[self startChatWith:[self me]];

	ASSERT_EQ([[LinphoneManager instance] fileTransferDelegates].count, 0);
	[self uploadImageWithQuality:@"Minimum"];
	ASSERT_EQ([[LinphoneManager instance] fileTransferDelegates].count, 1);

	UITableView *tv = [self findTableView:@"Chat list"];
	ASSERT_EQ([tv numberOfRowsInSection:0], 1);

	// wait for the upload to terminate...
	for (int i = 0; i < 15; i++) {
		[tester waitForTimeInterval:1.f];
		if ([[[LinphoneManager instance] fileTransferDelegates] count] == 0)
			break;
	}
	[tester waitForViewWithAccessibilityLabel:@"Download"];

	ASSERT_EQ([tv numberOfRowsInSection:0], 2);
	ASSERT_EQ([[[LinphoneManager instance] fileTransferDelegates] count], 0);
}

@end
