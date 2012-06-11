//
//  LinphoneCoreSettingsStore.m
//  linphone
//
//  Created by Pierre-Eric Pelloux-Prayer on 22/05/12.
//  Copyright (c) 2012 __MyCompanyName__. All rights reserved.
//

#import "LinphoneCoreSettingsStore.h"

#include "lpconfig.h"


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

- (NSString*) stringForKey:(NSString*) key{
	return [self objectForKey: key];
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
	NSString *changed_value=[[NSString alloc] initWithFormat:@"%@_changed", key];
	[dict setValue:[NSNumber numberWithBool:TRUE] forKey:changed_value];
	[changed_value release];
}

- (id)objectForKey:(NSString*)key {
    return [dict valueForKey:key];
}

- (BOOL)synchronize {
	LinphoneCore *lc=[LinphoneManager getLc];
	LinphoneManager* lLinphoneMgr = [LinphoneManager instance];
	
	NSLog(@"Called in SettingsStore synchronize");
	if ([self boolForKey:@"username_preference_changed"])
		NSLog(@"username_preference_changed !!");
	return YES;
	/* unregister before modifying any settings */
    {
        LinphoneProxyConfig* proxyCfg;
        linphone_core_get_default_proxy(lc, &proxyCfg);
        
        if (proxyCfg) {
            // this will force unregister WITHOUT destorying the proxyCfg object
            linphone_proxy_config_edit(proxyCfg);
            
            int i=0;
            while (linphone_proxy_config_get_state(proxyCfg)!=LinphoneRegistrationNone &&
                   linphone_proxy_config_get_state(proxyCfg)!=LinphoneRegistrationCleared && 
				   linphone_proxy_config_get_state(proxyCfg)!=LinphoneRegistrationFailed && 
                   i++<40 ) {
                linphone_core_iterate(lc);
                usleep(10000);
            }
        }
    }
    
	NSString* transport = [self stringForKey:@"transport_preference"];
	
	LCSipTransports transportValue;
	if (transport!=nil) {
		if (linphone_core_get_sip_transports(lc, &transportValue)) {
			ms_error("cannot get current transport");	
		}
		// Only one port can be set at one time, the others's value is 0
		if ([transport isEqualToString:@"tcp"]) {
			if (transportValue.tcp_port == 0) transportValue.tcp_port=transportValue.udp_port + transportValue.tls_port;
			transportValue.udp_port=0;
            transportValue.tls_port=0;
		} else if ([transport isEqualToString:@"udp"]){
			if (transportValue.udp_port == 0) transportValue.udp_port=transportValue.tcp_port + transportValue.tls_port;
			transportValue.tcp_port=0;
            transportValue.tls_port=0;
		} else if ([transport isEqualToString:@"tls"]){
			if (transportValue.tls_port == 0) transportValue.tls_port=transportValue.udp_port + transportValue.tcp_port;
			transportValue.tcp_port=0;
            transportValue.udp_port=0;
		} else {
			ms_error("unexpected transport [%s]",[transport cStringUsingEncoding:[NSString defaultCStringEncoding]]);
		}
		if (linphone_core_set_sip_transports(lc, &transportValue)) {
			ms_error("cannot set transport");	
		}
	}
	
	
	//configure sip account
	
	//mandatory parameters
	
	NSString* username = [self stringForKey:@"username_preference"];
	NSString* domain = [self stringForKey:@"domain_preference"];
	NSString* accountPassword = [self stringForKey:@"password_preference"];
	bool isOutboundProxy= [self boolForKey:@"outbound_proxy_preference"];
	
	
	//clear auth info list
	linphone_core_clear_all_auth_info(lc);
    //clear existing proxy config
    linphone_core_clear_proxy_config(lc);
	if (username && [username length] >0 && domain && [domain length]>0) {
		const char* identity = [[NSString stringWithFormat:@"sip:%@@%@",username,domain] cStringUsingEncoding:[NSString defaultCStringEncoding]];
		const char* password = [accountPassword cStringUsingEncoding:[NSString defaultCStringEncoding]];
		
		NSString* proxyAddress = [[NSUserDefaults standardUserDefaults] stringForKey:@"proxy_preference"];
		if ((!proxyAddress || [proxyAddress length] <1 ) && domain) {
			proxyAddress = [NSString stringWithFormat:@"sip:%@",domain] ;
		} else {
			proxyAddress = [NSString stringWithFormat:@"sip:%@",proxyAddress] ;
		}
		
		const char* proxy = [proxyAddress cStringUsingEncoding:[NSString defaultCStringEncoding]];
		
		NSString* prefix = [[NSUserDefaults standardUserDefaults] stringForKey:@"prefix_preference"];
        bool substitute_plus_by_00 = [[NSUserDefaults standardUserDefaults] boolForKey:@"substitute_+_by_00_preference"];
		//possible valid config detected
		LinphoneProxyConfig* proxyCfg;	
		proxyCfg = linphone_proxy_config_new();
        
		// add username password
		LinphoneAddress *from = linphone_address_new(identity);
		LinphoneAuthInfo *info;
		if (from !=0){
			info=linphone_auth_info_new(linphone_address_get_username(from),NULL,password,NULL,NULL);
			linphone_core_add_auth_info(lc,info);
		}
		linphone_address_destroy(from);
		
		// configure proxy entries
		linphone_proxy_config_set_identity(proxyCfg,identity);
		linphone_proxy_config_set_server_addr(proxyCfg,proxy);
		linphone_proxy_config_enable_register(proxyCfg,true);
		BOOL isWifiOnly = [self boolForKey:@"wifi_only_preference"];
		
		if (isWifiOnly && lLinphoneMgr.connectivity == wwan) {
			linphone_proxy_config_expires(proxyCfg, 0);
		} else {
			linphone_proxy_config_expires(proxyCfg, lLinphoneMgr.defaultExpires);
		}
		
		if (isOutboundProxy)
			linphone_proxy_config_set_route(proxyCfg,proxy);
		
		if ([prefix length]>0) {
			linphone_proxy_config_set_dial_prefix(proxyCfg, [prefix cStringUsingEncoding:[NSString defaultCStringEncoding]]);
		}
		linphone_proxy_config_set_dial_escape_plus(proxyCfg,substitute_plus_by_00);
		
		linphone_core_add_proxy_config(lc,proxyCfg);
		//set to default proxy
		linphone_core_set_default_proxy(lc,proxyCfg);
		
	}		
	
	//Configure Codecs
	
	PayloadType *pt;
	const MSList *elem;
	//disable all codecs
	for (elem=linphone_core_get_audio_codecs(lc);elem!=NULL;elem=elem->next){
		pt=(PayloadType*)elem->data;
		linphone_core_enable_payload_type(lc,pt,[self boolForKey: getPrefForCodec(pt->mime_type,pt->clock_rate)]);
	}
	for (elem=linphone_core_get_video_codecs(lc);elem!=NULL;elem=elem->next){
		pt=(PayloadType*)elem->data;
		linphone_core_enable_payload_type(lc,pt,[self boolForKey: getPrefForCodec(pt->mime_type,pt->clock_rate)]);
	}
	
	bool enableVideo = [self boolForKey:@"enable_video_preference"];
	linphone_core_enable_video(lc, enableVideo, enableVideo);

	NSString *menc = [self stringForKey:@"media_encryption_preference"];
	if (menc && [menc compare:@"SRTP"])
		linphone_core_set_media_encryption(lc, LinphoneMediaEncryptionSRTP);
	else if (menc && [menc compare:@"ZRTP"])
		linphone_core_set_media_encryption(lc, LinphoneMediaEncryptionZRTP);
	else linphone_core_set_media_encryption(lc, LinphoneMediaEncryptionNone);
	
    NSString* stun_server = [self stringForKey:@"stun_preference"];
    if ([stun_server length]>0){
        linphone_core_set_stun_server(lc,[stun_server cStringUsingEncoding:[NSString defaultCStringEncoding]]);
        linphone_core_set_firewall_policy(lc, LinphonePolicyUseStun);
    }else{
        linphone_core_set_stun_server(lc, NULL);
        linphone_core_set_firewall_policy(lc, LinphonePolicyNoFirewall);
    }
	
    LinphoneVideoPolicy policy;
    policy.automatically_accept = [self boolForKey:@"start_video_preference"];;
    policy.automatically_initiate = [self boolForKey:@"start_video_preference"];
    linphone_core_set_video_policy(lc, &policy);
    
	UIDevice* device = [UIDevice currentDevice];
	bool backgroundSupported = false;
	if ([device respondsToSelector:@selector(isMultitaskingSupported)])
		backgroundSupported = [device isMultitaskingSupported];
	BOOL isbackgroundModeEnabled;
	if (backgroundSupported) {
		isbackgroundModeEnabled = [self boolForKey:@"backgroundmode_preference"];
	} else {
		isbackgroundModeEnabled=false;
	}
	lp_config_set_int(linphone_core_get_config(lc),"app","backgroundmode_preference",backgroundSupported);
    return YES;
}

@end
