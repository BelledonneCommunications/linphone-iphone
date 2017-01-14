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
Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
*/


#include "sal/sal.h"

#include "linphone/core.h"
#include "private.h"
#include "mediastreamer2/mediastream.h"
#include "linphone/lpconfig.h"

// stat
#ifndef _WIN32
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#endif

static void register_failure(SalOp *op);

static int media_parameters_changed(LinphoneCall *call, SalMediaDescription *oldmd, SalMediaDescription *newmd) {
	int result=0;
	int otherdesc_changed;
	char *tmp1=NULL;
	char *tmp2=NULL;
	if (call->params->in_conference != call->current_params->in_conference) return SAL_MEDIA_DESCRIPTION_FORCE_STREAM_RECONSTRUCTION;
	if (call->up_bw != linphone_core_get_upload_bandwidth(call->core)) return SAL_MEDIA_DESCRIPTION_FORCE_STREAM_RECONSTRUCTION;
	if (call->localdesc_changed) ms_message("Local description has changed: %s", tmp1 = sal_media_description_print_differences(call->localdesc_changed));
	otherdesc_changed = sal_media_description_equals(oldmd, newmd);
	if (otherdesc_changed) ms_message("Other description has changed: %s", tmp2 = sal_media_description_print_differences(otherdesc_changed));
	result = call->localdesc_changed | otherdesc_changed;
	if (tmp1) ms_free(tmp1);
	if (tmp2) ms_free(tmp2);
	return result;
}

void linphone_core_update_streams_destinations(LinphoneCore *lc, LinphoneCall *call, SalMediaDescription *old_md, SalMediaDescription *new_md) {
	SalStreamDescription *new_audiodesc = NULL;
	SalStreamDescription *new_videodesc = NULL;
	char *rtp_addr, *rtcp_addr;
	int i;

	for (i = 0; i < SAL_MEDIA_DESCRIPTION_MAX_STREAMS; i++) {
		if (!sal_stream_description_active(&new_md->streams[i])) continue;
		if (new_md->streams[i].type == SalAudio) {
			new_audiodesc = &new_md->streams[i];
		} else if (new_md->streams[i].type == SalVideo) {
			new_videodesc = &new_md->streams[i];
		}
	}
	if (call->audiostream && new_audiodesc) {
		rtp_addr = (new_audiodesc->rtp_addr[0] != '\0') ? new_audiodesc->rtp_addr : new_md->addr;
		rtcp_addr = (new_audiodesc->rtcp_addr[0] != '\0') ? new_audiodesc->rtcp_addr : new_md->addr;
		ms_message("Change audio stream destination: RTP=%s:%d RTCP=%s:%d", rtp_addr, new_audiodesc->rtp_port, rtcp_addr, new_audiodesc->rtcp_port);
		rtp_session_set_remote_addr_full(call->audiostream->ms.sessions.rtp_session, rtp_addr, new_audiodesc->rtp_port, rtcp_addr, new_audiodesc->rtcp_port);
	}
#ifdef VIDEO_ENABLED
	if (call->videostream && new_videodesc) {
		rtp_addr = (new_videodesc->rtp_addr[0] != '\0') ? new_videodesc->rtp_addr : new_md->addr;
		rtcp_addr = (new_videodesc->rtcp_addr[0] != '\0') ? new_videodesc->rtcp_addr : new_md->addr;
		ms_message("Change video stream destination: RTP=%s:%d RTCP=%s:%d", rtp_addr, new_videodesc->rtp_port, rtcp_addr, new_videodesc->rtcp_port);
		rtp_session_set_remote_addr_full(call->videostream->ms.sessions.rtp_session, rtp_addr, new_videodesc->rtp_port, rtcp_addr, new_videodesc->rtcp_port);
	}
#else
	(void)new_videodesc;
#endif
}

static void _clear_early_media_destinations(LinphoneCall *call, MediaStream *ms){
	RtpSession *session=ms->sessions.rtp_session;
	rtp_session_clear_aux_remote_addr(session);
	if (!call->ice_session) rtp_session_set_symmetric_rtp(session,linphone_core_symmetric_rtp_enabled(call->core));/*restore symmetric rtp if ICE is not used*/
}

static void clear_early_media_destinations(LinphoneCall *call){
	if (call->audiostream){
		_clear_early_media_destinations(call,(MediaStream*)call->audiostream);
	}
	if (call->videostream){
		_clear_early_media_destinations(call,(MediaStream*)call->videostream);
	}
}

static void prepare_early_media_forking(LinphoneCall *call){
	/*we need to disable symmetric rtp otherwise our outgoing streams will be switching permanently between the multiple destinations*/
	if (call->audiostream){
		rtp_session_set_symmetric_rtp(call->audiostream->ms.sessions.rtp_session,FALSE);
	}
	if (call->videostream){
		rtp_session_set_symmetric_rtp(call->videostream->ms.sessions.rtp_session,FALSE);
	}
}

void linphone_call_update_frozen_payloads(LinphoneCall *call, SalMediaDescription *result_desc){
	SalMediaDescription *local=call->localdesc;
	int i;
	for(i=0;i<result_desc->nb_streams;++i){
		bctbx_list_t *elem;
		for (elem=result_desc->streams[i].payloads;elem!=NULL;elem=elem->next){
			PayloadType *pt=(PayloadType*)elem->data;
			if (is_payload_type_number_available(local->streams[i].already_assigned_payloads, payload_type_get_number(pt), NULL)){
				/*new codec, needs to be added to the list*/
				local->streams[i].already_assigned_payloads=bctbx_list_append(local->streams[i].already_assigned_payloads, payload_type_clone(pt));
				ms_message("LinphoneCall[%p] : payload type %i %s/%i fmtp=%s added to frozen list.",
					   call, payload_type_get_number(pt), pt->mime_type, pt->clock_rate, pt->recv_fmtp ? pt->recv_fmtp : "");
			}
		}
	}
}

