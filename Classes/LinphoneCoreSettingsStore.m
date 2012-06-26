/* LinphoneCoreSettingsStore.m
 *
 * Copyright (C) 2012  Belledonne Comunications, Grenoble, France
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

#import "LinphoneCoreSettingsStore.h"

#include "lpconfig.h"

extern void linphone_iphone_log_handler(int lev, const char *fmt, va_list args);

@implementation LinphoneCoreSettingsStore

- (void)handleMigration {
	NSUserDefaults *oldconfig=[NSUserDefaults standardUserDefaults];
	NSArray *allkeys=[[oldconfig dictionaryRepresentation] allKeys];
    for(NSString* key in allkeys){
        NSLog(@"Migrating old config item %@ to in app settings.",key);
        [self setObject:[oldconfig objectForKey:key] forKey:key];
    }
    [self synchronize];
    NSLog(@"Migration done");
}

- (id)init {
	self = [super init];
	if (self){
		dict=[[NSMutableDictionary alloc] init];
		changedDict=[[NSMutableDictionary alloc] init];
		[self transformLinphoneCoreToKeys];
        LinphoneCore *lc=[LinphoneManager getLc];
		if (lp_config_get_int(linphone_core_get_config(lc),"app","config_migrated",0) == 0) {
			[self handleMigration];
			lp_config_set_int(linphone_core_get_config(lc),"app","config_migrated",1);
		}
	}
	return self;
}

- (void)dealloc {
	[dict release];
	[changedDict release];
	[super dealloc];
}

- (void)transformKeysToLinphoneCore {
	//LinphoneCore *lc=[LinphoneManager getLc];
	
}

- (void)setString:(const char*)value forKey:(NSString*)key {
	id obj=Nil;
	if (value) obj=[[NSString alloc] initWithCString:value encoding:[NSString defaultCStringEncoding] ];
	[self setObject: obj forKey:key];
}

- (NSString*)stringForKey:(NSString*) key {
	return [self objectForKey: key];
}

- (void)transformCodecsToKeys: (const MSList *)codecs {
	LinphoneCore *lc=[LinphoneManager getLc];
	const MSList *elem=codecs;
	for(;elem!=NULL;elem=elem->next){
		PayloadType *pt=(PayloadType*)elem->data;
		NSString *pref=[LinphoneManager getPreferenceForCodec:pt->mime_type withRate:pt->clock_rate];
		if (pref){
            bool_t value = linphone_core_payload_type_enabled(lc,pt);
			[self setBool:value  forKey: pref];
		}else{
			ms_warning("Codec %s/%i supported by core is not shown in iOS app config view.",
					   pt->mime_type,pt->clock_rate);
		}
	}
}

- (void)transformLinphoneCoreToKeys {
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
	[self setBool: lp_config_get_int(linphone_core_get_config(lc),"app","enable_first_login_view_preference", 0) forKey:@"enable_first_login_view_preference"];
	[self setBool: lp_config_get_int(linphone_core_get_config(lc),"app","debugenable_preference", 0) forKey:@"debugenable_preference"];
	[self setBool: lp_config_get_int(linphone_core_get_config(lc),"app","check_config_disable_preference", 0) forKey:@"check_config_disable_preference"];
	[self setBool: lp_config_get_int(linphone_core_get_config(lc),"app","wifi_only_preference", 0) forKey:@"wifi_only_preference"];
	[self setBool: lp_config_get_int(linphone_core_get_config(lc),"app","backgroundmode_preference", TRUE) forKey:@"backgroundmode_preference"];
	/*keep this one also in the standardUserDefaults so that it can be read before starting liblinphone*/
	BOOL start_at_boot;
	if ([[NSUserDefaults standardUserDefaults] objectForKey:@"start_at_boot_preference"]==Nil)
		start_at_boot=TRUE;
	else start_at_boot=[[NSUserDefaults standardUserDefaults]  boolForKey:@"start_at_boot_preference"];
	[self setBool: start_at_boot forKey:@"start_at_boot_preference"];
	
	if (linphone_core_tunnel_available()){
		/*FIXME: enhance linphonecore API to handle tunnel more easily in applications */
		//LinphoneTunnel *tun=linphone_core_get_tunnel(lc);
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
	if (lp_config_get_int(linphone_core_get_config(lc),"app","debugenable_preference",0))
		linphone_core_enable_logs_with_cb((OrtpLogFunc)linphone_iphone_log_handler);
	
	[changedDict release];
	changedDict=[[NSMutableDictionary alloc] init];
}

