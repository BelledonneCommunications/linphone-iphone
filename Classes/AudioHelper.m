//
//  AudioHelper.m
//  linphone
//
//  Created by REIS Benjamin on 01/03/2017.
//
//

#import "AudioHelper.h"

@implementation AudioHelper

+ (NSArray *)bluetoothRoutes {
	return @[ AVAudioSessionPortBluetoothA2DP, AVAudioSessionPortBluetoothLE, AVAudioSessionPortBluetoothHFP ];
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