void linphone_core_update_streams(LinphoneCore *lc, LinphoneCall *call, SalMediaDescription *new_md, LinphoneCallState target_state){
	SalMediaDescription *oldmd=call->resultdesc;
	int md_changed=0;


	if (!((call->state == LinphoneCallIncomingEarlyMedia) && (linphone_core_get_ring_during_incoming_early_media(lc)))) {
		linphone_core_stop_ringing(lc);
	}
	if (!new_md) {
		ms_error("linphone_core_update_streams() called with null media description");
		return;
	}
	linphone_call_update_biggest_desc(call, call->localdesc);
	sal_media_description_ref(new_md);
	call->resultdesc=new_md;
	if ((call->audiostream && call->audiostream->ms.state==MSStreamStarted) || (call->videostream && call->videostream->ms.state==MSStreamStarted)){
		clear_early_media_destinations(call);

		/* we already started media: check if we really need to restart it*/
		if (oldmd){
			md_changed = media_parameters_changed(call, oldmd, new_md);
			if ((md_changed & (	SAL_MEDIA_DESCRIPTION_CODEC_CHANGED
								|SAL_MEDIA_DESCRIPTION_STREAMS_CHANGED
								|SAL_MEDIA_DESCRIPTION_NETWORK_XXXCAST_CHANGED
								|SAL_MEDIA_DESCRIPTION_ICE_RESTART_DETECTED
								|SAL_MEDIA_DESCRIPTION_FORCE_STREAM_RECONSTRUCTION ))){
				ms_message("Media descriptions are different, need to restart the streams.");
			} else if ( call->playing_ringbacktone) {
				ms_message("Playing ringback tone, will restart the streams.");
			} else {
				if (md_changed == SAL_MEDIA_DESCRIPTION_UNCHANGED) {
					if (call->all_muted){
						ms_message("Early media finished, unmuting inputs...");
						/*we were in early media, now we want to enable real media */
						call->all_muted = FALSE;
						if (call->audiostream)
							linphone_core_enable_mic(lc, linphone_core_mic_enabled(lc));
#ifdef VIDEO_ENABLED
						if (call->videostream && call->camera_enabled)
							linphone_call_enable_camera(call, linphone_call_camera_enabled(call));
#endif
					}
					/*FIXME ZRTP, might be restarted in any cases ? */
					ms_message("No need to restart streams, SDP is unchanged.");
					goto end;
				}else {
					if (md_changed & SAL_MEDIA_DESCRIPTION_NETWORK_CHANGED) {
						ms_message("Network parameters have changed, update them.");
						linphone_core_update_streams_destinations(lc, call, oldmd, new_md);
					}
					if (md_changed & SAL_MEDIA_DESCRIPTION_CRYPTO_KEYS_CHANGED) {
						ms_message("Crypto parameters have changed, update them.");
						linphone_call_update_crypto_parameters(call, oldmd, new_md);
					}
					goto end;
				}
			}
		}
		linphone_call_stop_media_streams (call);
		if (md_changed & SAL_MEDIA_DESCRIPTION_NETWORK_XXXCAST_CHANGED){
			ms_message("Media ip type has changed, destroying sessions context on call [%p]",call);
			ms_media_stream_sessions_uninit(&call->sessions[call->main_audio_stream_index]);
			ms_media_stream_sessions_uninit(&call->sessions[call->main_video_stream_index]);
			ms_media_stream_sessions_uninit(&call->sessions[call->main_text_stream_index]);
		}
		linphone_call_init_media_streams (call);
	}

	if (call->audiostream==NULL){
		/*this happens after pausing the call locally. The streams are destroyed and then we wait the 200Ok to recreate them*/
		linphone_call_init_media_streams (call);
	}

	if (call->params->real_early_media && call->state==LinphoneCallOutgoingEarlyMedia){
		prepare_early_media_forking(call);
	}
	linphone_call_start_media_streams(call, target_state);
	if (call->state==LinphoneCallPausing && call->paused_by_app && bctbx_list_size(lc->calls)==1){
		linphone_core_play_named_tone(lc,LinphoneToneCallOnHold);
	}
	linphone_call_update_frozen_payloads(call, new_md);
	end:
	if (oldmd)
		sal_media_description_unref(oldmd);

}
#if 0
static bool_t is_duplicate_call(LinphoneCore *lc, const LinphoneAddress *from, const LinphoneAddress *to){
	bctbx_list_t *elem;
	for(elem=lc->calls;elem!=NULL;elem=elem->next){
		LinphoneCall *call=(LinphoneCall*)elem->data;
		if (linphone_address_weak_equal(call->log->from,from) &&
			linphone_address_weak_equal(call->log->to, to)){
			return TRUE;
		}
	}
	return FALSE;
}
#endif

static bool_t already_a_call_with_remote_address(const LinphoneCore *lc, const LinphoneAddress *remote) {
	bctbx_list_t *elem;
	ms_message("Searching for already_a_call_with_remote_address.");

	for(elem=lc->calls;elem!=NULL;elem=elem->next){
		const LinphoneCall *call=(LinphoneCall*)elem->data;
		const LinphoneAddress *cRemote=linphone_call_get_remote_address(call);
		if (linphone_address_weak_equal(cRemote,remote)) {
			ms_warning("already_a_call_with_remote_address found.");
			return TRUE;
		}
	}
	return FALSE;
}


static LinphoneCall * look_for_broken_call_to_replace(SalOp *h, LinphoneCore *lc) {
	const bctbx_list_t *calls = linphone_core_get_calls(lc);
	const bctbx_list_t *it = calls;
	while (it != NULL) {
		LinphoneCall *replaced_call = NULL;
		LinphoneCall *call = (LinphoneCall *)bctbx_list_get_data(it);
		SalOp *replaced_op = sal_call_get_replaces(h);
		if (replaced_op) replaced_call = (LinphoneCall*)sal_op_get_user_pointer(replaced_op);
		if ((call->broken && sal_call_compare_op(h, call->op))
			|| ((replaced_call == call) && (strcmp(sal_op_get_from(h), sal_op_get_from(replaced_op)) == 0) && (strcmp(sal_op_get_to(h), sal_op_get_to(replaced_op)) == 0))) {
			return call;
		}
		it = bctbx_list_next(it);
	}
	
	return NULL;
}

static void call_received(SalOp *h){
	LinphoneCore *lc=(LinphoneCore *)sal_get_user_pointer(sal_op_get_sal(h));
	LinphoneCall *call;
	LinphoneCall *replaced_call;
	char *alt_contact;
	LinphoneAddress *from_addr=NULL;
	LinphoneAddress  *to_addr=NULL;
	LinphoneAddress *from_address_to_search_if_me=NULL; /*address used to know if I'm the caller*/
	SalMediaDescription *md;
	const char * p_asserted_id;

	/* Look if this INVITE is for a call that has already been notified but broken because of network failure */
	replaced_call = look_for_broken_call_to_replace(h, lc);
	if (replaced_call != NULL) {
		linphone_call_replace_op(replaced_call, h);
		return;
	}

	/* first check if we can answer successfully to this invite */
	if (linphone_presence_model_get_basic_status(lc->presence_model) == LinphonePresenceBasicStatusClosed) {
		LinphonePresenceActivity *activity = linphone_presence_model_get_activity(lc->presence_model);
		switch (linphone_presence_activity_get_type(activity)) {
			case LinphonePresenceActivityPermanentAbsence:
				alt_contact = linphone_presence_model_get_contact(lc->presence_model);
				if (alt_contact != NULL) {
					sal_call_decline(h,SalReasonRedirect,alt_contact);
					ms_free(alt_contact);
					sal_op_release(h);
					return;
				}
				break;
			default:
				/*nothing special to be done*/
				break;
		}
	}

	if (!linphone_core_can_we_add_call(lc)){/*busy*/
		sal_call_decline(h,SalReasonBusy,NULL);
		sal_op_release(h);
		return;
	}
	p_asserted_id = sal_custom_header_find(sal_op_get_recv_custom_header(h),"P-Asserted-Identity");
	/*in some situation, better to trust the network rather than the UAC*/
	if (lp_config_get_int(lc->config,"sip","call_logs_use_asserted_id_instead_of_from",0)) {
		LinphoneAddress *p_asserted_id_addr;
		if (!p_asserted_id) {
			ms_warning("No P-Asserted-Identity header found so cannot use it for op [%p] instead of from",h);
		} else {
			p_asserted_id_addr = linphone_address_new(p_asserted_id);
			if (!p_asserted_id_addr) {
				ms_warning("Unsupported P-Asserted-Identity header for op [%p] ",h);
			} else {
				ms_message("Using P-Asserted-Identity [%s] instead of from [%s] for op [%p]",p_asserted_id,sal_op_get_from(h),h);
				from_addr=p_asserted_id_addr;
			}
		}
	}

	if (!from_addr)
		from_addr=linphone_address_new(sal_op_get_from(h));
	to_addr=linphone_address_new(sal_op_get_to(h));

	if (sal_op_get_privacy(h) == SalPrivacyNone) {
		from_address_to_search_if_me=linphone_address_clone(from_addr);
	} else if (p_asserted_id) {
		from_address_to_search_if_me  = linphone_address_new(p_asserted_id);
	} else {
		ms_warning ("Hidden from identity, don't know if it's me");
	}

	if (from_address_to_search_if_me && already_a_call_with_remote_address(lc,from_address_to_search_if_me)){
		char *addr = linphone_address_as_string(from_addr);
		ms_warning("Receiving a call while one with same address [%s] is initiated, refusing this one with busy message.",addr);
		sal_call_decline(h,SalReasonBusy,NULL);
		sal_op_release(h);
		linphone_address_unref(from_addr);
		linphone_address_unref(to_addr);
		linphone_address_unref(from_address_to_search_if_me);
		ms_free(addr);
		return;
	} else if (from_address_to_search_if_me) {
		linphone_address_unref(from_address_to_search_if_me);
	}

	call=linphone_call_new_incoming(lc,from_addr,to_addr,h);

	linphone_call_make_local_media_description(call);
	sal_call_set_local_media_description(call->op,call->localdesc);
	md=sal_call_get_final_media_description(call->op);
	if (md){
		if (sal_media_description_empty(md) || linphone_core_incompatible_security(lc,md)){
			sal_call_decline(call->op,SalReasonNotAcceptable,NULL);
			linphone_call_unref(call);
			return;
		}
	}

	/* the call is acceptable so we can now add it to our list */
	linphone_core_add_call(lc,call);
	linphone_call_ref(call); /*prevent the call from being destroyed while we are notifying, if the user declines within the state callback */

	call->bg_task_id=sal_begin_background_task("liblinphone call notification", NULL, NULL);

	if (call->defer_notify_incoming) {
		/* Defer ringing until the end of the ICE candidates gathering process. */
		ms_message("Defer ringing to gather ICE candidates");
		return;
	}
#ifdef BUILD_UPNP
	if ((linphone_core_get_firewall_policy(lc) == LinphonePolicyUseUpnp) && (call->upnp_session != NULL)) {
		/* Defer ringing until the end of the ICE candidates gathering process. */
		ms_message("Defer ringing to gather uPnP candidates");
		return;
	}
#endif //BUILD_UPNP

	linphone_core_notify_incoming_call(lc,call);
}

