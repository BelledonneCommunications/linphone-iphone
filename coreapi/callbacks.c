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


#include "sal.h"

#include "linphonecore.h"
#include "private.h"
#include "mediastreamer2/mediastream.h"


static void linphone_connect_incoming(LinphoneCore *lc, LinphoneCall *call){
	if (lc->vtable.show)
		lc->vtable.show(lc);
	if (lc->vtable.display_status)
		lc->vtable.display_status(lc,_("Connected."));
	if (lc->vtable.connected_recv)
		lc->vtable.connected_recv(lc,call);
	call->state=LinphoneCallAVRunning;
	if (lc->ringstream!=NULL){
		ring_stop(lc->ringstream);
		lc->ringstream=NULL;
	}
	if(!linphone_core_in_call(lc))
	{
		linphone_core_set_as_current_call(lc,call);
	}
	if(call == linphone_core_get_current_call(lc))
		linphone_core_start_media_streams(lc,call);
}

static void call_received(SalOp *h){
	LinphoneCore *lc=(LinphoneCore *)sal_get_user_pointer(sal_op_get_sal(h));
	char *barmesg;
	LinphoneCall *call;
	const char *from,*to;
	char *tmp;
	LinphoneAddress *from_parsed;
	LinphoneGeneralStateContext gctx;

	/* first check if we can answer successfully to this invite */
	if (lc->presence_mode!=LINPHONE_STATUS_ONLINE){
		ms_message("Not present !! presence mode : %d\n",lc->presence_mode);
		if (lc->presence_mode==LINPHONE_STATUS_BUSY)
			sal_call_decline(h,SalReasonBusy,NULL);
		else if (lc->presence_mode==LINPHONE_STATUS_AWAY
			 ||lc->presence_mode==LINPHONE_STATUS_BERIGHTBACK
			 ||lc->presence_mode==LINPHONE_STATUS_ONTHEPHONE
			 ||lc->presence_mode==LINPHONE_STATUS_OUTTOLUNCH
			 ||lc->presence_mode==LINPHONE_STATUS_OFFLINE)
			sal_call_decline(h,SalReasonTemporarilyUnavailable,NULL);
		else if (lc->presence_mode==LINPHONE_STATUS_NOT_DISTURB)
			sal_call_decline(h,SalReasonTemporarilyUnavailable,NULL);
		else if (lc->alt_contact!=NULL && lc->presence_mode==LINPHONE_STATUS_MOVED)
			sal_call_decline(h,SalReasonRedirect,lc->alt_contact);
		else
			sal_call_decline(h,SalReasonBusy,NULL);
		sal_op_release(h);
		return;
	}
	if (!linphone_core_can_we_add_call(lc)){/*busy*/
		sal_call_decline(h,SalReasonBusy,NULL);
		sal_op_release(h);
		return;
	}
	from=sal_op_get_from(h);
	to=sal_op_get_to(h);
	
	call=linphone_call_new_incoming(lc,linphone_address_new(from),linphone_address_new(to),h);
	
	if(linphone_core_get_current_call(lc)!=NULL) //we are already in call just inform that an incoming call is going on
	{
		char temp[256];
		snprintf(temp,sizeof(temp)-1,"A new incoming call from %s during call",from);
		lc->vtable.display_status(lc,temp);
	}
	sal_call_set_local_media_description(h,call->localdesc);
	call->resultdesc=sal_call_get_final_media_description(h);
	if (call->resultdesc)
		sal_media_description_ref(call->resultdesc);
	if (call->resultdesc && sal_media_description_empty(call->resultdesc)){
		sal_call_decline(h,SalReasonMedia,NULL);
		linphone_call_unref(call);
		return;
	}
	/* the call is acceptable so we can now add it to our list */
	if(linphone_core_add_call(lc,call)!= 0)
	{
		ms_warning("we cannot handle anymore call\n");
		sal_call_decline(h,SalReasonMedia,NULL);
		linphone_call_unref(call);
		return;
	}
	from_parsed=linphone_address_new(sal_op_get_from(h));
	linphone_address_clean(from_parsed);
	tmp=linphone_address_as_string(from_parsed);
	linphone_address_destroy(from_parsed);
	gctx.call=call;
	gstate_new_state(lc, GSTATE_CALL_IN_INVITE, gctx, tmp);
	barmesg=ortp_strdup_printf("%s %s%s",tmp,_("is contacting you"),
	    (sal_call_autoanswer_asked(h)) ?_(" and asked autoanswer."):_("."));
	if (lc->vtable.show) lc->vtable.show(lc);
	if (lc->vtable.display_status) 
	    lc->vtable.display_status(lc,barmesg);

	/* play the ring */
	if (lc->sound_conf.ring_sndcard!=NULL && !linphone_core_in_call(lc)){
		if(lc->ringstream==NULL){
			MSSndCard *ringcard=lc->sound_conf.lsd_card ?lc->sound_conf.lsd_card : lc->sound_conf.ring_sndcard;
			ms_message("Starting local ring...");
			lc->ringstream=ring_start(lc->sound_conf.local_ring,2000,ringcard);
		}
		else
		{
			ms_message("the local ring is already started");
		}
	}
	call->state=LinphoneCallRinging;
	sal_call_notify_ringing(h);
#if !(__IPHONE_OS_VERSION_MIN_REQUIRED >= 40000)
	linphone_core_init_media_streams(lc,call);
#endif
	if (lc->vtable.inv_recv) lc->vtable.inv_recv(lc,call);
	ms_free(barmesg);
	ms_free(tmp);
}

