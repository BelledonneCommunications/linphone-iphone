//
//  NotificationTester.m
//  linphone
//
//  Created by Gautier Pelloux-Prayer on 10/03/16.
//
//

#import "NotificationTester.h"

@implementation NotificationTester

- (void)beforeAll {
	[super beforeAll];
	[self switchToValidAccountIfNeeded];
}

- (void)testCallNotificationInBackground {
#if !TARGET_OS_SIMULATOR
	const LinphoneAddress *addr =
		linphone_proxy_config_get_identity_address(linphone_core_get_default_proxy_config(LC));
	LinphoneChatMessage *msg = linphone_chat_room_create_message(linphone_core_get_chat_room(LC, addr), "hello my own");
	linphone_chat_room_send_chat_message(linphone_core_get_chat_room(LC, addr), msg);
	linphone_core_set_network_reachable(LC, NO);
	[tester waitForViewWithAccessibilityLabel:@"hello my own"];
#endif
}

@end
