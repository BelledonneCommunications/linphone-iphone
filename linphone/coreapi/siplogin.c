/*
linphone
Copyright (C) 2009  Simon MORLAT (simon.morlat@linphone.org)

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
*/

#ifdef HAVE_CONFIG_H
#include "../config.h"
#endif

#include "linphonecore.h"
#include <ctype.h>

static void sip_login_init_instance(SipSetupContext *ctx){
	LinphoneProxyConfig *cfg=sip_setup_context_get_proxy_config(ctx);
	/*disable registration until the user logs in*/
	linphone_proxy_config_enable_register(cfg,FALSE);
}

static void guess_display_name(osip_from_t *from){
	char *dn=(char*)osip_malloc(strlen(from->url->username)+3);
	char *it=from->url->username;
	char *wptr=dn;
	bool_t begin=TRUE;
	bool_t surname=0;
	for(it=from->url->username;*it!='\0';++it){
		if (begin){
			*wptr=toupper(*it);
			begin=FALSE;
		}else if (*it=='.'){
			if (surname) break;
			*wptr=' ';
			begin=TRUE;
			surname=TRUE;
		}else *wptr=*it;
		wptr++;
	}
	if (from->displayname!=NULL) osip_free(from->displayname);
	from->displayname=dn;
}

static int sip_login_do_login(SipSetupContext * ctx, const char *uri, const char *passwd){
	LinphoneProxyConfig *cfg=sip_setup_context_get_proxy_config(ctx);
	LinphoneCore *lc=linphone_proxy_config_get_core(cfg);
	LinphoneAuthInfo *auth;
	osip_from_t *parsed_uri;
	char *tmp;

	osip_from_init(&parsed_uri);
	if (osip_from_parse(parsed_uri,uri)==-1){
		osip_from_free(parsed_uri);
		return -1;
	}
	if (parsed_uri->displayname==NULL || strlen(parsed_uri->displayname)==0){
		guess_display_name(parsed_uri);
	}
	osip_from_to_str(parsed_uri,&tmp);
	linphone_proxy_config_set_identity(cfg,tmp);
	if (passwd ) {
		auth=linphone_auth_info_new(parsed_uri->url->username,NULL,passwd,NULL,NULL);
		linphone_core_add_auth_info(lc,auth);
	}
	linphone_proxy_config_enable_register(cfg,TRUE);
	linphone_proxy_config_done(cfg);
	osip_free(tmp);
	osip_from_free(parsed_uri);
	ms_message("SipLogin: done");
	return 0;
}

static int sip_login_do_logout(SipSetupContext * ctx){
	LinphoneProxyConfig *cfg=sip_setup_context_get_proxy_config(ctx);
	linphone_proxy_config_enable_register(cfg,FALSE);
	linphone_proxy_config_done(cfg);
	return 0;
}

/* a simple SipSetup built-in plugin to allow specify the user/password for proxy config at runtime*/
SipSetup linphone_sip_login={
	.name="SipLogin",
	.capabilities=SIP_SETUP_CAP_LOGIN,
	.init_instance=sip_login_init_instance,
	.login_account=sip_login_do_login,
	.logout_account=sip_login_do_logout
};

