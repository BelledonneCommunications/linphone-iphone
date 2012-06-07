//
//  LinphoneCoreSettingsStore.m
//  linphone
//
//  Created by Pierre-Eric Pelloux-Prayer on 22/05/12.
//  Copyright (c) 2012 __MyCompanyName__. All rights reserved.
//

#import "LinphoneCoreSettingsStore.h"

#include "lpconfig.h"


#if 0
// linphone_core_enable_logs_with_cb - linphone_core_disable_logs
debugenable_preference

// on change: edit/create linphone_core_get_default_proxy()
transport_preference
username_preference
domain_preference
password_preference
outbound_proxy_preference
proxy_preference
prefix_preference ++
substitute_+_by_00_preference ++

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
g722_preference
g729_preference

// linphone_core_enable_payload_type
mp4v-es_preference
h264_preference
vp8_preference

// linphone_core_enable_video
enable_video_preference
// linphone_core_set_video_policy
start_video_preference

// linphone_core_set_media_encryption
enable_srtp_preference

// linphone_core_set_stun_server
// linphone_core_set_firewall_policy
stun_preference



#endif

struct codec_name_pref_table{
	const char *name;
	int rate;
	NSString *prefname;
};

struct codec_name_pref_table codec_pref_table[]={
	{ "speex", 8000, @"speex_8k_preference" },
	{ "speex", 16000, @"speex_16k_preference" },
	{ "silk", 24000, @"silk_24k_preference" },
	{ "silk", 16000, @"silk_16k_preference" },
	{ "amr", 8000, @"amr_8k_preference" },
	{ "ilbc", 8000, @"ilbc_preference"},
	{ "pcmu", 8000, @"pcmu_preference"},
	{ "pcma", 8000, @"pcma_preference"},
	{ "g722", 8000, @"g722_preference"},
	{ "g729", 8000, @"g729_preference"},
	{ "mp4v-es", 90000, @"mp4v-es_preference"},
	{ "h264", 90000, @"h264_preference"},
	{ "vp8", 90000, @"vp8_preference"},
	{ NULL,0,Nil }
};

static NSString *getPrefForCodec(const char *name, int rate){
	int i;
	for(i=0;codec_pref_table[i].name!=NULL;++i){
		if (strcasecmp(codec_pref_table[i].name,name)==0 && codec_pref_table[i].rate==rate)
			return codec_pref_table[i].prefname;
	}
	return Nil;
}

@implementation LinphoneCoreSettingsStore

-(id) init{
	self = [super init];
	if (self){
		dict=[[NSMutableDictionary alloc] init];
		[self transformLinphoneCoreToKeys];
	}
	return self;
}

-(void) dealloc{
	[super dealloc];
	[dict release];
}

-(void) transformKeysToLinphoneCore{
	//LinphoneCore *lc=[LinphoneManager getLc];
	
}

- (void) setString:(const char*)value forKey:(NSString*)key{
	id obj=Nil;
	if (value) obj=[[NSString alloc] initWithCString:value encoding:[NSString defaultCStringEncoding] ];
	[self setObject: obj forKey:key];
}

-(void) transformCodecsToKeys: (const MSList *)codecs{
	LinphoneCore *lc=[LinphoneManager getLc];
	const MSList *elem=codecs;
	for(;elem!=NULL;elem=elem->next){
		PayloadType *pt=(PayloadType*)elem->data;
		NSString *pref=getPrefForCodec(pt->mime_type,pt->clock_rate);
		if (pref){
			[self setBool: linphone_core_payload_type_enabled(lc,pt) forKey: pref];
		}else{
			ms_warning("Codec %s/%i supported by core is not shown in iOS app config view.",
					   pt->mime_type,pt->clock_rate);
		}
	}
}

