/* UIToggleVideoButton.m
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
 *  GNU Library General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */

#import "UIVideoButton.h"
#include "LinphoneManager.h"
#import "Utils.h"

@implementation UIVideoButton {
	BOOL last_update_state;
}

@synthesize waitView;

INIT_WITH_COMMON_CF {
	last_update_state = FALSE;
	return self;
}

- (void)onOn {

	if (!linphone_core_video_display_enabled(LC))
		return;

	[self setEnabled:FALSE];
	[waitView startAnimating];

	LinphoneCall *call = linphone_core_get_current_call(LC);
	if (call) {
		LinphoneCallAppData *callAppData = (__bridge LinphoneCallAppData *)linphone_call_get_user_pointer(call);
		callAppData->videoRequested =
			TRUE; /* will be used later to notify user if video was not activated because of the linphone core*/
		LinphoneCallParams *call_params = linphone_core_create_call_params(LC,call);
		linphone_call_params_enable_video(call_params, TRUE);
		linphone_core_update_call(LC, call, call_params);
		linphone_call_params_destroy(call_params);
	} else {
		LOGW(@"Cannot toggle video button, because no current call");
	}
}

- (void)onOff {

	if (!linphone_core_video_display_enabled(LC))
		return;

	[self setEnabled:FALSE];
	[waitView startAnimating];

	LinphoneCall *call = linphone_core_get_current_call(LC);
	if (call) {
		LinphoneCallParams *call_params = linphone_core_create_call_params(LC,call);
		linphone_call_params_enable_video(call_params, FALSE);
		linphone_core_update_call(LC, call, call_params);
		linphone_call_params_destroy(call_params);
	} else {
		LOGW(@"Cannot toggle video button, because no current call");
	}
}

- (bool)onUpdate {
	bool video_enabled = false;
	LinphoneCall *currentCall = linphone_core_get_current_call(LC);
	if (linphone_core_video_supported(LC)) {
		if (linphone_core_video_display_enabled(LC) && currentCall && !linphone_core_sound_resources_locked(LC) &&
			linphone_call_get_state(currentCall) == LinphoneCallStreamsRunning) {
			video_enabled = TRUE;
		}
	}

	[self setEnabled:video_enabled];
	if (last_update_state != video_enabled)
		[waitView stopAnimating];
	if (video_enabled) {
		video_enabled = linphone_call_params_video_enabled(linphone_call_get_current_params(currentCall));
	}
	last_update_state = video_enabled;

	return video_enabled;
}

@end
