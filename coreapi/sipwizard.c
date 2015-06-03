/*
sipwizard.c
Copyright (C) 2011 Belledonne Communication, Grenoble, France

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

#include "linphonecore.h"
#include "private.h"
#include <ctype.h>


static const char *XMLRPC_URL = "https://www.linphone.org/wizard.php";

static void sip_wizard_init_instance(SipSetupContext *ctx){
	LinphoneProxyConfig *cfg=sip_setup_context_get_proxy_config(ctx);
	/*disable registration until the user logs in*/
	linphone_proxy_config_enable_register(cfg,FALSE);
}

static void sip_wizard_uninit_instance(SipSetupContext *ctx) {
	if (ctx->xmlrpc_session != NULL) {
		linphone_xml_rpc_session_unref(ctx->xmlrpc_session);
		ctx->xmlrpc_session = NULL;
	}
}

static const char ** sip_wizard_get_domains(SipSetupContext *ctx) {
	LinphoneProxyConfig *cfg=sip_setup_context_get_proxy_config(ctx);
	const char **domains = (const char**) &cfg->reg_proxy;
	return domains;
}


static int do_simple_xmlrpc_request(SipSetupContext *ctx, LinphoneXmlRpcRequest *request) {
	int ret = -1;

	if (!request) {
		ms_error("Fail to create XML-RPC request!");
		return -1;
	}

	if (ctx->xmlrpc_session == NULL) {
		ctx->xmlrpc_session = linphone_xml_rpc_session_new(ctx->cfg->lc, XMLRPC_URL);
	}
	if (linphone_xml_rpc_session_send_request(ctx->xmlrpc_session, request) == LinphoneXmlRpcStatusOk) {
		ret = linphone_xml_rpc_request_get_int_response(request);
	}
	linphone_xml_rpc_request_unref(request);

	return ret;
}

/*
 * Return 1 if account already exists
 * 0 if account doesn't exists
 * -1 if information isn't available
 */
static int sip_wizard_account_exists(SipSetupContext *ctx, const char *identity) {
	LinphoneXmlRpcRequest *request = linphone_xml_rpc_request_new_with_args("check_account",
		LinphoneXmlRpcArgInt,
		LinphoneXmlRpcArgString, identity,
		LinphoneXmlRpcArgNone);
	return do_simple_xmlrpc_request(ctx, request);
}

static int sip_wizard_account_validated(SipSetupContext *ctx, const char *identity) {
	LinphoneXmlRpcRequest *request = linphone_xml_rpc_request_new_with_args("check_account_validated",
		LinphoneXmlRpcArgInt,
		LinphoneXmlRpcArgString, identity,
		LinphoneXmlRpcArgNone);
        return do_simple_xmlrpc_request(ctx, request);
}

static int sip_wizard_create_account(SipSetupContext *ctx, const char *identity, const char *passwd, const char *email, int subscribe) {
	LinphoneXmlRpcRequest *request = linphone_xml_rpc_request_new_with_args("create_account",
		LinphoneXmlRpcArgInt,
		LinphoneXmlRpcArgString, identity,
		LinphoneXmlRpcArgString, passwd,
		LinphoneXmlRpcArgString, email,
		LinphoneXmlRpcArgInt, subscribe,
		LinphoneXmlRpcArgNone);
	return do_simple_xmlrpc_request(ctx, request);
}

static void guess_display_name(LinphoneAddress *from){
	const char *username=linphone_address_get_username(from);
	char *dn=(char*)ms_malloc(strlen(username)+1);
	const char *it;
	char *wptr=dn;
	bool_t begin=TRUE;
	bool_t surname=FALSE;
	for(it=username;*it!='\0';++it){
		if (begin){
			*wptr=toupper(*it);
			begin=FALSE;
		}else if (*it=='.'){
			if (surname) break;
			*wptr=' ';
			begin=TRUE;
			surname=TRUE;
		}else {
			*wptr=*it;
		}
		wptr++;
	}
	*wptr='\0';
	linphone_address_set_display_name(from,dn);
	ms_free(dn);
}

static int sip_wizard_do_login(SipSetupContext * ctx, const char *uri, const char *passwd, const char *userid){
	LinphoneProxyConfig *cfg=sip_setup_context_get_proxy_config(ctx);
	LinphoneCore *lc=linphone_proxy_config_get_core(cfg);
	LinphoneAuthInfo *auth;
	LinphoneAddress *parsed_uri;
	char *tmp;

	parsed_uri=linphone_address_new(uri);
	if (parsed_uri==NULL){
		return -1;
	}
	if (linphone_address_get_display_name(parsed_uri)!=NULL){
		guess_display_name(parsed_uri);
	}
	tmp=linphone_address_as_string(parsed_uri);
	linphone_proxy_config_set_identity(cfg,tmp);
	if (passwd) {
		auth=linphone_auth_info_new(linphone_address_get_username(parsed_uri),NULL,passwd,NULL,NULL,NULL);
		linphone_core_add_auth_info(lc,auth);
	}
	linphone_proxy_config_enable_register(cfg,TRUE);
	linphone_proxy_config_done(cfg);
	ms_free(tmp);
	linphone_address_destroy(parsed_uri);
	return 0;
}

/* a simple SipSetup built-in plugin to allow creating accounts at runtime*/

#ifndef _MSC_VER

SipSetup linphone_sip_wizard={
	.name="SipWizard",
	.capabilities=SIP_SETUP_CAP_ACCOUNT_MANAGER,
	.init_instance=sip_wizard_init_instance,
	.uninit_instance=sip_wizard_uninit_instance,
	.account_exists=sip_wizard_account_exists,
	.create_account=sip_wizard_create_account,
	.login_account=sip_wizard_do_login,
	.get_domains=sip_wizard_get_domains,
	.account_validated=sip_wizard_account_validated
};

#else
SipSetup linphone_sip_wizard={
	"SipWizard",
	SIP_SETUP_CAP_ACCOUNT_MANAGER,
	0,
	NULL,
	NULL,
	sip_wizard_init_instance,
	sip_wizard_uninit_instance,
	sip_wizard_account_exists,
	sip_wizard_create_account,
	sip_wizard_do_login,
	NULL,
	NULL,
	NULL,
	NULL,
	sip_wizard_get_domains,
	NULL,
	NULL,
	sip_wizard_account_validated
};

#endif
