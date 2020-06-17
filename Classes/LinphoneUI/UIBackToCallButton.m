/*
 * Copyright (c) 2010-2020 Belledonne Communications SARL.
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

#import "UIBackToCallButton.h"
#import "LinphoneManager.h"
#import "PhoneMainView.h"

@implementation UIBackToCallButton

- (instancetype)init {
	if (self = [super init]) {
		[NSNotificationCenter.defaultCenter addObserver:self
											   selector:@selector(callUpdateEvent:)
												   name:kLinphoneCallUpdate
												 object:nil];
	}
	return self;
}

- (void)dealloc {
	[NSNotificationCenter.defaultCenter removeObserver:self];
}

- (void)callUpdateEvent:(NSNotification *)notif {
	[self update];
}

- (void)update {
	self.hidden = (_tableView.isEditing || linphone_core_get_calls_nb(LC) == 0);
}

- (IBAction)onBackToCallClick:(id)sender {
	[PhoneMainView.instance popToView:CallView.compositeViewDescription];
}

@end
