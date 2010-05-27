/*
linphone
Copyright (C) 2010  Simon MORLAT (simon.morlat@free.fr)

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
#include "config.h"
#endif

#include "sal_eXosip2.h"

#include "offeranswer.h"

static void text_received(Sal *sal, eXosip_event_t *ev);

static void _osip_list_set_empty(osip_list_t *l, void (*freefunc)(void*)){
	void *data;
	while((data=osip_list_get(l,0))!=NULL){
		osip_list_remove(l,0);
		freefunc(data);
	}
}

void sal_get_default_local_ip(Sal *sal, int address_family,char *ip, size_t iplen){
	if (eXosip_guess_localip(address_family,ip,iplen)<0){
		/*default to something */
		strncpy(ip,address_family==AF_INET6 ? "::1" : "127.0.0.1",iplen);
		ms_error("Could not find default routable ip address !");
	}
}


static SalOp * sal_find_call(Sal *sal, int cid){
	const MSList *elem;
	SalOp *op;
	for(elem=sal->calls;elem!=NULL;elem=elem->next){
		op=(SalOp*)elem->data;
		if (op->cid==cid) return op;
	}
	return NULL;
}

static void sal_add_call(Sal *sal, SalOp *op){
	sal->calls=ms_list_append(sal->calls,op);
}

static void sal_remove_call(Sal *sal, SalOp *op){
	sal->calls=ms_list_remove(sal->calls, op);
}

static SalOp * sal_find_register(Sal *sal, int rid){
	const MSList *elem;
	SalOp *op;
	for(elem=sal->registers;elem!=NULL;elem=elem->next){
		op=(SalOp*)elem->data;
		if (op->rid==rid) return op;
	}
	return NULL;
}

static void sal_add_register(Sal *sal, SalOp *op){
	sal->registers=ms_list_append(sal->registers,op);
}

static void sal_remove_register(Sal *sal, int rid){
	MSList *elem;
	SalOp *op;
	for(elem=sal->registers;elem!=NULL;elem=elem->next){
		op=(SalOp*)elem->data;
		if (op->rid==rid) {
			sal->registers=ms_list_remove_link(sal->registers,elem);
			return;
		}
	}
}

static SalOp * sal_find_other(Sal *sal, osip_message_t *response){
	const MSList *elem;
	SalOp *op;
	osip_call_id_t *callid=osip_message_get_call_id(response);
	if (callid==NULL) {
		ms_error("There is no call-id in this response !");
		return NULL;
	}
	for(elem=sal->other_transactions;elem!=NULL;elem=elem->next){
		op=(SalOp*)elem->data;
		if (osip_call_id_match(callid,op->call_id)==0) return op;
	}
	return NULL;
}

static void sal_add_other(Sal *sal, SalOp *op, osip_message_t *request){
	osip_call_id_t *callid=osip_message_get_call_id(request);
	if (callid==NULL) {
		ms_error("There is no call id in the request !");
		return;
	}
	osip_call_id_clone(callid,&op->call_id);
	sal->other_transactions=ms_list_append(sal->other_transactions,op);
}

static void sal_remove_other(Sal *sal, SalOp *op){
	sal->other_transactions=ms_list_remove(sal->other_transactions,op);
}


static void sal_add_pending_auth(Sal *sal, SalOp *op){
	sal->pending_auths=ms_list_append(sal->pending_auths,op);
}


static void sal_remove_pending_auth(Sal *sal, SalOp *op){
	sal->pending_auths=ms_list_remove(sal->pending_auths,op);
}

void sal_exosip_fix_route(SalOp *op){
	if (sal_op_get_route(op)!=NULL){
		osip_route_t *rt=NULL;
		osip_uri_param_t *lr_param=NULL;
		
		osip_route_init(&rt);
		if (osip_route_parse(rt,sal_op_get_route(op))<0){
			ms_warning("Bad route  %s!",sal_op_get_route(op));
			sal_op_set_route(op,NULL);
		}else{
			/* check if the lr parameter is set , if not add it */
			osip_uri_uparam_get_byname(rt->url, "lr", &lr_param);
		  	if (lr_param==NULL){
				char *tmproute;
				osip_uri_uparam_add(rt->url,osip_strdup("lr"),NULL);
				osip_route_to_str(rt,&tmproute);
				sal_op_set_route(op,tmproute);
				osip_free(tmproute);
			}
		}
		osip_route_free(rt);
	}
}

SalOp * sal_op_new(Sal *sal){
	SalOp *op=ms_new(SalOp,1);
	__sal_op_init(op,sal);
	op->cid=op->did=op->tid=op->rid=op->nid=op->sid=-1;
	op->result=NULL;
	op->supports_session_timers=FALSE;
	op->sdp_offering=TRUE;
	op->pending_auth=NULL;
	op->sdp_answer=NULL;
	op->reinvite=FALSE;
	op->call_id=NULL;
	op->masquerade_via=FALSE;
	op->auto_answer_asked=FALSE;
	return op;
}

bool_t sal_call_autoanswer_asked(SalOp *op)
{
	return op->auto_answer_asked;
}

void sal_op_release(SalOp *op){
	if (op->sdp_answer)
		sdp_message_free(op->sdp_answer);
	if (op->pending_auth)
		eXosip_event_free(op->pending_auth);
	if (op->rid!=-1){
		sal_remove_register(op->base.root,op->rid);
	}
	if (op->cid!=-1){
		ms_message("Cleaning cid %i",op->cid);
		sal_remove_call(op->base.root,op);
	}
	if (op->sid!=-1){
		sal_remove_out_subscribe(op->base.root,op);
	}
	if (op->nid!=-1){
		sal_remove_in_subscribe(op->base.root,op);
		if (op->call_id)
			osip_call_id_free(op->call_id);
		op->call_id=NULL;
	}
	if (op->pending_auth){
		sal_remove_pending_auth(op->base.root,op);
	}
	if (op->result)
		sal_media_description_unref(op->result);
	if (op->call_id){
		sal_remove_other(op->base.root,op);
		osip_call_id_free(op->call_id);
	}
	__sal_op_free(op);
}

static void _osip_trace_func(char *fi, int li, osip_trace_level_t level, char *chfr, va_list ap){
	int ortp_level=ORTP_DEBUG;
	switch(level){
		case OSIP_INFO1:
		case OSIP_INFO2:
		case OSIP_INFO3:
		case OSIP_INFO4:
			ortp_level=ORTP_MESSAGE;
			break;
		case OSIP_WARNING:
			ortp_level=ORTP_WARNING;
			break;
		case OSIP_ERROR:
		case OSIP_BUG:
			ortp_level=ORTP_ERROR;
			break;
		case OSIP_FATAL:
			ortp_level=ORTP_FATAL;
			break;
		case END_TRACE_LEVEL:
			break;
	}
	if (ortp_log_level_enabled(level)){
		int len=strlen(chfr);
		char *chfrdup=ortp_strdup(chfr);
		/*need to remove endline*/
		if (len>1){
			if (chfrdup[len-1]=='\n')
				chfrdup[len-1]='\0';
			if (chfrdup[len-2]=='\r')
				chfrdup[len-2]='\0';
		}
		ortp_logv(ortp_level,chfrdup,ap);
		ortp_free(chfrdup);
	}
}


Sal * sal_init(){
	static bool_t firsttime=TRUE;
	Sal *sal;
	if (firsttime){
		osip_trace_initialize_func (OSIP_INFO4,&_osip_trace_func);
		firsttime=FALSE;
	}
	eXosip_init();
	sal=ms_new0(Sal,1);
	return sal;
}