static void try_early_media_forking(LinphoneCall *call, SalMediaDescription *md){
	SalMediaDescription *cur_md=call->resultdesc;
	int i;
	SalStreamDescription *ref_stream,*new_stream;
	ms_message("Early media response received from another branch, checking if media can be forked to this new destination.");

	for (i=0;i<SAL_MEDIA_DESCRIPTION_MAX_STREAMS;++i){
		if (!sal_stream_description_active(&cur_md->streams[i])) continue;
		ref_stream=&cur_md->streams[i];
		new_stream=&md->streams[i];
		if (ref_stream->type==new_stream->type && ref_stream->payloads && new_stream->payloads){
			PayloadType *refpt, *newpt;
			refpt=(PayloadType*)ref_stream->payloads->data;
			newpt=(PayloadType*)new_stream->payloads->data;
			if (strcmp(refpt->mime_type,newpt->mime_type)==0 && refpt->clock_rate==newpt->clock_rate
				&& payload_type_get_number(refpt)==payload_type_get_number(newpt)){
				MediaStream *ms=NULL;
				if (ref_stream->type==SalAudio){
					ms=(MediaStream*)call->audiostream;
				}else if (ref_stream->type==SalVideo){
					ms=(MediaStream*)call->videostream;
				}
				if (ms){
					RtpSession *session=ms->sessions.rtp_session;
					const char *rtp_addr=new_stream->rtp_addr[0]!='\0' ? new_stream->rtp_addr : md->addr;
					const char *rtcp_addr=new_stream->rtcp_addr[0]!='\0' ? new_stream->rtcp_addr : md->addr;
					if (ms_is_multicast(rtp_addr))
						ms_message("Multicast addr [%s/%i] does not need auxiliary rtp's destination for call [%p]",
							   rtp_addr,new_stream->rtp_port,call);
					else
						rtp_session_add_aux_remote_addr_full(session,rtp_addr,new_stream->rtp_port,rtcp_addr,new_stream->rtcp_port);
				}
			}
		}
	}
}

static void start_remote_ring(LinphoneCore *lc, LinphoneCall *call) {
	if (lc->sound_conf.play_sndcard!=NULL){
		MSSndCard *ringcard=lc->sound_conf.lsd_card ? lc->sound_conf.lsd_card : lc->sound_conf.play_sndcard;
		if (call->localdesc->streams[0].max_rate>0) ms_snd_card_set_preferred_sample_rate(ringcard, call->localdesc->streams[0].max_rate);
		/*we release sound before playing ringback tone*/
		if (call->audiostream)
			audio_stream_unprepare_sound(call->audiostream);
		if( lc->sound_conf.remote_ring ){
			lc->ringstream=ring_start(lc->factory, lc->sound_conf.remote_ring,2000,ringcard);
		}
	}
}

static void call_ringing(SalOp *h){
	LinphoneCore *lc=(LinphoneCore *)sal_get_user_pointer(sal_op_get_sal(h));
	LinphoneCall *call=(LinphoneCall*)sal_op_get_user_pointer(h);
	SalMediaDescription *md;

	if (call==NULL) return;

	/*set privacy*/
	call->current_params->privacy=(LinphonePrivacyMask)sal_op_get_privacy(call->op);

	linphone_core_notify_display_status(lc,_("Remote ringing."));

	md=sal_call_get_final_media_description(h);
	if (md==NULL){
		linphone_core_stop_dtmf_stream(lc);
		if (call->state==LinphoneCallOutgoingEarlyMedia){
			/*already doing early media */
			return;
		}
		if (lc->ringstream == NULL) start_remote_ring(lc, call);
		ms_message("Remote ringing...");
		linphone_core_notify_display_status(lc,_("Remote ringing..."));
		linphone_call_set_state(call,LinphoneCallOutgoingRinging,"Remote ringing");
	}else{
		/*initialize the remote call params by invoking linphone_call_get_remote_params(). This is useful as the SDP may not be present in the 200Ok*/
		linphone_call_get_remote_params(call);
		/*accept early media */
		if ((call->audiostream && audio_stream_started(call->audiostream))
#ifdef VIDEO_ENABLED
			|| (call->videostream && video_stream_started(call->videostream))
#endif
			) {
			/*streams already started */
			try_early_media_forking(call,md);
			#ifdef VIDEO_ENABLED
			if (call->videostream){
				/*just request for iframe*/
				video_stream_send_vfu(call->videostream);
			}
			#endif
		return;
		}

		linphone_core_notify_show_interface(lc);
		linphone_core_notify_display_status(lc,_("Early media."));
		linphone_call_set_state(call,LinphoneCallOutgoingEarlyMedia,"Early media");
		linphone_core_stop_ringing(lc);
		ms_message("Doing early media...");
		linphone_core_update_streams(lc,call,md, call->state);
		if ((linphone_call_params_get_audio_direction(linphone_call_get_current_params(call)) == LinphoneMediaDirectionInactive) && call->audiostream) {
			if (lc->ringstream != NULL) return; /* Already ringing! */
			start_remote_ring(lc, call);
		}
	}
}

static void start_pending_refer(LinphoneCall *call){
	linphone_core_start_refered_call(call->core, call,NULL);
}

