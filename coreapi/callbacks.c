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
	call->state=LCStateAVRunning;
	if (lc->ringstream!=NULL){
		ring_stop(lc->ringstream);
		lc->ringstream=NULL;
	}
	linphone_core_start_media_streams(lc,call);
}

static void call_received(SalOp *h){
	LinphoneCore *lc=(LinphoneCore *)sal_get_user_pointer(sal_op_get_sal(h));
	char *barmesg;
	LinphoneCall *call;
	const char *from,*to;
	char *tmp;
	LinphoneAddress *from_parsed;

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
	if (lc->call!=NULL){/*busy*/
		sal_call_decline(h,SalReasonBusy,NULL);
		sal_op_release(h);
		return;
	}
	from=sal_op_get_from(h);
	to=sal_op_get_to(h);
	
	call=linphone_call_new_incoming(lc,linphone_address_new(from),linphone_address_new(to),h);
	lc->call=call;
	sal_call_set_local_media_description(h,call->localdesc);
	call->resultdesc=sal_call_get_final_media_description(h);
	if (call->resultdesc)
		sal_media_description_ref(call->resultdesc);
	if (call->resultdesc && sal_media_description_empty(call->resultdesc)){
		sal_call_decline(h,SalReasonMedia,NULL);
		linphone_call_destroy(call);
		lc->call=NULL;
		return;
	}
	
	from_parsed=linphone_address_new(sal_op_get_from(h));
	linphone_address_clean(from_parsed);
	tmp=linphone_address_as_string(from_parsed);
	linphone_address_destroy(from_parsed);
	gstate_new_state(lc, GSTATE_CALL_IN_INVITE, tmp);
	barmesg=ortp_strdup_printf(_("%s is contacting you"),tmp);
	if (lc->vtable.show) lc->vtable.show(lc);
	if (lc->vtable.display_status) 
	    lc->vtable.display_status(lc,barmesg);

	/* play the ring */
	if (lc->sound_conf.ring_sndcard!=NULL){
		ms_message("Starting local ring...");
		lc->ringstream=ring_start(lc->sound_conf.local_ring,2000,lc->sound_conf.ring_sndcard);
	}
	linphone_call_set_state(call,LCStateRinging);
	sal_call_notify_ringing(h);
	linphone_core_init_media_streams(lc,lc->call);
	if (lc->vtable.inv_recv) lc->vtable.inv_recv(lc,tmp);
	ms_free(barmesg);
	ms_free(tmp);
}