-(void) transformLinphoneCoreToKeys{
	LinphoneCore *lc=[LinphoneManager getLc];
	LinphoneProxyConfig *cfg=NULL;
	linphone_core_get_default_proxy(lc,&cfg);
	if (cfg){
		const char *identity=linphone_proxy_config_get_identity(cfg);
		LinphoneAddress *addr=linphone_address_new(identity);
		if (addr){
			const char *proxy=linphone_proxy_config_get_addr(cfg);
			LinphoneAddress *proxy_addr=linphone_address_new(proxy);
			const char *port=linphone_address_get_port(proxy_addr);
			
			[self setString: linphone_address_get_username(addr) forKey:@"username_preference"];
			[self setString: linphone_address_get_domain(addr) forKey:@"domain_preference"];
			if (strcmp(linphone_address_get_domain(addr),linphone_address_get_domain(proxy_addr))!=0
				|| port!=NULL){
				char tmp[256]={0};
				if (port!=NULL) {
					snprintf(tmp,sizeof(tmp)-1,"%s:%s",linphone_address_get_domain(proxy_addr),port);
				}else snprintf(tmp,sizeof(tmp)-1,"%s",linphone_address_get_domain(proxy_addr));
				[self setString: tmp forKey:@"proxy_preference"];
			}
			linphone_address_destroy(addr);
			linphone_address_destroy(proxy_addr);
			
			[self setBool: (linphone_proxy_config_get_route(cfg)!=NULL) forKey:@"outbound_proxy_preference"];
			
		}
	}
	{
		LCSipTransports tp;
		const char *tname="udp";
		linphone_core_get_sip_transports(lc, &tp);
		if (tp.udp_port>0) tname="udp";
		else if (tp.tcp_port>0) tname="tcp";
		else if (tp.tls_port>0) tname="tls";
		[self setString: tname forKey:@"transport_preference"];
	}
	{
		LinphoneAuthInfo *ai;
		const MSList *elem=linphone_core_get_auth_info_list(lc);
		if (elem && (ai=(LinphoneAuthInfo*)elem->data)){
			[self setString: linphone_auth_info_get_passwd(ai) forKey:@"password_preference"];
		}
	}
	{
		[self setString: linphone_core_get_stun_server(lc) forKey:@"stun_preference"];
	}
	
	{
		[self transformCodecsToKeys: linphone_core_get_audio_codecs(lc)];
		[self transformCodecsToKeys: linphone_core_get_video_codecs(lc)]; 
	}
	
	{	
		LinphoneMediaEncryption menc=linphone_core_get_media_encryption(lc);
		const char *val;
		switch(menc){
			LinphoneMediaEncryptionSRTP:
				val="SRTP";
				break;
			LinphoneMediaEncryptionZRTP:
				val="ZRTP";
				break;
			default:
				val="None";
		}
		[self setString:val forKey:@"media_encryption_preference"];
	}
	
	[self setBool: lp_config_get_int(linphone_core_get_config(lc),"app","debugenable_preference",0) forKey:@"debugenable_preference"];
	[self setBool: lp_config_get_int(linphone_core_get_config(lc),"app","check_config_disable_preference",0) forKey:@"check_config_disable_preference"];
	[self setBool: lp_config_get_int(linphone_core_get_config(lc),"app","wifi_only_preference",0) forKey:@"wifi_only_preference"];
	[self setBool: lp_config_get_int(linphone_core_get_config(lc),"app","backgroundmode_preference",TRUE) forKey:@"backgroundmode_preference"];
	[self setBool: lp_config_get_int(linphone_core_get_config(lc),"app","start_at_boot_preference",TRUE) forKey:@"disable_autoboot_preference"];

	if (linphone_core_tunnel_available()){
		/*FIXME: enhance linphonecore API to handle tunnel more easily in applications */
		LinphoneTunnel *tun=linphone_core_get_tunnel(lc);
		//[self setString: linphone_tunnel_get_servers(tun) forKey:tunnel_address_preference];
		//[self setInteger: blabla forKey:tunnel_port_preference];
		//[self setString: forKey:@"tunnel_enabled_preference"];
	}
	{
		const LinphoneVideoPolicy *pol;
		[self setBool: linphone_core_video_enabled(lc) forKey:@"enable_video_preference"];
		pol=linphone_core_get_video_policy(lc);
		[self setBool:(pol->automatically_accept && pol->automatically_initiate) forKey:@"start_video_preference"];
	}
}

-(void) setObject:(id)value forKey:(NSString *)key {
	[dict setValue:value forKey:key];
}

- (id)objectForKey:(NSString*)key {
    return [dict valueForKey:key];
}

- (BOOL)synchronize {
	ms_message("Called in SettingsStore synchronize");
    return YES;
}

- (void) enableCodecWithName: (const char*) name andRate: (int) rate to:(id)value{
	LinphoneCore *lc=[LinphoneManager getLc];
	PayloadType *pt;
	pt=linphone_core_find_payload_type(lc, name, rate);
	if (pt){
		linphone_core_enable_payload_type(lc, pt, [value boolValue]);
	}
}

@end