static void call_ringing(SalOp *h){
	LinphoneCore *lc=(LinphoneCore *)sal_get_user_pointer(sal_op_get_sal(h));
	LinphoneCall *call=(LinphoneCall*)sal_op_get_user_pointer(h);
	SalMediaDescription *md;
	LinphoneGeneralStateContext gctx;
	
	if (call==NULL) return;
	gctx.call=call;
	
	if (lc->vtable.display_status)
		lc->vtable.display_status(lc,_("Remote ringing."));
	if (lc->vtable.ringing_recv)
		lc->vtable.ringing_recv(lc,call);
	md=sal_call_get_final_media_description(h);
	if (md==NULL){
		if (lc->ringstream!=NULL) return;	/*already ringing !*/
		if (lc->sound_conf.play_sndcard!=NULL){
			MSSndCard *ringcard=lc->sound_conf.lsd_card ? lc->sound_conf.lsd_card : lc->sound_conf.play_sndcard;
			ms_message("Remote ringing...");
			lc->ringstream=ring_start(lc->sound_conf.remote_ring,2000,ringcard);
			
			gstate_new_state(lc, GSTATE_CALL_OUT_RINGING, gctx, NULL);
		}
	}else{
		/*accept early media */
		if (lc->audiostream && lc->audiostream->ticker!=NULL){
			/*streams already started */
			ms_message("Early media already started.");
			return;
		}
		sal_media_description_ref(md);
		call->resultdesc=md;
		if (lc->vtable.show) lc->vtable.show(lc);
		if (lc->vtable.display_status) 
			lc->vtable.display_status(lc,_("Early media."));
		gstate_new_state(lc, GSTATE_CALL_OUT_RINGING, gctx, NULL);
		if (lc->ringstream!=NULL){
			ring_stop(lc->ringstream);
			lc->ringstream=NULL;
		}
		ms_message("Doing early media...");
		if(call == linphone_core_get_current_call(lc))
			linphone_core_start_media_streams(lc,call);
		call->media_pending=TRUE;
	}
	call->state=LinphoneCallRinging;
}

/*
 * could be reach :
 *  - when the call is accepted
 *  - when a request is accepted (pause, resume)
 */
