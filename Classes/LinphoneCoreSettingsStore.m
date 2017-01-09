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
#import "Utils.h"
#import "PhoneMainView.h"

#include "linphone/linphone_tunnel.h"
#include "linphone/lpconfig.h"
#include <stdio.h>
#include <stdlib.h>

@implementation LinphoneCoreSettingsStore

- (id)init {
	self = [super init];
	if (self) {
		dict = [[NSMutableDictionary alloc] init];
		changedDict = [[NSMutableDictionary alloc] init];
	}
	return self;
}

- (void)setCString:(const char *)value forKey:(NSString *)key {
	id obj = @"";
	if (value)
		obj = [[NSString alloc] initWithCString:value encoding:NSUTF8StringEncoding];
	[self setObject:obj forKey:key];
}

- (NSString *)stringForKey:(NSString *)key {
	return [self objectForKey:key];
}

- (void)setObject:(id)value forKey:(NSString *)key {
	[dict setValue:value forKey:key];
	[changedDict setValue:[NSNumber numberWithBool:TRUE] forKey:key];
}

- (id)objectForKey:(NSString *)key {
	return [dict valueForKey:key];
}

- (BOOL)valueChangedForKey:(NSString *)key {
	return [[changedDict valueForKey:key] boolValue];
}

+ (int)validPort:(int)port {
	if (port < 0) {
		return 0;
	}
	if (port > 65535) {
		return 65535;
	}
	return port;
}

+ (BOOL)parsePortRange:(NSString *)text minPort:(int *)minPort maxPort:(int *)maxPort {
	/*NSError *error = nil;
	*minPort = -1;
	*maxPort = -1;
	NSRegularExpression *regex =
		[NSRegularExpression regularExpressionWithPattern:@"([0-9]+)(([^0-9]+)([0-9]+))?" options:0 error:&error];
	if (error != NULL)
		return FALSE;
	NSArray *matches = [regex matchesInString:text options:0 range:NSMakeRange(0, [text length])];
	if ([matches count] == 1) {
		NSTextCheckingResult *match = [matches objectAtIndex:0];
		bool range = [match rangeAtIndex:2].length > 0;
		if (!range) {
			NSRange rangeMinPort = [match rangeAtIndex:1];
			*minPort = [LinphoneCoreSettingsStore validPort:[[text substringWithRange:rangeMinPort] intValue]];
			*maxPort = *minPort;
			return TRUE;
		} else {
			NSRange rangeMinPort = [match rangeAtIndex:1];
			*minPort = [LinphoneCoreSettingsStore validPort:[[text substringWithRange:rangeMinPort] intValue]];
			NSRange rangeMaxPort = [match rangeAtIndex:4];
			*maxPort = [LinphoneCoreSettingsStore validPort:[[text substringWithRange:rangeMaxPort] intValue]];
			if (*minPort > *maxPort) {
				*minPort = *maxPort;
			}
			return TRUE;
		}
	}*/
	int err;

	err = sscanf(text.UTF8String, "%i - %i", minPort, maxPort);
	if (err == 0) {
		*minPort = *maxPort = -1;
	} else if (err == 1) {
		*maxPort = -1;
	}

	// Minimal port allowed
	if (*minPort < 1024) {
		*minPort = -1;
	}
	// Maximal port allowed
	if (*maxPort > 65535) {
		*maxPort = -1;
	}
	// minPort must be inferior or equal to maxPort
	if (*minPort > *maxPort) {
		*maxPort = *minPort;
	}

	return TRUE;
}

- (void)transformCodecsToKeys:(const MSList *)codecs {

	const MSList *elem = codecs;
	for (; elem != NULL; elem = elem->next) {
		PayloadType *pt = (PayloadType *)elem->data;
		NSString *pref = [LinphoneManager getPreferenceForCodec:pt->mime_type withRate:pt->clock_rate];
		if (pref) {
			bool_t value = linphone_core_payload_type_enabled(LC, pt);
			[self setBool:value forKey:pref];
		} else {
			LOGW(@"Codec %s/%i supported by core is not shown in iOS app config view.", pt->mime_type, pt->clock_rate);
		}
	}
}

