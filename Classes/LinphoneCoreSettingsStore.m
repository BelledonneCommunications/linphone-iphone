//
//  LinphoneCoreSettingsStore.m
//  linphone
//
//  Created by Pierre-Eric Pelloux-Prayer on 22/05/12.
//  Copyright (c) 2012 __MyCompanyName__. All rights reserved.
//

#import "LinphoneCoreSettingsStore.h"

// linphone_core_enable_logs_with_cb - linphone_core_disable_logs
debugenable_preference

// on change: edit/create linphone_core_get_default_proxy()
transport_preference
username_preference
domain_preference
password_preference
outbound_proxy_preference
proxy_preference
prefix_preference
substitute_+_by_00_preference

// app internal setting
check_config_disable_preference
wifi_only_preference
backgroundmode_preference

// linphone_core_enable_payload_type
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

// linphone_core_enable_payload_type
mp4v-es_preference
h264_preference
vp8_preference

// linphone_core_enable_video
enable_video_preference

// linphone_core_set_media_encryption
enable_srtp_preference

// linphone_core_set_stun_server
// linphone_core_set_firewall_policy
stun_preference

// linphone_core_set_video_policy
start_video_preference


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
