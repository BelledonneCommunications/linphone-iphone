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

- (id)init {
	self = [super init];
	if (self){
		dict = [[NSMutableDictionary alloc] init];
		changedDict = [[NSMutableDictionary alloc] init];
		[self transformLinphoneCoreToKeys];
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
			[LinphoneLogger logc:LinphoneLoggerWarning format:"Codec %s/%i supported by core is not shown in iOS app config view.",
					   pt->mime_type,pt->clock_rate];
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
            [self setInteger: linphone_proxy_config_get_expires(cfg) forKey:@"expire_preference"];
			[self setString: linphone_proxy_config_get_dial_prefix(cfg) forKey:@"prefix_preference"];
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
			[self setBool:linphone_proxy_config_get_dial_escape_plus(cfg) forKey:@"substitute_+_by_00_preference"];
			
		}
	} else {
		[self setInteger: lp_config_get_int(linphone_core_get_config(lc),"default_values","reg_expires", 600) forKey:@"expire_preference"];
        [self setObject:@"" forKey:@"username_preference"];
        [self setObject:@"" forKey:@"domain_preference"];
        [self setObject:@"" forKey:@"proxy_preference"];
        [self setObject:@"" forKey:@"password_preference"];
        [self setBool:FALSE forKey:@"outbound_proxy_preference"];
	}
    
    [self setBool:lp_config_get_int(linphone_core_get_config(lc), LINPHONERC_APPLICATION_KEY, "pushnotification_preference", 0) forKey:@"pushnotification_preference"];
    {
        LinphoneAddress *parsed = linphone_core_get_primary_contact_parsed(lc);
        if(parsed != NULL) {
            [self setString: linphone_address_get_display_name(parsed) forKey:@"primary_displayname_preference"];
            [self setString: linphone_address_get_username(parsed) forKey:@"primary_username_preference"];
        }
        linphone_address_destroy(parsed);
    }
    {
        {
            int minPort, maxPort;
            linphone_core_get_audio_port_range(lc, &minPort, &maxPort);
            if(minPort != maxPort)
                [self setObject:[NSString stringWithFormat:@"%d-%d", minPort, maxPort] forKey:@"audio_port_preference"];
            else
                [self setObject:[NSString stringWithFormat:@"%d", minPort] forKey:@"audio_port_preference"];
        }
        {
            int minPort, maxPort;
            linphone_core_get_video_port_range(lc, &minPort, &maxPort);
            if(minPort != maxPort)
                [self setObject:[NSString stringWithFormat:@"%d-%d", minPort, maxPort] forKey:@"video_port_preference"];
            else
                [self setObject:[NSString stringWithFormat:@"%d", minPort] forKey:@"video_port_preference"];
        }
    }
    {
        [self setInteger: linphone_core_get_upload_bandwidth(lc) forKey:@"upload_bandwidth_preference"];
        [self setInteger: linphone_core_get_download_bandwidth(lc) forKey:@"download_bandwidth_preference"];
    }
    {
        [self setFloat:linphone_core_get_playback_gain_db(lc) forKey:@"playback_gain_preference"];
        [self setFloat:linphone_core_get_mic_gain_db(lc) forKey:@"microphone_gain_preference"];
    }
	{
		LCSipTransports tp;
		const char *tname = "udp";
        int port = 5060;
		linphone_core_get_sip_transports(lc, &tp);
		if (tp.udp_port>0) {
            tname = "udp";
            port = tp.udp_port;
        } else if (tp.tcp_port>0) {
            tname = "tcp";
            port = tp.tcp_port;
        } else if (tp.tls_port>0) {
            tname = "tls";
            port = tp.tls_port;
        }
		[self setString:tname forKey:@"transport_preference"];
        [self setInteger:port forKey:@"port_preference"];
        
        [self setInteger:lp_config_get_int(linphone_core_get_config(lc),"sip","sip_random_port", 1) forKey:@"random_port_preference"];
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
        [self
            setBool:linphone_core_get_firewall_policy(lc)==LinphonePolicyUseIce forKey:@"ice_preference"];
	}
	
	{
		[self transformCodecsToKeys: linphone_core_get_audio_codecs(lc)];
		[self transformCodecsToKeys: linphone_core_get_video_codecs(lc)]; 
	}
	
	{	
		LinphoneMediaEncryption menc=linphone_core_get_media_encryption(lc);
		const char *val;
		switch(menc){
			case LinphoneMediaEncryptionSRTP:
				val="SRTP";
				break;
			case LinphoneMediaEncryptionZRTP:
				val="ZRTP";
				break;
			default:
				val="None";
		}
		[self setString:val forKey:@"media_encryption_preference"];
	}
    [self setString: lp_config_get_string(linphone_core_get_config(lc), LINPHONERC_APPLICATION_KEY, "rotation_preference", "auto") forKey:@"rotation_preference"];
	[self setBool: lp_config_get_int(linphone_core_get_config(lc), LINPHONERC_APPLICATION_KEY, "enable_first_login_view_preference", 0) forKey:@"enable_first_login_view_preference"];
	[self setBool: lp_config_get_int(linphone_core_get_config(lc), LINPHONERC_APPLICATION_KEY, "debugenable_preference", 0) forKey:@"debugenable_preference"];
	[self setBool: lp_config_get_int(linphone_core_get_config(lc), LINPHONERC_APPLICATION_KEY, "animations_preference", 1) forKey:@"animations_preference"];
	[self setBool: lp_config_get_int(linphone_core_get_config(lc), LINPHONERC_APPLICATION_KEY, "wifi_only_preference", 0) forKey:@"wifi_only_preference"];
	[self setString: lp_config_get_string(linphone_core_get_config(lc), LINPHONERC_APPLICATION_KEY, "sharing_server_preference", NULL) forKey:@"sharing_server_preference"];
	
	/*keep this one also in the standardUserDefaults so that it can be read before starting liblinphone*/
	BOOL start_at_boot = TRUE;
	if ([[NSUserDefaults standardUserDefaults] objectForKey:@"start_at_boot_preference"] != Nil)
        start_at_boot = [[NSUserDefaults standardUserDefaults]  boolForKey:@"start_at_boot_preference"];
	[self setBool: start_at_boot forKey:@"start_at_boot_preference"];
	BOOL background_mode = TRUE;
	if ([[NSUserDefaults standardUserDefaults] objectForKey:@"backgroundmode_preference"] != Nil)
        background_mode =[[NSUserDefaults standardUserDefaults]  boolForKey:@"backgroundmode_preference"];
	[self setBool: background_mode forKey:@"backgroundmode_preference"];
	
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
		[self setBool:(pol->automatically_initiate) forKey:@"start_video_preference"];
        [self setBool:(pol->automatically_accept) forKey:@"accept_video_preference"];
        [self setBool:linphone_core_self_view_enabled(lc) forKey:@"self_video_preference"];
        [self setBool:linphone_core_video_preview_enabled(lc) forKey:@"preview_preference"];
	}
    {
        [self setBool:linphone_core_get_use_info_for_dtmf(lc) forKey:@"sipinfo_dtmf_preference"];
        [self setBool:linphone_core_get_use_rfc2833_for_dtmf(lc) forKey:@"rfc_dtmf_preference"];
        
        [self setInteger:linphone_core_get_inc_timeout(lc) forKey:@"incoming_call_timeout_preference"];
        [self setInteger:linphone_core_get_in_call_timeout(lc) forKey:@"in_call_timeout_preference"];
    }
    

	
	[changedDict release];
	changedDict = [[NSMutableDictionary alloc] init];
    
    // Post event
    NSDictionary *eventDic = [NSDictionary dictionaryWithObject:self forKey:@"settings"];
    [[NSNotificationCenter defaultCenter] postNotificationName:kLinphoneLogsUpdate object:self userInfo:eventDic];
}