static void process_call_accepted(LinphoneCore *lc, LinphoneCall *call, SalOp *op){
	SalMediaDescription *md, *rmd;
	LinphoneCallState next_state = LinphoneCallIdle;
	const char *next_state_str = NULL;
	LinphoneTaskList tl;

	switch (call->state){/*immediately notify the connected state, even if errors occur after*/
		case LinphoneCallOutgoingProgress:
		case LinphoneCallOutgoingRinging:
		case LinphoneCallOutgoingEarlyMedia:
			/*immediately notify the connected state*/
			linphone_call_set_state(call,LinphoneCallConnected,"Connected");
			{
				char *tmp=linphone_call_get_remote_address_as_string (call);
				char *msg=ms_strdup_printf(_("Call answered by %s"),tmp);
				linphone_core_notify_display_status(lc,msg);
				ms_free(tmp);
				ms_free(msg);
			}
		break;
		default:
		break;
	}

	linphone_task_list_init(&tl);
	rmd=sal_call_get_remote_media_description(op);
	/*set privacy*/
	call->current_params->privacy=(LinphonePrivacyMask)sal_op_get_privacy(call->op);
	/*reset the internal call update flag, so it doesn't risk to be copied and used in further re-INVITEs*/
	if (call->params->internal_call_update)
		call->params->internal_call_update = FALSE;


#ifdef BUILD_UPNP
	if (call->upnp_session != NULL && rmd) {
		linphone_core_update_upnp_from_remote_media_description(call, rmd);
	}
#endif //BUILD_UPNP

	md=sal_call_get_final_media_description(op);
	if (md == NULL && call->prevstate == LinphoneCallOutgoingEarlyMedia && call->resultdesc != NULL){
		ms_message("Using early media SDP since none was received with the 200 OK");
		md = call->resultdesc;
	}
	if (md && (sal_media_description_empty(md) || linphone_core_incompatible_security(lc,md))){
		md = NULL;
	}
	if (md){ /*there is a valid SDP in the response, either offer or answer, and we're able to start/update the streams*/

		/* Handle remote ICE attributes if any. */
		if (call->ice_session != NULL && rmd) {
			linphone_call_update_ice_from_remote_media_description(call, rmd, FALSE);
		}

		switch (call->state){
			case LinphoneCallResuming:
				linphone_core_notify_display_status(lc,_("Call resumed."));
			/*intentionally no break*/
			case LinphoneCallConnected:
				if (call->referer) linphone_core_notify_refer_state(lc,call->referer,call);
			/*intentionally no break*/
			case LinphoneCallUpdating:
			case LinphoneCallUpdatedByRemote:
				if (!sal_media_description_has_dir(call->localdesc, SalStreamInactive) &&
					(sal_media_description_has_dir(md,SalStreamRecvOnly) ||
					sal_media_description_has_dir(md,SalStreamInactive))){
					next_state = LinphoneCallPausedByRemote;
					next_state_str = "Call paused by remote";
				}else{
					if (!call->params->in_conference)
						lc->current_call=call;
					next_state = LinphoneCallStreamsRunning;
					next_state_str = "Streams running";
				}
			break;
			case LinphoneCallEarlyUpdating:
				next_state_str = "Early update accepted";
				next_state = call->prevstate;
			break;
			case LinphoneCallPausing:
				/*when we entered the pausing state, we always reach the paused state whatever the content of the remote SDP is.
					Our streams are all send-only (with music), soundcard and camera are never used*/
				next_state = LinphoneCallPaused;
				next_state_str = "Call paused";
				if (call->refer_pending)
					linphone_task_list_add(&tl, (LinphoneCoreIterateHook)start_pending_refer, call);
			break;
			default:
				ms_error("call_accepted(): don't know what to do in state [%s]", linphone_call_state_to_string(call->state));
			break;
		}

		if (next_state != LinphoneCallIdle){
			linphone_call_update_remote_session_id_and_ver(call);
			linphone_core_update_ice_state_in_call_stats(call);
			linphone_core_update_streams(lc, call, md, next_state);
			linphone_call_fix_call_parameters(call, rmd);
			linphone_call_set_state(call, next_state, next_state_str);
		}else{
			ms_error("BUG: next_state is not set in call_accepted(), current state is %s", linphone_call_state_to_string(call->state));
		}
	}else{ /*invalid or no SDP*/
		switch (call->prevstate){
			/*send a bye only in case of early states*/
			case LinphoneCallOutgoingInit:
			case LinphoneCallOutgoingProgress:
			case LinphoneCallOutgoingRinging:
			case LinphoneCallOutgoingEarlyMedia:
			case LinphoneCallIncomingReceived:
			case LinphoneCallIncomingEarlyMedia:
				ms_error("Incompatible SDP answer received, need to abort the call");
				linphone_core_abort_call(lc,call,_("Incompatible, check codecs or security settings..."));
				break;
			/*otherwise we are able to resume previous state*/
			default:
				ms_error("Incompatible SDP answer received");
				switch(call->state) {
					case LinphoneCallPausedByRemote:
						break;
					case LinphoneCallPaused:
						break;
					case LinphoneCallStreamsRunning:
						break;
					default:
						ms_message("Incompatible SDP answer received, restoring previous state [%s]",linphone_call_state_to_string(call->prevstate));
						linphone_call_set_state(call,call->prevstate,_("Incompatible media parameters."));
						break;
				}
				break;
		}
	}
	linphone_task_list_run(&tl);
	linphone_task_list_free(&tl);
}

/*
 * could be reach :
 *  - when the call is accepted
 *  - when a request is accepted (pause, resume)
 */
static void call_accepted(SalOp *op){
	LinphoneCore *lc=(LinphoneCore *)sal_get_user_pointer(sal_op_get_sal(op));
	LinphoneCall *call=(LinphoneCall*)sal_op_get_user_pointer(op);

	if (call == NULL){
		ms_warning("call_accepted: call does no longer exist.");
		return ;
	}
	process_call_accepted(lc, call, op);
}

static void call_resumed(LinphoneCore *lc, LinphoneCall *call){
	linphone_core_notify_display_status(lc,_("We have been resumed."));
	_linphone_core_accept_call_update(lc,call,NULL,LinphoneCallStreamsRunning,"Connected (streams running)");
}

static void call_paused_by_remote(LinphoneCore *lc, LinphoneCall *call){
	LinphoneCallParams *params;

	/* we are being paused */
	linphone_core_notify_display_status(lc,_("We are paused by other party."));
	params = linphone_call_params_copy(call->params);
	if (lp_config_get_int(lc->config, "sip", "inactive_video_on_pause", 0)) {
		linphone_call_params_set_video_direction(params, LinphoneMediaDirectionInactive);
	}
	_linphone_core_accept_call_update(lc,call,params,LinphoneCallPausedByRemote,"Call paused by remote");
	linphone_call_params_unref(params);
}

static void call_updated_by_remote(LinphoneCore *lc, LinphoneCall *call){
	linphone_core_notify_display_status(lc,_("Call is updated by remote."));
	linphone_call_set_state(call, LinphoneCallUpdatedByRemote,"Call updated by remote");
	if (call->defer_update == FALSE){
		if (call->state == LinphoneCallUpdatedByRemote){
			linphone_core_accept_call_update(lc,call,NULL);
		}else{
			/*otherwise it means that the app responded by linphone_core_accept_call_update
			 * within the callback, so job is already done.*/
		}
	}else{
		if (call->state == LinphoneCallUpdatedByRemote){
			ms_message("LinphoneCall [%p]: UpdatedByRemoted was signaled but defered. LinphoneCore expects the application to call "
				"linphone_core_accept_call_update() later.", call);
		}
	}
}

/* this callback is called when an incoming re-INVITE/ SIP UPDATE modifies the session*/
static void call_updated(LinphoneCore *lc, LinphoneCall *call, SalOp *op, bool_t is_update){
	SalMediaDescription *rmd=sal_call_get_remote_media_description(op);

	call->defer_update = lp_config_get_int(lc->config, "sip", "defer_update_default", FALSE);

	switch(call->state){
		case LinphoneCallPausedByRemote:
			if (sal_media_description_has_dir(rmd,SalStreamSendRecv) || sal_media_description_has_dir(rmd,SalStreamRecvOnly)){
				call_resumed(lc,call);
			}else{
				call_updated_by_remote(lc, call);
			}
		break;
		/*SIP UPDATE CASE*/
		case LinphoneCallOutgoingRinging:
		case LinphoneCallOutgoingEarlyMedia:
		case LinphoneCallIncomingEarlyMedia:
			if (is_update) {
				linphone_call_set_state(call, LinphoneCallEarlyUpdatedByRemote, "EarlyUpdatedByRemote");
				_linphone_core_accept_call_update(lc,call,NULL,call->prevstate,linphone_call_state_to_string(call->prevstate));
			}
			break;
		case LinphoneCallStreamsRunning:
		case LinphoneCallConnected:
		case LinphoneCallUpdatedByRemote: // Can happen on UAC connectivity loss
			if (sal_media_description_has_dir(rmd,SalStreamSendOnly) || sal_media_description_has_dir(rmd,SalStreamInactive)){
				call_paused_by_remote(lc,call);
			}else{
				call_updated_by_remote(lc, call);
			}
		break;
		case LinphoneCallPaused:
			/*we'll remain in pause state but accept the offer anyway according to default parameters*/
			_linphone_core_accept_call_update(lc,call,NULL,call->state,linphone_call_state_to_string(call->state));
		break;
		case LinphoneCallUpdating:
		case LinphoneCallPausing:
		case LinphoneCallResuming:
			sal_call_decline(call->op,SalReasonInternalError,NULL);
			/*no break*/
		case LinphoneCallIdle:
		case LinphoneCallOutgoingInit:
		case LinphoneCallEnd:
		case LinphoneCallIncomingReceived:
		case LinphoneCallOutgoingProgress:
		case LinphoneCallRefered:
		case LinphoneCallError:
		case LinphoneCallReleased:
		case LinphoneCallEarlyUpdatedByRemote:
		case LinphoneCallEarlyUpdating:
			ms_warning("Receiving reINVITE or UPDATE while in state [%s], should not happen.",linphone_call_state_to_string(call->state));
		break;
	}
}

