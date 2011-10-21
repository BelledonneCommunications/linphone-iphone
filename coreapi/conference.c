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

#include "mediastreamer2/msvolume.h"

static void conference_check_init(LinphoneConference *ctx){
	if (ctx->conf==NULL){
		ctx->conf=ms_audio_conference_new();
	}
}

static void remove_local_endpoint(LinphoneConference *ctx){
	if (ctx->local_endpoint){
		ms_audio_conference_remove_member(ctx->conf,ctx->local_endpoint);
		ms_audio_endpoint_release_from_stream(ctx->local_endpoint);
		ctx->local_endpoint=NULL;
		audio_stream_stop(ctx->local_participant);
		ctx->local_participant=NULL;
	}
}

void linphone_core_conference_check_uninit(LinphoneConference *ctx){
	if (ctx->conf){
		ms_message("conference_check_uninit(): nmembers=%i",ctx->conf->nmembers);
		if (ctx->conf->nmembers==1 && ctx->local_participant!=NULL){
			remove_local_endpoint(ctx);
		}
		if (ctx->conf->nmembers==0){
			ms_audio_conference_destroy(ctx->conf);
			ctx->conf=NULL;
		}
	}
}


void linphone_call_add_to_conf(LinphoneCall *call){
	LinphoneCore *lc=call->core;
	LinphoneConference *conf=&lc->conf_ctx;
	MSAudioEndpoint *ep;
	call->params.has_video = FALSE;
	call->camera_active = FALSE;
	ep=ms_audio_endpoint_get_from_stream(call->audiostream,TRUE);
	ms_audio_conference_add_member(conf->conf,ep);
	call->endpoint=ep;
}

void linphone_call_remove_from_conf(LinphoneCall *call){
	LinphoneCore *lc=call->core;
	LinphoneConference *conf=&lc->conf_ctx;
	
	ms_audio_conference_remove_member(conf->conf,call->endpoint);
	ms_audio_endpoint_release_from_stream(call->endpoint);
	call->endpoint=NULL;
}

static void add_local_endpoint(LinphoneConference *conf,LinphoneCore *lc){
	/*create a dummy audiostream in order to extract the local part of it */
	/* network address and ports have no meaning and are not used here. */
	AudioStream *st=audio_stream_new(65000,FALSE);
	MSSndCard *playcard=lc->sound_conf.lsd_card ? 
			lc->sound_conf.lsd_card : lc->sound_conf.play_sndcard;
	MSSndCard *captcard=lc->sound_conf.capt_sndcard;
	
	audio_stream_start_full(st, &av_profile,
				"127.0.0.1",
				65000,
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

int linphone_core_add_to_conference(LinphoneCore *lc, LinphoneCall *call){
	LinphoneCallParams params;
	LinphoneConference *conf=&lc->conf_ctx;
	
	if (call->current_params.in_conference){
		ms_error("Already in conference");
		return -1;
	}
	conference_check_init(&lc->conf_ctx);
	call->params.in_conference=TRUE;
	call->params.has_video=FALSE;
	params=call->params;
	if (call->state==LinphoneCallPaused)
		linphone_core_resume_call(lc,call);
	else if (call->state==LinphoneCallStreamsRunning){
		/*this will trigger a reINVITE that will later redraw the streams */
		if (call->audiostream || call->videostream){
			linphone_call_stop_media_streams (call); /*free the audio & video local resources*/
		}
		if (call==lc->current_call){
			lc->current_call=NULL;
		}
		linphone_core_update_call(lc,call,&params);
		add_local_endpoint(conf,lc);
	}else{
		ms_error("Call is in state %s, it cannot be added to the conference.",linphone_call_state_to_string(call->state));
		return -1;
	}
	return 0;
}

int linphone_core_remove_from_conference(LinphoneCore *lc, LinphoneCall *call){
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
	err=linphone_core_pause_call(lc,call);
	return err;
}

bool_t linphone_core_is_in_conference(const LinphoneCore *lc){
	return lc->conf_ctx.local_participant!=NULL;
}

int linphone_core_leave_conference(LinphoneCore *lc){
	LinphoneConference *conf=&lc->conf_ctx;
	if (linphone_core_is_in_conference(lc))
		remove_local_endpoint(conf);
	return 0;
}


int linphone_core_enter_conference(LinphoneCore *lc){
	LinphoneConference *conf=&lc->conf_ctx;
	if (conf->local_participant==NULL) add_local_endpoint(conf,lc);
	return 0;
}

int linphone_core_add_all_to_conference(LinphoneCore *lc) {
	MSList *calls=lc->calls;
	while (calls) {
		LinphoneCall *call=(LinphoneCall*)calls->data;
		calls=calls->next;
		if (!call->current_params.in_conference) {
			linphone_core_add_to_conference(lc, call);
		}
	}
	return 0;
}

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

int linphone_core_get_conference_size(LinphoneCore *lc) {
	return ms_audio_conference_size(lc->conf_ctx.conf);
}