- (void)setObject:(id)value forKey:(NSString *)key {
	[dict setValue:value forKey:key];
	[changedDict setValue:[NSNumber numberWithBool:TRUE] forKey:key];
}

- (id)objectForKey:(NSString*)key {
    return [dict valueForKey:key];
}

- (BOOL)valueChangedForKey:(NSString*)key {
	return [[changedDict valueForKey:key] boolValue];
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
	int port_preference = [self integerForKey:@"port_preference"];
    
    BOOL random_port_preference = [self boolForKey:@"random_port_preference"];
    lp_config_set_int(linphone_core_get_config(lc),"sip","sip_random_port", random_port_preference);
	lp_config_set_int(linphone_core_get_config(lc),"sip","sip_tcp_random_port", random_port_preference);
    lp_config_set_int(linphone_core_get_config(lc),"sip","sip_tls_random_port", random_port_preference);
    if(random_port_preference) {
        port_preference = (0xDFFF&random())+1024;
        [self setInteger:port_preference forKey:@"port_preference"]; // Update back preference
    }
    
	LCSipTransports transportValue={0};
	if (transport!=nil) {
		if (linphone_core_get_sip_transports(lc, &transportValue)) {
			[LinphoneLogger logc:LinphoneLoggerError format:"cannot get current transport"];	
		}
		// Only one port can be set at one time, the others's value is 0
		if ([transport isEqualToString:@"tcp"]) {
			transportValue.tcp_port=port_preference;
			transportValue.udp_port=0;
            transportValue.tls_port=0;
		} else if ([transport isEqualToString:@"udp"]){
			transportValue.udp_port=port_preference;
			transportValue.tcp_port=0;
            transportValue.tls_port=0;
		} else if ([transport isEqualToString:@"tls"]){
			transportValue.tls_port=port_preference;
			transportValue.tcp_port=0;
            transportValue.udp_port=0;
		} else {
			[LinphoneLogger logc:LinphoneLoggerError format:"unexpected transport [%s]",[transport cStringUsingEncoding:[NSString defaultCStringEncoding]]];
		}
		if (linphone_core_set_sip_transports(lc, &transportValue)) {
			[LinphoneLogger logc:LinphoneLoggerError format:"cannot set transport"];	
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
		
		
        
		//possible valid config detected
		
		proxyCfg = linphone_core_create_proxy_config(lc);
        
		// add username password
		LinphoneAddress *from = linphone_address_new(identity);
		LinphoneAuthInfo *info;
		if (from != 0){
			info=linphone_auth_info_new(linphone_address_get_username(from),NULL,password,NULL,NULL);
			linphone_core_add_auth_info(lc,info);
            linphone_address_destroy(from);
		}
		
		// configure proxy entries
		linphone_proxy_config_set_identity(proxyCfg, identity);
		linphone_proxy_config_set_server_addr(proxyCfg, proxy);
		linphone_proxy_config_enable_register(proxyCfg, true);
		

		int expire = [self integerForKey:@"expire_preference"];
		linphone_proxy_config_expires(proxyCfg,expire);
		
		BOOL isWifiOnly = [self boolForKey:@"wifi_only_preference"];
		
		if (isWifiOnly && lLinphoneMgr.connectivity == wwan) {
			linphone_proxy_config_expires(proxyCfg, 0);
		} else {
            linphone_proxy_config_expires(proxyCfg, expire);
		}
		
		if (isOutboundProxy)
			linphone_proxy_config_set_route(proxyCfg, proxy);
		
		if ([self objectForKey:@"prefix_preference"]) {		
			NSString* prefix = [self stringForKey:@"prefix_preference"];
			if ([prefix length]>0) {
				linphone_proxy_config_set_dial_prefix(proxyCfg, [prefix cStringUsingEncoding:[NSString defaultCStringEncoding]]);
			}
		}
		if ([self objectForKey:@"substitute_+_by_00_preference"]) {
			bool substitute_plus_by_00 = [self boolForKey:@"substitute_+_by_00_preference"];
			linphone_proxy_config_set_dial_escape_plus(proxyCfg,substitute_plus_by_00);
		}
		
        BOOL pushnotification = [self boolForKey:@"pushnotification_preference"];
        lp_config_set_int(linphone_core_get_config(lc), LINPHONERC_APPLICATION_KEY, "pushnotification_preference", pushnotification);
        
        [[LinphoneManager instance] addPushTokenToProxyConfig:proxyCfg];
        
		linphone_core_add_proxy_config(lc,proxyCfg);
		//set to default proxy
		linphone_core_set_default_proxy(lc,proxyCfg);
		
	}
    [[[LinphoneManager instance] fastAddressBook] reload];
}

+ (int)validPort:(int)port {
    if(port < 0) {
        return 0;
    }
    if(port > 65535) {
        return 65535;
    }
    return port;
}

+ (BOOL)parsePortRange:(NSString*)text minPort:(int*)minPort maxPort:(int*)maxPort {
    NSError* error = nil;
    *minPort = -1;
    *maxPort = -1;
    NSRegularExpression* regex = [NSRegularExpression regularExpressionWithPattern:@"([0-9]+)(([^0-9]+)([0-9]+))?" options:0 error:&error];
    if(error != NULL)
        return FALSE;
    NSArray* matches = [regex matchesInString:text options:0 range:NSMakeRange(0, [text length])];
    if([matches count] == 1) {
        NSTextCheckingResult *match = [matches objectAtIndex:0];
        NSLog(@"%d", [match numberOfRanges]);
        bool range = [match rangeAtIndex:2].length > 0;
        if(!range) {
            NSRange rangeMinPort = [match rangeAtIndex:1];
            *minPort = [LinphoneCoreSettingsStore validPort:[[text substringWithRange:rangeMinPort] integerValue]];
            *maxPort = *minPort;
            return TRUE;
        } else {
            NSRange rangeMinPort = [match rangeAtIndex:1];
            *minPort = [LinphoneCoreSettingsStore validPort:[[text substringWithRange:rangeMinPort] integerValue]];
            NSRange rangeMaxPort = [match rangeAtIndex:4];
            *maxPort = [LinphoneCoreSettingsStore validPort:[[text substringWithRange:rangeMaxPort] integerValue]];
            if(*minPort > *maxPort) {
                *minPort = *maxPort;
            }
           return TRUE;
        }
    }
    return FALSE;
}

- (BOOL)synchronize {
	if (![LinphoneManager isLcReady]) return YES;
	LinphoneCore *lc=[LinphoneManager getLc];
	
	BOOL account_changed;
	
	account_changed=[self valueChangedForKey:@"username_preference"] 
				|| [self valueChangedForKey:@"password_preference"] 
				|| [self valueChangedForKey:@"domain_preference"] 
                || [self valueChangedForKey:@"expire_preference"] 
				|| [self valueChangedForKey:@"proxy_preference"]
				|| [self valueChangedForKey:@"outbound_proxy_preference"]
				|| [self valueChangedForKey:@"transport_preference"]
                || [self valueChangedForKey:@"port_preference"]
                || [self valueChangedForKey:@"random_port_preference"]
				|| [self valueChangedForKey:@"prefix_preference"]
				|| [self valueChangedForKey:@"substitute_+_by_00_preference"]
                || [self valueChangedForKey:@"pushnotification_preference"];
	
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
	
    linphone_core_set_use_info_for_dtmf(lc, [self boolForKey:@"sipinfo_dtmf_preference"]);
    linphone_core_set_use_rfc2833_for_dtmf(lc, [self boolForKey:@"rfc_dtmf_preference"]);
    linphone_core_set_inc_timeout(lc, [self integerForKey:@"incoming_call_timeout_preference"]);
    linphone_core_set_in_call_timeout(lc, [self integerForKey:@"in_call_timeout_preference"]);
    
	bool enableVideo = [self boolForKey:@"enable_video_preference"];
	linphone_core_enable_video(lc, enableVideo, enableVideo);

	NSString *menc = [self stringForKey:@"media_encryption_preference"];
	if (menc && [menc compare:@"SRTP"] == NSOrderedSame)
		linphone_core_set_media_encryption(lc, LinphoneMediaEncryptionSRTP);
	else if (menc && [menc compare:@"ZRTP"] == NSOrderedSame)
		linphone_core_set_media_encryption(lc, LinphoneMediaEncryptionZRTP);
	else linphone_core_set_media_encryption(lc, LinphoneMediaEncryptionNone);
	
    NSString* stun_server = [self stringForKey:@"stun_preference"];
    if ([stun_server length] > 0){
        linphone_core_set_stun_server(lc, [stun_server UTF8String]);
        BOOL ice_preference = [self boolForKey:@"ice_preference"];
        if(ice_preference) {
            linphone_core_set_firewall_policy(lc, LinphonePolicyUseIce);
        } else {
            linphone_core_set_firewall_policy(lc, LinphonePolicyUseStun);
        }
    } else {
        linphone_core_set_stun_server(lc, NULL);
        linphone_core_set_firewall_policy(lc, LinphonePolicyNoFirewall);
    }
	
    LinphoneVideoPolicy policy;
    policy.automatically_accept = [self boolForKey:@"accept_video_preference"];
    policy.automatically_initiate = [self boolForKey:@"start_video_preference"];
    linphone_core_set_video_policy(lc, &policy);
    linphone_core_enable_self_view(lc, [self boolForKey:@"self_video_preference"]);
    linphone_core_enable_video_preview(lc, [self boolForKey:@"preview_preference"]);
    
    // Primary contact
    NSString* displayname = [self stringForKey:@"primary_displayname_preference"];
    NSString* username = [self stringForKey:@"primary_username_preference"];
    LinphoneAddress *parsed = linphone_core_get_primary_contact_parsed(lc);
    if(parsed != NULL) {
        linphone_address_set_display_name(parsed,[displayname cStringUsingEncoding:[NSString defaultCStringEncoding]]);
        linphone_address_set_username(parsed,[username cStringUsingEncoding:[NSString defaultCStringEncoding]]);
        char *contact = linphone_address_as_string(parsed);
        linphone_core_set_primary_contact(lc, contact);
        ms_free(contact);
        linphone_address_destroy(parsed);
    }
    

    // Audio & Video Port
    {
        NSString *audio_port_preference = [self stringForKey:@"audio_port_preference"];
        int minPort, maxPort;
        [LinphoneCoreSettingsStore parsePortRange:audio_port_preference minPort:&minPort maxPort:&maxPort];
        linphone_core_set_audio_port_range(lc, minPort, maxPort);
    }
    {
        NSString *video_port_preference = [self stringForKey:@"video_port_preference"];
        int minPort, maxPort;
        [LinphoneCoreSettingsStore parsePortRange:video_port_preference minPort:&minPort maxPort:&maxPort];
        linphone_core_set_video_port_range(lc, minPort, maxPort);
    }
    
    int upload_bandwidth = [self integerForKey:@"upload_bandwidth_preference"];
    linphone_core_set_upload_bandwidth(lc, upload_bandwidth);
    
    int download_bandwidth = [self integerForKey:@"download_bandwidth_preference"];
    linphone_core_set_download_bandwidth(lc, download_bandwidth);
    
    float playback_gain = [self floatForKey:@"playback_gain_preference"];
    linphone_core_set_playback_gain_db(lc, playback_gain);
    
    float mic_gain = [self floatForKey:@"microphone_gain_preference"];
    linphone_core_set_mic_gain_db(lc, mic_gain);
    
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
	lp_config_set_int(linphone_core_get_config(lc), LINPHONERC_APPLICATION_KEY, "backgroundmode_preference", isbackgroundModeEnabled);
	
    BOOL firstloginview = [self boolForKey:@"enable_first_login_view_preference"];
    lp_config_set_int(linphone_core_get_config(lc), LINPHONERC_APPLICATION_KEY, "enable_first_login_view_preference", firstloginview);
    
    NSString *landscape = [self stringForKey:@"rotation_preference"];
    lp_config_set_string(linphone_core_get_config(lc), LINPHONERC_APPLICATION_KEY, "rotation_preference", [landscape UTF8String]);
    
	BOOL debugmode = [self boolForKey:@"debugenable_preference"];
	lp_config_set_int(linphone_core_get_config(lc), LINPHONERC_APPLICATION_KEY, "debugenable_preference", debugmode);
	if (debugmode) linphone_core_enable_logs_with_cb((OrtpLogFunc)linphone_iphone_log_handler);
	else linphone_core_disable_logs();
    [[NSUserDefaults standardUserDefaults]  setBool:debugmode forKey:@"debugenable_preference"]; //to be used at linphone core startup
	
    BOOL animations = [self boolForKey:@"animations_preference"];
	lp_config_set_int(linphone_core_get_config(lc), LINPHONERC_APPLICATION_KEY, "animations_preference", animations);
    
    BOOL wifiOnly = [self boolForKey:@"wifi_only_preference"];
	lp_config_set_int(linphone_core_get_config(lc), LINPHONERC_APPLICATION_KEY, "wifi_only_preference", wifiOnly);
    if([self valueChangedForKey:@"wifi_only_preference"]) {
        [[LinphoneManager instance] setupNetworkReachabilityCallback];
    }
    
	NSString*  sharing_server = [self stringForKey:@"sharing_server_preference"];
	[[LinphoneManager instance] lpConfigSetString:sharing_server forKey:@"sharing_server_preference"];
	

	
	/*keep this one also in the standardUserDefaults so that it can be read before starting liblinphone*/
	BOOL start_at_boot = [self boolForKey:@"start_at_boot_preference"];
	[[NSUserDefaults standardUserDefaults] setBool: start_at_boot forKey:@"start_at_boot_preference"];
	BOOL background_mode = [self boolForKey:@"backgroundmode_preference"];
	[[NSUserDefaults standardUserDefaults] setBool: background_mode forKey:@"backgroundmode_preference"];
    
    // Force synchronize
    [[NSUserDefaults standardUserDefaults] synchronize];
    
	[changedDict release];
	changedDict = [[NSMutableDictionary alloc] init];
    
    // Post event
    NSDictionary *eventDic = [NSDictionary dictionaryWithObject:self forKey:@"settings"];
    [[NSNotificationCenter defaultCenter] postNotificationName:kLinphoneSettingsUpdate object:self userInfo:eventDic];
    
    return YES;
}

@end
