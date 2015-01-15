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

#include "linphone/lpconfig.h"

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
	[self setObject:obj forKey:key];
	[obj release];
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
	LpConfig* conf = linphone_core_get_config(lc);
	linphone_core_get_default_proxy(lc,&cfg);
	if (cfg){
		const char *identity=linphone_proxy_config_get_identity(cfg);
		LinphoneAddress *addr=linphone_address_new(identity);
		if (addr){
			const char *proxy=linphone_proxy_config_get_addr(cfg);
			LinphoneAddress *proxy_addr=linphone_address_new(proxy);
			int port=linphone_address_get_port(proxy_addr);

			[self setString: linphone_address_get_username(addr) forKey:@"username_preference"];
			[self setString: linphone_address_get_domain(addr) forKey:@"domain_preference"];
			[self setInteger: linphone_proxy_config_get_expires(cfg) forKey:@"expire_preference"];
			[self setString: linphone_proxy_config_get_dial_prefix(cfg) forKey:@"prefix_preference"];
			if (strcmp(linphone_address_get_domain(addr),linphone_address_get_domain(proxy_addr))!=0 || port>0){
				char tmp[256]={0};
				if (port>0) {
					snprintf(tmp,sizeof(tmp)-1,"%s:%i",linphone_address_get_domain(proxy_addr),port);
				}else snprintf(tmp,sizeof(tmp)-1,"%s",linphone_address_get_domain(proxy_addr));
				[self setString:tmp forKey:@"proxy_preference"];
			}

			const char* tname = "udp";
			switch (linphone_address_get_transport(proxy_addr)) {
				case LinphoneTransportTcp: tname = "tcp"; break;
				case LinphoneTransportTls: tname = "tls"; break;
				default:                                  break;
			}
			[self setString:tname forKey:@"transport_preference"];

			linphone_address_destroy(addr);
			linphone_address_destroy(proxy_addr);

			[self setBool:(linphone_proxy_config_get_route(cfg)!=NULL) forKey:@"outbound_proxy_preference"];
			[self setBool:linphone_proxy_config_get_dial_escape_plus(cfg) forKey:@"substitute_+_by_00_preference"];
			[self setBool:linphone_proxy_config_avpf_enabled(cfg) forKey:@"avpf_preference"];

		}
	} else {
		[self setInteger: lp_config_get_int(conf,"default_values","reg_expires", 600) forKey:@"expire_preference"];
		[self setObject:@""   forKey:@"username_preference"];
		[self setObject:@""   forKey:@"domain_preference"];
		[self setObject:@""   forKey:@"proxy_preference"];
		[self setObject:@""   forKey:@"password_preference"];
		[self setBool:FALSE   forKey:@"outbound_proxy_preference"];
		[self setString:"udp" forKey:@"transport_preference"];
		[self setBool:FALSE   forKey:@"avpf_preference"];

	}

	[self setBool:lp_config_get_int(conf, LINPHONERC_APPLICATION_KEY, "pushnotification_preference", 0) forKey:@"pushnotification_preference"];
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
		[self setInteger: linphone_core_get_upload_bandwidth(lc)   forKey:@"upload_bandwidth_preference"];
		[self setInteger: linphone_core_get_download_bandwidth(lc) forKey:@"download_bandwidth_preference"];
	}
	{
		[self setFloat:linphone_core_get_playback_gain_db(lc) forKey:@"playback_gain_preference"];
		[self setFloat:linphone_core_get_mic_gain_db(lc)      forKey:@"microphone_gain_preference"];
	}
	{
		int port = lp_config_get_int(conf, LINPHONERC_APPLICATION_KEY, "port_preference", 5060);
		[self setInteger:port forKey:@"port_preference"];
		int random_port_preference = lp_config_get_int(conf,LINPHONERC_APPLICATION_KEY,"random_port_preference", 1);
		[self setInteger:random_port_preference forKey:@"random_port_preference"];
	}
	{
		LinphoneAuthInfo *ai;
		const MSList *elem=linphone_core_get_auth_info_list(lc);
		if (elem && (ai=(LinphoneAuthInfo*)elem->data)){
			[self setString: linphone_auth_info_get_passwd(ai) forKey:@"password_preference"];
			[self setString: linphone_auth_info_get_ha1(ai)    forKey:@"ha1_preference"]; // hidden but useful if provisioned
			[self setString:linphone_auth_info_get_userid(ai)  forKey:@"userid_preference"];
		}
	}
	{
		[self setString: linphone_core_get_stun_server(lc) forKey:@"stun_preference"];
		[self setBool:linphone_core_get_firewall_policy(lc)==LinphonePolicyUseIce forKey:@"ice_preference"];
	}

	{
		[self transformCodecsToKeys: linphone_core_get_audio_codecs(lc)];
		[self transformCodecsToKeys: linphone_core_get_video_codecs(lc)];
		[self setBool:linphone_core_adaptive_rate_control_enabled(lc) forKey:@"adaptive_rate_control_preference"];
		[self setString:linphone_core_get_adaptive_rate_algorithm(lc) forKey:@"adaptive_rate_algorithm_preference"];
		
		[self setInteger:lp_config_get_int(conf, "audio", "codec_bitrate_limit", kLinphoneAudioVbrCodecDefaultBitrate) forKey:@"audio_codec_bitrate_limit_preference"];
        [self setInteger:lp_config_get_int(conf, LINPHONERC_APPLICATION_KEY, "voiceproc_preference", 1) forKey:@"voiceproc_preference"];
        [self setInteger:lp_config_get_int(conf, "sound", "eq_active", 0) forKey:@"eq_active"];
	}

	[self setBool:lp_config_get_int(conf, LINPHONERC_APPLICATION_KEY, "advanced_account_preference", 0) forKey:@"advanced_account_preference"];

	{
		LinphoneMediaEncryption menc=linphone_core_get_media_encryption(lc);
		const char *val;
		switch(menc){
			case LinphoneMediaEncryptionSRTP: val="SRTP"; break;
			case LinphoneMediaEncryptionZRTP: val="ZRTP"; break;
			default:                          val="None"; break;
		}
		[self setString:val forKey:@"media_encryption_preference"];
	}
	[self setBool: lp_config_get_int(conf, LINPHONERC_APPLICATION_KEY, "edge_opt_preference", 0) forKey:@"edge_opt_preference"];
	[self setBool: lp_config_get_int(conf, LINPHONERC_APPLICATION_KEY, "enable_first_login_view_preference", 0) forKey:@"enable_first_login_view_preference"];
	[self setBool: lp_config_get_int(conf, LINPHONERC_APPLICATION_KEY, "debugenable_preference", 0) forKey:@"debugenable_preference"];
	[self setBool: lp_config_get_int(conf, LINPHONERC_APPLICATION_KEY, "animations_preference", 1) forKey:@"animations_preference"];
	[self setBool: lp_config_get_int(conf, LINPHONERC_APPLICATION_KEY, "wifi_only_preference", 0) forKey:@"wifi_only_preference"];
	[self setString: lp_config_get_string(conf, LINPHONERC_APPLICATION_KEY, "sharing_server_preference", NULL) forKey:@"sharing_server_preference"];
	[self setBool:lp_config_get_int(conf, "sip", "use_ipv6", 0) forKey:@"use_ipv6"];


	[self setBool: lp_config_get_int(conf,LINPHONERC_APPLICATION_KEY,"start_at_boot_preference",1)  forKey:@"start_at_boot_preference"];
	[self setBool: lp_config_get_int(conf,LINPHONERC_APPLICATION_KEY,"backgroundmode_preference",1) forKey:@"backgroundmode_preference"];
	[self setBool: lp_config_get_int(conf,LINPHONERC_APPLICATION_KEY,"autoanswer_notif_preference",1) forKey:@"autoanswer_notif_preference"];


	{
		const LinphoneVideoPolicy *pol;
		[self setBool: linphone_core_video_enabled(lc) forKey:@"enable_video_preference"];
		pol=linphone_core_get_video_policy(lc);
		[self setBool:(pol->automatically_initiate) forKey:@"start_video_preference"];
		[self setBool:(pol->automatically_accept) forKey:@"accept_video_preference"];
		[self setBool:linphone_core_self_view_enabled(lc) forKey:@"self_video_preference"];
		BOOL previewEnabled=lp_config_get_int(conf,LINPHONERC_APPLICATION_KEY,"preview_preference",1);
		[self setBool:previewEnabled forKey:@"preview_preference"];
		MSVideoSize vsize = linphone_core_get_preferred_video_size(lc);
		int index;
		if ((vsize.width == MS_VIDEO_SIZE_720P_W) && (vsize.height == MS_VIDEO_SIZE_720P_H)) {
			index = 0;
		} else if ((vsize.width == MS_VIDEO_SIZE_VGA_W) && (vsize.height == MS_VIDEO_SIZE_VGA_H)) {
			index = 1;
		} else {
			index = 2;
		}
		[self setInteger:index forKey:@"video_preferred_size_preference"];
	}
	{
		[self setBool:linphone_core_get_use_info_for_dtmf(lc) forKey:@"sipinfo_dtmf_preference"];
		[self setBool:linphone_core_get_use_rfc2833_for_dtmf(lc) forKey:@"rfc_dtmf_preference"];

		[self setInteger:linphone_core_get_inc_timeout(lc) forKey:@"incoming_call_timeout_preference"];
		[self setInteger:linphone_core_get_in_call_timeout(lc) forKey:@"in_call_timeout_preference"];
	}

	// Tunnel
	if (linphone_core_tunnel_available()){
		LinphoneTunnel *tunnel = linphone_core_get_tunnel([LinphoneManager getLc]);
		[self setString:lp_config_get_string(linphone_core_get_config(lc), LINPHONERC_APPLICATION_KEY, "tunnel_mode_preference", "off") forKey:@"tunnel_mode_preference"];
		const MSList* configs = linphone_tunnel_get_servers(tunnel);
		if(configs != NULL) {
			LinphoneTunnelConfig *ltc = (LinphoneTunnelConfig *)configs->data;
			[self setString:linphone_tunnel_config_get_host(ltc) forKey:@"tunnel_address_preference"];
			[self setInteger:linphone_tunnel_config_get_port(ltc) forKey:@"tunnel_port_preference"];
		} else {
			[self setString:"" forKey:@"tunnel_address_preference"];
			[self setInteger:443 forKey:@"tunnel_port_preference"];
		}
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

- (void)alertAccountError:(NSString*)error {
	UIAlertView* alertview = [[UIAlertView alloc] initWithTitle:NSLocalizedString(@"Error", nil)
														message:error
													   delegate:nil
											  cancelButtonTitle:NSLocalizedString(@"OK", nil)
											  otherButtonTitles: nil];
	[alertview show];
	[alertview release];
}

+ (BOOL)hasSipPrefix:(NSString*)str {
    return [str hasPrefix:@"sip:"] || [str hasPrefix:@"sips:"];
}

- (void)synchronizeAccount {
	LinphoneCore *lc = [LinphoneManager getLc];
	LpConfig*   conf = linphone_core_get_config(lc);
	LinphoneProxyConfig* proxyCfg = NULL;
	BOOL isEditing = FALSE;
	NSString* error = nil;

	int port_preference = [self integerForKey:@"port_preference"];

	BOOL random_port_preference = [self boolForKey:@"random_port_preference"];
	lp_config_set_int(conf, LINPHONERC_APPLICATION_KEY, "random_port_preference", random_port_preference);
	if(random_port_preference) {
		port_preference = -1;
	}

	LCSipTransports transportValue={ port_preference, port_preference, -1, -1 };

	// will also update the sip_*_port section of the config
	if (linphone_core_set_sip_transports(lc, &transportValue)) {
		[LinphoneLogger logc:LinphoneLoggerError format:"cannot set transport"];
	}

	port_preference = linphone_core_get_sip_port(lc);
	[self setInteger:port_preference forKey:@"port_preference"]; // Update back preference


	BOOL enable_ipv6 = [self boolForKey:@"use_ipv6"];
	lp_config_set_int(conf, "sip", "use_ipv6", enable_ipv6);
	if( linphone_core_ipv6_enabled(lc) != enable_ipv6){
		[LinphoneLogger logc:LinphoneLoggerDebug format:"%@ IPV6", enable_ipv6?@"ENABLING":@"DISABLING"];
		linphone_core_enable_ipv6(lc, enable_ipv6);
	}


	//configure sip account

	//mandatory parameters
	NSString*        username = [self stringForKey:@"username_preference"];
	NSString*          userID = [self stringForKey:@"userid_preference"];
	NSString*          domain = [self stringForKey:@"domain_preference"];
	NSString*       transport = [self stringForKey:@"transport_preference"];
	NSString*      accountHa1 = [self stringForKey:@"ha1_preference"];
	NSString* accountPassword = [self stringForKey:@"password_preference"];
	bool      isOutboundProxy = [self boolForKey:@"outbound_proxy_preference"];
	BOOL             use_avpf = [self boolForKey:@"avpf_preference"];

	if (username && [username length] >0 && domain && [domain length]>0) {
		int             expire = [self integerForKey:@"expire_preference"];
		BOOL        isWifiOnly = [self boolForKey:@"wifi_only_preference"];
		BOOL  pushnotification = [self boolForKey:@"pushnotification_preference"];
		NSString*       prefix = [self stringForKey:@"prefix_preference"];
		NSString* proxyAddress = [self stringForKey:@"proxy_preference"];

		LinphoneAuthInfo *info  = NULL;
		const char* route       = NULL;

		if( isWifiOnly && [LinphoneManager instance].connectivity == wwan ) expire = 0;

		if ((!proxyAddress || [proxyAddress length] <1 ) && domain) {
			proxyAddress = domain;
        }

        if( ![LinphoneCoreSettingsStore hasSipPrefix:proxyAddress] ) {
			proxyAddress = [NSString stringWithFormat:@"sip:%@",proxyAddress];
		}

		char* proxy = ms_strdup([proxyAddress cStringUsingEncoding:[NSString defaultCStringEncoding]]);
		LinphoneAddress* proxy_addr = linphone_address_new(proxy);

		if( proxy_addr ){
			LinphoneTransportType type = LinphoneTransportUdp;
			if      ( [transport isEqualToString:@"tcp"] ) type = LinphoneTransportTcp;
			else if ( [transport isEqualToString:@"tls"] ) type = LinphoneTransportTls;

			linphone_address_set_transport(proxy_addr, type);
			ms_free(proxy);
			proxy = linphone_address_as_string_uri_only(proxy_addr);
		}

		// use proxy as route if outbound_proxy is enabled
		route = isOutboundProxy? proxy : NULL;

		//possible valid config detected, try to modify current proxy or create new one if none existing
		linphone_core_get_default_proxy(lc, &proxyCfg);
		if( proxyCfg == NULL ){
			proxyCfg = linphone_core_create_proxy_config(lc);
		} else {
			isEditing = TRUE;
			linphone_proxy_config_edit(proxyCfg);
		}

		char normalizedUserName[256];
		LinphoneAddress* linphoneAddress = linphone_address_new("sip:user@domain.com");
		linphone_proxy_config_normalize_number(proxyCfg, [username cStringUsingEncoding:[NSString defaultCStringEncoding]], normalizedUserName, sizeof(normalizedUserName));
		linphone_address_set_username(linphoneAddress, normalizedUserName);
		linphone_address_set_domain(linphoneAddress, [domain cStringUsingEncoding:[NSString defaultCStringEncoding]]);

		const char* identity = linphone_address_as_string_uri_only(linphoneAddress);
		const char* password = [accountPassword cStringUsingEncoding:[NSString defaultCStringEncoding]];
		const char*      ha1 = [accountHa1 cStringUsingEncoding:[NSString defaultCStringEncoding]];


		if( linphone_proxy_config_set_identity(proxyCfg, identity) == -1 ) { error = NSLocalizedString(@"Invalid username or domain",nil); goto bad_proxy;}
		if( linphone_proxy_config_set_server_addr(proxyCfg, proxy) == -1 ) { error = NSLocalizedString(@"Invalid proxy address", nil); goto bad_proxy; }
		if( linphone_proxy_config_set_route(proxyCfg, route) == -1 )       { error = NSLocalizedString(@"Invalid route", nil); goto bad_proxy; }

		if ([prefix length]>0) {
			linphone_proxy_config_set_dial_prefix(proxyCfg, [prefix cStringUsingEncoding:[NSString defaultCStringEncoding]]);
		}

		if ([self objectForKey:@"substitute_+_by_00_preference"]) {
			bool substitute_plus_by_00 = [self boolForKey:@"substitute_+_by_00_preference"];
			linphone_proxy_config_set_dial_escape_plus(proxyCfg,substitute_plus_by_00);
		}

		lp_config_set_int(conf, LINPHONERC_APPLICATION_KEY, "pushnotification_preference", pushnotification);
		[[LinphoneManager instance] configurePushTokenForProxyConfig:proxyCfg];

		linphone_proxy_config_enable_register(proxyCfg, true);
		linphone_proxy_config_enable_avpf(proxyCfg, use_avpf);
		linphone_proxy_config_set_expires(proxyCfg, expire);

		// setup auth info
		LinphoneAddress *from = linphone_address_new(identity);
		if (from != 0){
			const char* userid_str = (userID != nil)? [userID UTF8String] : NULL;
			info=linphone_auth_info_new(linphone_address_get_username(from),userid_str,password,ha1,NULL,linphone_proxy_config_get_domain(proxyCfg));
			linphone_address_destroy(from);
		}

		// We reached here without hitting the goto: the new settings are correct, so replace the previous ones.

		// add auth info
		linphone_core_clear_all_auth_info(lc);
		if( info ) { linphone_core_add_auth_info(lc,info); }

		// setup new proxycfg
		if( isEditing ){
			linphone_proxy_config_done(proxyCfg);
		} else {
			// was a new proxy config, add it
			linphone_core_add_proxy_config(lc, proxyCfg);
			linphone_core_set_default_proxy(lc,proxyCfg);
		}

	bad_proxy:
		if( linphoneAddress)
			linphone_address_destroy(linphoneAddress);
		if( proxy)
			ms_free(proxy);
		if( info )
			linphone_auth_info_destroy(info);

		// in case of error, show an alert to the user
		if( error != nil ){
			if( isEditing ) linphone_proxy_config_done(proxyCfg);
			else            linphone_proxy_config_destroy(proxyCfg);

			[self alertAccountError:error];
		}
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
		bool range = [match rangeAtIndex:2].length > 0;
		if(!range) {
			NSRange rangeMinPort = [match rangeAtIndex:1];
			*minPort = [LinphoneCoreSettingsStore validPort:[[text substringWithRange:rangeMinPort] intValue]];
			*maxPort = *minPort;
			return TRUE;
		} else {
			NSRange rangeMinPort = [match rangeAtIndex:1];
			*minPort = [LinphoneCoreSettingsStore validPort:[[text substringWithRange:rangeMinPort] intValue]];
			NSRange rangeMaxPort = [match rangeAtIndex:4];
			*maxPort = [LinphoneCoreSettingsStore validPort:[[text substringWithRange:rangeMaxPort] intValue]];
			if(*minPort > *maxPort) {
				*minPort = *maxPort;
			}
		   return TRUE;
		}
	}
	return FALSE;
}

- (BOOL)synchronize {
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
				|| [self valueChangedForKey:@"use_ipv6"]
				|| [self valueChangedForKey:@"avpf_preference"]
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

	LpConfig *config = linphone_core_get_config(lc);
	lp_config_set_int(config, "audio", "codec_bitrate_limit", [self integerForKey:@"audio_codec_bitrate_limit_preference"]);
	[[LinphoneManager instance] configureVbrCodecs];
	linphone_core_enable_adaptive_rate_control(lc, [self boolForKey:@"adaptive_rate_control_preference"]);
	linphone_core_set_adaptive_rate_algorithm(lc, [
                                                   [self stringForKey:@"adaptive_rate_algorithm_preference"] cStringUsingEncoding:[NSString defaultCStringEncoding]
                                                   ]);

	// Voice processing
	BOOL voice_processing = [self boolForKey:@"voiceproc_preference"];
    lp_config_set_int(config, LINPHONERC_APPLICATION_KEY, "voiceproc_preference", voice_processing);
	NSString* au_device = @"AU: Audio Unit Receiver";
	if( !voice_processing ){ au_device = @"AU: Audio Unit NoVoiceProc";	}
	linphone_core_set_capture_device(lc, [au_device UTF8String]);
	linphone_core_set_playback_device(lc, [au_device UTF8String]);

	BOOL equalizer = [self boolForKey:@"eq_active"];
	lp_config_set_int(config, "sound", "eq_active", equalizer);

    linphone_core_set_use_info_for_dtmf(lc, [self boolForKey:@"sipinfo_dtmf_preference"]);
    linphone_core_set_use_rfc2833_for_dtmf(lc, [self boolForKey:@"rfc_dtmf_preference"]);
    linphone_core_set_inc_timeout(lc, [self integerForKey:@"incoming_call_timeout_preference"]);
    linphone_core_set_in_call_timeout(lc, [self integerForKey:@"in_call_timeout_preference"]);
	lp_config_set_string(config, "app", "voice_mail_uri", [[self stringForKey:@"voice_mail_uri_preference"] UTF8String]);

	bool enableVideo = [self boolForKey:@"enable_video_preference"];
	linphone_core_enable_video(lc, enableVideo, enableVideo);

	NSString *menc = [self stringForKey:@"media_encryption_preference"];
	if (menc && [menc compare:@"SRTP"] == NSOrderedSame)
		linphone_core_set_media_encryption(lc, LinphoneMediaEncryptionSRTP);
	else if (menc && [menc compare:@"ZRTP"] == NSOrderedSame)
		linphone_core_set_media_encryption(lc, LinphoneMediaEncryptionZRTP);
	else
		linphone_core_set_media_encryption(lc, LinphoneMediaEncryptionNone);

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
	BOOL preview_preference=[self boolForKey:@"preview_preference"];
	lp_config_set_int(config, LINPHONERC_APPLICATION_KEY, "preview_preference", preview_preference);
	MSVideoSize vsize;
	int bw;
	switch ([self integerForKey:@"video_preferred_size_preference"]) {
		case 0:
			MS_VIDEO_SIZE_ASSIGN(vsize, 720P);
			// 128 = margin for audio, the BW includes both video and audio
			bw = 1024 + 128;
			break;
		case 1:
			MS_VIDEO_SIZE_ASSIGN(vsize, VGA);
			// no margin for VGA or QVGA, because video encoders can encode the
			// target resulution in less than the asked bandwidth
			bw = 512;
			break;
		case 2:
		default:
			MS_VIDEO_SIZE_ASSIGN(vsize, QVGA);
			bw = 380;
			break;
	}
	linphone_core_set_preferred_video_size(lc, vsize);
	[self setInteger: bw forKey:@"upload_bandwidth_preference"];
	[self setInteger: bw forKey:@"download_bandwidth_preference"];

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

	lp_config_set_int(config, LINPHONERC_APPLICATION_KEY, "backgroundmode_preference", isbackgroundModeEnabled);
	lp_config_set_int(config, LINPHONERC_APPLICATION_KEY, "start_at_boot_preference", [self boolForKey:@"start_at_boot_preference"]);
	lp_config_set_int(config, LINPHONERC_APPLICATION_KEY, "autoanswer_notif_preference", [self boolForKey:@"autoanswer_notif_preference"]);
	lp_config_set_int(config, LINPHONERC_APPLICATION_KEY, "advanced_account_preference", [self boolForKey:@"advanced_account_preference"]);

	BOOL firstloginview = [self boolForKey:@"enable_first_login_view_preference"];
	lp_config_set_int(config, LINPHONERC_APPLICATION_KEY, "enable_first_login_view_preference", firstloginview);

	BOOL edgeOpt = [self boolForKey:@"edge_opt_preference"];
	lp_config_set_int(config, LINPHONERC_APPLICATION_KEY, "edge_opt_preference", edgeOpt);

	BOOL debugmode = [self boolForKey:@"debugenable_preference"];
	lp_config_set_int(config, LINPHONERC_APPLICATION_KEY, "debugenable_preference", debugmode);
	[[LinphoneManager instance] setLogsEnabled:debugmode];
	
	BOOL animations = [self boolForKey:@"animations_preference"];
	lp_config_set_int(config, LINPHONERC_APPLICATION_KEY, "animations_preference", animations);

	BOOL wifiOnly = [self boolForKey:@"wifi_only_preference"];
	lp_config_set_int(config, LINPHONERC_APPLICATION_KEY, "wifi_only_preference", wifiOnly);
	if([self valueChangedForKey:@"wifi_only_preference"]) {
		[[LinphoneManager instance] setupNetworkReachabilityCallback];
	}

	NSString*  sharing_server = [self stringForKey:@"sharing_server_preference"];
	[[LinphoneManager instance] lpConfigSetString:sharing_server forKey:@"sharing_server_preference"];


	//Tunnel
	if (linphone_core_tunnel_available()){
		NSString* lTunnelPrefMode = [self stringForKey:@"tunnel_mode_preference"];
		NSString* lTunnelPrefAddress = [self stringForKey:@"tunnel_address_preference"];
		int lTunnelPrefPort = [self integerForKey:@"tunnel_port_preference"];
		LinphoneTunnel *tunnel = linphone_core_get_tunnel([LinphoneManager getLc]);
		TunnelMode mode = tunnel_off;
		int lTunnelPort = 443;
		if (lTunnelPrefPort) {
			lTunnelPort = lTunnelPrefPort;
		}

		linphone_tunnel_clean_servers(tunnel);
		if (lTunnelPrefAddress && [lTunnelPrefAddress length]) {
			LinphoneTunnelConfig *ltc = linphone_tunnel_config_new();
			linphone_tunnel_config_set_host(ltc, [lTunnelPrefAddress UTF8String]);
			linphone_tunnel_config_set_port(ltc, lTunnelPort);
			linphone_tunnel_add_server(tunnel, ltc);

			if ([lTunnelPrefMode isEqualToString:@"off"]) {
				mode = tunnel_off;
			} else if ([lTunnelPrefMode isEqualToString:@"on"]) {
				mode = tunnel_on;
			} else if ([lTunnelPrefMode isEqualToString:@"wwan"]) {
				mode = tunnel_wwan;
			} else if ([lTunnelPrefMode isEqualToString:@"auto"]) {
				mode = tunnel_auto;
			} else {
				[LinphoneLogger logc:LinphoneLoggerError format:"Unexpected tunnel mode [%s]",[lTunnelPrefMode cStringUsingEncoding:[NSString defaultCStringEncoding]]];
			}
		}

		lp_config_set_string(linphone_core_get_config(lc), LINPHONERC_APPLICATION_KEY, "tunnel_mode_preference", [lTunnelPrefMode UTF8String]);
		[[LinphoneManager instance] setTunnelMode:mode];
	}

	[changedDict release];
	changedDict = [[NSMutableDictionary alloc] init];

	// Post event
	NSDictionary *eventDic = [NSDictionary dictionaryWithObject:self forKey:@"settings"];
	[[NSNotificationCenter defaultCenter] postNotificationName:kLinphoneSettingsUpdate object:self userInfo:eventDic];

	return YES;
}

@end
