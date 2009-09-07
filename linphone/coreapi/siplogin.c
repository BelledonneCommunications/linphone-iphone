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


static void sip_login_init_instance(SipSetupContext *ctx){
	LinphoneProxyConfig *cfg=sip_setup_context_get_proxy_config(ctx);
	/*disable registration until the user logs in*/
	linphone_proxy_config_enable_register(cfg,FALSE);
}

static int sip_login_do_login(SipSetupContext * ctx, const char *uri, const char *passwd){
	LinphoneProxyConfig *cfg=sip_setup_context_get_proxy_config(ctx);
	LinphoneCore *lc=linphone_proxy_config_get_core(cfg);
	LinphoneAuthInfo *auth=linphone_auth_info_new(ctx->username,NULL,passwd,NULL,NULL);
	linphone_proxy_config_set_identity(cfg,uri);
	linphone_core_add_auth_info(lc,auth);
	linphone_proxy_config_enable_register(cfg,TRUE);
	return 0;
}

/* a simple SipSetup built-in plugin to allow specify the user/password for proxy config at runtime*/
SipSetup linphone_sip_login={
	.name="SipLogin",
	.capabilities=SIP_SETUP_CAP_LOGIN,
	.init_instance=sip_login_init_instance,
	.login_account=sip_login_do_login,
};

