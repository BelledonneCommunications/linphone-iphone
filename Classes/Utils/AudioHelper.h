//
//  AudioHelper.h
//  linphone
//
//  Created by REIS Benjamin on 01/03/2017.
//
//

#ifndef AudioHelper_h
#define AudioHelper_h

@interface AudioHelper : NSObject

+ (NSArray *)bluetoothRoutes;
+ (AVAudioSessionPortDescription *)bluetoothAudioDevice;
+ (AVAudioSessionPortDescription *)builtinAudioDevice;
+ (AVAudioSessionPortDescription *)speakerAudioDevice;
+ (AVAudioSessionPortDescription *)audioDeviceFromTypes:(NSArray *)types;
@end

#endif /* AudioHelper_h */
