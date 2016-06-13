//
//  NotificationTester.m
//  linphone
//
//  Created by Gautier Pelloux-Prayer on 10/03/16.
//
//

#import "NotificationTester.h"

@implementation NotificationTester

#if !TARGET_IPHONE_SIMULATOR

- (void)beforeAll {
	[super beforeAll];
	[self switchToValidAccountIfNeeded];
}

- (void)testChatRemoteNotification {
	[tester tapViewWithAccessibilityLabel:@"Chat"];
	[self removeAllRooms];

	const LinphoneAddress *addr =
		linphone_proxy_config_get_identity_address(linphone_core_get_default_proxy_config(LC));
	LinphoneChatMessage *msg = linphone_chat_room_create_message(linphone_core_get_chat_room(LC, addr), "hello my own");
	linphone_chat_room_send_chat_message(linphone_core_get_chat_room(LC, addr), msg);
	linphone_core_set_network_reachable(LC, NO);

	[tester tapViewWithAccessibilityLabel:@"Chat"];

	// it can take several seconds to receive the remote push notification...
	int timeout = 35;
	while (timeout > 0) {
		[tester tryFindingViewWithAccessibilityLabel:@"Contact name, Message" error:nil];
		timeout--;
	}
	[tester waitForViewWithAccessibilityLabel:@"Contact name, Message"
										value:[NSString stringWithFormat:@"%@, hello my own (1)", self.me]
									   traits:UIAccessibilityTraitStaticText];
}

#endif

@end
