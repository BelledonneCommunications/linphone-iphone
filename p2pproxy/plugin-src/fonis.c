/*
linphone
Copyright (C) 2000  Simon MORLAT (simon.morlat@linphone.org)

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

#include "p2pproxy.h"

typedef struct _FonisContext{
	int toto;
}FonisContext;

static ms_thread_t fonis_thread;


static void *fonis_thread_func(void *arg){
	char *argv[]={"-edge-only","-sip", "5058",NULL};
	if (p2pproxy_application_start(3,argv)!=0){
		ms_error("Fail to start fonis thread !");
	}
	return NULL;
}

static bool_t fonis_init(void){
	static bool_t initd=FALSE;
	if (!initd){
		ms_thread_create(&fonis_thread,NULL,fonis_thread_func,NULL);
		initd=TRUE;
		while( p2pproxy_application_get_state()==P2PPROXY_ERROR_APPLICATION_NOT_STARTED){
			usleep(200000);
			ms_message("Waiting for p2pproxy to start");
		}
	}
	return TRUE;
}

static bool_t fonis_check_connected(SipSetupContext *ctx){
	int retries;
	LinphoneCore *lc=linphone_proxy_config_get_core(sip_setup_context_get_proxy_config(ctx));
	for(retries=0;retries<1200;retries++){
		if (p2pproxy_application_get_state()==P2PPROXY_CONNECTED){
			if (retries>0) linphone_core_stop_waiting(lc);
			return TRUE;
		}
		if (retries==0){
			ms_message("Waiting for p2pproxy to connect to the network...");
			linphone_core_start_waiting(lc,"Trying to connect to the fonis network...");
		}else linphone_core_update_progress(lc,NULL,-1);
	}
	linphone_core_stop_waiting(lc);
	return FALSE;
}

static int fonis_test_account(SipSetupContext *ctx, const char *uri){
	int ret;
	LinphoneCore *lc=linphone_proxy_config_get_core(sip_setup_context_get_proxy_config(ctx));
	if (!fonis_check_connected(ctx)) return -1;
	linphone_core_start_waiting(lc,"Checking...");
	ret=(p2pproxy_accountmgt_isValidAccount(uri)==P2PPROXY_ACCOUNTMGT_USER_EXIST) ? 1 : 0;
	linphone_core_update_progress(lc,NULL,1);
	linphone_core_stop_waiting(lc);
	return ret;
}

static int fonis_create_account(SipSetupContext *ctx, const char *uri, const char *passwd){
	int err;
	LinphoneCore *lc=linphone_proxy_config_get_core(sip_setup_context_get_proxy_config(ctx));
	if (!fonis_check_connected(ctx)) return -1;
	linphone_core_start_waiting(lc,"Creating account...");
	err=p2pproxy_accountmgt_createAccount(uri);
	ms_message("Account creation result for %s: %i",uri,err);
	linphone_core_update_progress(lc,NULL,1);
	linphone_core_stop_waiting(lc);
	if (err<0) return -1;
	return 0;
}

static int fonis_login_account(SipSetupContext * ctx,const char *uri, const char *passwd){
	if (!fonis_check_connected(ctx)) return -1;
	if (p2pproxy_accountmgt_isValidAccount(uri)==P2PPROXY_ACCOUNTMGT_USER_EXIST) {
		return 0;
	}
	else return -1;
}

static int fonis_get_proxy(SipSetupContext *ctx, const char *domain, char *proxy, size_t sz){
	int err;
	if (!fonis_check_connected(ctx)) return -1;
	err=p2pproxy_resourcemgt_lookup_sip_proxy(proxy,sz,(char*)domain);
	if (err==0) return 0;
	else return -1;
}

static int fonis_get_stun_servers(SipSetupContext *ctx, char *stun1, char *stun2, size_t size){
	FonisContext *fc=(FonisContext*)ctx->data;
	int ret=-1;
	p2pproxy_resourcemgt_resource_list_t* l=p2pproxy_resourcemgt_new_resource_list();
	if (p2pproxy_resourcemgt_lookup_media_resource(l,ctx->domain)==0){
		if (l->size>0) strncpy(stun1,l->resource_uri[0],size);
		if (l->size>1) strncpy(stun2,l->resource_uri[1],size);
		ret=0;
	}
	p2pproxy_resourcemgt_delete_resource_list(l);
	return ret;
}

static int fonis_get_relay(SipSetupContext *ctx, char *relay, size_t size){
	FonisContext *fc=(FonisContext*)ctx->data;
	int ret=-1;
	p2pproxy_resourcemgt_resource_list_t* l=p2pproxy_resourcemgt_new_resource_list();
	if (p2pproxy_resourcemgt_lookup_media_resource(l,ctx->domain)==0){
		if (l->size>0) strncpy(relay,l->resource_uri[0],size);
		ret=0;
	}
	p2pproxy_resourcemgt_delete_resource_list(l);
	return ret;
}

static void fonis_exit(){
	p2pproxy_application_stop();
}

static const char *fonis_get_notice(SipSetupContext *ssctx){
	return "<b>WARNING: experimental feature !</b>"
"FONIS stands for Free Overlay Network for Instant SIP.\n"
"Based on SIP and Peer to Peer technologies, it allows people to find each other through the help of a virtual network."
"Once you and your friends are registered, you'll be able to call each other simply by entering your friend's username in "
"linphone's sip uri box."
"Read more information about FONIS at http://www.fonis.org";
}

static const char *fonis_domains[]={
	"p2p.linphone.org",
	NULL
};

static const char ** fonis_get_domains(SipSetupContext *ssctx){
	return fonis_domains;
}



static SipSetup fonis_sip_setup={
	.capabilities=SIP_SETUP_CAP_PROXY_PROVIDER|SIP_SETUP_CAP_STUN_PROVIDER|
				SIP_SETUP_CAP_RELAY_PROVIDER|SIP_SETUP_CAP_ACCOUNT_MANAGER,
	.name="fonis",
	.init=fonis_init,
	.account_exists=fonis_test_account,
	.create_account=fonis_create_account,
	.login_account=fonis_login_account,
	.get_proxy=fonis_get_proxy,
	.get_stun_servers=fonis_get_stun_servers,
	.get_relay=fonis_get_relay,
	.exit=fonis_exit,
	.get_notice=fonis_get_notice,
	.get_domains=fonis_get_domains
};

void libfonisprovider_init(void){
	sip_setup_register(&fonis_sip_setup);
}