static void call_accepted(SalOp *op){
	LinphoneCore *lc=(LinphoneCore *)sal_get_user_pointer(sal_op_get_sal(op));
	LinphoneCall *call=(LinphoneCall*)sal_op_get_user_pointer(op);
	LinphoneGeneralStateContext gctx;
	if (call==NULL){
		ms_warning("No call to accept.");
		return ;
	}
	gctx.call=call;
	if (call->state==LinphoneCallAVRunning){
		ms_message("GET 200Ok of resume\n");
		if(lc->vtable.ack_resumed_recv)
			lc->vtable.ack_resumed_recv(lc,call);
		return ; //already accepted
	}
	if ((lc->audiostream!=NULL) && (lc->audiostream->ticker!=NULL)){
		/*case where we accepted early media */
		if(call == linphone_core_get_current_call(lc))
		{
			linphone_core_stop_media_streams(lc,call);
			linphone_core_init_media_streams(lc,call);
		}
	}
	if (call->resultdesc)
		sal_media_description_unref(call->resultdesc);
	call->resultdesc=sal_call_get_final_media_description(op);
	if (call->resultdesc){
		sal_media_description_ref(call->resultdesc);
		call->media_pending=FALSE;
	}
	if (call->resultdesc && !sal_media_description_empty(call->resultdesc)){
		//if we initiated a pause
		if(call->state == LinphoneCallPaused)
		{
			ms_message("GET 200Ok of pause\n");
			if(lc->vtable.ack_paused_recv)
				lc->vtable.ack_paused_recv(lc,call);
		}//if there is an accepted incoming call
		else
		{
			linphone_core_set_as_current_call (lc,call);
			gstate_new_state(lc, GSTATE_CALL_OUT_CONNECTED, gctx, NULL);
			linphone_connect_incoming(lc,call);
		}		
	}else{
		/*send a bye*/
		ms_error("Incompatible SDP offer received in 200Ok, need to abort the call");
		linphone_core_terminate_call(lc,NULL);
	}
}

static void call_ack(SalOp *op){
	LinphoneCore *lc=(LinphoneCore *)sal_get_user_pointer(sal_op_get_sal(op));
	LinphoneCall *call=(LinphoneCall*)sal_op_get_user_pointer(op);
	LinphoneGeneralStateContext gctx;
	if (call==NULL){
		ms_warning("No call to be ACK'd");
		return ;
	}
	gctx.call=call;
	if (call->media_pending){
		if (lc->audiostream->ticker!=NULL){
			/*case where we accepted early media */
			if(call == linphone_core_get_current_call(lc))
			{
				linphone_core_stop_media_streams(lc,call);
				linphone_core_init_media_streams(lc,call);
			}
		}
		if (call->resultdesc)
			sal_media_description_unref(call->resultdesc);
		call->resultdesc=sal_call_get_final_media_description(op);
		if (call->resultdesc)
			sal_media_description_ref(call->resultdesc);
		if (call->resultdesc && !sal_media_description_empty(call->resultdesc)){
			gstate_new_state(lc, GSTATE_CALL_IN_CONNECTED, gctx, NULL);
			linphone_connect_incoming(lc,call);
		}else{
			/*send a bye*/
			ms_error("Incompatible SDP response received in ACK, need to abort the call");
			linphone_core_terminate_call(lc,NULL);
		}
		call->media_pending=FALSE;
	}
}