void sal_uninit(Sal* sal){
	eXosip_quit();
	ms_free(sal);
}

void sal_set_user_pointer(Sal *sal, void *user_data){
	sal->up=user_data;
}

void *sal_get_user_pointer(const Sal *sal){
	return sal->up;
}

static void unimplemented_stub(){
	ms_warning("Unimplemented SAL callback");
}

void sal_set_callbacks(Sal *ctx, const SalCallbacks *cbs){
	memcpy(&ctx->callbacks,cbs,sizeof(*cbs));
	if (ctx->callbacks.call_received==NULL) 
		ctx->callbacks.call_received=(SalOnCallReceived)unimplemented_stub;
	if (ctx->callbacks.call_ringing==NULL) 
		ctx->callbacks.call_ringing=(SalOnCallRinging)unimplemented_stub;
	if (ctx->callbacks.call_accepted==NULL) 
		ctx->callbacks.call_accepted=(SalOnCallAccepted)unimplemented_stub;
	if (ctx->callbacks.call_failure==NULL) 
		ctx->callbacks.call_failure=(SalOnCallFailure)unimplemented_stub;
	if (ctx->callbacks.call_terminated==NULL) 
		ctx->callbacks.call_terminated=(SalOnCallTerminated)unimplemented_stub;
	if (ctx->callbacks.call_updated==NULL) 
		ctx->callbacks.call_updated=(SalOnCallUpdated)unimplemented_stub;
	if (ctx->callbacks.auth_requested==NULL) 
		ctx->callbacks.auth_requested=(SalOnAuthRequested)unimplemented_stub;
	if (ctx->callbacks.auth_success==NULL) 
		ctx->callbacks.auth_success=(SalOnAuthSuccess)unimplemented_stub;
	if (ctx->callbacks.register_success==NULL) 
		ctx->callbacks.register_success=(SalOnRegisterSuccess)unimplemented_stub;
	if (ctx->callbacks.register_failure==NULL) 
		ctx->callbacks.register_failure=(SalOnRegisterFailure)unimplemented_stub;
	if (ctx->callbacks.dtmf_received==NULL) 
		ctx->callbacks.dtmf_received=(SalOnDtmfReceived)unimplemented_stub;
	if (ctx->callbacks.notify==NULL)
		ctx->callbacks.notify=(SalOnNotify)unimplemented_stub;
	if (ctx->callbacks.notify_presence==NULL)
		ctx->callbacks.notify_presence=(SalOnNotifyPresence)unimplemented_stub;
	if (ctx->callbacks.subscribe_received==NULL)
		ctx->callbacks.subscribe_received=(SalOnSubscribeReceived)unimplemented_stub;
	if (ctx->callbacks.text_received==NULL)
		ctx->callbacks.text_received=(SalOnTextReceived)unimplemented_stub;
	if (ctx->callbacks.internal_message==NULL)
		ctx->callbacks.internal_message=(SalOnInternalMsg)unimplemented_stub;
	if (ctx->callbacks.ping_reply==NULL)
		ctx->callbacks.ping_reply=(SalOnPingReply)unimplemented_stub;
}

int sal_listen_port(Sal *ctx, const char *addr, int port, SalTransport tr, int is_secure){
	int err;
	bool_t ipv6;
	int proto=IPPROTO_UDP;
	
	if (ctx->running){
		eXosip_quit();
		eXosip_init();
	}
	err=0;
	eXosip_set_option(13,&err); /*13=EXOSIP_OPT_SRV_WITH_NAPTR, as it is an enum value, we can't use it unless we are sure of the
					version of eXosip, which is not the case*/
	/*see if it looks like an IPv6 address*/
	ipv6=strchr(addr,':')!=NULL;
	eXosip_enable_ipv6(ipv6);

	if (tr!=SalTransportDatagram || is_secure){
		ms_fatal("SIP over TCP or TLS or DTLS is not supported yet.");
		return -1;
	}
	err=eXosip_listen_addr(proto, addr, port, ipv6 ?  PF_INET6 : PF_INET, 0);
#ifdef HAVE_EXOSIP_GET_SOCKET
	ms_message("Exosip has socket number %i",eXosip_get_socket(proto));
#endif
	ctx->running=TRUE;
	return err;
}

ortp_socket_t sal_get_socket(Sal *ctx){
#ifdef HAVE_EXOSIP_GET_SOCKET
	return eXosip_get_socket(IPPROTO_UDP);
#else
	ms_warning("Sorry, eXosip does not have eXosip_get_socket() method");
	return -1;
#endif
}

void sal_set_user_agent(Sal *ctx, const char *user_agent){
	eXosip_set_user_agent(user_agent);
}

void sal_use_session_timers(Sal *ctx, int expires){
	ctx->session_expires=expires;
}

MSList *sal_get_pending_auths(Sal *sal){
	return ms_list_copy(sal->pending_auths);
}

static int extract_received_rport(osip_message_t *msg, const char **received, int *rportval){
	osip_via_t *via=NULL;
	osip_generic_param_t *param=NULL;
	const char *rport=NULL;

	*rportval=5060;
	*received=NULL;
	osip_message_get_via(msg,0,&via);
	if (!via) return -1;
	if (via->port && via->port[0]!='\0')
		*rportval=atoi(via->port);
	
	osip_via_param_get_byname(via,"rport",&param);
	if (param) {
		rport=param->gvalue;
		if (rport && rport[0]!='\0') *rportval=atoi(rport);
		else *rportval=5060;
		*received=via->host;
	}
	param=NULL;
	osip_via_param_get_byname(via,"received",&param);
	if (param) *received=param->gvalue;

	if (rport==NULL && *received==NULL) return -1;
	return 0;
}

static void set_sdp(osip_message_t *sip,sdp_message_t *msg){
	int sdplen;
	char clen[10];
	char *sdp=NULL;
	sdp_message_to_str(msg,&sdp);
	sdplen=strlen(sdp);
	snprintf(clen,sizeof(clen),"%i",sdplen);
	osip_message_set_body(sip,sdp,sdplen);
	osip_message_set_content_type(sip,"application/sdp");
	osip_message_set_content_length(sip,clen);
	osip_free(sdp);
}

static void set_sdp_from_desc(osip_message_t *sip, const SalMediaDescription *desc){
	sdp_message_t *msg=media_description_to_sdp(desc);
	if (msg==NULL) {
		ms_error("Fail to print sdp message !");
		return;
	}
	set_sdp(sip,msg);
	sdp_message_free(msg);
}

static void sdp_process(SalOp *h){
	ms_message("Doing SDP offer/answer process");
	if (h->result){
		sal_media_description_unref(h->result);
	}
	h->result=sal_media_description_new();
	if (h->sdp_offering){	
		offer_answer_initiate_outgoing(h->base.local_media,h->base.remote_media,h->result);
	}else{
		int i;
		offer_answer_initiate_incoming(h->base.local_media,h->base.remote_media,h->result);
		h->sdp_answer=media_description_to_sdp(h->result);
		strcpy(h->result->addr,h->base.remote_media->addr);
		h->result->bandwidth=h->base.remote_media->bandwidth;
		for(i=0;i<h->result->nstreams;++i){
			if (h->result->streams[i].port>0){
				strcpy(h->result->streams[i].addr,h->base.remote_media->streams[i].addr);
				h->result->streams[i].ptime=h->base.remote_media->streams[i].ptime;
				h->result->streams[i].bandwidth=h->base.remote_media->streams[i].bandwidth;
				h->result->streams[i].port=h->base.remote_media->streams[i].port;
			}
		}
	}
	
}

