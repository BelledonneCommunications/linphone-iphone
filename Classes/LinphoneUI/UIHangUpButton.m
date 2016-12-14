/* UIHangUpButton.m
 *
 * Copyright (C) 2011  Belledonne Comunications, Grenoble, France
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */

#import "UIHangUpButton.h"
#import "LinphoneManager.h"

@implementation UIHangUpButton

#pragma mark - Static Functions

+ (bool)isInConference:(LinphoneCall *)call {
	if (!call)
		return false;
	return linphone_call_params_get_local_conference_mode(linphone_call_get_current_params(call));
}

+ (int)callCount {
	int count = 0;
	const MSList *calls = linphone_core_get_calls(LC);

	while (calls != 0) {
		if (![UIHangUpButton isInConference:((LinphoneCall *)calls->data)]) {
			count++;
		}
		calls = calls->next;
	}
	return count;
}

#pragma mark - Lifecycle Functions

- (void)initUIHangUpButton {
	[self addTarget:self action:@selector(touchUp:) forControlEvents:UIControlEventTouchUpInside];
}

- (id)init {
	self = [super init];
	if (self) {
		[self initUIHangUpButton];
	}
	return self;
}

- (id)initWithCoder:(NSCoder *)decoder {
	self = [super initWithCoder:decoder];
	if (self) {
		[self initUIHangUpButton];
	}
	return self;
}

- (id)initWithFrame:(CGRect)frame {
	self = [super initWithFrame:frame];
	if (self) {
		[self initUIHangUpButton];
	}
	return self;
}

#pragma mark -

- (void)update {
	if (linphone_core_get_calls_nb(LC) == 1 ||										   // One call
		linphone_core_get_current_call(LC) != NULL ||								   // In call
		linphone_core_is_in_conference(LC) ||										   // In conference
		(linphone_core_get_conference_size(LC) > 0 && [UIHangUpButton callCount] == 0) // Only one conf
		) {
		[self setEnabled:true];
		return;
	}
	[self setEnabled:false];
}

#pragma mark - Action Functions

- (void)touchUp:(id)sender {
	LinphoneCall *currentcall = linphone_core_get_current_call(LC);
	if (linphone_core_is_in_conference(LC) ||										   // In conference
		(linphone_core_get_conference_size(LC) > 0 && [UIHangUpButton callCount] == 0) // Only one conf
		) {
		LinphoneManager.instance.conf = TRUE;
		linphone_core_terminate_conference(LC);
	} else if (currentcall != NULL) {
		linphone_core_terminate_call(LC, currentcall);
	} else {
		const MSList *calls = linphone_core_get_calls(LC);
		if (bctbx_list_size(calls) == 1) { // Only one call
			linphone_core_terminate_call(LC, (LinphoneCall *)(calls->data));
		}
	}
}

@end
