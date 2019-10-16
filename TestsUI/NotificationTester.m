/*
 * Copyright (c) 2010-2019 Belledonne Communications SARL.
 *
 * This file is part of linphone-iphone
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 */

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