int sal_call_set_local_media_description(SalOp *h, SalMediaDescription *desc){
	if (desc)
		sal_media_description_ref(desc);
	if (h->base.local_media)
		sal_media_description_unref(h->base.local_media);
	h->base.local_media=desc;
	return 0;
}

int sal_call(SalOp *h, const char *from, const char *to){
	int err;
	osip_message_t *invite=NULL;
	sal_op_set_from(h,from);
	sal_op_set_to(h,to);
	sal_exosip_fix_route(h);
	err=eXosip_call_build_initial_invite(&invite,to,from,sal_op_get_route(h),"Phone call");
	if (err!=0){
		ms_error("Could not create call.");
		return -1;
	}
	osip_message_set_allow(invite, "INVITE, ACK, CANCEL, OPTIONS, BYE, REFER, NOTIFY, MESSAGE, SUBSCRIBE, INFO");
	if (h->base.contact){
		_osip_list_set_empty(&invite->contacts,(void (*)(void*))osip_contact_free);
		osip_message_set_contact(invite,h->base.contact);
	}
	if (h->base.root->session_expires!=0){
		osip_message_set_header(invite, "Session-expires", "200");
		osip_message_set_supported(invite, "timer");
	}
	if (h->base.local_media){
		h->sdp_offering=TRUE;
		set_sdp_from_desc(invite,h->base.local_media);
	}else h->sdp_offering=FALSE;
	eXosip_lock();
	err=eXosip_call_send_initial_invite(invite);
	eXosip_unlock();
	h->cid=err;
	if (err<0){
		ms_error("Fail to send invite !");
		return -1;
	}else{
		sal_add_call(h->base.root,h);
	}
	return 0;
}

int sal_call_notify_ringing(SalOp *h){
	eXosip_lock();
	eXosip_call_send_answer(h->tid,180,NULL);
	eXosip_unlock();
	return 0;
}

int sal_call_accept(SalOp * h){
	osip_message_t *msg;
	const char *contact=sal_op_get_contact(h);
	/* sends a 200 OK */
	int err=eXosip_call_build_answer(h->tid,200,&msg);
	if (err<0 || msg==NULL){
		ms_error("Fail to build answer for call: err=%i",err);
		return -1;
	}
	if (h->base.root->session_expires!=0){
		if (h->supports_session_timers) osip_message_set_supported(msg, "timer");
	}

	if (contact) {
		_osip_list_set_empty(&msg->contacts,(void (*)(void*))osip_contact_free);
		osip_message_set_contact(msg,contact);
	}
	
	if (h->base.local_media){
		/*this is the case where we received an invite without SDP*/
		if (h->sdp_offering) {
			set_sdp_from_desc(msg,h->base.local_media);
		}else{
			if (h->sdp_answer)
				set_sdp(msg,h->sdp_answer);
		}
	}else{
		ms_error("You are accepting a call but not defined any media capabilities !");
	}
	eXosip_call_send_answer(h->tid,200,msg);
	return 0;
}

int sal_call_decline(SalOp *h, SalReason reason, const char *redirect){
	if (reason==SalReasonBusy){
		eXosip_lock();
		eXosip_call_send_answer(h->tid,486,NULL);
		eXosip_unlock();
	}
	else if (reason==SalReasonTemporarilyUnavailable){
		eXosip_lock();
		eXosip_call_send_answer(h->tid,480,NULL);
		eXosip_unlock();
	}else if (reason==SalReasonDoNotDisturb){
		eXosip_lock();
		eXosip_call_send_answer(h->tid,600,NULL);
		eXosip_unlock();
	}else if (reason==SalReasonMedia){
		eXosip_lock();
		eXosip_call_send_answer(h->tid,415,NULL);
		eXosip_unlock();
	}else if (redirect!=NULL && reason==SalReasonRedirect){
		osip_message_t *msg;
		int code;
		if (strstr(redirect,"sip:")!=0) code=302;
		else code=380;
		eXosip_lock();
		eXosip_call_build_answer(h->tid,code,&msg);
		osip_message_set_contact(msg,redirect);
		eXosip_call_send_answer(h->tid,code,msg);
		eXosip_unlock();
	}else sal_call_terminate(h);
	return 0;
}

SalMediaDescription * sal_call_get_final_media_description(SalOp *h){
	if (h->base.local_media && h->base.remote_media && !h->result){
		sdp_process(h);
	}
	return h->result;
}

int sal_ping(SalOp *op, const char *from, const char *to){
	osip_message_t *options=NULL;
	
	sal_op_set_from(op,from);
	sal_op_set_to(op,to);
	eXosip_options_build_request (&options, sal_op_get_to(op),
			sal_op_get_from(op),sal_op_get_route(op));
	if (options){
		if (op->base.root->session_expires!=0){
			osip_message_set_header(options, "Session-expires", "200");
			osip_message_set_supported(options, "timer");
		}
		sal_add_other(sal_op_get_sal(op),op,options);
		return eXosip_options_send_request(options);
	}
	return -1;
}

int sal_refer_accept(SalOp *op){
	osip_message_t *msg=NULL;
	int err=0;
	eXosip_lock();
	err = eXosip_call_build_notify(op->did,EXOSIP_SUBCRSTATE_ACTIVE,&msg);
	if(msg != NULL)
	{
		osip_message_set_header(msg,(const char *)"event","refer");
		osip_message_set_content_type(msg,"message/sipfrag");
		osip_message_set_body(msg,"SIP/2.0 100 Trying",sizeof("SIP/2.0 100 Trying"));
		eXosip_call_send_request(op->did,msg);
	}
	else
	{
		ms_error("could not get a notify built\n");
	}
	eXosip_unlock();
	return err;
}

int sal_refer(SalOp *h, const char *refer_to){
	osip_message_t *msg=NULL;
	int err=0;
	eXosip_lock();
	eXosip_call_build_refer(h->did,refer_to, &msg);
	if (msg) err=eXosip_call_send_request(h->did, msg);
	else err=-1;
	eXosip_unlock();
	return err;
}

int sal_call_send_dtmf(SalOp *h, char dtmf){
	osip_message_t *msg=NULL;
	char dtmf_body[128];
	char clen[10];

	eXosip_lock();
	eXosip_call_build_info(h->did,&msg);
	if (msg){
		snprintf(dtmf_body, sizeof(dtmf_body), "Signal=%c\r\nDuration=250\r\n", dtmf);
		osip_message_set_body(msg,dtmf_body,strlen(dtmf_body));
		osip_message_set_content_type(msg,"application/dtmf-relay");
		snprintf(clen,sizeof(clen),"%lu",(unsigned long)strlen(dtmf_body));
		osip_message_set_content_length(msg,clen);		
		eXosip_call_send_request(h->did,msg);
	}
	eXosip_unlock();
	return 0;
}

int sal_call_terminate(SalOp *h){
	eXosip_lock();
	eXosip_call_terminate(h->cid,h->did);
	eXosip_unlock();
	sal_remove_call(h->base.root,h);
	h->cid=-1;
	return 0;
}

void sal_op_authenticate(SalOp *h, const SalAuthInfo *info){
	if (h->pending_auth){
		const char *userid;
		if (info->userid==NULL || info->userid[0]=='\0') userid=info->username;
		else userid=info->userid;
		ms_message("Authentication info for %s %s added to eXosip", info->username,info->realm);
		eXosip_add_authentication_info (info->username,userid,
                                      info->password, NULL,info->realm);
		eXosip_lock();
		eXosip_default_action(h->pending_auth);
		eXosip_unlock();
		ms_message("eXosip_default_action() done");
		eXosip_clear_authentication_info();
		eXosip_event_free(h->pending_auth);
		sal_remove_pending_auth(sal_op_get_sal(h),h);
		h->pending_auth=NULL;
	}
}