/* this callback is called when an incoming re-INVITE/ SIP UPDATE modifies the session*/
static void call_updating(SalOp *op, bool_t is_update){
	LinphoneCore *lc=(LinphoneCore *)sal_get_user_pointer(sal_op_get_sal(op));
	LinphoneCall *call=(LinphoneCall*)sal_op_get_user_pointer(op);
	SalMediaDescription *rmd=sal_call_get_remote_media_description(op);

	if (!call) {
		ms_error("call_updating(): call doesn't exist anymore");
		return ;
	}
	linphone_call_fix_call_parameters(call, rmd);
	if (call->state!=LinphoneCallPaused){
		/*Refresh the local description, but in paused state, we don't change anything.*/
		if (rmd == NULL && lp_config_get_int(call->core->config,"sip","sdp_200_ack_follow_video_policy",0)) {
			LinphoneCallParams *p=linphone_core_create_call_params(lc, NULL);
			ms_message("Applying default policy for offering SDP on call [%p]",call);
			linphone_call_set_new_params(call, p);
			linphone_call_params_unref(p);
		}
		linphone_call_make_local_media_description(call);
		sal_call_set_local_media_description(call->op,call->localdesc);
	}
	if (rmd == NULL){
		/* case of a reINVITE or UPDATE without SDP */
		call->expect_media_in_ack = TRUE;
		sal_call_accept(op); /*respond with an offer*/
		/*don't do anything else in this case, wait for the ACK to receive to notify the app*/
	}else {
		SalMediaDescription *md;
		SalMediaDescription *prev_result_desc=call->resultdesc;

		call->expect_media_in_ack = FALSE;

		md=sal_call_get_final_media_description(call->op);
		if (md && (sal_media_description_empty(md) || linphone_core_incompatible_security(lc,md))){
			sal_call_decline(call->op,SalReasonNotAcceptable,NULL);
			return;
		}
		if (is_update && prev_result_desc && md){
			int diff=sal_media_description_equals(prev_result_desc,md);
			if (diff & (SAL_MEDIA_DESCRIPTION_CRYPTO_POLICY_CHANGED|SAL_MEDIA_DESCRIPTION_STREAMS_CHANGED)){
				ms_warning("Cannot accept this update, it is changing parameters that require user approval");
				sal_call_decline(call->op,SalReasonNotAcceptable,NULL); /*FIXME should send 504 Cannot change the session parameters without prompting the user"*/
				return;
			}
		}
		call_updated(lc, call, op, is_update);
	}
}


static void call_ack(SalOp *op){
	LinphoneCore *lc=(LinphoneCore *)sal_get_user_pointer(sal_op_get_sal(op));
	LinphoneCall *call=(LinphoneCall*)sal_op_get_user_pointer(op);

	if (call == NULL){
		ms_warning("call_ack(): no call for which an ack is expected");
		return;
	}
	if (call->expect_media_in_ack){
		switch(call->state){
			case LinphoneCallStreamsRunning:
			case LinphoneCallPausedByRemote:
				linphone_call_set_state(call, LinphoneCallUpdatedByRemote, "UpdatedByRemote");
			break;
			default:
			break;
		}
		process_call_accepted(lc, call, op);
	}
}

static void call_terminated(SalOp *op, const char *from){
	LinphoneCore *lc=(LinphoneCore *)sal_get_user_pointer(sal_op_get_sal(op));
	LinphoneCall *call=(LinphoneCall*)sal_op_get_user_pointer(op);

	if (call==NULL) return;

	switch(linphone_call_get_state(call)){
		case LinphoneCallEnd:
		case LinphoneCallError:
			ms_warning("call_terminated: already terminated, ignoring.");
			return;
		break;
		case LinphoneCallIncomingReceived:
		case LinphoneCallIncomingEarlyMedia:
			sal_error_info_set(&call->non_op_error,SalReasonRequestTimeout,0,"Incoming call cancelled",NULL);
		break;
		default:
		break;
	}
	ms_message("Current call terminated...");
	if (call->refer_pending){
		linphone_core_start_refered_call(lc,call,NULL);
	}
	//we stop the call only if we have this current call or if we are in call
	if ((bctbx_list_size(lc->calls)  == 1) || linphone_core_in_call(lc)) {
		linphone_core_stop_ringing(lc);
	}
	linphone_call_stop_media_streams(call);
	linphone_core_notify_show_interface(lc);
	linphone_core_notify_display_status(lc,_("Call terminated."));

#ifdef BUILD_UPNP
	linphone_call_delete_upnp_session(call);
#endif //BUILD_UPNP

	linphone_call_set_state(call, LinphoneCallEnd,"Call ended");
}

static int resume_call_after_failed_transfer(LinphoneCall *call){
	if (call->was_automatically_paused && call->state==LinphoneCallPausing)
		return BELLE_SIP_CONTINUE; /*was still in pausing state*/

	if (call->was_automatically_paused && call->state==LinphoneCallPaused){
		if (sal_op_is_idle(call->op)){
			linphone_core_resume_call(call->core,call);
		}else {
			ms_message("resume_call_after_failed_transfer(), salop was busy");
			return BELLE_SIP_CONTINUE;
		}
	}
	linphone_call_unref(call);
	return BELLE_SIP_STOP;
}

