/*
 * Copyright (c) 2010-2020 Belledonne Communications SARL.
 *
 * This file is part of linphone-iphone
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 */

#import "linphoneapp-Swift.h"
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
	//const MSList *accountList = linphone_core_get_account_list(LC);
	MSList *accountListToBeFreed = [LinphoneManager.instance createAccountsNotHiddenList];
	MSList *accountList = accountListToBeFreed;
	while (username && accountList &&
		   strcmp(username.UTF8String,
				  linphone_address_get_username(linphone_account_params_get_identity_address(linphone_account_get_params(accountList->data)))) != 0) {
		accountList = accountList->next;
	}
	LinphoneAccount *account = NULL;

	// default values
	{
		[self setBool:NO forKey:@"account_pushnotification_preference"];
		[self setBool:NO forKey:@"account_bundle_mode_preference"];
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
		[self setCString:"" forKey:@"ha1_algo_preference"];
		[self setInteger:-1 forKey:@"account_expire_preference"];
		[self setInteger:-1 forKey:@"current_proxy_config_preference"];
		[self setCString:"" forKey:@"account_prefix_preference"];
		[self setBool:YES forKey:@"apply_international_prefix_for_calls_and_chats"];
		[self setBool:NO forKey:@"account_substitute_+_by_00_preference"];
		[self setBool:NO forKey:@"account_ice_preference"];
		[self setCString:"" forKey:@"account_stun_preference"];
	}

	if (accountList) {
		account = accountList->data;
		LinphoneAccountParams const *accountParams = linphone_account_get_params(account);
		// root section
		{
			BOOL pushEnabled = linphone_account_params_get_push_notification_allowed(accountParams);
			[self setBool:pushEnabled forKey:@"account_pushnotification_preference"];
			
			BOOL bundleModeEnabled = linphone_account_params_rtp_bundle_enabled(accountParams);
			[self setBool:bundleModeEnabled forKey:@"account_bundle_mode_preference"];
			
			const LinphoneAddress *identity_addr = linphone_account_params_get_identity_address(accountParams);
			const char *server_addr = linphone_account_params_get_server_addr(accountParams);
			LinphoneAddress *proxy_addr = linphone_core_interpret_url_2(LC, server_addr, false);
			if (identity_addr && proxy_addr) {
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
				linphone_address_unref(proxy_addr);
				[self setCString:tname forKey:@"account_transport_preference"];
			}

			[self setBool:(linphone_account_params_get_routes_addresses(accountParams) != NULL) forKey:@"account_outbound_proxy_preference"];
			[self setBool:linphone_account_is_avpf_enabled(account) forKey:@"account_avpf_preference"];
			[self setBool:linphone_account_params_get_register_enabled(accountParams) forKey:@"account_is_enabled_preference"];
			[self setBool:(linphone_core_get_default_account(LC) == account)
				   forKey:@"account_is_default_preference"];

			const LinphoneAuthInfo *ai = linphone_core_find_auth_info(
				LC, NULL, [self stringForKey:@"account_mandatory_username_preference"].UTF8String,
				[self stringForKey:@"account_mandatory_domain_preference"].UTF8String);
			if (ai) {
				[self setCString:linphone_auth_info_get_userid(ai) forKey:@"account_userid_preference"];
				[self setCString:linphone_auth_info_get_passwd(ai) forKey:@"account_mandatory_password_preference"];
				// hidden but useful if provisioned
				[self setCString:linphone_auth_info_get_ha1(ai) forKey:@"ha1_preference"];
				[self setCString:linphone_auth_info_get_algorithm(ai) forKey:@"ha1_algo_preference"];
			}

			MSList *accountsList = [LinphoneManager.instance createAccountsNotHiddenList];
			int idx = (int)bctbx_list_index(linphone_core_get_account_list(LC), account);
			[self setInteger:idx forKey:@"current_proxy_config_preference"];
			bctbx_list_free(accountsList);
			
			int expires = linphone_account_params_get_expires(accountParams);
			[self setInteger:expires forKey:@"account_expire_preference"];

			LinphoneNatPolicy *policy = linphone_account_params_get_nat_policy(accountParams);
			if (policy) {
				[self setBool:linphone_nat_policy_ice_enabled(policy) forKey:@"account_ice_preference"];
				[self setCString:linphone_nat_policy_get_stun_server(policy) forKey:@"account_stun_preference"];
			}
		}

		// call section
		{
			const char *dial_prefix = linphone_account_params_get_international_prefix(accountParams);
			[self setCString:dial_prefix forKey:@"account_prefix_preference"];
			BOOL apply_prefix = linphone_account_params_get_use_international_prefix_for_calls_and_chats(accountParams);
			[self setBool:apply_prefix forKey:@"apply_international_prefix_for_calls_and_chats"];
			BOOL dial_escape_plus = linphone_account_params_get_dial_escape_plus_enabled(accountParams);
			[self setBool:dial_escape_plus forKey:@"account_substitute_+_by_00_preference"];
		}
	}
	bctbx_list_free(accountListToBeFreed);
}