static void set_network_origin(SalOp *op, osip_message_t *req){
	const char *received=NULL;
	int rport=5060;
	char origin[64];
	if (extract_received_rport(req,&received,&rport)!=0){
		osip_via_t *via=NULL;
		char *tmp;
		osip_message_get_via(req,0,&via);
		received=osip_via_get_host(via);
		tmp=osip_via_get_port(via);
		if (tmp) rport=atoi(tmp);
	}
	snprintf(origin,sizeof(origin)-1,"sip:%s:%i",received,rport);
	__sal_op_set_network_origin(op,origin);
}

static SalOp *find_op(Sal *sal, eXosip_event_t *ev){
	if (ev->cid>0){
		return sal_find_call(sal,ev->cid);
	}
	if (ev->rid>0){
		return sal_find_register(sal,ev->rid);
	}
	if (ev->response) return sal_find_other(sal,ev->response);
	return NULL;
}

static void inc_new_call(Sal *sal, eXosip_event_t *ev){
	SalOp *op=sal_op_new(sal);
	osip_from_t *from,*to;
	osip_call_info_t *call_info;
	char *tmp;
	sdp_message_t *sdp=eXosip_get_sdp_info(ev->request);

	set_network_origin(op,ev->request);
	
	if (sdp){
		op->sdp_offering=FALSE;
		op->base.remote_media=sal_media_description_new();
		sdp_to_media_description(sdp,op->base.remote_media);
		sdp_message_free(sdp);
	}else op->sdp_offering=TRUE;

	from=osip_message_get_from(ev->request);
	to=osip_message_get_to(ev->request);
	osip_from_to_str(from,&tmp);
	sal_op_set_from(op,tmp);
	osip_free(tmp);
	osip_from_to_str(to,&tmp);
	sal_op_set_to(op,tmp);
	osip_free(tmp);

	osip_message_get_call_info(ev->request,0,&call_info);
	if(call_info)
	{
		osip_call_info_to_str(call_info,&tmp);
		if( strstr(tmp,"answer-after=") != NULL)
		{
			op->auto_answer_asked=TRUE;
			ms_message("The caller asked to automatically answer the call(Emergency?)\n");
		}
		osip_free(tmp);
	}

	op->tid=ev->tid;
	op->cid=ev->cid;
	op->did=ev->did;
	
	sal_add_call(op->base.root,op);
	sal->callbacks.call_received(op);
}

static void handle_reinvite(Sal *sal,  eXosip_event_t *ev){
	SalOp *op=find_op(sal,ev);
	sdp_message_t *sdp;
	osip_message_t *msg=NULL;

	if (op==NULL) {
		ms_warning("Reinvite for non-existing operation !");
		return;
	}
	op->reinvite=TRUE;
	op->tid=ev->tid;
	sdp=eXosip_get_sdp_info(ev->request);
	if (op->base.remote_media){
		sal_media_description_unref(op->base.remote_media);
		op->base.remote_media=NULL;
	}
	eXosip_lock();
	eXosip_call_build_answer(ev->tid,200,&msg);
	eXosip_unlock();
	if (msg==NULL) return;
	if (op->base.root->session_expires!=0){
		if (op->supports_session_timers) osip_message_set_supported(msg, "timer");
	}
	if (op->base.contact){
		_osip_list_set_empty(&msg->contacts,(void (*)(void*))osip_contact_free);
		osip_message_set_contact(msg,op->base.contact);
	}
	if (sdp){
		op->sdp_offering=FALSE;
		op->base.remote_media=sal_media_description_new();
		sdp_to_media_description(sdp,op->base.remote_media);
		sdp_message_free(sdp);
		sdp_process(op);
		set_sdp(msg,op->sdp_answer);
	}else {
		op->sdp_offering=TRUE;
		set_sdp_from_desc(msg,op->base.local_media);
	}
	eXosip_lock();
	eXosip_call_send_answer(ev->tid,200,msg);
	eXosip_unlock();
}

static void handle_ack(Sal *sal,  eXosip_event_t *ev){
	SalOp *op=find_op(sal,ev);
	sdp_message_t *sdp;

	if (op==NULL) {
		ms_warning("ack for non-existing call !");
		return;
	}
	sdp=eXosip_get_sdp_info(ev->ack);
	if (sdp){
		op->base.remote_media=sal_media_description_new();
		sdp_to_media_description(sdp,op->base.remote_media);
		sdp_process(op);
		sdp_message_free(sdp);
	}
	if (op->reinvite){
		sal->callbacks.call_updated(op);
		op->reinvite=FALSE;
	}else{
		sal->callbacks.call_ack(op);
	}
}

static void update_contact_from_response(SalOp *op, osip_message_t *response){
	const char *received;
	int rport;
	if (extract_received_rport(response,&received,&rport)==0){
		const char *contact=sal_op_get_contact(op);
		if (!contact){
			/*no contact given yet, use from instead*/
			contact=sal_op_get_from(op);
		}
		if (contact){
			SalAddress *addr=sal_address_new(contact);
			char *tmp;
			sal_address_set_domain(addr,received);
			sal_address_set_port_int(addr,rport);
			tmp=sal_address_as_string(addr);
			ms_message("Contact address updated to %s for this dialog",tmp);
			sal_op_set_contact(op,tmp);
			ms_free(tmp);
		}
	}
}

static int call_proceeding(Sal *sal, eXosip_event_t *ev){
	SalOp *op=find_op(sal,ev);
	
	if (op==NULL) {
		ms_warning("This call has been canceled.");
		eXosip_lock();
		eXosip_call_terminate(ev->cid,ev->did);
		eXosip_unlock();
		return -1;
	}
	op->did=ev->did;
	op->tid=ev->tid;
	
	/* update contact if received and rport are set by the server
	 note: will only be used by remote for next INVITE, if any...*/
	update_contact_from_response(op,ev->response);
	return 0;
}

static void call_ringing(Sal *sal, eXosip_event_t *ev){
	sdp_message_t *sdp;
	SalOp *op=find_op(sal,ev);
	if (call_proceeding(sal, ev)==-1) return;
	
	sdp=eXosip_get_sdp_info(ev->response);
	if (sdp){
		op->base.remote_media=sal_media_description_new();
		sdp_to_media_description(sdp,op->base.remote_media);
		sdp_message_free(sdp);
		if (op->base.local_media) sdp_process(op);
	}
	sal->callbacks.call_ringing(op);
}

static void call_accepted(Sal *sal, eXosip_event_t *ev){
	sdp_message_t *sdp;
	osip_message_t *msg=NULL;
	SalOp *op=find_op(sal,ev);
	const char *contact;
	
	if (op==NULL){
		ms_error("A closed call is accepted ?");
		return;
	}
	sdp=eXosip_get_sdp_info(ev->response);
	if (sdp){
		op->base.remote_media=sal_media_description_new();
		sdp_to_media_description(sdp,op->base.remote_media);
		sdp_message_free(sdp);
		if (op->base.local_media) sdp_process(op);
	}
	eXosip_call_build_ack(ev->did,&msg);
	contact=sal_op_get_contact(op);
	if (contact) {
		_osip_list_set_empty(&msg->contacts,(void (*)(void*))osip_contact_free);
		osip_message_set_contact(msg,contact);
	}
	if (op->sdp_answer)
			set_sdp(msg,op->sdp_answer);
	eXosip_call_send_ack(ev->did,msg);
	sal->callbacks.call_accepted(op);
}