- (void)transformAccountToKeys:(NSString *)username {
	const MSList *proxies = linphone_core_get_proxy_config_list(LC);
	while (username && proxies &&
		   strcmp(username.UTF8String,
				  linphone_address_get_username(linphone_proxy_config_get_identity_address(proxies->data))) != 0) {
		proxies = proxies->next;
	}
	LinphoneProxyConfig *proxy = NULL;

	// default values
	{
		[self setBool:NO forKey:@"account_pushnotification_preference"];
		[self setObject:@"" forKey:@"account_mandatory_username_preference"];
		[self setObject:@"" forKey:@"account_mandatory_domain_preference"];
		[self setCString:"" forKey:@"account_display_name_preference"];
		[self setObject:@"" forKey:@"account_proxy_preference"];
		[self setObject:@"udp" forKey:@"account_transport_preference"];
		[self setBool:NO forKey:@"account_outbound_proxy_preference"];
		[self setBool:NO forKey:@"account_avpf_preference"];
		[self setBool:YES forKey:@"account_is_default_preference"];
		[self setBool:YES forKey:@"account_is_enabled_preference"];
		[self setCString:"" forKey:@"account_userid_preference"];
		[self setCString:"" forKey:@"account_mandatory_password_preference"];
		[self setCString:"" forKey:@"ha1_preference"];
		[self setInteger:-1 forKey:@"account_expire_preference"];
		[self setInteger:-1 forKey:@"current_proxy_config_preference"];
		[self setCString:"" forKey:@"account_prefix_preference"];
		[self setBool:NO forKey:@"account_substitute_+_by_00_preference"];
	}

	if (proxies) {
		proxy = proxies->data;
		// root section
		{
			const char *refkey = linphone_proxy_config_get_ref_key(proxy);
			if (refkey) {
				BOOL pushEnabled = (strcmp(refkey, "push_notification") == 0);
				[self setBool:pushEnabled forKey:@"account_pushnotification_preference"];
			}
			const LinphoneAddress *identity_addr = linphone_proxy_config_get_identity_address(proxy);
			if (identity_addr) {
				const char *server_addr = linphone_proxy_config_get_server_addr(proxy);
				LinphoneAddress *proxy_addr = linphone_core_interpret_url(LC, server_addr);
				int port = linphone_address_get_port(proxy_addr);

				[self setCString:linphone_address_get_username(identity_addr)
						  forKey:@"account_mandatory_username_preference"];
				[self setCString:linphone_address_get_display_name(identity_addr)
						  forKey:@"account_display_name_preference"];
				[self setCString:linphone_address_get_domain(identity_addr)
						  forKey:@"account_mandatory_domain_preference"];
				if (strcmp(linphone_address_get_domain(identity_addr), linphone_address_get_domain(proxy_addr)) != 0 ||
					port > 0) {
					char tmp[256] = {0};
					if (port > 0) {
						snprintf(tmp, sizeof(tmp) - 1, "%s:%i", linphone_address_get_domain(proxy_addr), port);
					} else
						snprintf(tmp, sizeof(tmp) - 1, "%s", linphone_address_get_domain(proxy_addr));
					[self setCString:tmp forKey:@"account_proxy_preference"];
				}
				const char *tname = "udp";
				switch (linphone_address_get_transport(proxy_addr)) {
					case LinphoneTransportTcp:
						tname = "tcp";
						break;
					case LinphoneTransportTls:
						tname = "tls";
						break;
					default:
						break;
				}
				linphone_address_destroy(proxy_addr);
				[self setCString:tname forKey:@"account_transport_preference"];
			}

			[self setBool:(linphone_proxy_config_get_route(proxy) != NULL) forKey:@"account_outbound_proxy_preference"];
			[self setBool:linphone_proxy_config_avpf_enabled(proxy) forKey:@"account_avpf_preference"];
			[self setBool:linphone_proxy_config_register_enabled(proxy) forKey:@"account_is_enabled_preference"];
			[self setBool:(linphone_core_get_default_proxy_config(LC) == proxy)
				   forKey:@"account_is_default_preference"];

			const LinphoneAuthInfo *ai = linphone_core_find_auth_info(
				LC, NULL, [self stringForKey:@"account_mandatory_username_preference"].UTF8String,
				[self stringForKey:@"account_mandatory_domain_preference"].UTF8String);
			if (ai) {
				[self setCString:linphone_auth_info_get_userid(ai) forKey:@"account_userid_preference"];
				[self setCString:linphone_auth_info_get_passwd(ai) forKey:@"account_mandatory_password_preference"];
				// hidden but useful if provisioned
				[self setCString:linphone_auth_info_get_ha1(ai) forKey:@"ha1_preference"];
			}

			int idx = (int)bctbx_list_index(linphone_core_get_proxy_config_list(LC), proxy);
			[self setInteger:idx forKey:@"current_proxy_config_preference"];

			int expires = linphone_proxy_config_get_expires(proxy);
			[self setInteger:expires forKey:@"account_expire_preference"];
		}

		// call section
		{
			const char *dial_prefix = linphone_proxy_config_get_dial_prefix(proxy);
			[self setCString:dial_prefix forKey:@"account_prefix_preference"];
			BOOL dial_escape_plus = linphone_proxy_config_get_dial_escape_plus(proxy);
			[self setBool:dial_escape_plus forKey:@"account_substitute_+_by_00_preference"];
		}
	}
}

