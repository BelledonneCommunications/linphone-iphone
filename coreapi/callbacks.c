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
#include "lpconfig.h"

static void register_failure(SalOp *op, SalError error, SalReason reason, const char *details);

static bool_t media_parameters_changed(LinphoneCall *call, SalMediaDescription *oldmd, SalMediaDescription *newmd){
	return !sal_media_description_equals(oldmd,newmd)  || call->up_bw!=linphone_core_get_upload_bandwidth(call->core);
}

void linphone_core_update_streams(LinphoneCore *lc, LinphoneCall *call, SalMediaDescription *new_md){
	SalMediaDescription *oldmd=call->resultdesc;
	
	if (lc->ringstream!=NULL){
		ring_stop(lc->ringstream);
		lc->ringstream=NULL;
	}
	if (new_md!=NULL){
		sal_media_description_ref(new_md);
		call->media_pending=FALSE;
	}else{
		call->media_pending=TRUE;
	}
	call->resultdesc=new_md;
	if (call->audiostream && call->audiostream->ticker){
		/* we already started media: check if we really need to restart it*/
		if (oldmd){
			if (!media_parameters_changed(call,oldmd,new_md) && !call->playing_ringbacktone){
				sal_media_description_unref(oldmd);
				if (call->all_muted){
					ms_message("Early media finished, unmuting inputs...");
					/*we were in early media, now we want to enable real media */
					linphone_call_enable_camera (call,linphone_call_camera_enabled (call));
					if (call->audiostream)
						linphone_core_mute_mic (lc, linphone_core_is_mic_muted(lc));
#ifdef VIDEO_ENABLED
					if (call->videostream && call->camera_active)
						video_stream_change_camera(call->videostream,lc->video_conf.device );
#endif
				}
				ms_message("No need to restart streams, SDP is unchanged.");
				return;
			}else{
				ms_message("Media descriptions are different, need to restart the streams.");
			}
		}
		linphone_call_stop_media_streams (call);
		linphone_call_init_media_streams (call);
	}
	if (oldmd) 
		sal_media_description_unref(oldmd);
	
	if (new_md) {
		bool_t all_muted=FALSE;
		bool_t send_ringbacktone=FALSE;
		
		if (call->audiostream==NULL){
			/*this happens after pausing the call locally. The streams is destroyed and then we wait the 200Ok to recreate it*/
			linphone_call_init_media_streams (call);
		}
		if (call->state==LinphoneCallIncomingEarlyMedia && linphone_core_get_remote_ringback_tone (lc)!=NULL){
			send_ringbacktone=TRUE;
		}
		if (call->state==LinphoneCallIncomingEarlyMedia ||
		    (call->state==LinphoneCallOutgoingEarlyMedia && !call->params.real_early_media)){
			all_muted=TRUE;
		}
		linphone_call_start_media_streams(call,all_muted,send_ringbacktone);
	}
}

static bool_t is_duplicate_call(LinphoneCore *lc, const LinphoneAddress *from, const LinphoneAddress *to){
	MSList *elem;
	for(elem=lc->calls;elem!=NULL;elem=elem->next){
		LinphoneCall *call=(LinphoneCall*)elem->data;
		if (linphone_address_weak_equal(call->log->from,from) &&
		    linphone_address_weak_equal(call->log->to, to)){
			return TRUE;
		}
	}
	return FALSE;
}