static void call_terminated(Sal *sal, eXosip_event_t *ev){
	char *from=NULL;
	SalOp *op=find_op(sal,ev);
	if (op==NULL){
		ms_warning("Call terminated for already closed call ?");
		return;
	}
	if (ev->request){
		osip_from_to_str(ev->request->from,&from);
	}
	sal_remove_call(sal,op);
	op->cid=-1;
	sal->callbacks.call_terminated(op,from!=NULL ? from : sal_op_get_from(op));
	if (from) osip_free(from);
}

static void call_released(Sal *sal, eXosip_event_t *ev){
	SalOp *op=find_op(sal,ev);
	if (op==NULL){
		ms_warning("No op associated to this call_released()");
		return;
	}
	op->cid=-1;
	if (op->did==-1) 
		sal->callbacks.call_failure(op,SalErrorNoResponse,SalReasonUnknown,NULL);
}

static int get_auth_data_from_response(osip_message_t *resp, const char **realm, const char **username){
	const char *prx_realm=NULL,*www_realm=NULL;
	osip_proxy_authenticate_t *prx_auth;
	osip_www_authenticate_t *www_auth;
	
	*username=osip_uri_get_username(resp->from->url);
	prx_auth=(osip_proxy_authenticate_t*)osip_list_get(&resp->proxy_authenticates,0);
	www_auth=(osip_proxy_authenticate_t*)osip_list_get(&resp->www_authenticates,0);
	if (prx_auth!=NULL)
		prx_realm=osip_proxy_authenticate_get_realm(prx_auth);
	if (www_auth!=NULL)
		www_realm=osip_www_authenticate_get_realm(www_auth);

	if (prx_realm){
		*realm=prx_realm;
	}else if (www_realm){
		*realm=www_realm;
	}else{
		return -1;
	}
	return 0;
}

static int get_auth_data_from_request(osip_message_t *msg, const char **realm, const char **username){
	osip_authorization_t *auth=NULL;
	osip_proxy_authorization_t *prx_auth=NULL;
	
	*username=osip_uri_get_username(msg->from->url);
	osip_message_get_authorization(msg, 0, &auth);
	if (auth){
		*realm=osip_authorization_get_realm(auth);
		return 0;
	}
	osip_message_get_proxy_authorization(msg,0,&prx_auth);
	if (prx_auth){
		*realm=osip_proxy_authorization_get_realm(prx_auth);
		return 0;
	}
	return -1;
}

static int get_auth_data(eXosip_event_t *ev, const char **realm, const char **username){
	if (ev->response && get_auth_data_from_response(ev->response,realm,username)==0) return 0;
	if (ev->request && get_auth_data_from_request(ev->request,realm,username)==0) return 0;
	return -1;
}

int sal_op_get_auth_requested(SalOp *op, const char **realm, const char **username){
	if (op->pending_auth){
		return get_auth_data(op->pending_auth,realm,username);
	}
	return -1;
}

static bool_t process_authentication(Sal *sal, eXosip_event_t *ev){
	SalOp *op;
	const char *username,*realm;
	op=find_op(sal,ev);
	if (op==NULL){
		ms_warning("No operation associated with this authentication !");
		return TRUE;
	}
	if (get_auth_data(ev,&realm,&username)==0){
		if (op->pending_auth!=NULL)
			eXosip_event_free(op->pending_auth);
		op->pending_auth=ev;
		sal_add_pending_auth (sal,op);
		sal->callbacks.auth_requested(op,realm,username);
		return FALSE;
	}
	return TRUE;
}

static void authentication_ok(Sal *sal, eXosip_event_t *ev){
	SalOp *op;
	const char *username,*realm;
	op=find_op(sal,ev);
	if (op==NULL){
		ms_warning("No operation associated with this authentication_ok!");
		return ;
	}
	if (get_auth_data(ev,&realm,&username)==0){
		sal->callbacks.auth_success(op,realm,username);
	}
}

static bool_t call_failure(Sal *sal, eXosip_event_t *ev){
	SalOp *op;
	int code=0;
	char* computedReason=NULL;
	const char *reason=NULL;
	SalError error=SalErrorUnknown;
	SalReason sr=SalReasonUnknown;
	

	op=(SalOp*)find_op(sal,ev);

	if (op==NULL) {
		ms_warning("Call failure reported for a closed call, ignored.");
		return TRUE;
	}

	if (ev->response){
		code=osip_message_get_status_code(ev->response);
		reason=osip_message_get_reason_phrase(ev->response);
		osip_header_t *h=NULL;
		if (!osip_message_header_get_byname(	ev->response
											,"Reason"
											,0
											,&h)) {
			computedReason = ms_strdup_printf("%s %s",reason,osip_header_get_value(h));
			reason = computedReason;

		}
	}
	switch(code)
	{
		case 401:
		case 407:
			return process_authentication(sal,ev);
			break;
		case 400:
			error=SalErrorUnknown;
		break;
		case 404:
			error=SalErrorFailure;
			sr=SalReasonNotFound;
		break;
		case 415:
			error=SalErrorFailure;
			sr=SalReasonMedia;
		break;
		case 422:
			eXosip_default_action(ev);
			return TRUE;
		break;
		case 480:
			error=SalErrorFailure;
			sr=SalReasonTemporarilyUnavailable;
		case 486:
			error=SalErrorFailure;
			sr=SalReasonBusy;
		break;
		case 487:
		break;
		case 600:
			error=SalErrorFailure;
			sr=SalReasonDoNotDisturb;
		break;
		case 603:
			error=SalErrorFailure;
			sr=SalReasonDeclined;
		break;
		default:
			if (code>0){
				error=SalErrorFailure;
				sr=SalReasonUnknown;
			}else error=SalErrorNoResponse;
	}
	sal->callbacks.call_failure(op,error,sr,reason);
	if (computedReason != NULL){
		ms_free(computedReason);
	}
	return TRUE;
}


static void process_media_control_xml(Sal *sal, eXosip_event_t *ev){
	SalOp *op=find_op(sal,ev);
	osip_body_t *body=NULL;

	if (op==NULL){
		ms_warning("media control xml received without operation context!");
		return ;
	}
	
	osip_message_get_body(ev->request,0,&body);
	if (body && body->body!=NULL &&
		strstr(body->body,"picture_fast_update")){
		osip_message_t *ans=NULL;
		ms_message("Receiving VFU request !");
		if (sal->callbacks.vfu_request){
			sal->callbacks.vfu_request(op);
			eXosip_call_build_answer(ev->tid,200,&ans);
			if (ans)
				eXosip_call_send_answer(ev->tid,200,ans);
		}
	}
}

static void process_dtmf_relay(Sal *sal, eXosip_event_t *ev){
	SalOp *op=find_op(sal,ev);
	osip_body_t *body=NULL;

	if (op==NULL){
		ms_warning("media dtmf relay received without operation context!");
		return ;
	}
	
	osip_message_get_body(ev->request,0,&body);
	if (body && body->body!=NULL){
		osip_message_t *ans=NULL;
		const char *name=strstr(body->body,"Signal");
		if (name==NULL) name=strstr(body->body,"signal");
		if (name==NULL) {
			ms_warning("Could not extract the dtmf name from the SIP INFO.");
		}else{
			char tmp[2];
			name+=strlen("signal");
			if (sscanf(name," = %1s",tmp)==1){
				ms_message("Receiving dtmf %s via SIP INFO.",tmp);
				if (sal->callbacks.dtmf_received != NULL)
					sal->callbacks.dtmf_received(op, tmp[0]);
			}
		}
		eXosip_call_build_answer(ev->tid,200,&ans);
		if (ans)
			eXosip_call_send_answer(ev->tid,200,ans);
	}
}