- (void)transformLinphoneCoreToKeys {
	LinphoneManager *lm = LinphoneManager.instance;

	// root section
	{
		const bctbx_list_t *accounts = linphone_core_get_proxy_config_list(LC);
		size_t count = bctbx_list_size(accounts);
		for (size_t i = 1; i <= count; i++, accounts = accounts->next) {
			NSString *key = [NSString stringWithFormat:@"menu_account_%lu", i];
			LinphoneProxyConfig *proxy = (LinphoneProxyConfig *)accounts->data;
			[self setCString:linphone_address_get_username(linphone_proxy_config_get_identity_address(proxy))
					  forKey:key];
		}

		[self setBool:linphone_core_video_display_enabled(LC) forKey:@"enable_video_preference"];
		[self setBool:[LinphoneManager.instance lpConfigBoolForKey:@"auto_answer"]
			   forKey:@"enable_auto_answer_preference"];
		[self setBool:[lm lpConfigBoolForKey:@"account_mandatory_advanced_preference"]
			   forKey:@"account_mandatory_advanced_preference"];
	}

	// account section
	{ [self transformAccountToKeys:nil]; }

	// audio section
	{
		[self transformCodecsToKeys:linphone_core_get_audio_codecs(LC)];
		[self setFloat:linphone_core_get_playback_gain_db(LC) forKey:@"playback_gain_preference"];
		[self setFloat:linphone_core_get_mic_gain_db(LC) forKey:@"microphone_gain_preference"];
		[self setInteger:[lm lpConfigIntForKey:@"codec_bitrate_limit"
									 inSection:@"audio"
								   withDefault:kLinphoneAudioVbrCodecDefaultBitrate]
				  forKey:@"audio_codec_bitrate_limit_preference"];
		[self setInteger:[lm lpConfigIntForKey:@"voiceproc_preference" withDefault:1] forKey:@"voiceproc_preference"];
		[self setInteger:[lm lpConfigIntForKey:@"eq_active" inSection:@"sound" withDefault:0] forKey:@"eq_active"];
	}

	// video section
	{
		[self transformCodecsToKeys:linphone_core_get_video_codecs(LC)];

		const LinphoneVideoPolicy *pol;
		pol = linphone_core_get_video_policy(LC);
		[self setBool:(pol->automatically_initiate) forKey:@"start_video_preference"];
		[self setBool:(pol->automatically_accept) forKey:@"accept_video_preference"];
		[self setBool:linphone_core_self_view_enabled(LC) forKey:@"self_video_preference"];
		BOOL previewEnabled = [lm lpConfigBoolForKey:@"preview_preference" withDefault:YES];
		[self setBool:IPAD && previewEnabled forKey:@"preview_preference"];

		const char *preset = linphone_core_get_video_preset(LC);
		[self setCString:preset ? preset : "default" forKey:@"video_preset_preference"];
		MSVideoSize vsize = linphone_core_get_preferred_video_size(LC);
		int index;
		if ((vsize.width == MS_VIDEO_SIZE_720P_W) && (vsize.height == MS_VIDEO_SIZE_720P_H)) {
			index = 0;
		} else if ((vsize.width == MS_VIDEO_SIZE_VGA_W) && (vsize.height == MS_VIDEO_SIZE_VGA_H)) {
			index = 1;
		} else {
			index = 2;
		}
		[self setInteger:index forKey:@"video_preferred_size_preference"];
		[self setInteger:linphone_core_get_preferred_framerate(LC) forKey:@"video_preferred_fps_preference"];
		[self setInteger:linphone_core_get_download_bandwidth(LC) forKey:@"download_bandwidth_preference"];
	}

	// call section
	{
		[self setBool:linphone_core_get_use_info_for_dtmf(LC) forKey:@"sipinfo_dtmf_preference"];
		[self setBool:linphone_core_get_use_rfc2833_for_dtmf(LC) forKey:@"rfc_dtmf_preference"];

		[self setInteger:linphone_core_get_inc_timeout(LC) forKey:@"incoming_call_timeout_preference"];
		[self setInteger:linphone_core_get_in_call_timeout(LC) forKey:@"in_call_timeout_preference"];

		[self setBool:[lm lpConfigBoolForKey:@"repeat_call_notification"]
			   forKey:@"repeat_call_notification_preference"];
	}

	// chat section
	{
		[self setInteger:linphone_core_lime_enabled(LC) forKey:@"use_lime_preference"];
		[self setCString:linphone_core_get_file_transfer_server(LC) forKey:@"file_transfer_server_url_preference"];
	}

	// network section
	{
		[self setBool:[lm lpConfigBoolForKey:@"edge_opt_preference" withDefault:NO] forKey:@"edge_opt_preference"];
		[self setBool:[lm lpConfigBoolForKey:@"wifi_only_preference" withDefault:NO] forKey:@"wifi_only_preference"];
		[self setCString:linphone_core_get_stun_server(LC) forKey:@"stun_preference"];
		[self setBool:linphone_nat_policy_ice_enabled(linphone_core_get_nat_policy(LC)) forKey:@"ice_preference"];
		[self setBool:linphone_nat_policy_turn_enabled(linphone_core_get_nat_policy(LC)) forKey:@"turn_preference"];
		[self setCString:linphone_nat_policy_get_stun_server_username(linphone_core_get_nat_policy(LC))
				  forKey:@"turn_username"];

		int random_port_preference = [lm lpConfigIntForKey:@"random_port_preference" withDefault:1];
		[self setInteger:random_port_preference forKey:@"random_port_preference"];
		int port = [lm lpConfigIntForKey:@"port_preference" withDefault:5060];
		[self setInteger:port forKey:@"port_preference"];
		{
			int minPort, maxPort;
			linphone_core_get_audio_port_range(LC, &minPort, &maxPort);
			if (minPort != maxPort)
				[self setObject:[NSString stringWithFormat:@"%d-%d", minPort, maxPort] forKey:@"audio_port_preference"];
			else
				[self setObject:[NSString stringWithFormat:@"%d", minPort] forKey:@"audio_port_preference"];
		}
		{
			int minPort, maxPort;
			linphone_core_get_video_port_range(LC, &minPort, &maxPort);
			if (minPort != maxPort)
				[self setObject:[NSString stringWithFormat:@"%d-%d", minPort, maxPort] forKey:@"video_port_preference"];
			else
				[self setObject:[NSString stringWithFormat:@"%d", minPort] forKey:@"video_port_preference"];
		}
		[self setBool:linphone_core_ipv6_enabled(LC) forKey:@"use_ipv6"];
		LinphoneMediaEncryption menc = linphone_core_get_media_encryption(LC);
		const char *val;
		switch (menc) {
			case LinphoneMediaEncryptionSRTP:
				val = "SRTP";
				break;
			case LinphoneMediaEncryptionZRTP:
				val = "ZRTP";
				break;
			case LinphoneMediaEncryptionDTLS:
				val = "DTLS";
				break;
			case LinphoneMediaEncryptionNone:
				val = "None";
				break;
		}
		[self setCString:val forKey:@"media_encryption_preference"];
		[self setInteger:linphone_core_get_upload_bandwidth(LC) forKey:@"upload_bandwidth_preference"];
		[self setInteger:linphone_core_get_download_bandwidth(LC) forKey:@"download_bandwidth_preference"];
		[self setBool:linphone_core_adaptive_rate_control_enabled(LC) forKey:@"adaptive_rate_control_preference"];
	}

	// tunnel section
	if (linphone_core_tunnel_available()) {
		LinphoneTunnel *tunnel = linphone_core_get_tunnel(LC);
		[self setObject:[lm lpConfigStringForKey:@"tunnel_mode_preference" withDefault:@"off"]
				 forKey:@"tunnel_mode_preference"];
		const MSList *configs = linphone_tunnel_get_servers(tunnel);
		if (configs != NULL) {
			LinphoneTunnelConfig *ltc = (LinphoneTunnelConfig *)configs->data;
			[self setCString:linphone_tunnel_config_get_host(ltc) forKey:@"tunnel_address_preference"];
			[self setInteger:linphone_tunnel_config_get_port(ltc) forKey:@"tunnel_port_preference"];
		} else {
			[self setCString:"" forKey:@"tunnel_address_preference"];
			[self setInteger:443 forKey:@"tunnel_port_preference"];
		}
	}

	// advanced section
	{
		[self setObject:[lm lpConfigStringForKey:@"debugenable_preference"] forKey:@"debugenable_preference"];
		[self setBool:ANIMATED forKey:@"animations_preference"];
		[self setBool:[lm lpConfigBoolForKey:@"backgroundmode_preference"] forKey:@"backgroundmode_preference"];
		[self setBool:[lm lpConfigBoolForKey:@"start_at_boot_preference"] forKey:@"start_at_boot_preference"];
		[self setBool:[lm lpConfigBoolForKey:@"autoanswer_notif_preference"] forKey:@"autoanswer_notif_preference"];
		[self setBool:[lm lpConfigBoolForKey:@"show_msg_in_notif" withDefault:YES] forKey:@"show_msg_in_notif"];
		[self setBool:[lm lpConfigBoolForKey:@"enable_first_login_view_preference"]
			   forKey:@"enable_first_login_view_preference"];
		LinphoneAddress *parsed = linphone_core_get_primary_contact_parsed(LC);
		if (parsed != NULL) {
			[self setCString:linphone_address_get_display_name(parsed) forKey:@"primary_displayname_preference"];
			[self setCString:linphone_address_get_username(parsed) forKey:@"primary_username_preference"];
			linphone_address_destroy(parsed);
		}
	}

	changedDict = [[NSMutableDictionary alloc] init];

	// Post event
	NSDictionary *eventDic = [NSDictionary dictionaryWithObject:self forKey:@"settings"];
	[NSNotificationCenter.defaultCenter postNotificationName:kLinphoneLogsUpdate object:self userInfo:eventDic];
}

