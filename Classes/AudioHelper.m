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

#import "AudioHelper.h"

@implementation AudioHelper

+ (NSArray *)bluetoothRoutes {
	return @[AVAudioSessionPortBluetoothHFP, AVAudioSessionPortCarAudio, AVAudioSessionPortBluetoothA2DP, AVAudioSessionPortBluetoothLE ];
}

+ (AVAudioSessionPortDescription *)bluetoothAudioDevice {
	return [AudioHelper audioDeviceFromTypes:[AudioHelper bluetoothRoutes]];
}

+ (AVAudioSessionPortDescription *)builtinAudioDevice {
	NSArray *builtinRoutes = @[ AVAudioSessionPortBuiltInMic ];
	return [AudioHelper audioDeviceFromTypes:builtinRoutes];
}

+ (AVAudioSessionPortDescription *)speakerAudioDevice {
	NSArray *builtinRoutes = @[ AVAudioSessionPortBuiltInSpeaker ];
	return [AudioHelper audioDeviceFromTypes:builtinRoutes];
}

+ (AVAudioSessionPortDescription *)audioDeviceFromTypes:(NSArray *)types {
	NSArray *routes = [[AVAudioSession sharedInstance] availableInputs];
	for (AVAudioSessionPortDescription *route in routes) {
		if ([types containsObject:route.portType]) {
			return route;
		}
	}
	return nil;
}

@end