static void call_message_new(Sal *sal, eXosip_event_t *ev){
	osip_message_t *ans=NULL;
	if (ev->request){
		if (MSG_IS_INFO(ev->request)){
			osip_content_type_t *ct;
			ct=osip_message_get_content_type(ev->request);
			if (ct && ct->subtype){
				if (strcmp(ct->subtype,"media_control+xml")==0)
					process_media_control_xml(sal,ev);
				else if (strcmp(ct->subtype,"dtmf-relay")==0)
					process_dtmf_relay(sal,ev);
				else {
					ms_message("Unhandled SIP INFO.");
					/*send an "Not implemented" answer*/
					eXosip_lock();
					eXosip_call_build_answer(ev->tid,501,&ans);
					if (ans)
						eXosip_call_send_answer(ev->tid,501,ans);
					eXosip_unlock();
				}
			}else{
				/*empty SIP INFO, probably to test we are alive. Send an empty answer*/
				eXosip_lock();
				eXosip_call_build_answer(ev->tid,200,&ans);
				if (ans)
					eXosip_call_send_answer(ev->tid,200,ans);
				eXosip_unlock();
			}
		}
		if(MSG_IS_MESSAGE(ev->request)){
			/* SIP messages could be received into call */
			text_received(sal, ev);
			eXosip_lock();
			eXosip_call_build_answer(ev->tid,200,&ans);
			if (ans)
				eXosip_call_send_answer(ev->tid,200,ans);
			eXosip_unlock();
		}
		if(MSG_IS_REFER(ev->request)){
			osip_header_t *h=NULL;
			SalOp *op=find_op(sal,ev);
			
			ms_message("Receiving REFER request !");
			osip_message_header_get_byname(ev->request,"Refer-To",0,&h);
			eXosip_lock();
			eXosip_call_build_answer(ev->tid,202,&ans);
			if (ans)
				eXosip_call_send_answer(ev->tid,202,ans);
			eXosip_unlock();
			if (h){
				sal->callbacks.refer_received(sal,op,h->hvalue);
			}
			else
			{
				ms_warning("cannot do anything with the refer without destination\n");
			}
		}
		if(MSG_IS_NOTIFY(ev->request)){
			osip_header_t *h=NULL;
			char *from=NULL;
			SalOp *op=find_op(sal,ev);

			ms_message("Receiving NOTIFY request !");
			osip_from_to_str(ev->request->from,&from);
			osip_message_header_get_byname(ev->request,"Event",0,&h);
			if(h)
				sal->callbacks.notify(op,from,h->hvalue);
			/*answer that we received the notify*/
			eXosip_lock();
			eXosip_call_build_answer(ev->tid,200,&ans);
			if (ans)
				eXosip_call_send_answer(ev->tid,200,ans);
			eXosip_unlock();
			osip_free(from);
		}
	}else ms_warning("call_message_new: No request ?");
}

static void inc_update(Sal *sal, eXosip_event_t *ev){
	osip_message_t *msg=NULL;
	ms_message("Processing incoming UPDATE");
	eXosip_lock();
	eXosip_message_build_answer(ev->tid,200,&msg);
	if (msg!=NULL)
		eXosip_message_send_answer(ev->tid,200,msg);
	eXosip_unlock();
}

static bool_t comes_from_local_if(osip_message_t *msg){
	osip_via_t *via=NULL;
	osip_message_get_via(msg,0,&via);
	if (via){
		const char *host;
		host=osip_via_get_host(via);
		if (strcmp(host,"127.0.0.1")==0 || strcmp(host,"::1")==0){
			osip_generic_param_t *param=NULL;
			osip_via_param_get_byname(via,"received",&param);
			if (param==NULL) return TRUE;
			if (param->gvalue &&
				(strcmp(param->gvalue,"127.0.0.1")==0 || strcmp(param->gvalue,"::1")==0)){
				return TRUE;
			}
		}
	}
	return FALSE;
}

static void text_received(Sal *sal, eXosip_event_t *ev){
	osip_body_t *body=NULL;
	char *from=NULL,*msg;
	
	osip_message_get_body(ev->request,0,&body);
	if (body==NULL){
		ms_error("Could not get text message from SIP body");
		return;
	}
	msg=body->body;
	osip_from_to_str(ev->request->from,&from);
	sal->callbacks.text_received(sal,from,msg);
	osip_free(from);
}

static void other_request(Sal *sal, eXosip_event_t *ev){
	ms_message("in other_request");
	if (ev->request==NULL) return;
	if (strcmp(ev->request->sip_method,"MESSAGE")==0){
		text_received(sal,ev);
		eXosip_message_send_answer(ev->tid,200,NULL);
	}else if (strcmp(ev->request->sip_method,"OPTIONS")==0){
		osip_message_t *options=NULL;
		eXosip_options_build_answer(ev->tid,200,&options);
		osip_message_set_allow(options,"INVITE, ACK, BYE, CANCEL, OPTIONS, MESSAGE, SUBSCRIBE, NOTIFY, INFO");
		osip_message_set_accept(options,"application/sdp");
		eXosip_options_send_answer(ev->tid,200,options);
	}else if (strcmp(ev->request->sip_method,"WAKEUP")==0
		&& comes_from_local_if(ev->request)) {
		eXosip_message_send_answer(ev->tid,200,NULL);
		ms_message("Receiving WAKEUP request !");
		sal->callbacks.internal_message(sal,"WAKEUP");
	}else if (strncmp(ev->request->sip_method, "REFER", 5) == 0){
		ms_message("Receiving REFER request !");
		if (comes_from_local_if(ev->request)) {
			osip_header_t *h=NULL;
			osip_message_header_get_byname(ev->request,"Refer-To",0,&h);
			eXosip_message_send_answer(ev->tid,200,NULL);
			if (h){
				sal->callbacks.refer_received(sal,NULL,h->hvalue);
			}
		}else ms_warning("Ignored REFER not coming from this local loopback interface.");
	}else if (strncmp(ev->request->sip_method, "UPDATE", 6) == 0){
		inc_update(sal,ev);
    }else {
		char *tmp=NULL;
		size_t msglen=0;
		osip_message_to_str(ev->request,&tmp,&msglen);
		if (tmp){
			ms_message("Unsupported request received:\n%s",tmp);
			osip_free(tmp);
		}
		/*answer with a 501 Not implemented*/
		eXosip_message_send_answer(ev->tid,501,NULL);
	}
}

static void masquerade_via(osip_message_t *msg, const char *ip, const char *port){
	osip_via_t *via=NULL;
	osip_message_get_via(msg,0,&via);
	if (via){
		osip_free(via->port);
		via->port=osip_strdup(port);
		osip_free(via->host);
		via->host=osip_strdup(ip);
	}
}