static void call_updated(SalOp *op){
	LinphoneCore *lc=(LinphoneCore *)sal_get_user_pointer(sal_op_get_sal(op));
	LinphoneCall *call=(LinphoneCall*)sal_op_get_user_pointer(op);
	if (call->resultdesc)
		sal_media_description_unref(call->resultdesc);
	call->resultdesc=sal_call_get_final_media_description(op);
	if (call->resultdesc)
		sal_media_description_ref(call->resultdesc);

	if (call->resultdesc && !sal_media_description_empty(call->resultdesc))
	{
		if( (call->state == LinphoneCallPaused) && strcmp(call->resultdesc->addr,"0.0.0.0"))
		{
			if(lc->vtable.display_status)
				lc->vtable.display_status(lc,"we have been resumed...");
			call->state = LinphoneCallAVRunning;
			lc->vtable.resumed_recv(lc,call);
			//we have to keep sending when holded
			//linphone_core_start_media_streams(lc,call);
		}
		else if( (call->state != LinphoneCallPaused) && !strcmp(call->resultdesc->addr,"0.0.0.0"))
		{
			if(lc->vtable.display_status)
				lc->vtable.display_status(lc,"we have been paused...");
			call->state = LinphoneCallPaused;
			lc->vtable.paused_recv(lc,call);
			//we have to keep sending when holded
			/*
			if(call == linphone_core_get_current_call(lc))
			{
				linphone_core_stop_media_streams(lc,call);
				linphone_core_init_media_streams(lc,call);
			}
			*/
		}
		else
		{
			if(call == linphone_core_get_current_call(lc))
			{
				linphone_core_stop_media_streams(lc,call);
				linphone_core_init_media_streams(lc,call);
			}
			linphone_connect_incoming(lc,call);
		}
	}
}

static void call_terminated(SalOp *op, const char *from){
	LinphoneCore *lc=(LinphoneCore *)sal_get_user_pointer(sal_op_get_sal(op));
	LinphoneCall *call=(LinphoneCall*)sal_op_get_user_pointer(op);
	LinphoneGeneralStateContext gctx;
	gctx.call=call;
	if (linphone_call_get_state(call)==LinphoneCallTerminated){
		ms_warning("call_terminated: ignoring.");
		return;
	}
	ms_message("Current call terminated...");
	//we stop the call only if we have this current call or if we are in call
	if (lc->ringstream!=NULL && ( (ms_list_size(lc->calls)  == 1) || linphone_core_in_call(lc) )) {
		ring_stop(lc->ringstream);
		lc->ringstream=NULL;
	}
	if(call == linphone_core_get_current_call(lc))
		linphone_core_stop_media_streams(lc,call);
	if (lc->vtable.show!=NULL)
		lc->vtable.show(lc);
	if (lc->vtable.display_status!=NULL)
		lc->vtable.display_status(lc,_("Call terminated."));
	call->state=LinphoneCallTerminated;
	gstate_new_state(lc, GSTATE_CALL_END, gctx, NULL);
	if (lc->vtable.bye_recv!=NULL){
		LinphoneAddress *addr=linphone_address_new(from);
		char *tmp;
		linphone_address_clean(addr);
		tmp=linphone_address_as_string(addr);
		if (lc->vtable.bye_recv!=NULL)
			lc->vtable.bye_recv(lc,call);
		ms_free(tmp);
		linphone_address_destroy(addr);
	}
	linphone_call_set_terminated(call);
}

