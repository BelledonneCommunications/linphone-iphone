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


#include "sipsetup.h"
#include "p2pproxy.h"

static ms_thread_t fonis_thread;


static void *fonis_thread_func(void *arg){
	if (p2pproxy_application_start(0,NULL)!=0){
		ms_error("Fail to start fonis thread !");
	}
	return NULL;
}

static bool_t fonis_init(void){
	static bool_t initd=FALSE;
	if (!initd){
		ms_thread_create(&fonis_thread,NULL,fonis_thread_func,NULL);
		initd=TRUE;
	}
	return TRUE;
}


static int fonis_create_account(const char *uri, const char *passwd){
	int err=p2pproxy_accountmgt_createAccount(uri);
	if (err<0) return -1;
	return 0;
}

static int fonis_login_account(SipSetupContext * ctx,const char *uri, const char *passwd){
	if (p2pproxy_accountmgt_isValidAccount==P2PPROXY_ACCOUNTMGT_USER_EXIST) {
		return 0;
	}
	else return -1;
}

static int fonis_get_proxy(SipSetupContext *ctx, const char *domain, char *proxy, size_t sz){
	int err=p2pproxy_resourcemgt_lookup_sip_proxy(proxy,sz,(char*)domain);
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

static int fonis_get_stun_relay(SipSetupContext *ctx, char *relay, size_t size){
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


SipSetup fonis_sip_setup={
	.name="fonis",
	.init=fonis_init,
	.create_account=fonis_create_account,
	.login_account=fonis_login_account,
	.get_proxy=fonis_get_proxy,
	.get_stun_servers=fonis_get_stun_servers,
	.get_relay=fonis_get_relay,
	.exit=p2pproxy_application_stop
};

