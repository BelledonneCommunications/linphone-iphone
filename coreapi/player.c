
/*
linphone
Copyright (C) 2014 Belledonne Communications SARL

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

#include "private.h"

BELLE_SIP_DECLARE_NO_IMPLEMENTED_INTERFACES(LinphonePlayer);

BELLE_SIP_INSTANCIATE_VPTR(LinphonePlayer, belle_sip_object_t,
	_linphone_player_destroy, // destroy
	NULL, // clone
	NULL, // marshal
	FALSE
);

LinphonePlayer * linphone_player_new(void) {
	LinphonePlayer *player = belle_sip_object_new(LinphonePlayer);
	player->callbacks = linphone_player_cbs_new();
	return player;
}

LinphonePlayer * linphone_player_ref(LinphonePlayer *player) {
	belle_sip_object_ref(player);
	return player;
}

void linphone_player_unref(LinphonePlayer *player) {
	belle_sip_object_unref(player);
}

void *linphone_player_get_user_data(const LinphonePlayer *player) {
	return player->user_data;
}

void linphone_player_set_user_data(LinphonePlayer *player, void *ud) {
	player->user_data = ud;
}

LinphonePlayerCbs * linphone_player_get_callbacks(const LinphonePlayer *player) {
	return player->callbacks;
}

LinphoneStatus linphone_player_open(LinphonePlayer *obj, const char *filename){
	return obj->open(obj,filename);
}

LinphoneStatus linphone_player_start(LinphonePlayer *obj){
	return obj->start(obj);
}

LinphoneStatus linphone_player_pause(LinphonePlayer *obj){
	return obj->pause(obj);
}

LinphoneStatus linphone_player_seek(LinphonePlayer *obj, int time_ms){
	return obj->seek(obj,time_ms);
}

LinphonePlayerState linphone_player_get_state(LinphonePlayer *obj){
	switch (obj->get_state(obj)) {
		case MSPlayerClosed:
		default:
			return LinphonePlayerClosed;
		case MSPlayerPaused:
			return LinphonePlayerPaused;
		case MSPlayerPlaying:
			return LinphonePlayerPlaying;
	}
}

int linphone_player_get_duration(LinphonePlayer *obj) {
	return obj->get_duration(obj);
}

int linphone_player_get_current_position(LinphonePlayer *obj) {
	return obj->get_position(obj);
}

void linphone_player_close(LinphonePlayer *obj){
	obj->close(obj);
}

void linphone_player_destroy(LinphonePlayer *obj) {
	if(obj->destroy) obj->destroy(obj);
}

void _linphone_player_destroy(LinphonePlayer *player) {
	linphone_player_cbs_unref(player->callbacks);
}


/*
 * Call player implementation below.
 */


static bool_t call_player_check_state(LinphonePlayer *player, bool_t check_player){
	LinphoneCall *call=(LinphoneCall*)player->impl;
	if (call->state!=LinphoneCallStreamsRunning){
		ms_warning("Call [%p]: in-call player not usable in state [%s]",call,linphone_call_state_to_string(call->state));
		return FALSE;
	}
	if (call->audiostream==NULL) {
		ms_error("call_player_check_state(): no audiostream.");
		return FALSE;
	}
	if (check_player && call->audiostream->av_player.player==NULL){
		ms_error("call_player_check_state(): no player.");
		return FALSE;
	}
	return TRUE;
}

static void on_eof(void *user_data, MSFilter *f, unsigned int event_id, void *arg){
	LinphonePlayer *player=(LinphonePlayer *)user_data;
	switch (event_id){
		case MS_PLAYER_EOF:
			LinphonePlayerCbs *cbs = linphone_player_get_callbacks(player);
			LinphonePlayerCbsEofReachedCb cb = linphone_player_cbs_get_eof_reached(cbs);
			if (cb) cb(player);
		break;
	}
}