- (void)setObject:(id)value forKey:(NSString *)key {
	[dict setValue:value forKey:key];
	[changedDict setValue:[NSNumber numberWithBool:TRUE] forKey:key];
}

- (id)objectForKey:(NSString*)key {
    return [dict valueForKey:key];
}

- (BOOL)valueChangedForKey:(NSString*)key {
	return [ [changedDict valueForKey:key] boolValue];
}

- (void)synchronizeAccount {
	LinphoneCore *lc = [LinphoneManager getLc];
	LinphoneManager* lLinphoneMgr = [LinphoneManager instance];
	LinphoneProxyConfig* proxyCfg = NULL;
	/* unregister before modifying any settings */
    {
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
	
	LCSipTransports transportValue={0};
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
		
		NSString* proxyAddress = [self stringForKey:@"proxy_preference"];
		if ((!proxyAddress || [proxyAddress length] <1 ) && domain) {
			proxyAddress = [NSString stringWithFormat:@"sip:%@",domain] ;
		} else {
			proxyAddress = [NSString stringWithFormat:@"sip:%@",proxyAddress] ;
		}
		
		const char* proxy = [proxyAddress cStringUsingEncoding:[NSString defaultCStringEncoding]];
		
		NSString* prefix = [self stringForKey:@"prefix_preference"];
        bool substitute_plus_by_00 = [self boolForKey:@"substitute_+_by_00_preference"];
		//possible valid config detected
		
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
}

- (BOOL)synchronize {
	LinphoneCore *lc=[LinphoneManager getLc];
	
	if (lc==NULL) return YES;
	BOOL account_changed;
	
	account_changed=[self valueChangedForKey:@"username_preference"] 
				|| [self valueChangedForKey:@"password_preference"] 
				|| [self valueChangedForKey:@"domain_preference"] 
				|| [self valueChangedForKey:@"proxy_preference"]
				|| [self valueChangedForKey:@"outbound_proxy_preference"]
				|| [self valueChangedForKey:@"transport_preference"]
				|| [self valueChangedForKey:@"prefix_preference"]
				|| [self valueChangedForKey:@"substitute_+_by_00_preference"];
	
	if (account_changed)
		[self synchronizeAccount];
			
	//Configure Codecs
	
	PayloadType *pt;
	const MSList *elem;
	
	for (elem=linphone_core_get_audio_codecs(lc);elem!=NULL;elem=elem->next){
		pt=(PayloadType*)elem->data;
		NSString *pref=[LinphoneManager getPreferenceForCodec:pt->mime_type withRate:pt->clock_rate];
		linphone_core_enable_payload_type(lc,pt,[self boolForKey: pref]);
	}
	for (elem=linphone_core_get_video_codecs(lc);elem!=NULL;elem=elem->next){
		pt=(PayloadType*)elem->data;
		NSString *pref=[LinphoneManager getPreferenceForCodec:pt->mime_type withRate:pt->clock_rate];
		linphone_core_enable_payload_type(lc,pt,[self boolForKey: pref]);
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
    if ([stun_server length] > 0){
        linphone_core_set_stun_server(lc,[stun_server cStringUsingEncoding:[NSString defaultCStringEncoding]]);
        linphone_core_set_firewall_policy(lc, LinphonePolicyUseStun);
    } else {
        linphone_core_set_stun_server(lc, NULL);
        linphone_core_set_firewall_policy(lc, LinphonePolicyNoFirewall);
    }
	
    LinphoneVideoPolicy policy;
    policy.automatically_accept = [self boolForKey:@"start_video_preference"];
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
		isbackgroundModeEnabled = false;
	}
	lp_config_set_int(linphone_core_get_config(lc),"app","backgroundmode_preference", isbackgroundModeEnabled);
	
    BOOL firstloginview = [self boolForKey:@"enable_first_login_view_preference"];
    lp_config_set_int(linphone_core_get_config(lc),"app","enable_first_login_view_preference", firstloginview);
    
	BOOL debugmode = [self boolForKey:@"debugenable_preference"];
	lp_config_set_int(linphone_core_get_config(lc),"app","debugenable_preference", debugmode);
	if (debugmode) linphone_core_enable_logs_with_cb((OrtpLogFunc)linphone_iphone_log_handler);
	else linphone_core_disable_logs();
	
	/*keep this one also in the standardUserDefaults so that it can be read before starting liblinphone*/
	BOOL start_at_boot = [self boolForKey:@"start_at_boot_preference"];
	[[NSUserDefaults standardUserDefaults] setBool: start_at_boot forKey:@"start_at_boot_preference"];
	
	[changedDict release];
	changedDict = [[NSMutableDictionary alloc] init];
    return YES;
}

@end