static void call_received(SalOp *h){
	LinphoneCore *lc=(LinphoneCore *)sal_get_user_pointer(sal_op_get_sal(h));
	char *barmesg;
	LinphoneCall *call;
	const char *from,*to;
	char *tmp;
	LinphoneAddress *from_parsed;
	LinphoneAddress *from_addr, *to_addr;
	SalMediaDescription *md;
	bool_t propose_early_media=lp_config_get_int(lc->config,"sip","incoming_calls_early_media",FALSE);
	const char *ringback_tone=linphone_core_get_remote_ringback_tone (lc);
	
	/* first check if we can answer successfully to this invite */
	if (lc->presence_mode==LinphoneStatusBusy ||
	    lc->presence_mode==LinphoneStatusOffline ||
	    lc->presence_mode==LinphoneStatusDoNotDisturb ||
	    lc->presence_mode==LinphoneStatusMoved){
		if (lc->presence_mode==LinphoneStatusBusy )
			sal_call_decline(h,SalReasonBusy,NULL);
		else if (lc->presence_mode==LinphoneStatusOffline)
			sal_call_decline(h,SalReasonTemporarilyUnavailable,NULL);
		else if (lc->presence_mode==LinphoneStatusDoNotDisturb)
			sal_call_decline(h,SalReasonTemporarilyUnavailable,NULL);
		else if (lc->alt_contact!=NULL && lc->presence_mode==LinphoneStatusMoved)
			sal_call_decline(h,SalReasonRedirect,lc->alt_contact);
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
	from_addr=linphone_address_new(from);
	to_addr=linphone_address_new(to);

	if (is_duplicate_call(lc,from_addr,to_addr)){
		ms_warning("Receiving duplicated call, refusing this one.");
		sal_call_decline(h,SalReasonBusy,NULL);
		linphone_address_destroy(from_addr);
		linphone_address_destroy(to_addr);
		return;
	}
	
	call=linphone_call_new_incoming(lc,from_addr,to_addr,h);
	sal_call_set_local_media_description(h,call->localdesc);
	md=sal_call_get_final_media_description(h);

	if (md && sal_media_description_empty(md)){
		sal_call_decline(h,SalReasonMedia,NULL);
		linphone_call_unref(call);
		return;
	}
	
	/* the call is acceptable so we can now add it to our list */
	linphone_core_add_call(lc,call);
	
	from_parsed=linphone_address_new(sal_op_get_from(h));
	linphone_address_clean(from_parsed);
	tmp=linphone_address_as_string(from_parsed);
	linphone_address_destroy(from_parsed);
	barmesg=ortp_strdup_printf("%s %s%s",tmp,_("is contacting you"),
	    (sal_call_autoanswer_asked(h)) ?_(" and asked autoanswer."):_("."));
	if (lc->vtable.show) lc->vtable.show(lc);
	if (lc->vtable.display_status) 
	    lc->vtable.display_status(lc,barmesg);

	/* play the ring if this is the only call*/
	if (lc->sound_conf.ring_sndcard!=NULL && ms_list_size(lc->calls)==1){
		lc->current_call=call;
		if (lc->ringstream && lc->dmfs_playing_start_time!=0){
			ring_stop(lc->ringstream);
			lc->ringstream=NULL;
			lc->dmfs_playing_start_time=0;
		}
		if(lc->ringstream==NULL && lc->sound_conf.local_ring){
			MSSndCard *ringcard=lc->sound_conf.lsd_card ?lc->sound_conf.lsd_card : lc->sound_conf.ring_sndcard;
			ms_message("Starting local ring...");
			lc->ringstream=ring_start(lc->sound_conf.local_ring,2000,ringcard);
		}
		else
		{
			ms_message("the local ring is already started");
		}
	}else{
		/*TODO : play a tone within the context of the current call */
	}

	
	linphone_call_ref(call); /*prevent the call from being destroyed while we are notifying, if the user declines within the state callback */
	linphone_call_set_state(call,LinphoneCallIncomingReceived,"Incoming call");
	
	if (call->state==LinphoneCallIncomingReceived){
		sal_call_notify_ringing(h,propose_early_media || ringback_tone!=NULL);

		if (propose_early_media || ringback_tone!=NULL){
			linphone_call_set_state(call,LinphoneCallIncomingEarlyMedia,"Incoming call early media");
			linphone_core_update_streams(lc,call,md);
		}
		if (sal_call_get_replaces(call->op)!=NULL && lp_config_get_int(lc->config,"sip","auto_answer_replacing_calls",1)){
			linphone_core_accept_call(lc,call);
		}
	}
	linphone_call_unref(call);

	ms_free(barmesg);
	ms_free(tmp);
}

static void call_ringing(SalOp *h){
	LinphoneCore *lc=(LinphoneCore *)sal_get_user_pointer(sal_op_get_sal(h));
	LinphoneCall *call=(LinphoneCall*)sal_op_get_user_pointer(h);
	SalMediaDescription *md;
	
	if (call==NULL) return;
	
	if (lc->vtable.display_status)
		lc->vtable.display_status(lc,_("Remote ringing."));
	
	md=sal_call_get_final_media_description(h);
	if (md==NULL){
		if (lc->ringstream && lc->dmfs_playing_start_time!=0){
			ring_stop(lc->ringstream);
			lc->ringstream=NULL;
			lc->dmfs_playing_start_time=0;
		}
		if (lc->ringstream!=NULL) return;	/*already ringing !*/
		if (lc->sound_conf.play_sndcard!=NULL){
			MSSndCard *ringcard=lc->sound_conf.lsd_card ? lc->sound_conf.lsd_card : lc->sound_conf.play_sndcard;
			lc->ringstream=ring_start(lc->sound_conf.remote_ring,2000,ringcard);
		}
		ms_message("Remote ringing...");
		if (lc->vtable.display_status) 
			lc->vtable.display_status(lc,_("Remote ringing..."));
		linphone_call_set_state(call,LinphoneCallOutgoingRinging,"Remote ringing");
	}else{
		/*accept early media */
		if (call->audiostream && call->audiostream->ticker!=NULL){
			/*streams already started */
			ms_message("Early media already started.");
			return;
		}
		if (lc->vtable.show) lc->vtable.show(lc);
		if (lc->vtable.display_status) 
			lc->vtable.display_status(lc,_("Early media."));
		linphone_call_set_state(call,LinphoneCallOutgoingEarlyMedia,"Early media");
		if (lc->ringstream!=NULL){
			ring_stop(lc->ringstream);
			lc->ringstream=NULL;
		}
		ms_message("Doing early media...");
		linphone_core_update_streams (lc,call,md);
	}
}

/*
 * could be reach :
 *  - when the call is accepted
 *  - when a request is accepted (pause, resume)
 */
static void call_accepted(SalOp *op){
	LinphoneCore *lc=(LinphoneCore *)sal_get_user_pointer(sal_op_get_sal(op));
	LinphoneCall *call=(LinphoneCall*)sal_op_get_user_pointer(op);
	SalMediaDescription *md;
	
	if (call==NULL){
		ms_warning("No call to accept.");
		return ;
	}
	
	md=sal_call_get_final_media_description(op);
	
	if (call->state==LinphoneCallOutgoingProgress ||
	    call->state==LinphoneCallOutgoingRinging ||
	    call->state==LinphoneCallOutgoingEarlyMedia){
		linphone_call_set_state(call,LinphoneCallConnected,"Connected");
	}
	if (md && !sal_media_description_empty(md)){
		if (sal_media_description_has_dir(md,SalStreamSendOnly) ||
		    sal_media_description_has_dir(md,SalStreamInactive)){
			if (lc->vtable.display_status){
				char *tmp=linphone_call_get_remote_address_as_string (call);
				char *msg=ms_strdup_printf(_("Call with %s is paused."),tmp);
				lc->vtable.display_status(lc,msg);
				ms_free(tmp);
				ms_free(msg);
			}
			linphone_core_update_streams (lc,call,md);
			linphone_call_set_state(call,LinphoneCallPaused,"Call paused");
		}else if (sal_media_description_has_dir(md,SalStreamRecvOnly)){
			/*we are put on hold when the call is initially accepted */
			if (lc->vtable.display_status){
				char *tmp=linphone_call_get_remote_address_as_string (call);
				char *msg=ms_strdup_printf(_("Call answered by %s - on hold."),tmp);
				lc->vtable.display_status(lc,msg);
				ms_free(tmp);
				ms_free(msg);
			}
			linphone_core_update_streams (lc,call,md);
			linphone_call_set_state(call,LinphoneCallPausedByRemote,"Call paused by remote");
		}else{
			if (call->state==LinphoneCallStreamsRunning){
				/*media was running before, the remote as acceted a call modification (that is
					a reinvite made by us. We must notify the application this reinvite was accepted*/
				linphone_call_set_state(call, LinphoneCallUpdated, "Call updated");
			}else{
				if (call->state==LinphoneCallResuming){
					if (lc->vtable.display_status){
						lc->vtable.display_status(lc,_("Call resumed."));
					}
				}else{
					if (lc->vtable.display_status){
						char *tmp=linphone_call_get_remote_address_as_string (call);
						char *msg=ms_strdup_printf(_("Call answered by %s."),tmp);
						lc->vtable.display_status(lc,msg);
						ms_free(tmp);
						ms_free(msg);
					}
				}
			}
			linphone_core_update_streams (lc,call,md);
			linphone_call_set_state(call, LinphoneCallStreamsRunning, "Streams running");
			lc->current_call=call;
		}
	}else{
		/*send a bye*/
		ms_error("Incompatible SDP offer received in 200Ok, need to abort the call");
		linphone_core_abort_call(lc,call,"No codec intersection");
	}
}

static void call_ack(SalOp *op){
	LinphoneCore *lc=(LinphoneCore *)sal_get_user_pointer(sal_op_get_sal(op));
	LinphoneCall *call=(LinphoneCall*)sal_op_get_user_pointer(op);
	if (call==NULL){
		ms_warning("No call to be ACK'd");
		return ;
	}
	if (call->media_pending){
		SalMediaDescription *md=sal_call_get_final_media_description(op);
		if (md && !sal_media_description_empty(md)){
			if (call->state==LinphoneCallStreamsRunning){
				/*media was running before, the remote as acceted a call modification (that is
					a reinvite made by us. We must notify the application this reinvite was accepted*/
				linphone_call_set_state(call, LinphoneCallUpdated, "Call updated");
			}
			linphone_core_update_streams (lc,call,md);
			linphone_call_set_state (call,LinphoneCallStreamsRunning,"Connected (streams running)");
		}else{
			/*send a bye*/
			ms_error("Incompatible SDP response received in ACK, need to abort the call");
			linphone_core_abort_call(lc,call,"No codec intersection");
			return;
		}
	}
}


/* this callback is called when an incoming re-INVITE modifies the session*/
static void call_updating(SalOp *op){
	LinphoneCore *lc=(LinphoneCore *)sal_get_user_pointer(sal_op_get_sal(op));
	LinphoneCall *call=(LinphoneCall*)sal_op_get_user_pointer(op);
	LinphoneCallState prevstate=LinphoneCallIdle;
	SalMediaDescription *md;
	
	md=sal_call_get_final_media_description(op);
	
	if (md && !sal_media_description_empty(md))
	{
		if (sal_media_description_has_dir(call->localdesc,SalStreamSendRecv)){
			ms_message("Our local status is SalStreamSendRecv");
			if (sal_media_description_has_dir (md,SalStreamRecvOnly) || sal_media_description_has_dir(md,SalStreamInactive)){
				/* we are being paused */
				if(lc->vtable.display_status)
					lc->vtable.display_status(lc,_("We are being paused..."));
				linphone_call_set_state (call,LinphoneCallPausedByRemote,"Call paused by remote");
			}else if (!sal_media_description_has_dir(call->resultdesc,SalStreamSendRecv) && sal_media_description_has_dir(md,SalStreamSendRecv)){
				if(lc->vtable.display_status)
					lc->vtable.display_status(lc,_("We have been resumed..."));
				linphone_call_set_state (call,LinphoneCallStreamsRunning,"Connected (streams running)");
				lc->current_call=call;
			}else{
				prevstate=call->state;
				linphone_call_set_state(call, LinphoneCallUpdatedByRemote,"Call updated by remote");
			}
		}
		/*accept the modification (sends a 200Ok)*/
		sal_call_accept(op);
		linphone_core_update_streams (lc,call,md);
		if (prevstate!=LinphoneCallIdle){
			linphone_call_set_state (call,prevstate,"Connected (streams running)");
		}
	}
}

static void call_terminated(SalOp *op, const char *from){
	LinphoneCore *lc=(LinphoneCore *)sal_get_user_pointer(sal_op_get_sal(op));
	LinphoneCall *call=(LinphoneCall*)sal_op_get_user_pointer(op);

	if (call==NULL) return;
	
	if (linphone_call_get_state(call)==LinphoneCallEnd || linphone_call_get_state(call)==LinphoneCallError){
		ms_warning("call_terminated: ignoring.");
		return;
	}
	ms_message("Current call terminated...");
	//we stop the call only if we have this current call or if we are in call
	if (lc->ringstream!=NULL && ( (ms_list_size(lc->calls)  == 1) || linphone_core_in_call(lc) )) {
		ring_stop(lc->ringstream);
		lc->ringstream=NULL;
	}
	linphone_call_stop_media_streams(call);
	if (lc->vtable.show!=NULL)
		lc->vtable.show(lc);
	if (lc->vtable.display_status!=NULL)
		lc->vtable.display_status(lc,_("Call terminated."));

	linphone_call_set_state(call, LinphoneCallEnd,"Call ended");
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

	if (call==NULL){
		ms_warning("Call faillure reported on already cleaned call ?");
		return ;
	}
	
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

	if (lc->ringstream!=NULL) {
		ring_stop(lc->ringstream);
		lc->ringstream=NULL;
	}
	linphone_call_stop_media_streams (call);
	if (sr!=SalReasonDeclined) linphone_call_set_state(call,LinphoneCallError,msg);
	else{
		call->reason=LinphoneReasonDeclined;
		linphone_call_set_state(call,LinphoneCallEnd,"Call declined.");
	}
}

static void call_released(SalOp *op){
	LinphoneCall *call=(LinphoneCall*)sal_op_get_user_pointer(op);
	if (call!=NULL){
		linphone_call_set_state(call,LinphoneCallReleased,"Call released");
	}else ms_error("call_released() for already destroyed call ?");
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
		if (ai && ai->works==FALSE) {
			sal_op_cancel_authentication(h);
		} 
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
	
	cfg->registered=registered;
	linphone_proxy_config_set_error(cfg,LinphoneReasonNone);
	linphone_proxy_config_set_state(cfg, registered ? LinphoneRegistrationOk : LinphoneRegistrationCleared ,
	                                registered ? "Registration sucessful" : "Unregistration done");
	if (lc->vtable.display_status){
		if (cfg->registered) msg=ms_strdup_printf(_("Registration on %s successful."),sal_op_get_proxy(op));
		else msg=ms_strdup_printf(_("Unregistration on %s done."),sal_op_get_proxy(op));
		lc->vtable.display_status(lc,msg);
		ms_free(msg);
	}
	
}

static void register_failure(SalOp *op, SalError error, SalReason reason, const char *details){
	LinphoneCore *lc=(LinphoneCore *)sal_get_user_pointer(sal_op_get_sal(op));
	LinphoneProxyConfig *cfg=(LinphoneProxyConfig*)sal_op_get_user_pointer(op);

	if (cfg==NULL){
		ms_warning("Registration failed for unknown proxy config.");
		return ;
	}
	if (details==NULL)
		details=_("no response timeout");
	
	if (lc->vtable.display_status) {
		char *msg=ortp_strdup_printf(_("Registration on %s failed: %s"),sal_op_get_proxy(op),details  );
		lc->vtable.display_status(lc,msg);
		ms_free(msg);
	}
	if (error== SalErrorFailure && reason == SalReasonForbidden) {
		linphone_proxy_config_set_error(cfg, LinphoneReasonBadCredentials);
	} else if (error == SalErrorNoResponse) {
		linphone_proxy_config_set_error(cfg, LinphoneReasonNoResponse);
	}
	linphone_proxy_config_set_state(cfg,LinphoneRegistrationFailed,details);
}

static void vfu_request(SalOp *op){
#ifdef VIDEO_ENABLED
	LinphoneCall *call=(LinphoneCall*)sal_op_get_user_pointer (op);
	if (call==NULL){
		ms_warning("VFU request but no call !");
		return ;
	}
	if (call->videostream)
		video_stream_send_vfu(call->videostream);
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
	if (call){
		if (call->refer_to!=NULL){
			ms_free(call->refer_to);
		}
		call->refer_to=ms_strdup(referto);
		call->refer_pending=TRUE;
		linphone_call_set_state(call,LinphoneCallRefered,"Refered");
		if (lc->vtable.display_status){
			char *msg=ms_strdup_printf(_("We are transferred to %s"),referto);
			lc->vtable.display_status(lc,msg);
			ms_free(msg);
		}
		if (call->state!=LinphoneCallPaused){
			ms_message("Automatically pausing current call to accept transfer.");
			linphone_core_pause_call(lc,call);
		}
		linphone_core_start_refered_call(lc,call);
		sal_call_accept_refer(op);
	}else if (lc->vtable.refer_received){
		lc->vtable.refer_received(lc,referto);
		sal_call_accept_refer(op);
	}
}

static void text_received(Sal *sal, const char *from, const char *msg){
	LinphoneCore *lc=(LinphoneCore *)sal_get_user_pointer(sal);
	linphone_core_text_received(lc,from,msg);
}

static void notify(SalOp *op, const char *from, const char *msg){
	LinphoneCore *lc=(LinphoneCore *)sal_get_user_pointer(sal_op_get_sal(op));
	LinphoneCall *call=(LinphoneCall*)sal_op_get_user_pointer (op);
	ms_message("get a %s notify from %s",msg,from);
	if(lc->vtable.notify_recv)
		lc->vtable.notify_recv(lc,call,from,msg);
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
		if (call->state==LinphoneCallOutgoingInit){
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
	call_updating,
	call_terminated,
	call_failure,
	call_released,
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