- (void)transformLdapToKeys:(NSString *)ldap_server {
	const MSList *ldaps = linphone_core_get_ldap_list(LC);
	while (ldap_server && ldaps &&
		   strcmp(ldap_server.UTF8String,
				  linphone_ldap_params_get_server(linphone_ldap_get_params(ldaps->data))) != 0) {
		ldaps = ldaps->next;
	}
	LinphoneLdap *ldap = NULL;

	// default values
	{
		[self setBool:YES forKey:@"ldap_enabled"];
		[self setObject:@"" forKey:@"ldap_server"];
		[self setObject:@"" forKey:@"ldap_bind_dn"];
		[self setObject:@"" forKey:@"ldap_password"];
		[self setObject:@"simple" forKey:@"ldap_auth_method"];
		[self setBool:YES forKey:@"ldap_tls_enabled"];
		[self setObject:@"default" forKey:@"ldap_certificates_verification_mode"];
		[self setObject:@"" forKey:@"ldap_filter"];
		[self setInteger:50 forKey:@"ldap_max_results"];
		[self setInteger:5 forKey:@"ldap_timeout"];
		[self setInteger:500 forKey:@"ldap_delay"];
		[self setInteger:3 forKey:@"ldap_min_chars"];
		[self setObject:@"" forKey:@"ldap_name_attribute"];
		[self setObject:@"" forKey:@"ldap_sip_attribute"];
		[self setObject:@"" forKey:@"ldap_sip_domain"];
		[self setBool:NO forKey:@"ldap_logs_enabled"];
	}
	
	if (ldaps) {
		ldap = ldaps->data;
		LinphoneLdapParams const *ldapParams = linphone_ldap_get_params(ldap);
		
		int idx = (int)bctbx_list_index(linphone_core_get_ldap_list(LC), ldap);
		[self setInteger:idx forKey:@"current_ldap_index"];
		
		[self setBool:linphone_ldap_params_get_enabled(ldapParams) forKey:@"ldap_enabled"];
		[self setCString:linphone_ldap_params_get_server(ldapParams) forKey:@"ldap_server"];
		[self setCString:linphone_ldap_params_get_bind_dn(ldapParams) forKey:@"ldap_bind_dn"];
		[self setCString:linphone_ldap_params_get_password(ldapParams) forKey:@"ldap_password"];
		[self setBool:linphone_ldap_params_tls_enabled(ldapParams) forKey:@"ldap_tls_enabled"];
		
		switch (linphone_ldap_params_get_auth_method(ldapParams)) {
			case LinphoneLdapAuthMethodSimple:
				[self setObject:@"simple" forKey:@"ldap_auth_method"];
				break;
			case LinphoneLdapAuthMethodAnonymous:
				[self setObject:@"anonymous" forKey:@"ldap_auth_method"];
				break;
		}
		
		switch (linphone_ldap_params_get_server_certificates_verification_mode(ldapParams)) {
			case LinphoneLdapCertVerificationDefault:
				[self setObject:@"default" forKey:@"ldap_certificates_verification_mode"];
				break;
			case LinphoneLdapCertVerificationEnabled:
				[self setObject:@"enabled" forKey:@"ldap_certificates_verification_mode"];
				break;
			case LinphoneLdapCertVerificationDisabled:
				[self setObject:@"disabled" forKey:@"ldap_certificates_verification_mode"];
				break;
		}
		
		[self setCString:linphone_ldap_params_get_base_object(ldapParams) forKey:@"ldap_base_object"];
		[self setCString:linphone_ldap_params_get_filter(ldapParams) forKey:@"ldap_filter"];
		[self setInteger:linphone_ldap_params_get_max_results(ldapParams) forKey:@"ldap_max_results"];
		[self setInteger:linphone_ldap_params_get_timeout(ldapParams) forKey:@"ldap_timeout"];
		[self setInteger:linphone_ldap_params_get_delay(ldapParams) forKey:@"ldap_delay"];
		[self setInteger:linphone_ldap_params_get_min_chars(ldapParams) forKey:@"ldap_min_chars"];
		
		
		[self setCString:linphone_ldap_params_get_name_attribute(ldapParams) forKey:@"ldap_name_attribute"];
		[self setCString:linphone_ldap_params_get_sip_attribute(ldapParams) forKey:@"ldap_sip_attribute"];
		[self setCString:linphone_ldap_params_get_sip_domain(ldapParams) forKey:@"ldap_sip_domain"];
		
		bool ldapLogsEnabled = linphone_ldap_params_get_debug_level(ldapParams) == LinphoneLdapDebugLevelVerbose;
		[self setBool:ldapLogsEnabled forKey:@"ldap_logs_enabled"];
	}
}