static int call_player_open(LinphonePlayer* player, const char *filename){
	LinphoneCall *call=(LinphoneCall*)player->impl;
	MSFilter *filter;
	if (!call_player_check_state(player,FALSE)) return -1;
	filter=audio_stream_open_remote_play(call->audiostream,filename);
	if (!filter) return -1;
	ms_filter_add_notify_callback(filter,&on_eof,player,FALSE);
	return 0;
}

static int call_player_start(LinphonePlayer *player){
	LinphoneCall *call=(LinphoneCall*)player->impl;
	if (!call_player_check_state(player,TRUE)) return -1;
	return ms_filter_call_method_noarg(call->audiostream->av_player.player,MS_PLAYER_START);
}

static int call_player_pause(LinphonePlayer *player){
	LinphoneCall *call=(LinphoneCall*)player->impl;
	if (!call_player_check_state(player,TRUE)) return -1;
	return ms_filter_call_method_noarg(call->audiostream->av_player.player,MS_PLAYER_PAUSE);
}

static MSPlayerState call_player_get_state(LinphonePlayer *player){
	LinphoneCall *call=(LinphoneCall*)player->impl;
	MSPlayerState state=MSPlayerClosed;
	if (!call_player_check_state(player,TRUE)) return MSPlayerClosed;
	ms_filter_call_method(call->audiostream->av_player.player,MS_PLAYER_GET_STATE,&state);
	return state;
}

static int call_player_seek(LinphonePlayer *player, int time_ms){
	LinphoneCall *call=(LinphoneCall*)player->impl;
	if (!call_player_check_state(player,TRUE)) return -1;
	return ms_filter_call_method(call->audiostream->av_player.player,MS_PLAYER_SEEK_MS,&time_ms);
}

static void call_player_close(LinphonePlayer *player){
	LinphoneCall *call=(LinphoneCall*)player->impl;
	if (!call_player_check_state(player,TRUE)) return;
	audio_stream_close_remote_play(call->audiostream);
	
}

static void on_call_destroy(void *obj, belle_sip_object_t *call_being_destroyed){
	linphone_player_unref(reinterpret_cast<LinphonePlayer *>(obj));
}

LinphonePlayer *linphone_call_build_player(LinphoneCall *call){
	LinphonePlayer *obj = linphone_player_new();
	obj->open=call_player_open;
	obj->close=call_player_close;
	obj->start=call_player_start;
	obj->seek=call_player_seek;
	obj->pause=call_player_pause;
	obj->get_state=call_player_get_state;
	obj->impl=call;
	belle_sip_object_weak_ref(call,on_call_destroy,obj);
	return obj;
}


BELLE_SIP_DECLARE_NO_IMPLEMENTED_INTERFACES(LinphonePlayerCbs);

BELLE_SIP_INSTANCIATE_VPTR(LinphonePlayerCbs, belle_sip_object_t,
	NULL, // destroy
	NULL, // clone
	NULL, // marshal
	FALSE);

LinphonePlayerCbs *linphone_player_cbs_new(void) {
	return belle_sip_object_new(LinphonePlayerCbs);
}

LinphonePlayerCbs * linphone_player_cbs_ref(LinphonePlayerCbs *cbs) {
	belle_sip_object_ref(cbs);
	return cbs;
}

void linphone_player_cbs_unref(LinphonePlayerCbs *cbs) {
	belle_sip_object_unref(cbs);
}

void *linphone_player_cbs_get_user_data(const LinphonePlayerCbs *cbs) {
	return cbs->user_data;
}

void linphone_player_cbs_set_user_data(LinphonePlayerCbs *cbs, void *ud) {
	cbs->user_data = ud;
}

LinphonePlayerCbsEofReachedCb linphone_player_cbs_get_eof_reached(const LinphonePlayerCbs *cbs) {
	return cbs->eof;
}

void linphone_player_cbs_set_eof_reached(LinphonePlayerCbs *cbs, LinphonePlayerCbsEofReachedCb cb) {
	cbs->eof = cb;
}