static void call_failure(SalOp *op){
	LinphoneCore *lc=(LinphoneCore *)sal_get_user_pointer(sal_op_get_sal(op));
	const SalErrorInfo *ei=sal_op_get_error_info(op);
	char *msg486=_("User is busy.");
	char *msg480=_("User is temporarily unavailable.");
	/*char *retrymsg=_("%s. Retry after %i minute(s).");*/
	char *msg600=_("User does not want to be disturbed.");
	char *msg603=_("Call declined.");
	const char *msg=ei->full_string;
	LinphoneCall *referer;
	LinphoneCall *call=(LinphoneCall*)sal_op_get_user_pointer(op);
	
	if (call==NULL){
		ms_warning("Call faillure reported on already terminated call.");
		return ;
	}
	
	referer=call->referer;

	linphone_core_notify_show_interface(lc);
	switch(ei->reason){
		case SalReasonNone:
		break;
		case SalReasonRequestTimeout:
			msg=_("Request timeout.");
			linphone_core_notify_display_status(lc,msg);
		break;
		case SalReasonDeclined:
			msg=msg603;
			linphone_core_notify_display_status(lc,msg603);
		break;
		case SalReasonBusy:
			msg=msg486;
			linphone_core_notify_display_status(lc,msg486);
		break;
		case SalReasonRedirect:
		{
			linphone_call_stop_media_streams(call);
			if (	call->state==LinphoneCallOutgoingInit
					|| call->state==LinphoneCallOutgoingProgress
					|| call->state==LinphoneCallOutgoingRinging /*push case*/
					|| call->state==LinphoneCallOutgoingEarlyMedia){
				LinphoneAddress* redirection_to = (LinphoneAddress*)sal_op_get_remote_contact_address(call->op);
				if( redirection_to ){
					char* url = linphone_address_as_string(redirection_to);
					ms_warning("Redirecting call [%p] to %s",call, url);
					ms_free(url);
					if( call->log->to != NULL ) {
						linphone_address_unref(call->log->to);
					}
					call->log->to = linphone_address_ref(redirection_to);
					linphone_core_restart_invite(lc, call);
					return;
				}
			}
			msg=_("Redirected");
			linphone_core_notify_display_status(lc,msg);
		}
		break;
		case SalReasonTemporarilyUnavailable:
			msg=msg480;
			linphone_core_notify_display_status(lc,msg480);
		break;
		case SalReasonNotFound:
			linphone_core_notify_display_status(lc,msg);
		break;
		case SalReasonDoNotDisturb:
			msg=msg600;
			linphone_core_notify_display_status(lc,msg600);
		break;
		case SalReasonUnsupportedContent: /*<this is for compatibility: linphone sent 415 because of SDP offer answer failure*/
		case SalReasonNotAcceptable:
			ms_message("Outgoing call [%p] failed with SRTP and/or AVPF enabled", call);
			if ((call->state == LinphoneCallOutgoingInit)
				|| (call->state == LinphoneCallOutgoingProgress)
				|| (call->state == LinphoneCallOutgoingRinging) /* Push notification case */
				|| (call->state == LinphoneCallOutgoingEarlyMedia)) {
				int i;
				for (i = 0; i < call->localdesc->nb_streams; i++) {
					if (!sal_stream_description_active(&call->localdesc->streams[i])) continue;
					if (call->params->media_encryption == LinphoneMediaEncryptionSRTP) {
						if (call->params->avpf_enabled == TRUE) {
							if (i == 0) ms_message("Retrying call [%p] with SAVP", call);
							call->params->avpf_enabled = FALSE;
							linphone_core_restart_invite(lc, call);
							return;
						} else if (!linphone_core_is_media_encryption_mandatory(lc)) {
							if (i == 0) ms_message("Retrying call [%p] with AVP", call);
							call->params->media_encryption = LinphoneMediaEncryptionNone;
							memset(call->localdesc->streams[i].crypto, 0, sizeof(call->localdesc->streams[i].crypto));
							linphone_core_restart_invite(lc, call);
							return;
						}
					} else if (call->params->avpf_enabled == TRUE) {
						if (i == 0) ms_message("Retrying call [%p] with AVP", call);
						call->params->avpf_enabled = FALSE;
						linphone_core_restart_invite(lc, call);
						return;
					}
				}
			}
			msg=_("Incompatible media parameters.");
			linphone_core_notify_display_status(lc,msg);
		break;
		default:
			linphone_core_notify_display_status(lc,_("Call failed."));
	}

	/*some call errors are not fatal*/
	switch (call->state) {
	case LinphoneCallUpdating:
	case LinphoneCallPausing:
	case LinphoneCallResuming:
		if (ei->reason != SalReasonNoMatch){
			ms_message("Call error on state [%s], restoring previous state",linphone_call_state_to_string(call->prevstate));
			linphone_call_set_state(call, call->prevstate,ei->full_string);
			return;
		}
	default:
		break; /*nothing to do*/
	}

	linphone_core_stop_ringing(lc);
	linphone_call_stop_media_streams(call);

#ifdef BUILD_UPNP
	linphone_call_delete_upnp_session(call);
#endif //BUILD_UPNP

	if (call->state!=LinphoneCallEnd && call->state!=LinphoneCallError){
		if (ei->reason==SalReasonDeclined){
			linphone_call_set_state(call,LinphoneCallEnd,"Call declined.");
		}else{
			if (linphone_call_state_is_early(call->state)){
				linphone_call_set_state(call,LinphoneCallError,ei->full_string);
			}else{
				linphone_call_set_state(call, LinphoneCallEnd, ei->full_string);
			}
		}
		if (ei->reason!=SalReasonNone) linphone_core_play_call_error_tone(lc,linphone_reason_from_sal(ei->reason));
	}

	if (referer){
		/*notify referer of the failure*/
		linphone_core_notify_refer_state(lc,referer,call);
		/*schedule automatic resume of the call. This must be done only after the notifications are completed due to dialog serialization of requests.*/
		linphone_core_queue_task(lc,(belle_sip_source_func_t)resume_call_after_failed_transfer,linphone_call_ref(referer),"Automatic call resuming after failed transfer");
	}
}

static void call_released(SalOp *op){
	LinphoneCall *call=(LinphoneCall*)sal_op_get_user_pointer(op);
	if (call!=NULL){
		linphone_call_set_state(call,LinphoneCallReleased,"Call released");
	}else{
		/*we can arrive here when the core manages call at Sal level without creating a LinphoneCall object. Typicially:
		 * - when declining an incoming call with busy because maximum number of calls is reached.
		 */
	}
}

static void call_cancel_done(SalOp *op) {
	LinphoneCall *call = (LinphoneCall *)sal_op_get_user_pointer(op);
	if (call->reinvite_on_cancel_response_requested == TRUE) {
		call->reinvite_on_cancel_response_requested = FALSE;
		linphone_call_reinvite_to_recover_from_connection_loss(call);
	}
}

static void auth_failure(SalOp *op, SalAuthInfo* info) {
	LinphoneCore *lc = (LinphoneCore *)sal_get_user_pointer(sal_op_get_sal(op));
	LinphoneAuthInfo *ai = NULL;

	if (info != NULL) {
		ai = (LinphoneAuthInfo*)_linphone_core_find_auth_info(lc, info->realm, info->username, info->domain, TRUE);
		if (ai){
			LinphoneAuthMethod method = info->mode == SalAuthModeHttpDigest ? LinphoneAuthHttpDigest : LinphoneAuthTls;
			LinphoneAuthInfo *auth_info = linphone_core_create_auth_info(lc, info->username, NULL, NULL, NULL, info->realm, info->domain);
			ms_message("%s/%s/%s/%s authentication fails.", info->realm, info->username, info->domain, info->mode == SalAuthModeHttpDigest ? "HttpDigest" : "Tls");
			/*ask again for password if auth info was already supplied but apparently not working*/
			linphone_core_notify_authentication_requested(lc, auth_info, method);
			linphone_auth_info_destroy(auth_info);
			// Deprecated
			linphone_core_notify_auth_info_requested(lc, info->realm, info->username, info->domain);
		}
	}
}

static void register_success(SalOp *op, bool_t registered){
	LinphoneCore *lc=(LinphoneCore *)sal_get_user_pointer(sal_op_get_sal(op));
	LinphoneProxyConfig *cfg=(LinphoneProxyConfig*)sal_op_get_user_pointer(op);
	char *msg;

	if (!cfg){
		ms_message("Registration success for deleted proxy config, ignored");
		return;
	}
	linphone_proxy_config_set_state(cfg, registered ? LinphoneRegistrationOk : LinphoneRegistrationCleared ,
									registered ? "Registration successful" : "Unregistration done");
	{
		if (registered) msg=ms_strdup_printf(_("Registration on %s successful."),sal_op_get_proxy(op));
		else msg=ms_strdup_printf(_("Unregistration on %s done."),sal_op_get_proxy(op));
		linphone_core_notify_display_status(lc,msg);
		ms_free(msg);
	}

}

static void register_failure(SalOp *op){
	LinphoneCore *lc=(LinphoneCore *)sal_get_user_pointer(sal_op_get_sal(op));
	LinphoneProxyConfig *cfg=(LinphoneProxyConfig*)sal_op_get_user_pointer(op);
	const SalErrorInfo *ei=sal_op_get_error_info(op);
	const char *details=ei->full_string;

	if (cfg==NULL){
		ms_warning("Registration failed for unknown proxy config.");
		return ;
	}
	if (details==NULL)
		details=_("no response timeout");

	{
		char *msg=ortp_strdup_printf(_("Registration on %s failed: %s"),sal_op_get_proxy(op), details);
		linphone_core_notify_display_status(lc,msg);
		ms_free(msg);
	}

	if ((ei->reason == SalReasonServiceUnavailable || ei->reason == SalReasonIOError)
			&& linphone_proxy_config_get_state(cfg) == LinphoneRegistrationOk) {
		linphone_proxy_config_set_state(cfg,LinphoneRegistrationProgress,_("Service unavailable, retrying"));
	} else {
		linphone_proxy_config_set_state(cfg,LinphoneRegistrationFailed,details);
	}
	if (cfg->long_term_event){
		/*prevent publish to be sent now until registration gets successful*/
		linphone_event_terminate(cfg->long_term_event);
		linphone_event_unref(cfg->long_term_event);
		cfg->long_term_event=NULL;
		cfg->send_publish=cfg->publish;
	}
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
	if (!call) return;
	linphone_core_notify_dtmf_received(lc, call, dtmf);
}