- (void)transformLinphoneCoreToKeys {
	LinphoneManager *lm = LinphoneManager.instance;

	// root section
	{
		MSList *accountsListToBeFreed = [lm createAccountsNotHiddenList];
		MSList *accountsList = accountsListToBeFreed;
		size_t count = bctbx_list_size(accountsList);
		for (size_t i = 1; i <= count; i++, accountsList = accountsList->next) {
			NSString *key = [NSString stringWithFormat:@"menu_account_%lu", i];
			LinphoneAccount *account = (LinphoneAccount *)accountsList->data;
			[self setCString:linphone_address_get_username(linphone_account_params_get_identity_address(linphone_account_get_params(account)))
					  forKey:key];
		}
		bctbx_free(accountsListToBeFreed);
		
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
		
		[self setBool:[lm lpConfigBoolForKey:@"use_device_ringtone"] forKey:@"use_device_ringtone"];

		[self setBool:linphone_core_get_use_info_for_dtmf(LC) forKey:@"sipinfo_dtmf_preference"];
		[self setBool:linphone_core_get_use_rfc2833_for_dtmf(LC) forKey:@"rfc_dtmf_preference"];

		[self setInteger:linphone_core_get_inc_timeout(LC) forKey:@"incoming_call_timeout_preference"];
		[self setInteger:linphone_core_get_in_call_timeout(LC) forKey:@"in_call_timeout_preference"];

		[self setBool:[lm lpConfigBoolForKey:@"repeat_call_notification"]
			   forKey:@"repeat_call_notification_preference"];
		if (linphone_core_get_media_encryption(LC) == LinphoneMediaEncryptionNone) {
			[self setBool:FALSE forKey:@"media_encrption_mandatory_preference"];
			linphone_core_set_media_encryption_mandatory(LC, FALSE);
		} else {
			[self setBool:linphone_core_is_media_encryption_mandatory(LC) forKey:@"media_encrption_mandatory_preference"];
		}
		[self setBool:[lm lpConfigBoolForKey:@"pref_accept_early_media"]
			   forKey:@"pref_accept_early_media_preference"];
	}

	// chat section
	{
		[self setCString:linphone_core_get_file_transfer_server(LC) forKey:@"file_transfer_server_url_preference"];
        int maxSize = linphone_core_get_max_size_for_auto_download_incoming_files(LC);
        [self setObject:maxSize==0 ? @"Always" : (maxSize==-1 ? @"Never" : @"Customize") forKey:@"auto_download_mode"];
        [self setInteger:maxSize forKey:@"auto_download_incoming_files_max_size"];
		[self setBool:[VFSUtil vfsEnabledWithGroupName:kLinphoneMsgNotificationAppGroupId] forKey:@"vfs_enabled_mode"];
		[self setBool:[lm lpConfigBoolForKey:@"auto_write_to_gallery_preference" withDefault:NO] forKey:@"auto_write_to_gallery_mode"];
	}

	// network section
	{
		LinphoneNatPolicy *np = linphone_core_get_nat_policy(LC);
		[self setBool:[lm lpConfigBoolForKey:@"edge_opt_preference" withDefault:NO] forKey:@"edge_opt_preference"];
		[self setBool:[lm lpConfigBoolForKey:@"wifi_only_preference" withDefault:NO] forKey:@"wifi_only_preference"];
		[self setCString:linphone_nat_policy_get_stun_server(np) forKey:@"stun_preference"];
		[self setBool:linphone_nat_policy_ice_enabled(np) forKey:@"ice_preference"];
		[self setBool:linphone_nat_policy_turn_enabled(np) forKey:@"turn_preference"];
		
		[self setCString:linphone_nat_policy_get_stun_server_username(np)
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
		[self setCString:val forKey:linphone_core_get_post_quantum_available() ? @"media_encryption_preference_pq_enabled" : @"media_encryption_preference"];
		[self setInteger:linphone_core_get_upload_bandwidth(LC) forKey:@"upload_bandwidth_preference"];
		[self setInteger:linphone_core_get_download_bandwidth(LC) forKey:@"download_bandwidth_preference"];
		[self setBool:linphone_core_adaptive_rate_control_enabled(LC) forKey:@"adaptive_rate_control_preference"];
		[self setObject:[lm lpConfigStringForKey:@"dns_server_ip"] forKey:@"dns_server_preference"];
		[self setCString:[lm lpConfigStringForKey:@"ssids" inSection:@"local_push"].UTF8String forKey:@"local_push_ssids"];
		
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
	
	// contacts section
	{
		[self setInteger:[lm lpConfigBoolForKey:@"account_push_presence_preference" withDefault:YES] forKey:@"account_push_presence_preference"];
		if (linphone_core_ldap_available(LC)) {
			[self transformLdapToKeys:nil];
		}
	}

	// advanced section
	{
		[self setObject:[lm lpConfigStringForKey:@"debugenable_preference"] forKey:@"debugenable_preference"];
		[self setBool:ANIMATED forKey:@"animations_preference"];
		[self setBool:[lm lpConfigBoolForKey:@"backgroundmode_preference"] forKey:@"backgroundmode_preference"];
		[self setBool:[lm lpConfigBoolForKey:@"start_at_boot_preference"] forKey:@"start_at_boot_preference"];
		[self setBool:[lm lpConfigBoolForKey:@"screenshot_preference" withDefault:NO] forKey:@"screenshot_preference"];
		[self setBool:[lm lpConfigBoolForKey:@"autoanswer_notif_preference"] forKey:@"autoanswer_notif_preference"];
		[self setBool:[lm lpConfigBoolForKey:@"show_msg_in_notif" withDefault:YES] forKey:@"show_msg_in_notif"];
		[self setBool:[lm lpConfigBoolForKey:@"use_rls_presence" withDefault:YES] forKey:@"use_rls_presence"];
		[self setBool:[lm lpConfigBoolForKey:@"enable_first_login_view_preference"]
			   forKey:@"enable_first_login_view_preference"];
		[self setBool:[lm lpConfigBoolForKey:@"enable_broadcast_conference_feature" withDefault:NO] forKey:@"enable_broadcast_conference_feature"];
		LinphoneAddress *parsed = linphone_core_get_primary_contact_parsed(LC);
		if (parsed != NULL) {
			[self setCString:linphone_address_get_display_name(parsed) forKey:@"primary_displayname_preference"];
			[self setCString:linphone_address_get_username(parsed) forKey:@"primary_username_preference"];
			linphone_address_unref(parsed);
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
	LinphoneAccount *account = NULL;
	LinphoneAccountParams *newAccountParams = NULL;
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
	NSString *accountAlgoPreference = [self stringForKey:@"ha1_algo_preference"];
	BOOL isOutboundProxy = [self boolForKey:@"account_outbound_proxy_preference"];
	BOOL use_avpf = [self boolForKey:@"account_avpf_preference"];
	BOOL is_default = [self boolForKey:@"account_is_default_preference"];
	BOOL is_enabled = [self boolForKey:@"account_is_enabled_preference"];
	BOOL use_ice = [self boolForKey:@"account_ice_preference"];
	NSString *stun_preference = [self stringForKey:@"account_stun_preference"];

	if (username && [username length] > 0 && domain && [domain length] > 0) {
		int expire = [self integerForKey:@"account_expire_preference"];
		BOOL pushnotification = [self boolForKey:@"account_pushnotification_preference"];
		BOOL bundlemode = [self boolForKey:@"account_bundle_mode_preference"];
		NSString *prefix = [self stringForKey:@"account_prefix_preference"];
		BOOL use_prefix = [self boolForKey:@"apply_international_prefix_for_calls_and_chats"];
		NSString *proxyAddress = [self stringForKey:@"account_proxy_preference"];

		if ((!proxyAddress || [proxyAddress length] < 1) && domain) {
			proxyAddress = domain;
		}

		if (![proxyAddress hasPrefix:@"sip:"] && ![proxyAddress hasPrefix:@"sips:"]) {
			proxyAddress = [NSString stringWithFormat:@"sip:%@", proxyAddress];
		}

		LinphoneAddress *proxy_addr = linphone_core_interpret_url_2(LC, proxyAddress.UTF8String, false);

		if (proxy_addr) {
			LinphoneTransportType type = LinphoneTransportUdp;
			if ([transport isEqualToString:@"tcp"])
				type = LinphoneTransportTcp;
			else if ([transport isEqualToString:@"tls"])
				type = LinphoneTransportTls;

			linphone_address_set_transport(proxy_addr, type);
		}
		
		MSList *accountList= [LinphoneManager.instance createAccountsNotHiddenList];
		account = bctbx_list_nth_data(accountList,
									   [self integerForKey:@"current_proxy_config_preference"]);
		bctbx_free(accountList);
		
		// if account was deleted, it is not present anymore
		if (account == NULL)
			goto bad_proxy;


		LinphoneAddress *linphoneAddress = linphone_address_clone(linphone_account_params_get_identity_address(linphone_account_get_params(account)));
		linphone_address_set_username(linphoneAddress, username.UTF8String);
		if ([LinphoneManager.instance lpConfigBoolForKey:@"use_phone_number" inSection:@"assistant"]) {
			char *user = linphone_account_normalize_phone_number(account, username.UTF8String);
			if (user) {
				linphone_address_set_username(linphoneAddress, user);
				ms_free(user);
			}
		}
		linphone_address_set_domain(linphoneAddress, [domain UTF8String]);
		linphone_address_set_display_name(linphoneAddress, (displayName.length ? displayName.UTF8String : NULL));
		const char *password = [accountPassword UTF8String];
		const char *ha1 = [accountHa1 UTF8String];

		newAccountParams = linphone_account_params_clone(linphone_account_get_params(account));
		
		if (linphone_account_params_set_identity_address(newAccountParams, linphoneAddress) == -1) {
			error = NSLocalizedString(@"Invalid username or domain", nil);
			goto bad_proxy;
		}
		// use proxy as route if outbound_proxy is enabled
		if (proxy_addr && linphone_account_params_set_server_address(newAccountParams, proxy_addr) == -1) {
			error = NSLocalizedString(@"Invalid proxy address", nil);
			goto bad_proxy;
		}
		if (proxy_addr && linphone_account_params_set_routes_addresses(newAccountParams, isOutboundProxy ? bctbx_list_new((void*)proxy_addr) : NULL) == -1) {
			error = NSLocalizedString(@"Invalid route", nil);
			goto bad_proxy;
		}

		LinphoneNatPolicy *policy = linphone_account_params_get_nat_policy(newAccountParams) ?: linphone_core_create_nat_policy(LC);
		linphone_nat_policy_enable_stun(policy, use_ice); // We always use STUN with ICE
		linphone_nat_policy_enable_ice(policy, use_ice);
		linphone_nat_policy_set_stun_server(policy, stun_preference.UTF8String);
		linphone_account_params_set_nat_policy(newAccountParams, policy);

		linphone_account_params_set_international_prefix(newAccountParams, [prefix UTF8String]);
		linphone_account_params_set_use_international_prefix_for_calls_and_chats(newAccountParams, use_prefix);
		if ([self objectForKey:@"account_substitute_+_by_00_preference"]) {
			bool substitute_plus_by_00 = [self boolForKey:@"account_substitute_+_by_00_preference"];
			linphone_account_params_set_dial_escape_plus_enabled(newAccountParams, substitute_plus_by_00);
		}

		// use empty string "" instead of NULL to avoid being overwritten by default proxy config values
		linphone_account_params_set_push_notification_allowed(newAccountParams, pushnotification);
		linphone_account_params_enable_rtp_bundle(newAccountParams, bundlemode);
		linphone_account_params_set_push_notification_allowed(newAccountParams, pushnotification);
		linphone_account_params_set_remote_push_notification_allowed(newAccountParams, pushnotification);
		
		linphone_account_params_set_register_enabled(newAccountParams, is_enabled);
		linphone_account_params_set_avpf_mode(newAccountParams, use_avpf);
		linphone_account_params_set_expires(newAccountParams, expire);
		
		linphone_account_set_params(account, newAccountParams);
		linphone_account_params_unref(newAccountParams);
		if (is_default) {
			linphone_core_set_default_account(LC, account);
		} else if (linphone_core_get_default_account(LC) == account) {
			linphone_core_set_default_account(LC, NULL);
		}

		LinphoneAuthInfo *proxyAi = (LinphoneAuthInfo *)linphone_account_find_auth_info(account);
		char *realm;
		if (proxyAi) {
			realm = ms_strdup(linphone_auth_info_get_realm(proxyAi));
		} else {
			realm = NULL;
		}

		// modify auth info only after finishing editting the proxy config, so that
		// UNREGISTER succeed
		if (proxyAi) {
			linphone_core_remove_auth_info(LC, proxyAi);
		}
		if (strcmp(password,"") == 0) {
			password = NULL;
		}

		char *identity = linphone_address_as_string(linphoneAddress);
		LinphoneAddress *from = linphone_core_interpret_url_2(LC, identity, false);
		ms_free(identity);
		if (from) {
			const char *userid_str = (userID != nil) ? [userID UTF8String] : NULL;
			LinphoneAuthInfo *info;
			if (password) {
				info = linphone_auth_info_new(linphone_address_get_username(from), userid_str, password, NULL,
											  linphone_account_params_get_realm(newAccountParams),
											  linphone_account_params_get_domain(newAccountParams));
			} else {
				info = linphone_auth_info_new_for_algorithm(linphone_address_get_username(from)
															, userid_str
															, NULL
															, ha1
															, realm ? realm : linphone_account_params_get_realm(newAccountParams),
											  linphone_account_params_get_domain(newAccountParams), [accountAlgoPreference UTF8String]);
			}

			linphone_address_unref(from);
			linphone_core_add_auth_info(LC, info);
			linphone_auth_info_destroy(info);
			ms_free(realm);
		}
		
	bad_proxy:
		if (linphoneAddress)
			linphone_address_destroy(linphoneAddress);
			
		// in case of error, show an alert to the user
		if (error != nil) {
			if (newAccountParams != NULL) { // If we get here, then we're also sure that Account != NULL
				
				linphone_account_params_unref(newAccountParams);
			}
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
	[[LinphoneManager.instance fastAddressBook] fetchContactsInBackGroundThread];
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

- (void)synchronizeLdap {
	LOGI(@"LDAP config changed, synchronizing.");
	
	LinphoneLdap*ldap = bctbx_list_nth_data(linphone_core_get_ldap_list(LC), [self integerForKey:@"current_ldap_index"]);
	if (!ldap) {
		return;
	}
	
	if ([self stringForKey:@"ldap_base_object"].length == 0) {
		UIAlertController *errView = [UIAlertController alertControllerWithTitle:NSLocalizedString(@"Error", nil)
																		 message:@"LDAPResearch base must not be empty"
																  preferredStyle:UIAlertControllerStyleAlert];
		
		UIAlertAction* defaultAction = [UIAlertAction actionWithTitle:NSLocalizedString(@"OK", nil)
																style:UIAlertActionStyleDefault
															  handler:^(UIAlertAction * action) {}];
		
		[errView addAction:defaultAction];
		[PhoneMainView.instance presentViewController:errView animated:YES completion:nil];
		return;
	}
	
	LinphoneLdapParams *newLdapParams = linphone_ldap_params_clone(linphone_ldap_get_params(ldap));
	
	// Connexion parameters
	linphone_ldap_params_set_enabled(newLdapParams, [self boolForKey:@"ldap_enabled"]);
	linphone_ldap_params_set_server(newLdapParams, [self stringForKey:@"ldap_server"].UTF8String);
	linphone_ldap_params_set_bind_dn(newLdapParams, [self stringForKey:@"ldap_bind_dn"].UTF8String);
	linphone_ldap_params_set_password(newLdapParams, [self stringForKey:@"ldap_password"].UTF8String);
	
	LinphoneLdapAuthMethod authMethod = [[self stringForKey:@"ldap_auth_method"] isEqualToString:@"simple"] ? LinphoneLdapAuthMethodSimple : LinphoneLdapAuthMethodAnonymous;
	linphone_ldap_params_set_auth_method(newLdapParams, authMethod);
	linphone_ldap_params_enable_tls(newLdapParams, [self boolForKey:@"ldap_tls_enabled"]);
	
	
	LinphoneLdapCertVerificationMode certVerifMode = LinphoneLdapCertVerificationDefault;
	NSString *certVerifModeStr = [self stringForKey:@"ldap_certificates_verification_mode"];
	if ([certVerifModeStr isEqualToString:@"enabled"])
		certVerifMode = LinphoneLdapCertVerificationEnabled;
	else if ([certVerifModeStr isEqualToString:@"disabled"])
		certVerifMode = LinphoneLdapCertVerificationEnabled;
	linphone_ldap_params_set_server_certificates_verification_mode(newLdapParams, certVerifMode);
	
	// Research parameters
	linphone_ldap_params_set_base_object(newLdapParams, [self stringForKey:@"ldap_base_object"].UTF8String);
	linphone_ldap_params_set_filter(newLdapParams, [self stringForKey:@"ldap_filter"].UTF8String);
	linphone_ldap_params_set_max_results(newLdapParams, [self integerForKey:@"ldap_max_results"]);
	linphone_ldap_params_set_timeout(newLdapParams, [self integerForKey:@"ldap_timeout"]);
	linphone_ldap_params_set_delay(newLdapParams, [self integerForKey:@"ldap_delay"]);
	linphone_ldap_params_set_min_chars(newLdapParams, [self integerForKey:@"ldap_min_chars"]);
	
	
	
	// Analysis parameters
	linphone_ldap_params_set_name_attribute(newLdapParams, [self stringForKey:@"ldap_name_attribute"].UTF8String);
	linphone_ldap_params_set_sip_attribute(newLdapParams, [self stringForKey:@"ldap_sip_attribute"].UTF8String);
	linphone_ldap_params_set_sip_domain(newLdapParams, [self stringForKey:@"ldap_sip_domain"].UTF8String);
	
	// Miscellaneous parameters
	LinphoneLdapDebugLevel debugLevel = [self boolForKey:@"ldap_logs_enabled"] ? LinphoneLdapDebugLevelVerbose : LinphoneLdapDebugLevelOff;
	linphone_ldap_params_set_debug_level(newLdapParams, debugLevel);
	
	linphone_ldap_set_params(ldap, newLdapParams);
}

- (BOOL)synchronize {
	@try {
		LinphoneManager *lm = LinphoneManager.instance;

		// root section
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

		// audio section
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
		if (!voice_processing)
			au_device = @"AU: Audio Unit NoVoiceProc";

		linphone_core_set_capture_device(LC, [au_device UTF8String]);
		linphone_core_set_playback_device(LC, [au_device UTF8String]);

		// video section
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
		MSVideoSize vsize;
		switch ([self integerForKey:@"video_preferred_size_preference"]) {
			case 0:
				MS_VIDEO_SIZE_ASSIGN(vsize, 720P);
				break;
			case 1:
				MS_VIDEO_SIZE_ASSIGN(vsize, VGA);
				break;
			case 2:
			default:
				MS_VIDEO_SIZE_ASSIGN(vsize, QVGA);
				break;
		}
		linphone_core_set_preferred_video_size(LC, vsize);
		if (![videoPreset isEqualToString:@"custom"]) {
			[self setInteger:0 forKey:@"video_preferred_fps_preference"];
			[self setInteger:0 forKey:@"download_bandwidth_preference"];
		}
		linphone_core_set_preferred_framerate(LC, [self integerForKey:@"video_preferred_fps_preference"]);
		linphone_core_set_download_bandwidth(LC, [self integerForKey:@"download_bandwidth_preference"]);
		linphone_core_set_upload_bandwidth(LC, [self integerForKey:@"download_bandwidth_preference"]);

		// call section
		linphone_core_set_use_rfc2833_for_dtmf(LC, [self boolForKey:@"rfc_dtmf_preference"]);
		[lm lpConfigSetBool:[self boolForKey:@"use_device_ringtone"] forKey:@"use_device_ringtone"];
		[ProviderDelegate resetSharedProviderConfiguration];

		linphone_core_set_use_info_for_dtmf(LC, [self boolForKey:@"sipinfo_dtmf_preference"]);
		linphone_core_set_inc_timeout(LC, [self integerForKey:@"incoming_call_timeout_preference"]);
		linphone_core_set_in_call_timeout(LC, [self integerForKey:@"in_call_timeout_preference"]);
		[lm lpConfigSetString:[self stringForKey:@"voice_mail_uri_preference"] forKey:@"voice_mail_uri"];
		[lm lpConfigSetBool:[self boolForKey:@"repeat_call_notification_preference"] forKey:@"repeat_call_notification"];
		[lm lpConfigSetBool:[self boolForKey:@"pref_accept_early_media_preference"] forKey:@"pref_accept_early_media"];
		linphone_core_set_media_encryption_mandatory(LC, [self boolForKey:@"media_encrption_mandatory_preference"]);

		linphone_core_set_file_transfer_server(LC, [self stringForKey:@"file_transfer_server_url_preference"].UTF8String);
        int maxSize;
        NSString *downloadMode = [self stringForKey:@"auto_download_mode"];
        if ([downloadMode isEqualToString:@"Never"]) {
            maxSize = -1;
        } else if ([downloadMode isEqualToString:@"Always"]) {
            maxSize = 0;
        } else {
            maxSize = [[self stringForKey:@"auto_download_incoming_files_max_size"] intValue];
        }
        linphone_core_set_max_size_for_auto_download_incoming_files(LC, maxSize);
        [lm lpConfigSetString:[self stringForKey:@"auto_download_mode"] forKey:@"auto_download_mode"];
		BOOL vfsPrefEnabled = [self boolForKey:@"vfs_enabled_mode"] || [VFSUtil vfsEnabledWithGroupName:kLinphoneMsgNotificationAppGroupId];
		if (vfsPrefEnabled && ![VFSUtil vfsEnabledWithGroupName:kLinphoneMsgNotificationAppGroupId]) {
			if (TARGET_IPHONE_SIMULATOR) {
				LOGW(@"[VFS] Can not active for simulators.");
				[VFSUtil setVfsEnabbledWithEnabled:false groupName:kLinphoneMsgNotificationAppGroupId];
				[self setBool:FALSE forKey:@"vfs_enabled_mode"];
			} else if (![VFSUtil activateVFSForFirstTime:true]) {
				[VFSUtil log:@"[VFS] Error unable to activate ! Warning disabling VFS enabled preference." :OS_LOG_TYPE_ERROR];
				[VFSUtil setVfsEnabbledWithEnabled:false groupName:kLinphoneMsgNotificationAppGroupId];
				[self setBool:FALSE forKey:@"vfs_enabled_mode"];
			} else {
				[VFSUtil setVfsEnabbledWithEnabled:true groupName:kLinphoneMsgNotificationAppGroupId];
				[self setBool:TRUE forKey:@"vfs_enabled_mode"];
			}
		}
		[lm lpConfigSetBool:[self boolForKey:@"auto_write_to_gallery_mode"] forKey:@"auto_write_to_gallery_preference"];

		// network section
		BOOL edgeOpt = [self boolForKey:@"edge_opt_preference"];
		[lm lpConfigSetInt:edgeOpt forKey:@"edge_opt_preference"];

		BOOL wifiOnly = [self boolForKey:@"wifi_only_preference"];
		[lm lpConfigSetInt:wifiOnly forKey:@"wifi_only_preference"];

		LinphoneNatPolicy *LNP = linphone_core_get_nat_policy(LC);
		
		BOOL ice_preference = [self boolForKey:@"ice_preference"];
		linphone_nat_policy_enable_ice(LNP, ice_preference);
		
		linphone_nat_policy_enable_turn(LNP, [self boolForKey:@"turn_preference"]);
		
		NSString *stun_server = [self stringForKey:@"stun_preference"];
		if ([stun_server length] > 0) {
			linphone_nat_policy_set_stun_server(LNP, [stun_server UTF8String]);
			linphone_nat_policy_enable_stun(LNP, ice_preference); /*we always use STUN with ICE*/
			NSString *turn_username = [self stringForKey:@"turn_username"];
			NSString *turn_password = [self stringForKey:@"turn_password"];

			if ([turn_username length] > 0) {
				const LinphoneAuthInfo *turnAuthInfo = nil;
				if ([turn_password length] > 0){
					turnAuthInfo = linphone_auth_info_new([turn_username UTF8String], NULL, [turn_password UTF8String], NULL, NULL, NULL);
					linphone_core_add_auth_info(LC, turnAuthInfo);
				} else
					turnAuthInfo = linphone_core_find_auth_info(LC, NULL, [turn_username UTF8String], NULL);

				linphone_nat_policy_set_stun_server_username(LNP, [turn_username UTF8String]);
			}
		} else {
			linphone_nat_policy_enable_stun(LNP, FALSE);
			linphone_nat_policy_set_stun_server(LNP, NULL);
		}
		linphone_core_set_nat_policy(LC, LNP);

		NSString *audio_port_preference = [self stringForKey:@"audio_port_preference"];
		int audioMinPort, audioMaxPort;
		[LinphoneCoreSettingsStore parsePortRange:audio_port_preference minPort:&audioMinPort maxPort:&audioMaxPort];
		linphone_core_set_audio_port_range(LC, audioMinPort, audioMaxPort);

		NSString *video_port_preference = [self stringForKey:@"video_port_preference"];
		int videoMinPort, videoMaxPort;
		[LinphoneCoreSettingsStore parsePortRange:video_port_preference minPort:&videoMinPort maxPort:&videoMaxPort];
		linphone_core_set_video_port_range(LC, videoMinPort, videoMaxPort);

		NSString *menc = [self stringForKey:linphone_core_get_post_quantum_available() ? @"media_encryption_preference_pq_enabled" : @"media_encryption_preference"];
		if (menc && [menc compare:@"SRTP"] == NSOrderedSame)
			linphone_core_set_media_encryption(LC, LinphoneMediaEncryptionSRTP);
		else if (menc && [menc compare:@"ZRTP"] == NSOrderedSame)
			linphone_core_set_media_encryption(LC, LinphoneMediaEncryptionZRTP);
		else if (menc && [menc compare:@"DTLS"] == NSOrderedSame)
			linphone_core_set_media_encryption(LC, LinphoneMediaEncryptionDTLS);
		else {
			linphone_core_set_media_encryption(LC, LinphoneMediaEncryptionNone);
			[self setBool:FALSE forKey:@"media_encrption_mandatory_preference"];
		}

		linphone_core_enable_adaptive_rate_control(LC, [self boolForKey:@"adaptive_rate_control_preference"]);
		
		if ([self stringForKey:@"dns_server_preference"] != [lm lpConfigStringForKey:@"dns_server_ip"]) {
			[lm lpConfigSetString:[self stringForKey:@"dns_server_preference"] forKey:@"dns_server_ip"];
			[lm setDnsServer];
		}
		
		[lm lpConfigSetString:[self stringForKey:@"local_push_ssids"] forKey:@"ssids" inSection:@"local_push"];
		/* if (@available(iOS 15.0, *)) {
			[LocalPushManager.shared configureLocalPushWithCCoreConfig:lm.configDb];
		} else {
			LOGW(@"Local push notifications not available for this ios version (iOS 15 minimum)");
		} */


		// tunnel section
		if (linphone_core_tunnel_available()) {
			NSString *lTunnelPrefMode = [self stringForKey:@"tunnel_mode_preference"];
			NSString *lTunnelPrefAddress = [self stringForKey:@"tunnel_address_preference"];
			int lTunnelPrefPort = [self integerForKey:@"tunnel_port_preference"];
			LinphoneTunnel *tunnel = linphone_core_get_tunnel(LC);
			LinphoneTunnelMode mode = LinphoneTunnelModeDisable;
			int lTunnelPort = 443;
			if (lTunnelPrefPort)
				lTunnelPort = lTunnelPrefPort;

			linphone_tunnel_clean_servers(tunnel);
			if (lTunnelPrefAddress && [lTunnelPrefAddress length]) {
				LinphoneTunnelConfig *ltc = linphone_tunnel_config_new();
				linphone_tunnel_config_set_host(ltc, [lTunnelPrefAddress UTF8String]);
				linphone_tunnel_config_set_port(ltc, lTunnelPort);
				linphone_tunnel_add_server(tunnel, ltc);

				if ([lTunnelPrefMode isEqualToString:@"off"])
					mode = LinphoneTunnelModeDisable;
				else if ([lTunnelPrefMode isEqualToString:@"on"])
					mode = LinphoneTunnelModeEnable;
				else if ([lTunnelPrefMode isEqualToString:@"auto"])
					mode = LinphoneTunnelModeAuto;
				else
					LOGE(@"Unexpected tunnel mode [%s]", [lTunnelPrefMode UTF8String]);
			}

			[lm lpConfigSetString:lTunnelPrefMode forKey:@"tunnel_mode_preference"];
			linphone_tunnel_set_mode(tunnel, mode);
		}

		// contacts section
		BOOL push_presence = [self boolForKey:@"account_push_presence_preference"];
		if (push_presence) {
			linphone_core_set_consolidated_presence([LinphoneManager getLc], LinphoneConsolidatedPresenceOnline);
		} else {
			linphone_core_set_consolidated_presence([LinphoneManager getLc], LinphoneConsolidatedPresenceOffline);
		}
		[lm lpConfigSetInt:push_presence forKey:@"account_push_presence_preference"];
		
		BOOL ldap_changed = NO;
		for (NSString *key in self->changedDict) {
			if ([key hasPrefix:@"ldap_"] && [self valueChangedForKey:key]) {
				ldap_changed = YES;
				break;
			}
		}
		if (ldap_changed)
			[self synchronizeLdap];
		
		// advanced section
		BOOL animations = [self boolForKey:@"animations_preference"];
		[lm lpConfigSetInt:animations forKey:@"animations_preference"];
		
		BOOL screenshot = [self boolForKey:@"screenshot_preference"];
		[lm lpConfigSetInt:screenshot forKey:@"screenshot_preference"];
		
		BOOL broadcast = [self boolForKey:@"enable_broadcast_conference_feature"];
		[lm lpConfigSetInt:broadcast forKey:@"enable_broadcast_conference_feature"];

		UIDevice *device = [UIDevice currentDevice];
		BOOL backgroundSupported = [device respondsToSelector:@selector(isMultitaskingSupported)] && [device isMultitaskingSupported];
		BOOL isbackgroundModeEnabled = backgroundSupported && [self boolForKey:@"backgroundmode_preference"];
		[lm lpConfigSetInt:isbackgroundModeEnabled forKey:@"backgroundmode_preference"];
		[lm lpConfigSetInt:[self integerForKey:@"start_at_boot_preference"] forKey:@"start_at_boot_preference"];
		[lm lpConfigSetInt:[self integerForKey:@"autoanswer_notif_preference"] forKey:@"autoanswer_notif_preference"];
		[lm lpConfigSetInt:[self integerForKey:@"show_msg_in_notif"] forKey:@"show_msg_in_notif"];

		if ([self integerForKey:@"use_rls_presence"]) {
			[self setInteger:0 forKey:@"use_rls_presence"];
			NSString *rls_uri = [lm lpConfigStringForKey:@"rls_uri" inSection:@"sip" withDefault:@"sips:rls@sip.linphone.org"];
			LinphoneAddress *rls_addr = linphone_address_new(rls_uri.UTF8String);
			const char *rls_domain = linphone_address_get_domain(rls_addr);
			
			MSList *accountListToBeFreed = [LinphoneManager.instance createAccountsNotHiddenList];
			const MSList *accounts = accountListToBeFreed;
			if (!accounts) // Enable it if no proxy config for first launch of app
				[self setInteger:1 forKey:@"use_rls_presence"];
			else {
				while (accounts) {
					const char *proxy_domain = linphone_account_params_get_domain(linphone_account_get_params(accounts->data));
					if (strcmp(rls_domain, proxy_domain) == 0) {
						[self setInteger:1 forKey:@"use_rls_presence"];
						break;
					}
					accounts = accounts->next;
				}
			}
			linphone_address_unref(rls_addr);
			bctbx_free(accountListToBeFreed);
		}

		[lm lpConfigSetInt:[self integerForKey:@"use_rls_presence"] forKey:@"use_rls_presence"];
		linphone_core_enable_friend_list_subscription(LC, [self integerForKey:@"use_rls_presence"]);

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
		[lm lpConfigSetInt:[self integerForKey:@"account_mandatory_advanced_preference"] forKey:@"account_mandatory_advanced_preference"];

		changedDict = [[NSMutableDictionary alloc] init];

		// Post event
		NSDictionary *eventDic = [NSDictionary dictionaryWithObject:self forKey:@"settings"];
		[NSNotificationCenter.defaultCenter postNotificationName:kLinphoneSettingsUpdate object:self userInfo:eventDic];

		return YES;
	} @catch (NSException *exception) {
		// may happen when application is terminated, since we are destroying the core
		if ([exception.name isEqualToString:@"LinphoneCoreException"]) {
			LOGE(@"Core already destroyed, settings not synchronized");
			return NO;
		}
		LOGE(@"Uncaught exception : %@", exception.description);
		abort();
	}
	return NO;
}

- (void)removeAccount {
	
	MSList *accountList = [LinphoneManager.instance createAccountsNotHiddenList];
	LinphoneAccount *account = bctbx_list_nth_data(accountList,
													  [self integerForKey:@"current_proxy_config_preference"]);
	
	
	const MSList *lists = linphone_core_get_friends_lists(LC);
	while (lists) {
		linphone_friend_list_enable_subscriptions(lists->data, FALSE);
		linphone_friend_list_update_subscriptions(lists->data);
		lists = lists->next;
	}
	BOOL isDefault = (linphone_core_get_default_account(LC) == account);

	const LinphoneAuthInfo *ai = linphone_account_find_auth_info(account);
	linphone_core_remove_account(LC, account);
	if (ai) {
		// Friend list unsubscription above is not instantanous, so give a bit of a time margin before finishing the removal of the auth info
		dispatch_after(dispatch_time(DISPATCH_TIME_NOW, (int64_t)(5 * NSEC_PER_SEC)), dispatch_get_main_queue(), ^{
			linphone_core_remove_auth_info(LC, ai);
		});
	}
	[self setInteger:-1 forKey:@"current_proxy_config_preference"];

	if (isDefault) {
		// if we removed the default proxy config, set another one instead
		if (accountList != NULL) {
			linphone_core_set_default_account(LC, (LinphoneAccount *)(accountList->data));
		}
	}
	[self transformLinphoneCoreToKeys];
	bctbx_free(accountList);
}

- (void)removeLdap {
	LinphoneLdap *ldap = bctbx_list_nth_data(linphone_core_get_ldap_list(LC),
													  [self integerForKey:@"current_ldap_index"]);
	linphone_core_remove_ldap(LC, ldap);
	[self transformLinphoneCoreToKeys];
}
@end
