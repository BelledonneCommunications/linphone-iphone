//
//  LinphoneCoreSettingsStore.m
//  linphone
//
//  Created by Pierre-Eric Pelloux-Prayer on 22/05/12.
//  Copyright (c) 2012 __MyCompanyName__. All rights reserved.
//

#import "LinphoneCoreSettingsStore.h"


debugenable_preference
transport_preference
username_preference
domain_preference
password_preference
check_config_disable_preference
outbound_proxy_preference
proxy_preference
prefix_preference
substitute_+_by_00_preference
wifi_only_preference

silk_24k_preference
speex_16k_preference
speex_8k_preference
silk_16k_preference
amr_8k_preference
gsm_8k_preference
ilbc_preference
pcmu_preference
pcma_preference
g722_preferenceg
g729_preference

mp4v-es_preference
h264_preference
vp8_preference

enable_video_preference
enable_srtp_preference
stun_preference
start_video_preference
backgroundmode_preference

@implementation LinphoneCoreSettingsStore


-(void) setObject:(id)value forKey:(NSString *)key {
    
    
}

- (id)objectForKey:(NSString*)key {
    return nil;
}
- (BOOL)synchronize {
    return YES;
}

@end