static void refer_received(Sal *sal, SalOp *op, const char *referto){
	LinphoneCore *lc=(LinphoneCore *)sal_get_user_pointer(sal);
	LinphoneCall *call=(LinphoneCall*)sal_op_get_user_pointer(op);
	LinphoneAddress *refer_to_addr = linphone_address_new(referto);
	char method[20] = "";

	if(refer_to_addr) {
		const char *tmp = linphone_address_get_method_param(refer_to_addr);
		if(tmp) strncpy(method, tmp, sizeof(method));
		linphone_address_unref(refer_to_addr);
	}
	if (call && (strlen(method) == 0 || strcmp(method, "INVITE") == 0)) {
		if (call->refer_to!=NULL){
			ms_free(call->refer_to);
		}
		call->refer_to=ms_strdup(referto);
		call->refer_pending=TRUE;
		linphone_call_set_state(call,LinphoneCallRefered,"Refered");
		{
			char *msg=ms_strdup_printf(_("We are transferred to %s"),referto);
			linphone_core_notify_display_status(lc,msg);
			ms_free(msg);
		}
		if (call->refer_pending) linphone_core_start_refered_call(lc,call,NULL);
	}else {
		linphone_core_notify_refer_received(lc,referto);
	}
}

static bool_t is_duplicate_msg(LinphoneCore *lc, const char *msg_id){
	bctbx_list_t *elem=lc->last_recv_msg_ids;
	bctbx_list_t *tail=NULL;
	int i;
	bool_t is_duplicate=FALSE;
	for(i=0;elem!=NULL;elem=elem->next,i++){
		if (strcmp((const char*)elem->data,msg_id)==0){
			is_duplicate=TRUE;
		}
		tail=elem;
	}
	if (!is_duplicate){
		lc->last_recv_msg_ids=bctbx_list_prepend(lc->last_recv_msg_ids,ms_strdup(msg_id));
	}
	if (i>=10){
		ms_free(tail->data);
		lc->last_recv_msg_ids=bctbx_list_erase_link(lc->last_recv_msg_ids,tail);
	}
	return is_duplicate;
}


static void text_received(SalOp *op, const SalMessage *msg){
	LinphoneCore *lc=(LinphoneCore *)sal_get_user_pointer(sal_op_get_sal(op));
	LinphoneCall *call=(LinphoneCall*)sal_op_get_user_pointer(op);
	LinphoneReason reason = lc->chat_deny_code;
	if (reason == LinphoneReasonNone && is_duplicate_msg(lc, msg->message_id) == FALSE) {
		reason = linphone_core_message_received(lc, op, msg);
	}
	sal_message_reply(op, linphone_reason_to_sal(reason));
	if (!call) sal_op_release(op);
}

static void is_composing_received(SalOp *op, const SalIsComposing *is_composing) {
	LinphoneCore *lc = (LinphoneCore *)sal_get_user_pointer(sal_op_get_sal(op));
	LinphoneReason reason = linphone_core_is_composing_received(lc, op, is_composing);
	sal_message_reply(op, linphone_reason_to_sal(reason));
	sal_op_release(op);
}

static void imdn_received(SalOp *op, const SalImdn *imdn) {
	LinphoneCore *lc = (LinphoneCore *)sal_get_user_pointer(sal_op_get_sal(op));
	LinphoneReason reason = linphone_core_imdn_received(lc, op, imdn);
	sal_message_reply(op, linphone_reason_to_sal(reason));
	sal_op_release(op);
}

static void parse_presence_requested(SalOp *op, const char *content_type, const char *content_subtype, const char *body, SalPresenceModel **result) {
	linphone_notify_parse_presence(content_type, content_subtype, body, result);
}

static void convert_presence_to_xml_requested(SalOp *op, SalPresenceModel *presence, const char *contact, char **content) {
	/*for backward compatibility because still used by notify. No loguer used for publish*/

	if(linphone_presence_model_get_presentity((LinphonePresenceModel*)presence) == NULL) {
		LinphoneAddress * presentity = linphone_address_new(contact);
		linphone_presence_model_set_presentity((LinphonePresenceModel*)presence, presentity);
		linphone_address_unref(presentity);
	}
	*content = linphone_presence_model_to_xml((LinphonePresenceModel*)presence);
}

static void notify_presence(SalOp *op, SalSubscribeStatus ss, SalPresenceModel *model, const char *msg){
	LinphoneCore *lc=(LinphoneCore *)sal_get_user_pointer(sal_op_get_sal(op));
	linphone_notify_recv(lc,op,ss,model);
}

static void subscribe_presence_received(SalOp *op, const char *from){
	LinphoneCore *lc=(LinphoneCore *)sal_get_user_pointer(sal_op_get_sal(op));
	linphone_subscription_new(lc,op,from);
}

static void subscribe_presence_closed(SalOp *op, const char *from){
	LinphoneCore *lc=(LinphoneCore *)sal_get_user_pointer(sal_op_get_sal(op));
	linphone_subscription_closed(lc,op);
}

static void ping_reply(SalOp *op){
	LinphoneCall *call=(LinphoneCall*) sal_op_get_user_pointer(op);
	ms_message("ping reply !");
	if (call){
		if (call->state==LinphoneCallOutgoingInit){
			call->ping_replied=TRUE;
			linphone_core_proceed_with_invite_if_ready(call->core,call,NULL);
		}
	}
	else
	{
		ms_warning("ping reply without call attached...");
	}
}

static bool_t fill_auth_info_with_client_certificate(LinphoneCore *lc, SalAuthInfo* sai) {
	const char *chain_file = linphone_core_get_tls_cert_path(lc);
	const char *key_file = linphone_core_get_tls_key_path(lc);

	if (key_file && chain_file) {
#ifndef _WIN32
		// optinal check for files
		struct stat st;
		if (stat(key_file, &st)) {
			ms_warning("No client certificate key found in %s", key_file);
			return FALSE;
		}
		if (stat(chain_file, &st)) {
			ms_warning("No client certificate chain found in %s", chain_file);
			return FALSE;
		}
#endif
		sal_certificates_chain_parse_file(sai, chain_file, SAL_CERTIFICATE_RAW_FORMAT_PEM);
		sal_signing_key_parse_file(sai, key_file, "");
	} else if (lc->tls_cert && lc->tls_key) {
		sal_certificates_chain_parse(sai, lc->tls_cert, SAL_CERTIFICATE_RAW_FORMAT_PEM);
		sal_signing_key_parse(sai, lc->tls_key, "");
	}
	return sai->certificates && sai->key;
}

static bool_t fill_auth_info(LinphoneCore *lc, SalAuthInfo* sai) {
	LinphoneAuthInfo *ai = NULL;
	if (sai->mode == SalAuthModeTls) {
		ai = (LinphoneAuthInfo*)_linphone_core_find_tls_auth_info(lc);
	} else {
		ai = (LinphoneAuthInfo*)_linphone_core_find_auth_info(lc,sai->realm,sai->username,sai->domain, FALSE);
	}
	if (ai) {
		if (sai->mode == SalAuthModeHttpDigest) {
			sai->userid = ms_strdup(ai->userid ? ai->userid : ai->username);
			sai->password = ai->passwd?ms_strdup(ai->passwd) : NULL;
			sai->ha1 = ai->ha1 ? ms_strdup(ai->ha1) : NULL;
		} else if (sai->mode == SalAuthModeTls) {
			if (ai->tls_cert && ai->tls_key) {
				sal_certificates_chain_parse(sai, ai->tls_cert, SAL_CERTIFICATE_RAW_FORMAT_PEM);
				sal_signing_key_parse(sai, ai->tls_key, "");
			} else if (ai->tls_cert_path && ai->tls_key_path) {
				sal_certificates_chain_parse_file(sai, ai->tls_cert_path, SAL_CERTIFICATE_RAW_FORMAT_PEM);
				sal_signing_key_parse_file(sai, ai->tls_key_path, "");
			} else {
				fill_auth_info_with_client_certificate(lc, sai);
			}
		}
		
		if (sai->realm && !ai->realm){
			/*if realm was not known, then set it so that ha1 may eventually be calculated and clear text password dropped*/
			linphone_auth_info_set_realm(ai, sai->realm);
			linphone_core_write_auth_info(lc, ai);
		}
		return TRUE;
	} else {
		if (sai->mode == SalAuthModeTls) {
			return fill_auth_info_with_client_certificate(lc, sai);
		}
		return FALSE;
	}
}
static bool_t auth_requested(Sal* sal, SalAuthInfo* sai) {
	LinphoneCore *lc = (LinphoneCore *)sal_get_user_pointer(sal);
	if (fill_auth_info(lc,sai)) {
		return TRUE;
	} else {
		LinphoneAuthMethod method = sai->mode == SalAuthModeHttpDigest ? LinphoneAuthHttpDigest : LinphoneAuthTls;
		LinphoneAuthInfo *ai = linphone_core_create_auth_info(lc, sai->username, NULL, NULL, NULL, sai->realm, sai->domain);
		linphone_core_notify_authentication_requested(lc, ai, method);
		linphone_auth_info_destroy(ai);
		// Deprecated
		linphone_core_notify_auth_info_requested(lc, sai->realm, sai->username, sai->domain);
		if (fill_auth_info(lc, sai)) {
			return TRUE;
		}
		return FALSE;
	}
}