- (void)alertAccountError:(NSString *)error {
	UIAlertController *errView = [UIAlertController alertControllerWithTitle:NSLocalizedString(@"Error", nil)
																	 message:error
															  preferredStyle:UIAlertControllerStyleAlert];
	
	UIAlertAction* defaultAction = [UIAlertAction actionWithTitle:NSLocalizedString(@"OK", nil)
															style:UIAlertActionStyleDefault
														  handler:^(UIAlertAction * action) {}];
	
	[errView addAction:defaultAction];
	[PhoneMainView.instance presentViewController:errView animated:YES completion:nil];
}

- (void)synchronizeAccounts {
	LOGI(@"Account changed, synchronizing.");
	LinphoneManager *lm = LinphoneManager.instance;
	LinphoneProxyConfig *proxyCfg = NULL;
	NSString *error = nil;

	int port_preference = [self integerForKey:@"port_preference"];

	BOOL random_port_preference = [self boolForKey:@"random_port_preference"];
	[lm lpConfigSetInt:random_port_preference forKey:@"random_port_preference"];
	if (random_port_preference) {
		port_preference = -1;
	}

	LCSipTransports transportValue = {port_preference, port_preference, -1, -1};

	// will also update the sip_*_port section of the config
	if (linphone_core_set_sip_transports(LC, &transportValue)) {
		LOGE(@"cannot set transport");
	}

	port_preference = linphone_core_get_sip_port(LC);
	[self setInteger:port_preference forKey:@"port_preference"]; // Update back preference

	BOOL enable_ipv6 = [self boolForKey:@"use_ipv6"];
	[lm lpConfigSetBool:enable_ipv6 forKey:@"use_ipv6" inSection:@"sip"];
	LOGD(@"%@ IPV6", enable_ipv6 ? @"ENABLING" : @"DISABLING");
	linphone_core_enable_ipv6(LC, enable_ipv6);

	// configure sip account

	// mandatory parameters
	NSString *username = [self stringForKey:@"account_mandatory_username_preference"];
	NSString *displayName = [self stringForKey:@"account_display_name_preference"];
	NSString *userID = [self stringForKey:@"account_userid_preference"];
	NSString *domain = [self stringForKey:@"account_mandatory_domain_preference"];
	NSString *transport = [self stringForKey:@"account_transport_preference"];
	NSString *accountHa1 = [self stringForKey:@"ha1_preference"];
	NSString *accountPassword = [self stringForKey:@"account_mandatory_password_preference"];
	BOOL isOutboundProxy = [self boolForKey:@"account_outbound_proxy_preference"];
	BOOL use_avpf = [self boolForKey:@"account_avpf_preference"];
	BOOL is_default = [self boolForKey:@"account_is_default_preference"];
	BOOL is_enabled = [self boolForKey:@"account_is_enabled_preference"];

	if (username && [username length] > 0 && domain && [domain length] > 0) {
		int expire = [self integerForKey:@"account_expire_preference"];
		BOOL isWifiOnly = [self boolForKey:@"wifi_only_preference"];
		BOOL pushnotification = [self boolForKey:@"account_pushnotification_preference"];
		NSString *prefix = [self stringForKey:@"account_prefix_preference"];
		NSString *proxyAddress = [self stringForKey:@"account_proxy_preference"];

		const char *route = NULL;

		if (isWifiOnly && LinphoneManager.instance.connectivity == wwan)
			expire = 0;

		if ((!proxyAddress || [proxyAddress length] < 1) && domain) {
			proxyAddress = domain;
		}

		if (![proxyAddress hasPrefix:@"sip:"] && ![proxyAddress hasPrefix:@"sips:"]) {
			proxyAddress = [NSString stringWithFormat:@"sip:%@", proxyAddress];
		}

		char *proxy = ms_strdup(proxyAddress.UTF8String);
		LinphoneAddress *proxy_addr = linphone_core_interpret_url(LC, proxy);

		if (proxy_addr) {
			LinphoneTransportType type = LinphoneTransportUdp;
			if ([transport isEqualToString:@"tcp"])
				type = LinphoneTransportTcp;
			else if ([transport isEqualToString:@"tls"])
				type = LinphoneTransportTls;

			linphone_address_set_transport(proxy_addr, type);
			ms_free(proxy);
			proxy = linphone_address_as_string_uri_only(proxy_addr);
		}

		proxyCfg = bctbx_list_nth_data(linphone_core_get_proxy_config_list(LC),
									   [self integerForKey:@"current_proxy_config_preference"]);
		// if account was deleted, it is not present anymore
		if (proxyCfg == NULL) {
			goto bad_proxy;
		}

		LinphoneAddress *linphoneAddress = linphone_core_interpret_url(LC, "sip:user@domain.com");
		linphone_address_set_username(linphoneAddress, username.UTF8String);
		if ([LinphoneManager.instance lpConfigBoolForKey:@"use_phone_number" inSection:@"assistant"]) {
			char *user = linphone_proxy_config_normalize_phone_number(proxyCfg, username.UTF8String);
			if (user) {
				linphone_address_set_username(linphoneAddress, user);
				ms_free(user);
			}
		}
		linphone_address_set_domain(linphoneAddress, [domain UTF8String]);
		linphone_address_set_display_name(linphoneAddress, (displayName.length ? displayName.UTF8String : NULL));
		const char *identity = linphone_address_as_string(linphoneAddress);
		linphone_address_destroy(linphoneAddress);
		const char *password = [accountPassword UTF8String];
		const char *ha1 = [accountHa1 UTF8String];

		if (linphone_proxy_config_set_identity(proxyCfg, identity) == -1) {
			error = NSLocalizedString(@"Invalid username or domain", nil);
			goto bad_proxy;
		}
		// use proxy as route if outbound_proxy is enabled
		route = isOutboundProxy ? proxy : NULL;
		if (linphone_proxy_config_set_server_addr(proxyCfg, proxy) == -1) {
			error = NSLocalizedString(@"Invalid proxy address", nil);
			goto bad_proxy;
		}
		if (linphone_proxy_config_set_route(proxyCfg, route) == -1) {
			error = NSLocalizedString(@"Invalid route", nil);
			goto bad_proxy;
		}

		if ([prefix length] > 0) {
			linphone_proxy_config_set_dial_prefix(proxyCfg, [prefix UTF8String]);
		}

		if ([self objectForKey:@"account_substitute_+_by_00_preference"]) {
			bool substitute_plus_by_00 = [self boolForKey:@"account_substitute_+_by_00_preference"];
			linphone_proxy_config_set_dial_escape_plus(proxyCfg, substitute_plus_by_00);
		}

		// use empty string "" instead of NULL to avoid being overwritten by default proxy config values
		linphone_proxy_config_set_ref_key(proxyCfg, pushnotification ? "push_notification" : "no_push_notification");
		[LinphoneManager.instance configurePushTokenForProxyConfig:proxyCfg];

		linphone_proxy_config_enable_register(proxyCfg, is_enabled);
		linphone_proxy_config_enable_avpf(proxyCfg, use_avpf);
		linphone_proxy_config_set_expires(proxyCfg, expire);
		if (is_default) {
			linphone_core_set_default_proxy_config(LC, proxyCfg);
		} else if (linphone_core_get_default_proxy_config(LC) == proxyCfg) {
			linphone_core_set_default_proxy_config(LC, NULL);
		}

		LinphoneAuthInfo *proxyAi = (LinphoneAuthInfo *)linphone_proxy_config_find_auth_info(proxyCfg);

		// setup new proxycfg
		linphone_proxy_config_done(proxyCfg);

		// modify auth info only after finishing editting the proxy config, so that
		// UNREGISTER succeed
		if (proxyAi) {
			linphone_core_remove_auth_info(LC, proxyAi);
		}
		if (strcmp(password,"") == 0) {
			password = NULL;
		}
		
		LinphoneAddress *from = linphone_core_interpret_url(LC, identity);
		if (from) {
			const char *userid_str = (userID != nil) ? [userID UTF8String] : NULL;
			LinphoneAuthInfo *info = linphone_auth_info_new(
				linphone_address_get_username(from), userid_str, password ? password : NULL, password ? NULL : ha1,
				linphone_proxy_config_get_realm(proxyCfg), linphone_proxy_config_get_domain(proxyCfg));
			linphone_address_destroy(from);
			linphone_core_add_auth_info(LC, info);
			linphone_auth_info_destroy(info);
		}

	bad_proxy:
		if (proxy)
			ms_free(proxy);

		// in case of error, show an alert to the user
		if (error != nil) {
			linphone_proxy_config_done(proxyCfg);
			UIAlertController *errView = [UIAlertController alertControllerWithTitle:NSLocalizedString(@"Error", nil)
																			 message:error
																	  preferredStyle:UIAlertControllerStyleAlert];
			
			UIAlertAction* defaultAction = [UIAlertAction actionWithTitle:NSLocalizedString(@"OK", nil)
																	style:UIAlertActionStyleDefault
																  handler:^(UIAlertAction * action) {}];
			
			[errView addAction:defaultAction];
			[PhoneMainView.instance presentViewController:errView animated:YES completion:nil];
		}
	}
	// reload address book to prepend proxy config domain to contacts' phone number
	[[LinphoneManager.instance fastAddressBook] reload];
}