static bool_t register_again_with_updated_contact(SalOp *op, osip_message_t *orig_request, osip_message_t *last_answer){
	osip_message_t *msg;
	const char *received;
	int rport;
	osip_contact_t *ctt=NULL;
	char *tmp;
	char port[20];
	SalAddress *addr;
	
	if (extract_received_rport(last_answer,&received,&rport)==-1) return FALSE;
	osip_message_get_contact(orig_request,0,&ctt);
	if (strcmp(ctt->url->host,received)==0){
		/*ip address matches, check ports*/
		const char *contact_port=ctt->url->port;
		if (contact_port==NULL || contact_port[0]=='\0')
			contact_port="5060";
		if (atoi(contact_port)==rport){
			ms_message("Register has up to date contact, doing nothing.");
			return FALSE;
		}else ms_message("ports do not match, need to update the register (%s <> %i)", contact_port,rport);
	}
	eXosip_lock();
	msg=NULL;
	eXosip_register_build_register(op->rid,op->expires,&msg);
	if (msg==NULL){
		eXosip_unlock();
		ms_warning("Fail to create a contact updated register.");
		return FALSE;
	}
	osip_message_get_contact(msg,0,&ctt);
	if (ctt->url->host!=NULL){
		osip_free(ctt->url->host);
	}
	ctt->url->host=osip_strdup(received);
	if (ctt->url->port!=NULL){
		osip_free(ctt->url->port);
	}
	snprintf(port,sizeof(port),"%i",rport);
	ctt->url->port=osip_strdup(port);
	if (op->masquerade_via) masquerade_via(msg,received,port);
	eXosip_register_send_register(op->rid,msg);
	eXosip_unlock();
	osip_contact_to_str(ctt,&tmp);
	addr=sal_address_new(tmp);
	osip_free(tmp);
	sal_address_clean(addr);
	tmp=sal_address_as_string(addr);
	sal_op_set_contact(op,tmp);
	sal_address_destroy(addr);
	ms_message("Resending new register with updated contact %s",tmp);
	ms_free(tmp);
	return TRUE;
}

static void registration_success(Sal *sal, eXosip_event_t *ev){
	SalOp *op=sal_find_register(sal,ev->rid);
	osip_header_t *h=NULL;
	bool_t registered;
	if (op==NULL){
		ms_error("Receiving register response for unknown operation");
		return;
	}
	osip_message_get_expires(ev->request,0,&h);
	if (h!=NULL && atoi(h->hvalue)!=0){
		registered=TRUE;
		if (!register_again_with_updated_contact(op,ev->request,ev->response)){
			sal->callbacks.register_success(op,registered);
		}
	}else registered=FALSE;
}

static bool_t registration_failure(Sal *sal, eXosip_event_t *ev){
	int status_code=0;
	const char *reason=NULL;
	SalOp *op=sal_find_register(sal,ev->rid);
	SalReason sr=SalReasonUnknown;
	SalError se=SalErrorUnknown;
	
	if (op==NULL){
		ms_error("Receiving register failure for unknown operation");
		return TRUE;
	}
	if (ev->response){
		status_code=osip_message_get_status_code(ev->response);
		reason=osip_message_get_reason_phrase(ev->response);
	}else return TRUE;
	switch(status_code){
		case 401:
		case 407:
			return process_authentication(sal,ev);
			break;
		case 606: /*Not acceptable, workaround for proxies that don't like private addresses
				 in vias, such as ekiga.net 
				 On the opposite, freephonie.net bugs when via are masqueraded.
				 */
			op->masquerade_via=TRUE;
		default:
			/* if contact is up to date, process the failure, otherwise resend a new register with
				updated contact first, just in case the faillure is due to incorrect contact */
			if (register_again_with_updated_contact(op,ev->request,ev->response))
				return TRUE; /*we are retrying with an updated contact*/
			if (status_code==403){
				se=SalErrorFailure;
				sr=SalReasonForbidden;
			}else if (status_code==0){
				se=SalErrorNoResponse;
			}
			sal->callbacks.register_failure(op,se,sr,reason);
	}
	return TRUE;
}

static void other_request_reply(Sal *sal,eXosip_event_t *ev){
	SalOp *op=find_op(sal,ev);

	if (op==NULL){
		ms_warning("other_request_reply(): Receiving response to unknown request.");
		return;
	}
	if (ev->response){
		update_contact_from_response(op,ev->response);
		if (ev->request && strcmp(osip_message_get_method(ev->request),"OPTIONS")==0)
			sal->callbacks.ping_reply(op);
	}
}

static bool_t process_event(Sal *sal, eXosip_event_t *ev){
	ms_message("linphone process event get a message %d\n",ev->type);
	switch(ev->type){
		case EXOSIP_CALL_ANSWERED:
			ms_message("CALL_ANSWERED\n");
			call_accepted(sal,ev);
			authentication_ok(sal,ev);
			break;
		case EXOSIP_CALL_CLOSED:
		case EXOSIP_CALL_CANCELLED:
			ms_message("CALL_CLOSED or CANCELLED\n");
			call_terminated(sal,ev);
			break;
		case EXOSIP_CALL_TIMEOUT:
		case EXOSIP_CALL_NOANSWER:
			ms_message("CALL_TIMEOUT or NOANSWER\n");
			return call_failure(sal,ev);
			break;
		case EXOSIP_CALL_REQUESTFAILURE:
		case EXOSIP_CALL_GLOBALFAILURE:
		case EXOSIP_CALL_SERVERFAILURE:
			ms_message("CALL_REQUESTFAILURE or GLOBALFAILURE or SERVERFAILURE\n");
			return call_failure(sal,ev);
			break;
		case EXOSIP_CALL_RELEASED:
			ms_message("CALL_RELEASED\n");
			call_released(sal, ev);
			break;
		case EXOSIP_CALL_INVITE:
			ms_message("CALL_NEW\n");
			inc_new_call(sal,ev);
			break;
		case EXOSIP_CALL_REINVITE:
			handle_reinvite(sal,ev);
			break;
		case EXOSIP_CALL_ACK:
			ms_message("CALL_ACK");
			handle_ack(sal,ev);
			break;
		case EXOSIP_CALL_REDIRECTED:
			ms_message("CALL_REDIRECTED");
			eXosip_default_action(ev);
			break;
		case EXOSIP_CALL_PROCEEDING:
			ms_message("CALL_PROCEEDING");
			call_proceeding(sal,ev);
			break;
		case EXOSIP_CALL_RINGING:
			ms_message("CALL_RINGING");
			call_ringing(sal,ev);
			break;
		case EXOSIP_CALL_MESSAGE_NEW:
			ms_message("EXOSIP_CALL_MESSAGE_NEW");
			call_message_new(sal,ev);
			break;
		case EXOSIP_CALL_MESSAGE_REQUESTFAILURE:
			if (ev->did<0 && ev->response &&
				(ev->response->status_code==407 || ev->response->status_code==401)){
				 return process_authentication(sal,ev);
			}
			break;
		case EXOSIP_IN_SUBSCRIPTION_NEW:
			ms_message("CALL_IN_SUBSCRIPTION_NEW ");
			sal_exosip_subscription_recv(sal,ev);
			break;
		case EXOSIP_IN_SUBSCRIPTION_RELEASED:
			ms_message("CALL_SUBSCRIPTION_NEW ");
			sal_exosip_in_subscription_closed(sal,ev);
			break;
		case EXOSIP_SUBSCRIPTION_UPDATE:
			ms_message("CALL_SUBSCRIPTION_UPDATE");
			break;
		case EXOSIP_SUBSCRIPTION_NOTIFY:
			ms_message("CALL_SUBSCRIPTION_NOTIFY");
			sal_exosip_notify_recv(sal,ev);
			break;
		case EXOSIP_SUBSCRIPTION_ANSWERED:
			ms_message("EXOSIP_SUBSCRIPTION_ANSWERED, ev->sid=%i, ev->did=%i\n",ev->sid,ev->did);
			sal_exosip_subscription_answered(sal,ev);
			break;
		case EXOSIP_SUBSCRIPTION_CLOSED:
			ms_message("EXOSIP_SUBSCRIPTION_CLOSED\n");
			sal_exosip_subscription_closed(sal,ev);
			break;
		case EXOSIP_SUBSCRIPTION_REQUESTFAILURE:   /**< announce a request failure      */
			if (ev->response && (ev->response->status_code == 407 || ev->response->status_code == 401)){
				return process_authentication(sal,ev);
			}
    	case EXOSIP_SUBSCRIPTION_SERVERFAILURE:
   		case EXOSIP_SUBSCRIPTION_GLOBALFAILURE:
			sal_exosip_subscription_closed(sal,ev);
			break;
		case EXOSIP_REGISTRATION_FAILURE:
			ms_message("REGISTRATION_FAILURE\n");
			return registration_failure(sal,ev);
			break;
		case EXOSIP_REGISTRATION_SUCCESS:
			authentication_ok(sal,ev);
			registration_success(sal,ev);
			break;
		case EXOSIP_MESSAGE_NEW:
			other_request(sal,ev);
			break;
		case EXOSIP_MESSAGE_PROCEEDING:
		case EXOSIP_MESSAGE_ANSWERED:
		case EXOSIP_MESSAGE_REDIRECTED:
		case EXOSIP_MESSAGE_SERVERFAILURE:
		case EXOSIP_MESSAGE_GLOBALFAILURE:
			other_request_reply(sal,ev);
			break;
		case EXOSIP_MESSAGE_REQUESTFAILURE:
			if (ev->response && (ev->response->status_code == 407 || ev->response->status_code == 401)){
				return process_authentication(sal,ev);
			}
			other_request_reply(sal,ev);
			break;
		default:
			ms_message("Unhandled exosip event ! %i",ev->type);
			break;
	}
	return TRUE;
}

