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

#import "UIBluetoothButton.h"
#import "../Utils/AudioHelper.h"
#import "Utils.h"
#import <AVFoundation/AVAudioSession.h>
#import <AudioToolbox/AudioToolbox.h>

#include "linphone/linphonecore.h"

@implementation UIBluetoothButton
#define check_auresult(au, method)                                                                                     \
	if (au != 0)                                                                                                       \
	LOGE(@"UIBluetoothButton error for %s: ret=%ld", method, au)

- (void)onOn {
	[LinphoneManager.instance setBluetoothEnabled:TRUE];
}

- (void)onOff {
	[LinphoneManager.instance setBluetoothEnabled:FALSE];
}

- (bool)onUpdate {
	return false;
}

@end