static void call_ringing(SalOp *h){
	LinphoneCore *lc=(LinphoneCore *)sal_get_user_pointer(sal_op_get_sal(h));
	LinphoneCall *call=lc->call;
	SalMediaDescription *md;
	if (call==NULL) return;
	if (lc->vtable.display_status)
		lc->vtable.display_status(lc,_("Remote ringing."));
	md=sal_call_get_final_media_description(h);
	if (md==NULL){
		if (lc->ringstream!=NULL) return;	/*already ringing !*/
		if (lc->sound_conf.play_sndcard!=NULL){
			ms_message("Remote ringing...");
			lc->ringstream=ring_start(lc->sound_conf.remote_ring,2000,lc->sound_conf.play_sndcard);
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
		gstate_new_state(lc, GSTATE_CALL_OUT_CONNECTED, NULL);
		if (lc->ringstream!=NULL){
			ring_stop(lc->ringstream);
			lc->ringstream=NULL;
		}
		ms_message("Doing early media...");
		linphone_core_start_media_streams(lc,call);
		call->media_pending=TRUE;
	}
	call->state=LCStateRinging;
}

static void call_accepted(SalOp *op){
	LinphoneCore *lc=(LinphoneCore *)sal_get_user_pointer(sal_op_get_sal(op));
	LinphoneCall *call=lc->call;
	if (call==NULL){
		ms_warning("No call to accept.");
		return ;
	}
	if (sal_op_get_user_pointer(op)!=lc->call){
		ms_warning("call_accepted: ignoring.");
		return;
	}
	if (call->state==LCStateAVRunning){
		return ; /*already accepted*/
	}
	if (lc->audiostream->ticker!=NULL){
		/*case where we accepted early media */
		linphone_core_stop_media_streams(lc,call);
		linphone_core_init_media_streams(lc,call);
	}
	if (call->resultdesc)
		sal_media_description_unref(call->resultdesc);
	call->resultdesc=sal_call_get_final_media_description(op);
	if (call->resultdesc){
		sal_media_description_ref(call->resultdesc);
		call->media_pending=FALSE;
	}
	if (call->resultdesc && !sal_media_description_empty(call->resultdesc)){
		gstate_new_state(lc, GSTATE_CALL_OUT_CONNECTED, NULL);
		linphone_connect_incoming(lc,call);
	}else{
		/*send a bye*/
		ms_error("Incompatible SDP offer received in 200Ok, need to abort the call");
		linphone_core_terminate_call(lc,NULL);
	}
}

static void call_ack(SalOp *op){
	LinphoneCore *lc=(LinphoneCore *)sal_get_user_pointer(sal_op_get_sal(op));
	LinphoneCall *call=lc->call;
	if (call==NULL){
		ms_warning("No call to be ACK'd");
		return ;
	}
	if (sal_op_get_user_pointer(op)!=lc->call){
		ms_warning("call_ack: ignoring.");
		return;
	}
	if (call->media_pending){
		if (lc->audiostream->ticker!=NULL){
			/*case where we accepted early media */
			linphone_core_stop_media_streams(lc,call);
			linphone_core_init_media_streams(lc,call);
		}
		if (call->resultdesc)
			sal_media_description_unref(call->resultdesc);
		call->resultdesc=sal_call_get_final_media_description(op);
		if (call->resultdesc)
			sal_media_description_ref(call->resultdesc);
		if (call->resultdesc && !sal_media_description_empty(call->resultdesc)){
			gstate_new_state(lc, GSTATE_CALL_IN_CONNECTED, NULL);
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
	linphone_core_stop_media_streams(lc,call);
	linphone_core_init_media_streams(lc,call);
	if (call->resultdesc)
		sal_media_description_unref(call->resultdesc);
	call->resultdesc=sal_call_get_final_media_description(op);
	if (call->resultdesc && !sal_media_description_empty(call->resultdesc)){
		linphone_connect_incoming(lc,call);
	}
}

static void call_terminated(SalOp *op, const char *from){
	LinphoneCore *lc=(LinphoneCore *)sal_get_user_pointer(sal_op_get_sal(op));
	if (sal_op_get_user_pointer(op)!=lc->call){
		ms_warning("call_terminated: ignoring.");
		return;
	}
	ms_message("Current call terminated...");
	if (lc->ringstream!=NULL) {
		ring_stop(lc->ringstream);
		lc->ringstream=NULL;
	}
	linphone_core_stop_media_streams(lc,lc->call);
	lc->vtable.show(lc);
	lc->vtable.display_status(lc,_("Call terminated."));
	gstate_new_state(lc, GSTATE_CALL_END, NULL);
	if (lc->vtable.bye_recv!=NULL){
		LinphoneAddress *addr=linphone_address_new(from);
		char *tmp;
		linphone_address_clean(addr);
		tmp=linphone_address_as_string(addr);
		lc->vtable.bye_recv(lc,tmp);
		ms_free(tmp);
		linphone_address_destroy(addr);
	}
	linphone_call_destroy(lc->call);
	lc->call=NULL;
}

static void call_failure(SalOp *op, SalError error, SalReason sr, const char *details){
	LinphoneCore *lc=(LinphoneCore *)sal_get_user_pointer(sal_op_get_sal(op));
	char *msg486=_("User is busy.");
	char *msg480=_("User is temporarily unavailable.");
	/*char *retrymsg=_("%s. Retry after %i minute(s).");*/
	char *msg600=_("User does not want to be disturbed.");
	char *msg603=_("Call declined.");
	LinphoneCall *call=lc->call;

	if (sal_op_get_user_pointer(op)!=lc->call){
		ms_warning("call_failure: ignoring.");
		return;
	}
	if (lc->vtable.show) lc->vtable.show(lc);

	if (error==SalErrorNoResponse){
		if (lc->vtable.display_status)
			lc->vtable.display_status(lc,_("No response."));
	}else if (error==SalErrorProtocol){
		if (lc->vtable.display_status)
			lc->vtable.display_status(lc, details ? details : _("Error."));
	}else if (error==SalErrorFailure){
		switch(sr){
			case SalReasonDeclined:
				if (lc->vtable.display_status)
					lc->vtable.display_status(lc,msg603);
			break;
			case SalReasonBusy:
				if (lc->vtable.display_status)
					lc->vtable.display_status(lc,msg486);
			break;
			case SalReasonRedirect:
				if (lc->vtable.display_status)
					lc->vtable.display_status(lc,_("Redirected"));
			break;
			case SalReasonTemporarilyUnavailable:
				if (lc->vtable.display_status)
					lc->vtable.display_status(lc,msg480);
			break;
			case SalReasonNotFound:
				if (lc->vtable.display_status)
					lc->vtable.display_status(lc,_("Not found"));
			break;
			case SalReasonDoNotDisturb:
				if (lc->vtable.display_status)
					lc->vtable.display_status(lc,msg600);
			break;
			case SalReasonMedia:
				if (lc->vtable.display_status)
					lc->vtable.display_status(lc,_("No common codecs"));
			break;
			default:
				if (lc->vtable.display_status)
					lc->vtable.display_status(lc,_("Call failed."));
		}
	}
	if (lc->ringstream!=NULL) {
		ring_stop(lc->ringstream);
		lc->ringstream=NULL;
	}
	linphone_core_stop_media_streams(lc,call);
	if (call!=NULL) {
		linphone_call_destroy(call);
		gstate_new_state(lc, GSTATE_CALL_ERROR, NULL);
		lc->call=NULL;
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
	gstate_new_state(lc, GSTATE_REG_OK, NULL);
	cfg->registered=registered;
	if (cfg->registered) msg=ms_strdup_printf(_("Registration on %s successful."),sal_op_get_proxy(op));
	else msg=ms_strdup_printf(_("Unregistration on %s done."),sal_op_get_proxy(op));
	if (lc->vtable.display_status) 
		lc->vtable.display_status(lc,msg);
	ms_free(msg);
}

static void register_failure(SalOp *op, SalError error, SalReason reason, const char *details){
	LinphoneCore *lc=(LinphoneCore *)sal_get_user_pointer(sal_op_get_sal(op));
	char *msg=ortp_strdup_printf(_("Registration on %s failed: %s"),sal_op_get_proxy(op),(details!=NULL) ? details : _("no response timeout"));
	if (lc->vtable.display_status) lc->vtable.display_status(lc,msg);
	gstate_new_state(lc, GSTATE_REG_FAILED, msg);
	ms_free(msg);
}

static void vfu_request(SalOp *op){
	LinphoneCore *lc=(LinphoneCore *)sal_get_user_pointer(sal_op_get_sal(op));
#ifdef VIDEO_ENABLED
	if (lc->videostream)
		video_stream_send_vfu(lc->videostream);
#endif
}

static void dtmf_received(SalOp *op, char dtmf){
	LinphoneCore *lc=(LinphoneCore *)sal_get_user_pointer(sal_op_get_sal(op));
	if (lc->vtable.dtmf_received != NULL)
		lc->vtable.dtmf_received(lc, dtmf);
}

static void refer_received(Sal *sal, SalOp *op, const char *referto){
	LinphoneCore *lc=(LinphoneCore *)sal_get_user_pointer(sal);
	if (lc->vtable.refer_received)
		lc->vtable.refer_received(lc,referto);
}

static void text_received(Sal *sal, const char *from, const char *msg){
	LinphoneCore *lc=(LinphoneCore *)sal_get_user_pointer(sal);
	linphone_core_text_received(lc,from,msg);
}

static void notify(SalOp *op, SalSubscribeState ss, SalPresenceStatus status, const char *msg){
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
		if (call->state==LCStatePreEstablishing){
			linphone_core_start_invite(call->core,call,NULL);
		}
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
	subscribe_received,
	subscribe_closed,
	internal_message,
	ping_reply
};