int sal_iterate(Sal *sal){
	eXosip_event_t *ev;
	while((ev=eXosip_event_wait(0,0))!=NULL){
		if (process_event(sal,ev))
			eXosip_event_free(ev);
	}
	eXosip_lock();
	eXosip_automatic_refresh();
	eXosip_unlock();
	return 0;
}

int sal_register(SalOp *h, const char *proxy, const char *from, int expires){
	osip_message_t *msg;
	sal_op_set_route(h,proxy);
	if (h->rid==-1){
		eXosip_lock();
		h->rid=eXosip_register_build_initial_register(from,proxy,sal_op_get_contact(h),expires,&msg);
		sal_add_register(h->base.root,h);
	}else{
		eXosip_lock();
		eXosip_register_build_register(h->rid,expires,&msg);	
	}
	eXosip_register_send_register(h->rid,msg);
	eXosip_unlock();
	h->expires=expires;
	return 0;
}

int sal_unregister(SalOp *h){
	osip_message_t *msg=NULL;
	eXosip_lock();
	eXosip_register_build_register(h->rid,0,&msg);
	if (msg) eXosip_register_send_register(h->rid,msg);
	else ms_warning("Could not build unREGISTER !");
	eXosip_unlock();
	return 0;
}

SalAddress * sal_address_new(const char *uri){
	osip_from_t *from;
	osip_from_init(&from);
	if (osip_from_parse(from,uri)!=0){
		osip_from_free(from);
		return NULL;
	}
	if (from->displayname!=NULL && from->displayname[0]=='"'){
		char *unquoted=osip_strdup_without_quote(from->displayname);
		osip_free(from->displayname);
		from->displayname=unquoted;
	}
	return (SalAddress*)from;
}

SalAddress * sal_address_clone(const SalAddress *addr){
	osip_from_t *ret=NULL;
	osip_from_clone((osip_from_t*)addr,&ret);
	return (SalAddress*)ret;
}

#define null_if_empty(s) (((s)!=NULL && (s)[0]!='\0') ? (s) : NULL )

const char *sal_address_get_scheme(const SalAddress *addr){
	const osip_from_t *u=(const osip_from_t*)addr;
	return null_if_empty(u->url->scheme);
}

const char *sal_address_get_display_name(const SalAddress* addr){
	const osip_from_t *u=(const osip_from_t*)addr;
	return null_if_empty(u->displayname);
}

const char *sal_address_get_username(const SalAddress *addr){
	const osip_from_t *u=(const osip_from_t*)addr;
	return null_if_empty(u->url->username);
}

const char *sal_address_get_domain(const SalAddress *addr){
	const osip_from_t *u=(const osip_from_t*)addr;
	return null_if_empty(u->url->host);
}

void sal_address_set_display_name(SalAddress *addr, const char *display_name){
	osip_from_t *u=(osip_from_t*)addr;
	if (u->displayname!=NULL){
		osip_free(u->displayname);
		u->displayname=NULL;
	}
	if (display_name!=NULL && display_name[0]!='\0'){
		u->displayname=osip_strdup(display_name);
	}
}

void sal_address_set_username(SalAddress *addr, const char *username){
	osip_from_t *uri=(osip_from_t*)addr;
	if (uri->url->username!=NULL){
		osip_free(uri->url->username);
		uri->url->username=NULL;
	}
	if (username)
		uri->url->username=osip_strdup(username);
}

void sal_address_set_domain(SalAddress *addr, const char *host){
	osip_from_t *uri=(osip_from_t*)addr;
	if (uri->url->host!=NULL){
		osip_free(uri->url->host);
		uri->url->host=NULL;
	}
	if (host)
		uri->url->host=osip_strdup(host);
}

void sal_address_set_port(SalAddress *addr, const char *port){
	osip_from_t *uri=(osip_from_t*)addr;
	if (uri->url->port!=NULL){
		osip_free(uri->url->port);
		uri->url->port=NULL;
	}
	if (port)
		uri->url->port=osip_strdup(port);
}

void sal_address_set_port_int(SalAddress *uri, int port){
	char tmp[12];
	if (port==5060){
		/*this is the default, special case to leave the port field blank*/
		sal_address_set_port(uri,NULL);
		return;
	}
	snprintf(tmp,sizeof(tmp),"%i",port);
	sal_address_set_port(uri,tmp);
}

void sal_address_clean(SalAddress *addr){
	osip_generic_param_freelist(& ((osip_from_t*)addr)->gen_params);
	osip_uri_param_freelist(& ((osip_from_t*)addr)->url->url_params);
}

char *sal_address_as_string(const SalAddress *u){
	char *tmp,*ret;
	osip_from_t *from=(osip_from_t *)u;
	char *old_displayname=NULL;
	/* hack to force use of quotes around the displayname*/
	if (from->displayname!=NULL
	    && from->displayname[0]!='"'){
		old_displayname=from->displayname;
		from->displayname=osip_enquote(from->displayname);
	}
	osip_from_to_str(from,&tmp);
	if (old_displayname!=NULL){
		ms_free(from->displayname);
		from->displayname=old_displayname;
	}
	ret=ms_strdup(tmp);
	osip_free(tmp);
	return ret;
}

char *sal_address_as_string_uri_only(const SalAddress *u){
	char *tmp=NULL,*ret;
	osip_uri_to_str(((osip_from_t*)u)->url,&tmp);
	ret=ms_strdup(tmp);
	osip_free(tmp);
	return ret;
}

void sal_address_destroy(SalAddress *u){
	osip_from_free((osip_from_t*)u);
}

void sal_set_keepalive_period(Sal *ctx,unsigned int value) {
	eXosip_set_option (EXOSIP_OPT_UDP_KEEP_ALIVE, &value);
}

