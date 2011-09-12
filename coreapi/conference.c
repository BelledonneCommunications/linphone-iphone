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


static void conference_check_init(LinphoneConference *ctx){
	if (ctx->conf==NULL){
		ctx->conf=ms_audio_conference_new();
	}
}

static void conference_check_uninit(LinphoneConference *ctx){
	if (ctx->conf){
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
	conference_check_uninit(conf);
}

int linphone_core_add_to_conference(LinphoneCore *lc, LinphoneCall *call){
	LinphoneCallParams params;
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
		linphone_core_update_call(lc,call,&params);
	}else{
		ms_error("Call is in state %s, it cannot be added to the conference.",linphone_call_state_to_string(call->state));
		return -1;
	}
	return 0;
}

int linphone_core_remove_from_conference(LinphoneCore *lc, LinphoneCall *call){
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
	return linphone_core_pause_call(lc,call);
}

int linphone_core_pause_conference(LinphoneCore *lc){
	return 0;
}


int linphone_core_resume_conference(LinphoneCore *lc){
	return 0;
}