static void call_failure(SalOp *op, SalError error, SalReason sr, const char *details, int code){
	LinphoneCore *lc=(LinphoneCore *)sal_get_user_pointer(sal_op_get_sal(op));
	char *msg486=_("User is busy.");
	char *msg480=_("User is temporarily unavailable.");
	/*char *retrymsg=_("%s. Retry after %i minute(s).");*/
	char *msg600=_("User does not want to be disturbed.");
	char *msg603=_("Call declined.");
	const char *msg=details;
	LinphoneCall *call=(LinphoneCall*)sal_op_get_user_pointer(op);
	LinphoneGeneralStateContext gctx;

	gctx.call=call;
	if (lc->vtable.show) lc->vtable.show(lc);

	if (error==SalErrorNoResponse){
		msg=_("No response.");
		if (lc->vtable.display_status)
			lc->vtable.display_status(lc,msg);
	}else if (error==SalErrorProtocol){
		msg=details ? details : _("Protocol error.");
		if (lc->vtable.display_status)
			lc->vtable.display_status(lc, msg);
	}else if (error==SalErrorFailure){
		switch(sr){
			case SalReasonDeclined:
				msg=msg603;
				if (lc->vtable.display_status)
					lc->vtable.display_status(lc,msg603);
			break;
			case SalReasonBusy:
				msg=msg486;
				if (lc->vtable.display_status)
					lc->vtable.display_status(lc,msg486);
			break;
			case SalReasonRedirect:
				msg=_("Redirected");
				if (lc->vtable.display_status)
					lc->vtable.display_status(lc,msg);
			break;
			case SalReasonTemporarilyUnavailable:
				msg=msg480;
				if (lc->vtable.display_status)
					lc->vtable.display_status(lc,msg480);
			break;
			case SalReasonNotFound:
				msg=_("Not found");
				if (lc->vtable.display_status)
					lc->vtable.display_status(lc,msg);
			break;
			case SalReasonDoNotDisturb:
				msg=msg600;
				if (lc->vtable.display_status)
					lc->vtable.display_status(lc,msg600);
			break;
			case SalReasonMedia:
				msg=_("No common codecs");
				if (lc->vtable.display_status)
					lc->vtable.display_status(lc,msg);
			break;
			default:
				if (lc->vtable.display_status)
					lc->vtable.display_status(lc,_("Call failed."));
		}
	}
	if (lc->vtable.failure_recv)
		lc->vtable.failure_recv(lc,call,code);
	if (lc->ringstream!=NULL) {
		ring_stop(lc->ringstream);
		lc->ringstream=NULL;
	}
	if(call == linphone_core_get_current_call(lc))
		linphone_core_stop_media_streams(lc,call);
	if (call!=NULL) {
		if (sr!=SalReasonDeclined) gstate_new_state(lc, GSTATE_CALL_ERROR, gctx, msg);
		else gstate_new_state(lc, GSTATE_CALL_END, gctx, NULL);
		linphone_call_set_terminated(call);
	}
}

static void auth_requested(SalOp *h, const char *realm, const char *username){
	LinphoneCore *lc=(LinphoneCore *)sal_get_user_pointer(sal_op_get_sal(h));
	LinphoneAuthInfo *ai=(LinphoneAuthInfo*)linphone_core_find_auth_info(lc,realm,username);
	ms_message("auth_requested() for realm=%s, username=%s",realm,username);
	if (ai && (ai->works || ai->usecount<3)){
		SalAuthInfo sai;
		sai.username=ai->username;
		sai.userid=ai->userid;
		sai.realm=ai->realm;
		sai.password=ai->passwd;
		ms_message("auth_requested(): authenticating realm=%s, username=%s",realm,username);
		sal_op_authenticate(h,&sai);
		ai->usecount++;
	}else{
		if (lc->vtable.auth_info_requested)
			lc->vtable.auth_info_requested(lc,realm,username);
	}
}

static void auth_success(SalOp *h, const char *realm, const char *username){
	LinphoneCore *lc=(LinphoneCore *)sal_get_user_pointer(sal_op_get_sal(h));
	LinphoneAuthInfo *ai=(LinphoneAuthInfo*)linphone_core_find_auth_info(lc,realm,username);
	if (ai){
		ms_message("%s/%s authentication works.",realm,username);
		ai->works=TRUE;
	}
}

static void register_success(SalOp *op, bool_t registered){
	LinphoneCore *lc=(LinphoneCore *)sal_get_user_pointer(sal_op_get_sal(op));
	LinphoneProxyConfig *cfg=(LinphoneProxyConfig*)sal_op_get_user_pointer(op);
	char *msg;
	LinphoneGeneralStateContext gctx;
	gctx.proxy=cfg;
	cfg->registered=registered;
	gstate_new_state(lc, GSTATE_REG_OK,gctx, NULL);
	if (cfg->registered) msg=ms_strdup_printf(_("Registration on %s successful."),sal_op_get_proxy(op));
	else msg=ms_strdup_printf(_("Unregistration on %s done."),sal_op_get_proxy(op));
	if (lc->vtable.display_status) 
		lc->vtable.display_status(lc,msg);
	ms_free(msg);
}

