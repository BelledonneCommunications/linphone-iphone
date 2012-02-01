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
#include <libsoup/soup.h>

typedef struct _BLReq{
	int status;
	int result;
	SoupMessage *msg;
	SoupSession *session;
	ortp_thread_t th;
}BLReq;

static const int XMLRPC_FAILED = -1;
static const int XMLRPC_OK = 0;
static const char *XMLRPC_URL = "https://www.linphone.org/wizard.php";

static void sip_wizard_init_instance(SipSetupContext *ctx){
	LinphoneProxyConfig *cfg=sip_setup_context_get_proxy_config(ctx);
	/*disable registration until the user logs in*/
	linphone_proxy_config_enable_register(cfg,FALSE);
}

static const char ** sip_wizard_get_domains(SipSetupContext *ctx) {
	LinphoneProxyConfig *cfg=sip_setup_context_get_proxy_config(ctx);
	const char **domains = (const char**) &cfg->reg_proxy;
	return domains;
}


static int xml_rpc_parse_response(BLReq *blreq, SoupMessage *sm){
	SoupBuffer *sb;
	GValue retval;
	GError *error=NULL;
	sb=soup_message_body_flatten(sm->response_body);
	ms_message("This the xml-rpc response:\n%s\n",sb->data);
	if (soup_xmlrpc_parse_method_response(sb->data,sb->length,&retval,&error)==FALSE){
		if (error!=NULL){
			ms_error("xmlrpc fault: %s",error->message);
			g_error_free(error);
		}else{
			ms_error("Could not parse xml-rpc response !");
		}
		blreq->status=XMLRPC_FAILED;
	}else{
		ms_message("Extracting values from return type...");
		blreq->result = g_value_get_int(&retval);
		g_value_unset(&retval);
		blreq->status=XMLRPC_OK;
	}
	soup_buffer_free(sb);
	return blreq->status;
}

static void got_headers(BLReq *blreq, SoupMessage*msg){
	ms_message("Got headers !");
	blreq->status=XMLRPC_OK;
}

#if SERIALIZE_HTTPS
/*on windows libsoup support for threads with gnutls is not yet functionnal (only in git)
This will come in next release of libsoup, probably.
In the meantime, we are forced to serialize all soup https processing with a big
ugly global mutex...*/

static GStaticMutex big_mutex = G_STATIC_MUTEX_INIT;
#endif

static void * process_xml_rpc_request(void *up){
	BLReq *blreq=(BLReq*)up;
	SoupMessage *sm=blreq->msg;
	int code;
	g_signal_connect_swapped(G_OBJECT(sm),"got-headers",(GCallback)got_headers,blreq);
	blreq->status=XMLRPC_OK;
#if SERIALIZE_HTTPS
	g_static_mutex_lock(&big_mutex);
#endif
	code=soup_session_send_message(blreq->session,sm);
	if (code==200){
		xml_rpc_parse_response(blreq,sm);
	}else{
		ms_error("request failed, error-code=%i (%s)",code,soup_status_get_phrase(code));
		blreq->status=XMLRPC_FAILED;
	}
#if SERIALIZE_HTTPS
	g_static_mutex_unlock(&big_mutex);
#endif
	return NULL;
}


static int do_simple_xmlrpc_request(SoupMessage *msg) {
        int ret=-1;
        BLReq *req;

	if (!msg){
		ms_error("Fail to create SoupMessage !");
		return -1;
	}else{
		SoupBuffer *sb=soup_message_body_flatten(msg->request_body);
		ms_message("This is the XML-RPC request we are going to send:\n%s\n",sb->data);
		soup_buffer_free(sb);
	}

	req=ms_new0(BLReq, 1);
        req->session=soup_session_sync_new();
        req->msg=msg;

        process_xml_rpc_request(req);

        if (req->status == XMLRPC_OK) {
                ret=req->result;
        }

	// Freeing allocated structures lead to a crash (why?)
	//g_free(req->session);
	//g_free(msg);
        ms_free(req);

        return ret;
}

/*
 * Return 1 if account already exists
 * 0 if account doesn't exists
 * -1 if information isn't available
 */
static int sip_wizard_account_exists(SipSetupContext *ctx, const char *identity) {
	SoupMessage *msg=soup_xmlrpc_request_new(XMLRPC_URL,
                                "check_account",
                                G_TYPE_STRING, identity,
                                G_TYPE_INVALID);
	return do_simple_xmlrpc_request(msg);
}

static int sip_wizard_account_validated(SipSetupContext *ctx, const char *identity) {
	SoupMessage *msg=soup_xmlrpc_request_new(XMLRPC_URL,
                                "check_account_validated",
                                G_TYPE_STRING, identity,
                                G_TYPE_INVALID);
        return do_simple_xmlrpc_request(msg);
}

static int sip_wizard_create_account(SipSetupContext *ctx, const char *identity, const char *passwd, const char *email, int suscribe) {
	SoupMessage *msg=soup_xmlrpc_request_new(XMLRPC_URL,
				"create_account",
				G_TYPE_STRING, identity,
				G_TYPE_STRING, passwd,
				G_TYPE_STRING, email,
				G_TYPE_INT, suscribe,
				G_TYPE_INVALID);
	return do_simple_xmlrpc_request(msg);
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

static int sip_wizard_do_login(SipSetupContext * ctx, const char *uri, const char *passwd){
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
		auth=linphone_auth_info_new(linphone_address_get_username(parsed_uri),NULL,passwd,NULL,NULL);
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
	NULL,
	sip_wizard_account_exists,
	sip_wizard_create_account,
	sip_wizard_do_login,
	NULL,
	NULL,
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
