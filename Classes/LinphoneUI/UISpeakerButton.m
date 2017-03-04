/* UISpeakerButton.m
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

#import <AVFoundation/AVAudioSession.h>
#import <AudioToolbox/AudioToolbox.h>
#import "UISpeakerButton.h"
#import "Utils.h"
#import "LinphoneManager.h"

#include "linphone/linphonecore.h"

@implementation UISpeakerButton

INIT_WITH_COMMON_CF {
	[NSNotificationCenter.defaultCenter addObserver:self
										   selector:@selector(audioRouteChangeListenerCallback:)
											   name:AVAudioSessionRouteChangeNotification
											 object:nil];
	return self;
}

- (void)dealloc {
	[NSNotificationCenter.defaultCenter removeObserver:self];
}

#pragma mark - UIToggleButtonDelegate Functions

- (void)audioRouteChangeListenerCallback:(NSNotification *)notif {
#pragma deploymate push "ignored-api-availability"
	if (UIDevice.currentDevice.systemVersion.doubleValue < 7 ||
		[[notif.userInfo valueForKey:AVAudioSessionRouteChangeReasonKey] integerValue] ==
			AVAudioSessionRouteChangeReasonRouteConfigurationChange) {
		[self update];
	}
#pragma deploymate pop
}

- (void)onOn {
	[LinphoneManager.instance setSpeakerEnabled:TRUE];
}

- (void)onOff {
	[LinphoneManager.instance setSpeakerEnabled:FALSE];
}

- (bool)onUpdate {
	self.enabled = [LinphoneManager.instance allowSpeaker];
	return [LinphoneManager.instance speakerEnabled];
}

@end
