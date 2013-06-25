/***************************************************************************
 *            conference.c
 *
 *  Mon Sep 12, 2011
 *  Copyright  2011  Belledonne Communications
 *  Author: Simon Morlat
 *  Email simon dot morlat at linphone dot org
 ****************************************************************************/

/*
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */
 
#include "private.h"
#include "lpconfig.h"

#include "mediastreamer2/msvolume.h"

/**
 * @addtogroup conferencing
 * @{
**/


static int convert_conference_to_call(LinphoneCore *lc);

static void conference_check_init(LinphoneConference *ctx, int samplerate){
	if (ctx->conf==NULL){
		MSAudioConferenceParams params;
		params.samplerate=samplerate;
		ctx->conf=ms_audio_conference_new(&params);
	}
}

static void remove_local_endpoint(LinphoneConference *ctx){
	if (ctx->local_endpoint){
		ms_audio_conference_remove_member(ctx->conf,ctx->local_endpoint);
		ms_audio_endpoint_release_from_stream(ctx->local_endpoint);
		ctx->local_endpoint=NULL;
		audio_stream_stop(ctx->local_participant);
		ctx->local_participant=NULL;
		rtp_profile_destroy(ctx->local_dummy_profile);
	}
}

static int linphone_conference_get_size(LinphoneConference *conf){
	if (conf->conf == NULL) {
		return 0;
	}
	return ms_audio_conference_get_size(conf->conf) - (conf->record_endpoint ? 1 : 0);
}

static int remote_participants_count(LinphoneConference *ctx) {
	int count=linphone_conference_get_size(ctx);
	if (count==0) return 0;
	if (!ctx->local_participant) return count;
	return count -1;
}

void linphone_core_conference_check_uninit(LinphoneCore *lc){
	LinphoneConference *ctx=&lc->conf_ctx;
	if (ctx->conf){
		int remote_count=remote_participants_count(ctx);
		ms_message("conference_check_uninit(): size=%i",linphone_conference_get_size(ctx));
		if (remote_count==1){
			convert_conference_to_call(lc);
		}
		if (remote_count==0){
			if (ctx->local_participant!=NULL)
				remove_local_endpoint(ctx);
			if (ctx->record_endpoint){
				ms_audio_conference_remove_member(ctx->conf,ctx->record_endpoint);
				ms_audio_endpoint_destroy(ctx->record_endpoint);
				ctx->record_endpoint=NULL;
			}
		}
		
		if (ms_audio_conference_get_size(ctx->conf)==0){
			ms_audio_conference_destroy(ctx->conf);
			ctx->conf=NULL;
		}
	}
}

void linphone_call_add_to_conf(LinphoneCall *call, bool_t muted){
	LinphoneCore *lc=call->core;
	LinphoneConference *conf=&lc->conf_ctx;
	MSAudioEndpoint *ep;
	call->params.has_video = FALSE;
	call->camera_active = FALSE;
	ep=ms_audio_endpoint_get_from_stream(call->audiostream,TRUE);
	ms_audio_conference_add_member(conf->conf,ep);
	ms_audio_conference_mute_member(conf->conf,ep,muted);
	call->endpoint=ep;
}

void linphone_call_remove_from_conf(LinphoneCall *call){
	LinphoneCore *lc=call->core;
	LinphoneConference *conf=&lc->conf_ctx;
	
	ms_audio_conference_remove_member(conf->conf,call->endpoint);
	ms_audio_endpoint_release_from_stream(call->endpoint);
	call->endpoint=NULL;
}

static RtpProfile *make_dummy_profile(int samplerate){
	RtpProfile *prof=rtp_profile_new("dummy");
	PayloadType *pt=payload_type_clone(&payload_type_l16_mono);
	pt->clock_rate=samplerate;
	rtp_profile_set_payload(prof,0,pt);
	return prof;
}

static void add_local_endpoint(LinphoneConference *conf,LinphoneCore *lc){
	/*create a dummy audiostream in order to extract the local part of it */
	/* network address and ports have no meaning and are not used here. */
	AudioStream *st=audio_stream_new(65000,65001,FALSE);
	MSSndCard *playcard=lc->sound_conf.lsd_card ? 
			lc->sound_conf.lsd_card : lc->sound_conf.play_sndcard;
	MSSndCard *captcard=lc->sound_conf.capt_sndcard;
	const MSAudioConferenceParams *params=ms_audio_conference_get_params(conf->conf);
	conf->local_dummy_profile=make_dummy_profile(params->samplerate);
	
	audio_stream_start_full(st, conf->local_dummy_profile,
				"127.0.0.1",
				65000,
				"127.0.0.1",
				65001,
				0,
				40,
				NULL,
				NULL,
				playcard,
				captcard,
				linphone_core_echo_cancellation_enabled(lc)
				);
	_post_configure_audio_stream(st,lc,FALSE);
	conf->local_participant=st;
	conf->local_endpoint=ms_audio_endpoint_get_from_stream(st,FALSE);
	ms_audio_conference_add_member(conf->conf,conf->local_endpoint);
	
}

