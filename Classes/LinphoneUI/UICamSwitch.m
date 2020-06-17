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

#import "UICamSwitch.h"
#include "LinphoneManager.h"
#import "Utils.h"

@implementation UICamSwitch
@synthesize preview;

#pragma mark - Lifecycle Functions

INIT_WITH_COMMON_CF {
	[self addTarget:self action:@selector(touchUp:) forControlEvents:UIControlEventTouchUpInside];
	return self;
}

#pragma mark -

- (void)touchUp:(id)sender {
	const char *currentCamId = (char *)linphone_core_get_video_device(LC);
	const char **cameras = linphone_core_get_video_devices(LC);
	const char *newCamId = NULL;
	int i;

	for (i = 0; cameras[i] != NULL; ++i) {
		if (strcmp(cameras[i], "StaticImage: Static picture") == 0)
			continue;
		if (strcmp(cameras[i], currentCamId) != 0) {
			newCamId = cameras[i];
			break;
		}
	}
	if (newCamId) {
		LOGI(@"Switching from [%s] to [%s]", currentCamId, newCamId);
		linphone_core_set_video_device(LC, newCamId);
		LinphoneCall *call = linphone_core_get_current_call(LC);
		if (call != NULL) {
			linphone_call_update(call, NULL);
		}
	}
}

@end
