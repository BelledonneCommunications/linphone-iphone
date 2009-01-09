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

#ifdef HAVE_FONIS
extern SipSetup fonis_sip_setup;
#endif

void sip_setup_register_all(void){
}

SipSetup *sip_setup_lookup(const char *type_name){
#ifdef HAVE_FONIS
	if (strcmp(type_name,"fonis")==0){
		if (!fonis_sip_setup.initialized){
			if (fonis_sip_setup.init()){
				fonis_sip_setup.initialized=TRUE;
			}
		}
		return &fonis_sip_setup;
	}
#endif
	ms_warning("no %s setup manager declared.",type_name);
	return NULL;
}

void sip_setup_unregister_all(void){
#ifdef HAVE_FONIS
	if (fonis_sip_setup.initialized)
		fonis_sip_setup.exit();
#endif
}


SipSetupContext *sip_setup_context_new(SipSetup *s){
	SipSetupContext *obj=(SipSetupContext*)ms_new0(SipSetupContext,1);
	obj->funcs=s;
	obj->data=NULL;
	return obj;
}

int sip_setup_new_account(SipSetup *funcs, const char *uri, const char *passwd){
	if (funcs->create_account)
		return funcs->create_account(uri, passwd);
	else return -1;
}

int sip_setup_context_login_account(SipSetupContext * ctx, const char *uri, const char *passwd){
	osip_from_t *from;
	osip_from_init(&from);
	osip_from_parse(from,uri);
	strncpy(ctx->domain,from->url->host,sizeof(ctx->domain));
	strncpy(ctx->username,from->url->username,sizeof(ctx->username));
	osip_from_free(from);
	if (ctx->funcs->login_account)
		return ctx->funcs->login_account(ctx,uri,passwd);
	return -1;
}

int sip_setup_context_get_proxy(SipSetupContext *ctx, const char *domain, char *proxy, size_t sz){
	if (ctx->funcs->get_proxy)
		return ctx->funcs->get_proxy(ctx,domain ? domain : ctx->domain,proxy,sz);
	return -1;
}

int sip_setup_context_get_stun_servers(SipSetupContext *ctx, char *stun1, char *stun2, size_t size){
	if (ctx->funcs->get_stun_servers)
		return ctx->funcs->get_stun_servers(ctx,stun1,stun2,size);
	return -1;
}

int sip_setup_context_get_relay(SipSetupContext *ctx,char *relay, size_t size){
	if (ctx->funcs->get_relay)
		return ctx->funcs->get_relay(ctx,relay,size);
	return -1;
}

void sip_setup_context_free(SipSetupContext *ctx){
	ms_free(ctx);
}