/**
 * Returns the sound volume (mic input) of the local participant of the conference.
 * @param lc the linphone core
 * @returns the measured input volume expressed in dbm0.
 **/
float linphone_core_get_conference_local_input_volume(LinphoneCore *lc){
	LinphoneConference *conf=&lc->conf_ctx;
	AudioStream *st=conf->local_participant;
	if (st && st->volsend && !conf->local_muted){
		float vol=0;
		ms_filter_call_method(st->volsend,MS_VOLUME_GET,&vol);
		return vol;
		
	}
	return LINPHONE_VOLUME_DB_LOWEST;
}

/**
 * Merge a call into a conference.
 * @param lc the linphone core
 * @param call an established call, either in LinphoneCallStreamsRunning or LinphoneCallPaused state.
 * 
 * If this is the first call that enters the conference, the virtual conference will be created automatically.
 * If the local user was actively part of the call (ie not in paused state), then the local user is automatically entered into the conference.
 * If the call was in paused state, then it is automatically resumed when entering into the conference.
 * 
 * @returns 0 if successful, -1 otherwise.
**/
int linphone_core_add_to_conference(LinphoneCore *lc, LinphoneCall *call){
	LinphoneConference *conf=&lc->conf_ctx;
	
	if (call->current_params.in_conference){
		ms_error("Already in conference");
		return -1;
	}
	conference_check_init(&lc->conf_ctx, lp_config_get_int(lc->config, "sound","conference_rate",16000));
	
	if (call->state==LinphoneCallPaused){
		call->params.in_conference=TRUE;
		call->params.has_video=FALSE;
		linphone_core_resume_call(lc,call);
	}else if (call->state==LinphoneCallStreamsRunning){
		LinphoneCallParams *params=linphone_call_params_copy(linphone_call_get_current_params(call));
		params->in_conference=TRUE;
		params->has_video=FALSE;
		
		if (call->audiostream || call->videostream){
			linphone_call_stop_media_streams (call); /*free the audio & video local resources*/
		}
		if (call==lc->current_call){
			lc->current_call=NULL;
		}
		/*this will trigger a reINVITE that will later redraw the streams */
		linphone_core_update_call(lc,call,params);
		linphone_call_params_destroy(params);
		add_local_endpoint(conf,lc);
	}else{
		ms_error("Call is in state %s, it cannot be added to the conference.",linphone_call_state_to_string(call->state));
		return -1;
	}
	return 0;
}

static int remove_from_conference(LinphoneCore *lc, LinphoneCall *call, bool_t active){
	int err=0;

	if (!call->current_params.in_conference){
		if (call->params.in_conference){
			ms_warning("Not (yet) in conference, be patient");
			return -1;
		}else{
			ms_error("Not in a conference.");
			return -1;
		}
	}
	call->params.in_conference=FALSE;

	char *str=linphone_call_get_remote_address_as_string(call);
	ms_message("%s will be removed from conference", str);
	ms_free(str);
	if (active){
		LinphoneCallParams *params=linphone_call_params_copy(linphone_call_get_current_params(call));
		params->in_conference=FALSE;
		// reconnect local audio with this call
		if (linphone_core_is_in_conference(lc)){
			ms_message("Leaving conference for reconnecting with unique call.");
			linphone_core_leave_conference(lc);
		}
		ms_message("Updating call to actually remove from conference");
		err=linphone_core_update_call(lc,call,params);
		linphone_call_params_destroy(params);
	} else{
		ms_message("Pausing call to actually remove from conference");
		err=_linphone_core_pause_call(lc,call);
	}

	return err;
}

static int convert_conference_to_call(LinphoneCore *lc){
	int err=0;
	MSList *calls=lc->calls;

	if (remote_participants_count(&lc->conf_ctx)!=1){
		ms_error("No unique call remaining in conference.");
		return -1;
	}

	while (calls) {
		LinphoneCall *rc=(LinphoneCall*)calls->data;
		calls=calls->next;
		if (rc->params.in_conference) { // not using current_param
			bool_t active_after_removed=linphone_core_is_in_conference(lc);
			err=remove_from_conference(lc, rc, active_after_removed);
			break;
		}
	}
	return err;
}

/**
 * Remove a call from the conference.
 * @param lc the linphone core
 * @param call a call that has been previously merged into the conference.
 * 
 * After removing the remote participant belonging to the supplied call, the call becomes a normal call in paused state.
 * If one single remote participant is left alone together with the local user in the conference after the removal, then the conference is
 * automatically transformed into a simple call in StreamsRunning state.
 * The conference's resources are then automatically destroyed.
 * 
 * In other words, unless linphone_core_leave_conference() is explicitely called, the last remote participant of a conference is automatically
 * put in a simple call in running state.
 * 
 * @returns 0 if successful, -1 otherwise.
 **/