static void register_failure(SalOp *op, SalError error, SalReason reason, const char *details){
	LinphoneCore *lc=(LinphoneCore *)sal_get_user_pointer(sal_op_get_sal(op));
	LinphoneGeneralStateContext gctx;
	char *msg=ortp_strdup_printf(_("Registration on %s failed: %s"),sal_op_get_proxy(op),(details!=NULL) ? details : _("no response timeout"));
	if (lc->vtable.display_status) lc->vtable.display_status(lc,msg);
	gctx.proxy=(LinphoneProxyConfig*)sal_op_get_user_pointer (op);
	gstate_new_state(lc, GSTATE_REG_FAILED, gctx, msg);
	ms_free(msg);
}

static void vfu_request(SalOp *op){
#ifdef VIDEO_ENABLED
	LinphoneCore *lc=(LinphoneCore *)sal_get_user_pointer(sal_op_get_sal(op));
	if (lc->videostream)
		video_stream_send_vfu(lc->videostream);
#endif
}

static void dtmf_received(SalOp *op, char dtmf){
	LinphoneCore *lc=(LinphoneCore *)sal_get_user_pointer(sal_op_get_sal(op));
	LinphoneCall *call=(LinphoneCall*)sal_op_get_user_pointer(op);
	if (lc->vtable.dtmf_received != NULL)
		lc->vtable.dtmf_received(lc, call, dtmf);
}

static void refer_received(Sal *sal, SalOp *op, const char *referto){
	LinphoneCore *lc=(LinphoneCore *)sal_get_user_pointer(sal);
	LinphoneCall *call=(LinphoneCall*)sal_op_get_user_pointer(op);
	if (lc->vtable.refer_received){
		lc->vtable.refer_received(lc,call,referto);
		if (op) sal_refer_accept(op);
	}
}

static void text_received(Sal *sal, const char *from, const char *msg){
	LinphoneCore *lc=(LinphoneCore *)sal_get_user_pointer(sal);
	linphone_core_text_received(lc,from,msg);
}

static void notify(SalOp *op, const char *from, const char *msg){
	LinphoneCore *lc=(LinphoneCore *)sal_get_user_pointer(sal_op_get_sal(op));

	ms_message("get a %s notify from %s",msg,from);
	if(lc->vtable.notify_recv)
		lc->vtable.notify_recv(lc,from,msg);
}

static void notify_presence(SalOp *op, SalSubscribeState ss, SalPresenceStatus status, const char *msg){
	LinphoneCore *lc=(LinphoneCore *)sal_get_user_pointer(sal_op_get_sal(op));
	linphone_notify_recv(lc,op,ss,status);
}

static void subscribe_received(SalOp *op, const char *from){
	LinphoneCore *lc=(LinphoneCore *)sal_get_user_pointer(sal_op_get_sal(op));
	linphone_subscription_new(lc,op,from);
}

static void subscribe_closed(SalOp *op, const char *from){
	LinphoneCore *lc=(LinphoneCore *)sal_get_user_pointer(sal_op_get_sal(op));
	linphone_subscription_closed(lc,op);
}

static void internal_message(Sal *sal, const char *msg){
	LinphoneCore *lc=(LinphoneCore *)sal_get_user_pointer(sal);
	if (lc->vtable.show)
		lc->vtable.show(lc);
}

static void ping_reply(SalOp *op){
	LinphoneCall *call=(LinphoneCall*) sal_op_get_user_pointer(op);
	ms_message("ping reply !");
	if (call){
		if (call->state==LinphoneCallPreEstablishing){
			linphone_core_start_invite(call->core,call,NULL);
		}
	}
	else
	{
		ms_warning("ping reply without call attached...");
	}
}

SalCallbacks linphone_sal_callbacks={
	call_received,
	call_ringing,
	call_accepted,
	call_ack,
	call_updated,
	call_terminated,
	call_failure,
	auth_requested,
	auth_success,
	register_success,
	register_failure,
	vfu_request,
	dtmf_received,
	refer_received,
	text_received,
	notify,
	notify_presence,
	subscribe_received,
	subscribe_closed,
	internal_message,
	ping_reply
};