- (void)synchronizeCodecs:(const MSList *)codecs {
	PayloadType *pt;
	const MSList *elem;

	for (elem = codecs; elem != NULL; elem = elem->next) {
		pt = (PayloadType *)elem->data;
		NSString *pref = [LinphoneManager getPreferenceForCodec:pt->mime_type withRate:pt->clock_rate];
		linphone_core_enable_payload_type(LC, pt, [self boolForKey:pref]);
	}
}

- (BOOL)synchronize {
	//@try {

	LinphoneManager *lm = LinphoneManager.instance;
	// root section
	{
		BOOL account_changed = NO;
		for (NSString *key in self->changedDict) {
			if ([key hasPrefix:@"account_"] && [self valueChangedForKey:key]) {
				account_changed = YES;
				break;
			}
		}
		account_changed |= [self valueChangedForKey:@"port_preference"];
		account_changed |= [self valueChangedForKey:@"random_port_preference"];
		account_changed |= [self valueChangedForKey:@"use_ipv6"];

		if (account_changed)
			[self synchronizeAccounts];

		bool enableVideo = [self boolForKey:@"enable_video_preference"];
		linphone_core_enable_video_capture(LC, enableVideo);
		linphone_core_enable_video_display(LC, enableVideo);

		bool enableAutoAnswer = [self boolForKey:@"enable_auto_answer_preference"];
		[LinphoneManager.instance lpConfigSetBool:enableAutoAnswer forKey:@"auto_answer"];
		}

		// audio section
		{
			[self synchronizeCodecs:linphone_core_get_audio_codecs(LC)];

			float playback_gain = [self floatForKey:@"playback_gain_preference"];
			linphone_core_set_playback_gain_db(LC, playback_gain);

			float mic_gain = [self floatForKey:@"microphone_gain_preference"];
			linphone_core_set_mic_gain_db(LC, mic_gain);

			[lm lpConfigSetInt:[self integerForKey:@"audio_codec_bitrate_limit_preference"]
						forKey:@"codec_bitrate_limit"
					 inSection:@"audio"];

			BOOL voice_processing = [self boolForKey:@"voiceproc_preference"];
			[lm lpConfigSetInt:voice_processing forKey:@"voiceproc_preference"];

			BOOL equalizer = [self boolForKey:@"eq_active"];
			[lm lpConfigSetBool:equalizer forKey:@"eq_active" inSection:@"sound"];

			[LinphoneManager.instance configureVbrCodecs];

			NSString *au_device = @"AU: Audio Unit Receiver";
			if (!voice_processing) {
				au_device = @"AU: Audio Unit NoVoiceProc";
			}
			linphone_core_set_capture_device(LC, [au_device UTF8String]);
			linphone_core_set_playback_device(LC, [au_device UTF8String]);
		}

		// video section
		{
			[self synchronizeCodecs:linphone_core_get_video_codecs(LC)];

			LinphoneVideoPolicy policy;
			policy.automatically_initiate = [self boolForKey:@"start_video_preference"];
			policy.automatically_accept = [self boolForKey:@"accept_video_preference"];
			linphone_core_set_video_policy(LC, &policy);
			linphone_core_enable_self_view(LC, [self boolForKey:@"self_video_preference"]);
			BOOL preview_preference = IPAD && [self boolForKey:@"preview_preference"];
			[lm lpConfigSetInt:preview_preference forKey:@"preview_preference"];

			NSString *videoPreset = [self stringForKey:@"video_preset_preference"];
			linphone_core_set_video_preset(LC, [videoPreset UTF8String]);
			int bw;
			MSVideoSize vsize;
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
					bw = 660;
					break;
				case 2:
				default:
					MS_VIDEO_SIZE_ASSIGN(vsize, QVGA);
					bw = 380;
					break;
			}
			linphone_core_set_preferred_video_size(LC, vsize);
			if (![videoPreset isEqualToString:@"custom"]) {
				[self setInteger:0 forKey:@"video_preferred_fps_preference"];
				[self setInteger:bw forKey:@"download_bandwidth_preference"];
			}
			linphone_core_set_preferred_framerate(LC, [self integerForKey:@"video_preferred_fps_preference"]);
			linphone_core_set_download_bandwidth(LC, [self integerForKey:@"download_bandwidth_preference"]);
			linphone_core_set_upload_bandwidth(LC, [self integerForKey:@"download_bandwidth_preference"]);
		}

		// call section
		{
			linphone_core_set_use_rfc2833_for_dtmf(LC, [self boolForKey:@"rfc_dtmf_preference"]);
			linphone_core_set_use_info_for_dtmf(LC, [self boolForKey:@"sipinfo_dtmf_preference"]);
			linphone_core_set_inc_timeout(LC, [self integerForKey:@"incoming_call_timeout_preference"]);
			linphone_core_set_in_call_timeout(LC, [self integerForKey:@"in_call_timeout_preference"]);
			[lm lpConfigSetString:[self stringForKey:@"voice_mail_uri_preference"] forKey:@"voice_mail_uri"];
			[lm lpConfigSetBool:[self boolForKey:@"repeat_call_notification_preference"]
						 forKey:@"repeat_call_notification"];
		}

		// chat section
		{
			int val = [self integerForKey:@"use_lime_preference"];
			linphone_core_enable_lime(LC, val);
			if (val == LinphoneLimeMandatory &&
				(linphone_core_get_media_encryption(LC) != LinphoneMediaEncryptionZRTP)) {
				linphone_core_set_media_encryption(LC, LinphoneMediaEncryptionZRTP);
				[self setCString:"ZRTP" forKey:@"media_encryption_preference"];
				UIAlertController *errView = [UIAlertController
					alertControllerWithTitle:NSLocalizedString(@"ZRTP activation", nil)
									 message:NSLocalizedString(
												 @"LIME requires ZRTP encryption.\n"
												 @"By activating LIME you automatically ZRTP media encryption.",
												 nil)
							  preferredStyle:UIAlertControllerStyleAlert];

				UIAlertAction *defaultAction = [UIAlertAction actionWithTitle:@"OK"
																		style:UIAlertActionStyleDefault
																	  handler:^(UIAlertAction *action){
																	  }];
				[errView addAction:defaultAction];
				[PhoneMainView.instance presentViewController:errView animated:YES completion:nil];
			}
			linphone_core_set_file_transfer_server(
				LC, [[self stringForKey:@"file_transfer_server_url_preference"] UTF8String]);
		}

		// network section
		{
			BOOL edgeOpt = [self boolForKey:@"edge_opt_preference"];
			[lm lpConfigSetInt:edgeOpt forKey:@"edge_opt_preference"];

			BOOL wifiOnly = [self boolForKey:@"wifi_only_preference"];
			[lm lpConfigSetInt:wifiOnly forKey:@"wifi_only_preference"];
			if ([self valueChangedForKey:@"wifi_only_preference"]) {
				[LinphoneManager.instance setupNetworkReachabilityCallback];
			}

			LinphoneNatPolicy *LNP = linphone_core_get_nat_policy(LC);
			NSString *stun_server = [self stringForKey:@"stun_preference"];
			if ([stun_server length] > 0) {
				linphone_core_set_stun_server(LC, [stun_server UTF8String]);
				linphone_nat_policy_set_stun_server(LNP, [stun_server UTF8String]);
				BOOL ice_preference = [self boolForKey:@"ice_preference"];
				linphone_nat_policy_enable_ice(LNP, ice_preference);
				linphone_nat_policy_enable_turn(LNP, [self boolForKey:@"turn_preference"]);
				NSString *turn_username = [self stringForKey:@"turn_username"];
				NSString *turn_password = [self stringForKey:@"turn_password"];

				if ([turn_username length] > 0) {
					const LinphoneAuthInfo *turnAuthInfo = nil;
					if ([turn_password length] > 0)
						turnAuthInfo = linphone_auth_info_new([turn_username UTF8String], NULL,
															  [turn_password UTF8String], NULL, NULL, NULL);
					else
						turnAuthInfo = linphone_core_find_auth_info(LC, NULL, [turn_username UTF8String], NULL);
					if (turnAuthInfo != NULL)
						linphone_core_add_auth_info(LC, turnAuthInfo);
					linphone_nat_policy_set_stun_server_username(LNP, linphone_auth_info_get_username(turnAuthInfo));
				}
			} else {
				linphone_nat_policy_enable_stun(LNP, FALSE);
				linphone_nat_policy_set_stun_server(LNP, NULL);
				linphone_core_set_stun_server(LC, NULL);
			}
			linphone_core_set_nat_policy(LC, LNP);
			{
				NSString *audio_port_preference = [self stringForKey:@"audio_port_preference"];
				int minPort, maxPort;
				[LinphoneCoreSettingsStore parsePortRange:audio_port_preference minPort:&minPort maxPort:&maxPort];
				linphone_core_set_audio_port_range(LC, minPort, maxPort);
			}
			{
				NSString *video_port_preference = [self stringForKey:@"video_port_preference"];
				int minPort, maxPort;
				[LinphoneCoreSettingsStore parsePortRange:video_port_preference minPort:&minPort maxPort:&maxPort];

				linphone_core_set_video_port_range(LC, minPort, maxPort);
			}

			NSString *menc = [self stringForKey:@"media_encryption_preference"];
			if (menc && [menc compare:@"SRTP"] == NSOrderedSame)
				linphone_core_set_media_encryption(LC, LinphoneMediaEncryptionSRTP);
			else if (menc && [menc compare:@"ZRTP"] == NSOrderedSame)
				linphone_core_set_media_encryption(LC, LinphoneMediaEncryptionZRTP);
			else if (menc && [menc compare:@"DTLS"] == NSOrderedSame)
				linphone_core_set_media_encryption(LC, LinphoneMediaEncryptionDTLS);
			else
				linphone_core_set_media_encryption(LC, LinphoneMediaEncryptionNone);

			linphone_core_enable_adaptive_rate_control(LC, [self boolForKey:@"adaptive_rate_control_preference"]);
		}

		// tunnel section
		{
			if (linphone_core_tunnel_available()) {
				NSString *lTunnelPrefMode = [self stringForKey:@"tunnel_mode_preference"];
				NSString *lTunnelPrefAddress = [self stringForKey:@"tunnel_address_preference"];
				int lTunnelPrefPort = [self integerForKey:@"tunnel_port_preference"];
				LinphoneTunnel *tunnel = linphone_core_get_tunnel(LC);
				LinphoneTunnelMode mode = LinphoneTunnelModeDisable;
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
						mode = LinphoneTunnelModeDisable;
					} else if ([lTunnelPrefMode isEqualToString:@"on"]) {
						mode = LinphoneTunnelModeEnable;
					} else if ([lTunnelPrefMode isEqualToString:@"auto"]) {
						mode = LinphoneTunnelModeAuto;
					} else {
						LOGE(@"Unexpected tunnel mode [%s]", [lTunnelPrefMode UTF8String]);
					}
				}

				[lm lpConfigSetString:lTunnelPrefMode forKey:@"tunnel_mode_preference"];
				linphone_tunnel_set_mode(tunnel, mode);
			}
		}

		// advanced section
		{
			BOOL animations = [self boolForKey:@"animations_preference"];
			[lm lpConfigSetInt:animations forKey:@"animations_preference"];

			UIDevice *device = [UIDevice currentDevice];
			bool backgroundSupported =
				[device respondsToSelector:@selector(isMultitaskingSupported)] && [device isMultitaskingSupported];
			BOOL isbackgroundModeEnabled = backgroundSupported && [self boolForKey:@"backgroundmode_preference"];
			[lm lpConfigSetInt:isbackgroundModeEnabled forKey:@"backgroundmode_preference"];

			[lm lpConfigSetInt:[self integerForKey:@"start_at_boot_preference"] forKey:@"start_at_boot_preference"];
			[lm lpConfigSetInt:[self integerForKey:@"autoanswer_notif_preference"]
						forKey:@"autoanswer_notif_preference"];
			[lm lpConfigSetInt:[self integerForKey:@"show_msg_in_notif"] forKey:@"show_msg_in_notif"];

			BOOL firstloginview = [self boolForKey:@"enable_first_login_view_preference"];
			[lm lpConfigSetInt:firstloginview forKey:@"enable_first_login_view_preference"];

			NSString *displayname = [self stringForKey:@"primary_displayname_preference"];
			NSString *username = [self stringForKey:@"primary_username_preference"];
			LinphoneAddress *parsed = linphone_core_get_primary_contact_parsed(LC);
			if (parsed != NULL) {
				linphone_address_set_display_name(parsed, [displayname UTF8String]);
				linphone_address_set_username(parsed, [username UTF8String]);
				char *contact = linphone_address_as_string(parsed);
				linphone_core_set_primary_contact(LC, contact);
				ms_free(contact);
				linphone_address_destroy(parsed);
			}

			[lm lpConfigSetInt:[self integerForKey:@"account_mandatory_advanced_preference"]
						forKey:@"account_mandatory_advanced_preference"];
		}

		changedDict = [[NSMutableDictionary alloc] init];

		// Post event
		NSDictionary *eventDic = [NSDictionary dictionaryWithObject:self forKey:@"settings"];
		[NSNotificationCenter.defaultCenter postNotificationName:kLinphoneSettingsUpdate object:self userInfo:eventDic];

		return YES;
		//} @catch (NSException *e) {
		//	// may happen when application is terminated, since we are destroying the core
		//	LOGI(@"Core probably already destroyed, cannot synchronize settings. Skipping. %@", [e callStackSymbols]);
		//}
		// return NO;
}

- (void)removeAccount {
	LinphoneProxyConfig *config = bctbx_list_nth_data(linphone_core_get_proxy_config_list(LC),
													  [self integerForKey:@"current_proxy_config_preference"]);

	BOOL isDefault = (linphone_core_get_default_proxy_config(LC) == config);

	const LinphoneAuthInfo *ai = linphone_proxy_config_find_auth_info(config);
	linphone_core_remove_proxy_config(LC, config);
	if (ai) {
		linphone_core_remove_auth_info(LC, ai);
	}
	[self setInteger:-1 forKey:@"current_proxy_config_preference"];

	if (isDefault) {
		// if we removed the default proxy config, set another one instead
		if (linphone_core_get_proxy_config_list(LC) != NULL) {
			linphone_core_set_default_proxy_index(LC, 0);
		}
	}
	[self transformLinphoneCoreToKeys];
}
@end