int linphone_core_remove_from_conference(LinphoneCore *lc, LinphoneCall *call){
	char * str=linphone_call_get_remote_address_as_string(call);
	ms_message("Removing call %s from the conference", str);
	ms_free(str);
	int err=remove_from_conference(lc,call, FALSE);
	if (err){
		ms_error("Error removing participant from conference.");
		return err;
	}

	if (remote_participants_count(&lc->conf_ctx)==1){
		ms_message("conference size is 1: need to be converted to plain call");
		err=convert_conference_to_call(lc);
	} else {
		ms_message("the conference need not to be converted as size is %i", remote_participants_count(&lc->conf_ctx));
	}
	return err;
}

/**
 * Indicates whether the local participant is part of the conference.
 * @param lc the linphone core
 * @returns TRUE if the local participant is in the conference, FALSE otherwise.
**/
bool_t linphone_core_is_in_conference(const LinphoneCore *lc){
	return lc->conf_ctx.local_participant!=NULL;
}

/**
 * Moves the local participant out of the conference.
 * @param lc the linphone core
 * When the local participant is out of the conference, the remote participants can continue to talk normally.
 * @returns 0 if successful, -1 otherwise.
**/
int linphone_core_leave_conference(LinphoneCore *lc){
	LinphoneConference *conf=&lc->conf_ctx;
	if (linphone_core_is_in_conference(lc))
		remove_local_endpoint(conf);
	return 0;
}

/**
 * Moves the local participant inside the conference.
 * @param lc the linphone core
 * 
 * Makes the local participant to join the conference. 
 * Typically, the local participant is by default always part of the conference when joining an active call into a conference.
 * However, by calling linphone_core_leave_conference() and linphone_core_enter_conference() the application can decide to temporarily
 * move out and in the local participant from the conference.
 * 
 * @returns 0 if successful, -1 otherwise
**/
int linphone_core_enter_conference(LinphoneCore *lc){
	if (linphone_core_sound_resources_locked(lc)) {
		return -1;
	}
	if (lc->current_call != NULL) {
		_linphone_core_pause_call(lc, lc->current_call);
	}
	LinphoneConference *conf=&lc->conf_ctx;
	if (conf->local_participant==NULL) add_local_endpoint(conf,lc);
	return 0;
}

/**
 * Add all calls into a conference.
 * @param lc the linphone core
 * 
 * Merge all established calls (either in LinphoneCallStreamsRunning or LinphoneCallPaused) into a conference.
 * 
 * @returns 0 if successful, -1 otherwise
**/
int linphone_core_add_all_to_conference(LinphoneCore *lc) {
	MSList *calls=lc->calls;
	while (calls) {
		LinphoneCall *call=(LinphoneCall*)calls->data;
		calls=calls->next;
		if (!call->current_params.in_conference) {
			linphone_core_add_to_conference(lc, call);
		}
	}
	linphone_core_enter_conference(lc);
	return 0;
}

/**
 * Terminates the conference and the calls associated with it.
 * @param lc the linphone core
 * 
 * All the calls that were merged to the conference are terminated, and the conference resources are destroyed.
 * 
 * @returns 0 if successful, -1 otherwise
**/
int linphone_core_terminate_conference(LinphoneCore *lc) {
	MSList *calls=lc->calls;
	while (calls) {
		LinphoneCall *call=(LinphoneCall*)calls->data;
		calls=calls->next;
		if (call->current_params.in_conference) {
			linphone_core_terminate_call(lc, call);
		}
	}
	return 0;
}

/**
 * Returns the number of participants to the conference, including the local participant.
 * @param lc the linphone core
 * 
 * Typically, after merging two calls into the conference, there is total of 3 participants:
 * the local participant (or local user), and two remote participants that were the destinations of the two previously establised calls.
 * 
 * @returns the number of participants to the conference
**/
int linphone_core_get_conference_size(LinphoneCore *lc) {
	LinphoneConference *conf=&lc->conf_ctx;
	return linphone_conference_get_size(conf);
}


int linphone_core_start_conference_recording(LinphoneCore *lc, const char *path){
	LinphoneConference *conf=&lc->conf_ctx;
	if (conf->conf == NULL) {
		ms_warning("linphone_core_start_conference_recording(): no conference now.");
		return -1;
	}
	if (conf->record_endpoint==NULL){
		conf->record_endpoint=ms_audio_endpoint_new_recorder();
		ms_audio_conference_add_member(conf->conf,conf->record_endpoint);
	}
	ms_audio_recorder_endpoint_start(conf->record_endpoint,path);
	return 0;
}

int linphone_core_stop_conference_recording(LinphoneCore *lc){
	LinphoneConference *conf=&lc->conf_ctx;
	if (conf->conf == NULL) {
		ms_warning("linphone_core_stop_conference_recording(): no conference now.");
		return -1;
	}
	if (conf->record_endpoint==NULL){
		ms_warning("linphone_core_stop_conference_recording(): no record active.");
		return -1;
	}
	ms_audio_recorder_endpoint_stop(conf->record_endpoint);
	return 0;
}

/**
 * @}
**/