static void notify_refer(SalOp *op, SalReferStatus status){
	LinphoneCore *lc=(LinphoneCore *)sal_get_user_pointer(sal_op_get_sal(op));
	LinphoneCall *call=(LinphoneCall*) sal_op_get_user_pointer(op);
	LinphoneCallState cstate;
	if (call==NULL) {
		ms_warning("Receiving notify_refer for unknown call.");
		return ;
	}
	switch(status){
		case SalReferTrying:
			cstate=LinphoneCallOutgoingProgress;
		break;
		case SalReferSuccess:
			cstate=LinphoneCallConnected;
		break;
		case SalReferFailed:
			cstate=LinphoneCallError;
		break;
		default:
			cstate=LinphoneCallError;
	}
	linphone_call_set_transfer_state(call, cstate);
	if (cstate==LinphoneCallConnected){
		/*automatically terminate the call as the transfer is complete.*/
		linphone_core_terminate_call(lc,call);
	}
}

static LinphoneChatMessageState chatStatusSal2Linphone(SalTextDeliveryStatus status){
	switch(status){
		case SalTextDeliveryInProgress:
			return LinphoneChatMessageStateInProgress;
		case SalTextDeliveryDone:
			return LinphoneChatMessageStateDelivered;
		case SalTextDeliveryFailed:
			return LinphoneChatMessageStateNotDelivered;
	}
	return LinphoneChatMessageStateIdle;
}

static void text_delivery_update(SalOp *op, SalTextDeliveryStatus status){
	LinphoneChatMessage *chat_msg=(LinphoneChatMessage* )sal_op_get_user_pointer(op);

	if (chat_msg == NULL) {
		// Do not handle delivery status for isComposing messages.
		return;
	}
	// check that the message does not belong to an already destroyed chat room - if so, do not invoke callbacks
	if (chat_msg->chat_room != NULL) {
		linphone_chat_message_update_state(chat_msg, chatStatusSal2Linphone(status));
	}
	if (status != SalTextDeliveryInProgress) { /*only release op if not in progress*/
		linphone_chat_message_destroy(chat_msg);
	}
}

static void info_received(SalOp *op, SalBodyHandler *body_handler){
	LinphoneCore *lc=(LinphoneCore *)sal_get_user_pointer(sal_op_get_sal(op));
	linphone_core_notify_info_message(lc,op,body_handler);
}

static void subscribe_response(SalOp *op, SalSubscribeStatus status, int will_retry){
	LinphoneEvent *lev=(LinphoneEvent*)sal_op_get_user_pointer(op);

	if (lev==NULL) return;

	if (status==SalSubscribeActive){
		linphone_event_set_state(lev,LinphoneSubscriptionActive);
	}else if (status==SalSubscribePending){
		linphone_event_set_state(lev,LinphoneSubscriptionPending);
	}else{
		if (will_retry){
			linphone_event_set_state(lev,LinphoneSubscriptionOutgoingProgress);
		}
		else linphone_event_set_state(lev,LinphoneSubscriptionError);
	}
}

static void notify(SalOp *op, SalSubscribeStatus st, const char *eventname, SalBodyHandler *body_handler){
	LinphoneEvent *lev=(LinphoneEvent*)sal_op_get_user_pointer(op);
	LinphoneCore *lc=(LinphoneCore *)sal_get_user_pointer(sal_op_get_sal(op));
	bool_t out_of_dialog = (lev==NULL);
	if (out_of_dialog) {
		/*out of dialog notify */
		lev = linphone_event_new_with_out_of_dialog_op(lc,op,LinphoneSubscriptionOutgoing,eventname);
	}
	{
		LinphoneContent *ct=linphone_content_from_sal_body_handler(body_handler);
		if (ct) {
			linphone_core_notify_notify_received(lc,lev,eventname,ct);
			linphone_content_unref(ct);
		}
	}
	if (out_of_dialog){
		/*out of dialog NOTIFY do not create an implicit subscription*/
		linphone_event_set_state(lev, LinphoneSubscriptionTerminated);
	}else if (st!=SalSubscribeNone){ 
		linphone_event_set_state(lev,linphone_subscription_state_from_sal(st));
	}
}

static void subscribe_received(SalOp *op, const char *eventname, const SalBodyHandler *body_handler){
	LinphoneEvent *lev=(LinphoneEvent*)sal_op_get_user_pointer(op);
	LinphoneCore *lc=(LinphoneCore *)sal_get_user_pointer(sal_op_get_sal(op));

	if (lev==NULL) {
		lev=linphone_event_new_with_op(lc,op,LinphoneSubscriptionIncoming,eventname);
		linphone_event_set_state(lev,LinphoneSubscriptionIncomingReceived);
	}else{
		/*subscribe refresh, unhandled*/
	}

}

static void incoming_subscribe_closed(SalOp *op){
	LinphoneEvent *lev=(LinphoneEvent*)sal_op_get_user_pointer(op);

	linphone_event_set_state(lev,LinphoneSubscriptionTerminated);
}

static void on_publish_response(SalOp* op){
	LinphoneEvent *lev=(LinphoneEvent*)sal_op_get_user_pointer(op);
	const SalErrorInfo *ei=sal_op_get_error_info(op);

	if (lev==NULL) return;
	if (ei->reason==SalReasonNone){
		if (!lev->terminating)
			linphone_event_set_publish_state(lev,LinphonePublishOk);
		else
			linphone_event_set_publish_state(lev,LinphonePublishCleared);
	}else{
		if (lev->publish_state==LinphonePublishOk){
			linphone_event_set_publish_state(lev,LinphonePublishProgress);
		}else{
			linphone_event_set_publish_state(lev,LinphonePublishError);
		}
	}
}


static void on_expire(SalOp *op){
	LinphoneEvent *lev=(LinphoneEvent*)sal_op_get_user_pointer(op);

	if (lev==NULL) return;

	if (linphone_event_get_publish_state(lev)==LinphonePublishOk){
		linphone_event_set_publish_state(lev,LinphonePublishExpiring);
	}else if (linphone_event_get_subscription_state(lev)==LinphoneSubscriptionActive){
		linphone_event_set_state(lev,LinphoneSubscriptionExpiring);
	}
}

static void on_notify_response(SalOp *op){
	LinphoneEvent *lev=(LinphoneEvent*)sal_op_get_user_pointer(op);

	if (lev==NULL) return;
	/*this is actually handling out of dialogs notify - for the moment*/
	if (!lev->is_out_of_dialog_op) return;
	switch (linphone_event_get_subscription_state(lev)){
		case LinphoneSubscriptionIncomingReceived:
			if (sal_op_get_error_info(op)->reason == SalReasonNone){
				linphone_event_set_state(lev, LinphoneSubscriptionTerminated);
			}else{
				linphone_event_set_state(lev, LinphoneSubscriptionError);
			}
		break;
		default:
			ms_warning("Unhandled on_notify_response() case %s", linphone_subscription_state_to_string(linphone_event_get_subscription_state(lev)));
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
	call_cancel_done,
	auth_failure,
	register_success,
	register_failure,
	vfu_request,
	dtmf_received,
	refer_received,
	text_received,
	text_delivery_update,
	is_composing_received,
	imdn_received,
	notify_refer,
	subscribe_received,
	incoming_subscribe_closed,
	subscribe_response,
	notify,
	subscribe_presence_received,
	subscribe_presence_closed,
	parse_presence_requested,
	convert_presence_to_xml_requested,
	notify_presence,
	ping_reply,
	auth_requested,
	info_received,
	on_publish_response,
	on_expire,
	on_notify_response
};


