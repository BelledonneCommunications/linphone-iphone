/*
 * c-call.cpp
 * Copyright (C) 2010-2017 Belledonne Communications SARL
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 */

#include "linphone/api/c-call.h"
#include "linphone/api/c-call-cbs.h"
#include "linphone/api/c-call-stats.h"
#include "linphone/wrapper_utils.h"

#include "c-wrapper/c-wrapper.h"
#include "call/call-p.h"
#include "call/call.h"
#include "call/local-conference-call.h"
#include "call/remote-conference-call.h"
#include "conference/params/media-session-params-p.h"
#include "core/core.h"

// =============================================================================

using namespace std;

static void _linphone_call_constructor (LinphoneCall *call);
static void _linphone_call_destructor (LinphoneCall *call);

L_DECLARE_C_OBJECT_IMPL_WITH_XTORS(Call,
	_linphone_call_constructor, _linphone_call_destructor,
	bctbx_list_t *callbacks; /* A list of LinphoneCallCbs object */
	LinphoneCallCbs *currentCbs; /* The current LinphoneCallCbs object used to call a callback */
	char *authenticationTokenCache;
	LinphoneCallParams *currentParamsCache;
	LinphoneCallParams *paramsCache;
	LinphoneCallParams *remoteParamsCache;
	LinphoneAddress *remoteAddressCache;
	char *remoteContactCache;
	char *remoteUserAgentCache;
	/* TODO: all the fields need to be removed */
	struct _LinphoneCore *core;
	LinphoneErrorInfo *ei;
	SalMediaDescription *localdesc;
	SalMediaDescription *resultdesc;
	struct _LinphoneCallLog *log;
	LinphonePrivate::SalOp *op;
	LinphonePrivate::SalOp *ping_op;
	LinphoneCallState transfer_state; /*idle if no transfer*/
	struct _AudioStream *audiostream;  /**/
	struct _VideoStream *videostream;
	struct _TextStream *textstream;
	MSAudioEndpoint *endpoint; /*used for conferencing*/
	char *refer_to;
	LinphoneCallParams *params;
	LinphoneCallParams *current_params;
	LinphoneCallParams *remote_params;
	LinphoneCallStats *audio_stats;
	LinphoneCallStats *video_stats;
	LinphoneCallStats *text_stats;
	LinphoneCall *referer; /*when this call is the result of a transfer, referer is set to the original call that caused the transfer*/
	LinphoneCall *transfer_target;/*if this call received a transfer request, then transfer_target points to the new call created to the refer target */
	LinphonePlayer *player;
	char *dtmf_sequence; /*DTMF sequence needed to be sent using #dtmfs_timer*/
	belle_sip_source_t *dtmfs_timer; /*DTMF timer needed to send a DTMF sequence*/
	LinphoneChatRoom *chat_room;
	LinphoneConference *conf_ref; /**> Point on the associated conference if this call is part of a conference. NULL instead. */
	bool_t refer_pending;
	bool_t defer_update;
	bool_t was_automatically_paused;
	bool_t paused_by_app;
	bool_t broken; /*set to TRUE when the call is in broken state due to network disconnection or transport */
	bool_t need_localip_refresh;
	bool_t reinvite_on_cancel_response_requested;
	bool_t non_op_error; /*set when the LinphoneErrorInfo was set at higher level than sal*/
)

static void _linphone_call_constructor (LinphoneCall *call) {
}

static void _linphone_call_destructor (LinphoneCall *call) {
	if (call->currentParamsCache) {
		linphone_call_params_unref(call->currentParamsCache);
		call->currentParamsCache = nullptr;
	}
	if (call->paramsCache) {
		linphone_call_params_unref(call->paramsCache);
		call->paramsCache = nullptr;
	}
	if (call->remoteParamsCache) {
		linphone_call_params_unref(call->remoteParamsCache);
		call->remoteParamsCache = nullptr;
	}
	if (call->remoteAddressCache) {
		linphone_address_unref(call->remoteAddressCache);
		call->remoteAddressCache = nullptr;
	}
	bctbx_list_free_with_data(call->callbacks, (bctbx_list_free_func)linphone_call_cbs_unref);
	if (call->audio_stats) {
		linphone_call_stats_unref(call->audio_stats);
		call->audio_stats = nullptr;
	}
	if (call->video_stats) {
		linphone_call_stats_unref(call->video_stats);
		call->video_stats = nullptr;
	}
	if (call->text_stats) {
		linphone_call_stats_unref(call->text_stats);
		call->text_stats = nullptr;
	}
	if (call->op) {
		call->op->release();
		call->op=nullptr;
	}
	if (call->resultdesc) {
		sal_media_description_unref(call->resultdesc);
		call->resultdesc=nullptr;
	}
	if (call->localdesc) {
		sal_media_description_unref(call->localdesc);
		call->localdesc=nullptr;
	}
	if (call->ping_op) {
		call->ping_op->release();
		call->ping_op=nullptr;
	}
	if (call->refer_to){
		ms_free(call->refer_to);
		call->refer_to=nullptr;
	}
	if (call->referer){
		linphone_call_unref(call->referer);
		call->referer=nullptr;
	}
	if (call->transfer_target){
		linphone_call_unref(call->transfer_target);
		call->transfer_target=nullptr;
	}
	if (call->log) {
		linphone_call_log_unref(call->log);
		call->log=nullptr;
	}
	if (call->dtmfs_timer) {
		linphone_call_cancel_dtmfs(call);
	}
	if (call->params){
		linphone_call_params_unref(call->params);
		call->params=nullptr;
	}
	if (call->current_params){
		linphone_call_params_unref(call->current_params);
		call->current_params=nullptr;
	}
	if (call->remote_params) {
		linphone_call_params_unref(call->remote_params);
		call->remote_params=nullptr;
	}
	if (call->ei) linphone_error_info_unref(call->ei);
}


// =============================================================================
// TODO: To remove!
// =============================================================================

MSWebCam *get_nowebcam_device (MSFactory* f) {
#ifdef VIDEO_ENABLED
	return ms_web_cam_manager_get_cam(ms_factory_get_web_cam_manager(f),"StaticImage: Static picture");
#else
	return nullptr;
#endif
}

void linphone_call_update_local_media_description_from_ice_or_upnp (LinphoneCall *call) {}

void linphone_call_make_local_media_description (LinphoneCall *call) {}

void linphone_call_create_op (LinphoneCall *call) {}

void linphone_call_set_state (LinphoneCall *call, LinphoneCallState cstate, const char *message) {}

void linphone_call_init_media_streams (LinphoneCall *call) {}

#if 0
static int dtmf_tab[16]={'0','1','2','3','4','5','6','7','8','9','*','#','A','B','C','D'};

static void linphone_core_dtmf_received (LinphoneCall *call, int dtmf) {
	if (dtmf<0 || dtmf>15){
		ms_warning("Bad dtmf value %i",dtmf);
		return;
	}
	linphone_call_notify_dtmf_received(call, dtmf_tab[dtmf]);
}
#endif

/*This function is not static because used internally in linphone-daemon project*/
void _post_configure_audio_stream (AudioStream *st, LinphoneCore *lc, bool_t muted) {}

#if 0
static void setup_ring_player (LinphoneCore *lc, LinphoneCall *call) {
	int pause_time=3000;
	audio_stream_play(call->audiostream,lc->sound_conf.ringback_tone);
	ms_filter_call_method(call->audiostream->soundread,MS_FILE_PLAYER_LOOP,&pause_time);
}
#endif

#if 0
static bool_t linphone_call_sound_resources_available (LinphoneCall *call) {
	LinphoneCore *lc=call->core;
	LinphoneCall *current=linphone_core_get_current_call(lc);
	return !linphone_core_is_in_conference(lc) &&
		(current==nullptr || current==call);
}
#endif

void linphone_call_delete_upnp_session (LinphoneCall *call) {}

void linphone_call_stop_media_streams (LinphoneCall *call) {}

#if 0
static void linphone_call_lost (LinphoneCall *call) {
	LinphoneCore *lc = call->core;
	char *temp = nullptr;
	char *from = nullptr;

	from = linphone_call_get_remote_address_as_string(call);
	temp = ms_strdup_printf("Media connectivity with %s is lost, call is going to be closed.", from ? from : "?");
	if (from) ms_free(from);
	ms_message("LinphoneCall [%p]: %s", call, temp);
	call->non_op_error = TRUE;
	linphone_error_info_set(call->ei,nullptr, LinphoneReasonIOError, 503, "Media lost", nullptr);
	linphone_call_terminate(call);
	linphone_core_play_named_tone(lc, LinphoneToneCallLost);
	ms_free(temp);
}
#endif

void linphone_call_set_transfer_state (LinphoneCall *call, LinphoneCallState state) {
#if 0
	if (state != call->transfer_state) {
		ms_message("Transfer state for call [%p] changed  from [%s] to [%s]",call
						,linphone_call_state_to_string(call->transfer_state)
						,linphone_call_state_to_string(state));
		call->transfer_state = state;
		linphone_call_notify_transfer_state_changed(call, state);
	}
#endif
}

void _linphone_call_set_new_params (LinphoneCall *call, const LinphoneCallParams *params) {}

#if 0
static int send_dtmf_handler (void *data, unsigned int revents) {
	LinphoneCall *call = (LinphoneCall*)data;
	/*By default we send DTMF RFC2833 if we do not have enabled SIP_INFO but we can also send RFC2833 and SIP_INFO*/
	if (linphone_core_get_use_rfc2833_for_dtmf(call->core)!=0 || linphone_core_get_use_info_for_dtmf(call->core)==0)
	{
		/* In Band DTMF */
		if (call->audiostream){
			audio_stream_send_dtmf(call->audiostream,*call->dtmf_sequence);
		}
		else
		{
			ms_error("Cannot send RFC2833 DTMF when we are not in communication.");
			return FALSE;
		}
	}
	if (linphone_core_get_use_info_for_dtmf(call->core)!=0){
		/* Out of Band DTMF (use INFO method) */
		sal_call_send_dtmf(call->op,*call->dtmf_sequence);
	}

	/*this check is needed because linphone_call_send_dtmf does not set the timer since its a single character*/
	if (call->dtmfs_timer) {
		memmove(call->dtmf_sequence, call->dtmf_sequence+1, strlen(call->dtmf_sequence));
	}
	/* continue only if the dtmf sequence is not empty*/
	if (call->dtmf_sequence && *call->dtmf_sequence!='\0') {
		return TRUE;
	} else {
		linphone_call_cancel_dtmfs(call);
		return FALSE;
	}
}
#endif

/* Internal version that does not play tone indication*/
int _linphone_call_pause (LinphoneCall *call) {
	return 0;
}

#if 0
static void terminate_call (LinphoneCall *call) {}
#endif

int linphone_call_start_update (LinphoneCall *call) {
	return 0;
}

int linphone_call_start_accept_update (LinphoneCall *call, LinphoneCallState next_state, const char *state_info) {
	return 0;
}

int linphone_call_proceed_with_invite_if_ready (LinphoneCall *call, LinphoneProxyConfig *dest_proxy) {
	return 0;
}

int linphone_call_start_invite (LinphoneCall *call, const LinphoneAddress *destination /* = NULL if to be taken from the call log */) {
	return 0;
}

void linphone_call_replace_op (LinphoneCall *call, LinphonePrivate::SalOp *op) {
#if 0
	SalOp *oldop = call->op;
	LinphoneCallState oldstate = linphone_call_get_state(call);
	call->op = op;
	sal_op_set_user_pointer(call->op, call);
	sal_call_set_local_media_description(call->op, call->localdesc);
	switch (linphone_call_get_state(call)) {
		case LinphoneCallIncomingEarlyMedia:
		case LinphoneCallIncomingReceived:
			sal_call_notify_ringing(call->op, (linphone_call_get_state(call) == LinphoneCallIncomingEarlyMedia) ? TRUE : FALSE);
			break;
		case LinphoneCallConnected:
		case LinphoneCallStreamsRunning:
			sal_call_accept(call->op);
			break;
		default:
			ms_warning("linphone_call_replace_op(): don't know what to do in state [%s]", linphone_call_state_to_string(call->state));
			break;
	}
	switch (oldstate) {
		case LinphoneCallIncomingEarlyMedia:
		case LinphoneCallIncomingReceived:
			sal_op_set_user_pointer(oldop, nullptr); /* To make the call does not get terminated by terminating this op. */
			/* Do not terminate a forked INVITE */
			if (sal_call_get_replaces(op)) {
				sal_call_terminate(oldop);
			} else {
				sal_op_kill_dialog(oldop);
			}
			break;
		case LinphoneCallConnected:
		case LinphoneCallStreamsRunning:
			sal_call_terminate(oldop);
			sal_op_kill_dialog(oldop);
			break;
		default:
			break;
	}
	sal_op_release(oldop);
#endif
}


// =============================================================================
// Private functions.
// =============================================================================

#if 0
static void linphone_call_repair_by_invite_with_replaces (LinphoneCall *call) {
	const char *call_id = sal_op_get_call_id(call->op);
	const char *from_tag = sal_call_get_local_tag(call->op);
	const char *to_tag = sal_call_get_remote_tag(call->op);
	sal_op_kill_dialog(call->op);
	linphone_call_create_op(call);
	sal_call_set_replaces(call->op, call_id, from_tag, to_tag);
	linphone_call_start_invite(call, nullptr);
}
#endif

MediaStream *linphone_call_get_stream (LinphoneCall *call, LinphoneStreamType type) {
	return L_GET_PRIVATE_FROM_C_OBJECT(call)->getMediaStream(type);
}

void linphone_call_set_broken (LinphoneCall *call) {
#if 0
	switch(call->state){
		/*for all the early states, we prefer to drop the call*/
		case LinphoneCallOutgoingInit:
		case LinphoneCallOutgoingProgress:
		case LinphoneCallOutgoingRinging:
		case LinphoneCallOutgoingEarlyMedia:
		case LinphoneCallIncomingReceived:
		case LinphoneCallIncomingEarlyMedia:
			/*during the early states, the SAL layer reports the failure from the dialog or transaction layer,
			 * hence, there is nothing special to do*/
		//break;
		case LinphoneCallStreamsRunning:
		case LinphoneCallUpdating:
		case LinphoneCallPausing:
		case LinphoneCallResuming:
		case LinphoneCallPaused:
		case LinphoneCallPausedByRemote:
		case LinphoneCallUpdatedByRemote:
			/*during these states, the dialog is established. A failure of a transaction is not expected to close it.
			 * Instead we have to repair the dialog by sending a reINVITE*/
			call->broken = TRUE;
			call->need_localip_refresh = TRUE;
		break;
		default:
			ms_error("linphone_call_set_broken() unimplemented case.");
		break;
	}
#endif
}

void linphone_call_reinvite_to_recover_from_connection_loss (LinphoneCall *call) {
#if 0
	LinphoneCallParams *params;
	ms_message("LinphoneCall[%p] is going to be updated (reINVITE) in order to recover from lost connectivity", call);
	if (call->ice_session){
		ice_session_reset(call->ice_session, IR_Controlling);
	}
	params = linphone_core_create_call_params(call->core, call);
	linphone_call_update(call, params);
	linphone_call_params_unref(params);
#endif
}

void linphone_call_repair_if_broken (LinphoneCall *call) {
#if 0
	SalErrorInfo sei;
	if (!call->broken) return;
	if (!call->core->media_network_reachable) return;

	memset(&sei, 0, sizeof(sei));

	/*Make sure that the proxy from which we received this call, or to which we routed this call is registered first*/
	if (call->dest_proxy){
		/*in all other cases, ie no proxy config, or a proxy config for which no registration was requested, we can start the
		 * call repair immediately.*/
		if (linphone_proxy_config_register_enabled(call->dest_proxy)
			&& linphone_proxy_config_get_state(call->dest_proxy) != LinphoneRegistrationOk) return;
	}

	switch (call->state){
		case LinphoneCallUpdating:
		case LinphoneCallPausing:
			if (sal_call_dialog_request_pending(call->op)) {
				/* Need to cancel first re-INVITE as described in section 5.5 of RFC 6141 */
				sal_call_cancel_invite(call->op);
				call->reinvite_on_cancel_response_requested = TRUE;
			}
			break;
		case LinphoneCallStreamsRunning:
		case LinphoneCallPaused:
		case LinphoneCallPausedByRemote:
			if (!sal_call_dialog_request_pending(call->op)) {
				linphone_call_reinvite_to_recover_from_connection_loss(call);
			}
			break;
		case LinphoneCallUpdatedByRemote:
			if (sal_call_dialog_request_pending(call->op)) {
				sal_error_info_set(&sei, SalReasonServiceUnavailable,"SIP", 0, nullptr, nullptr);
				sal_call_decline_with_error_info(call->op, &sei,nullptr);
			}
			linphone_call_reinvite_to_recover_from_connection_loss(call);
			break;
		case LinphoneCallOutgoingInit:
		case LinphoneCallOutgoingProgress:
			sal_call_cancel_invite(call->op);
			call->reinvite_on_cancel_response_requested = TRUE;
			break;
		case LinphoneCallOutgoingEarlyMedia:
		case LinphoneCallOutgoingRinging:
			linphone_call_repair_by_invite_with_replaces(call);
			break;
		case LinphoneCallIncomingEarlyMedia:
		case LinphoneCallIncomingReceived:
			/* Keep the call broken until a forked INVITE is received from the server. */
			break;
		default:
			ms_warning("linphone_call_repair_if_broken(): don't know what to do in state [%s]", linphone_call_state_to_string(call->state));
			call->broken = FALSE;
		break;
	}
	sal_error_info_reset(&sei);
#endif
}

void linphone_call_refresh_sockets (LinphoneCall *call) {
#if 0
	int i;
	for (i=0; i < SAL_MEDIA_DESCRIPTION_MAX_STREAMS; ++i){
		MSMediaStreamSessions *mss = &call->sessions[i];
		if (mss->rtp_session){
			rtp_session_refresh_sockets(mss->rtp_session);
		}
	}
#endif
}

LinphonePrivate::SalCallOp * linphone_call_get_op (const LinphoneCall *call) {
	return L_GET_PRIVATE_FROM_C_OBJECT(call)->getOp();
}

LinphoneProxyConfig * linphone_call_get_dest_proxy (const LinphoneCall *call) {
	return L_GET_PRIVATE_FROM_C_OBJECT(call)->getDestProxy();
}

LinphoneCallLog * linphone_call_get_log (const LinphoneCall *call) {
	return linphone_call_get_call_log(call);
}

IceSession * linphone_call_get_ice_session (const LinphoneCall *call) {
	return L_GET_PRIVATE_FROM_C_OBJECT(call)->getIceSession();
}

bool_t linphone_call_get_audio_muted (const LinphoneCall *call) {
	return L_GET_PRIVATE_FROM_C_OBJECT(call)->getAudioMuted();
}

void linphone_call_set_audio_muted (LinphoneCall *call, bool_t value) {
	L_GET_PRIVATE_FROM_C_OBJECT(call)->setAudioMuted(!!value);
}

bool_t linphone_call_get_all_muted (const LinphoneCall *call) {
	return L_GET_CPP_PTR_FROM_C_OBJECT(call)->getAllMuted();
}

#define NOTIFY_IF_EXIST(cbName, functionName, ...) \
	for (bctbx_list_t *it = call->callbacks; it; it = bctbx_list_next(it)) { \
		call->currentCbs = reinterpret_cast<LinphoneCallCbs *>(bctbx_list_get_data(it)); \
		LinphoneCallCbs ## cbName ## Cb cb = linphone_call_cbs_get_ ## functionName (call->currentCbs); \
		if (cb) \
			cb(__VA_ARGS__); \
	}

void linphone_call_notify_state_changed (LinphoneCall *call, LinphoneCallState cstate, const char *message) {
	NOTIFY_IF_EXIST(StateChanged, state_changed, call, cstate, message)
	linphone_core_notify_call_state_changed(linphone_call_get_core(call), call, cstate, message);
}

void linphone_call_notify_dtmf_received (LinphoneCall *call, int dtmf) {
	NOTIFY_IF_EXIST(DtmfReceived, dtmf_received, call, dtmf)
	linphone_core_notify_dtmf_received(linphone_call_get_core(call), call, dtmf);
}

void linphone_call_notify_encryption_changed (LinphoneCall *call, bool_t on, const char *authentication_token) {
	NOTIFY_IF_EXIST(EncryptionChanged, encryption_changed, call, on, authentication_token)
	linphone_core_notify_call_encryption_changed(linphone_call_get_core(call), call, on, authentication_token);
}

void linphone_call_notify_transfer_state_changed (LinphoneCall *call, LinphoneCallState cstate) {
	NOTIFY_IF_EXIST(TransferStateChanged, transfer_state_changed, call, cstate)
	linphone_core_notify_transfer_state_changed(linphone_call_get_core(call), call, cstate);
}

void linphone_call_notify_stats_updated (LinphoneCall *call, const LinphoneCallStats *stats) {
	NOTIFY_IF_EXIST(StatsUpdated, stats_updated, call, stats)
	linphone_core_notify_call_stats_updated(linphone_call_get_core(call), call, stats);
}

void linphone_call_notify_info_message_received (LinphoneCall *call, const LinphoneInfoMessage *msg) {
	NOTIFY_IF_EXIST(InfoMessageReceived, info_message_received, call, msg)
	linphone_core_notify_info_received(linphone_call_get_core(call), call, msg);
}

void linphone_call_notify_ack_processing (LinphoneCall *call, LinphoneHeaders *msg, bool_t is_received) {
	NOTIFY_IF_EXIST(AckProcessing, ack_processing, call, msg, is_received)
}


// =============================================================================
// Public functions.
// =============================================================================

LinphoneCore *linphone_call_get_core (const LinphoneCall *call) {
	return L_GET_CPP_PTR_FROM_C_OBJECT(call)->getCore()->getCCore();
}

LinphoneCallState linphone_call_get_state (const LinphoneCall *call) {
	return L_GET_CPP_PTR_FROM_C_OBJECT(call)->getState();
}

bool_t linphone_call_asked_to_autoanswer (LinphoneCall *call) {
	//return TRUE if the unique(for the moment) incoming call asked to be autoanswered
	if (call)
		return linphone_call_get_op(call)->autoanswer_asked();
	return FALSE;
}

const LinphoneAddress *linphone_call_get_remote_address (const LinphoneCall *call) {
	L_SET_CPP_PTR_FROM_C_OBJECT(call->remoteAddressCache, &L_GET_CPP_PTR_FROM_C_OBJECT(call)->getRemoteAddress());
	return call->remoteAddressCache;
}

const LinphoneAddress *linphone_call_get_to_address (const LinphoneCall *call){
#if 0
  return (const LinphoneAddress *)sal_op_get_to_address(call->op);
#else
	return nullptr;
#endif
}

const char *linphone_call_get_to_header (const LinphoneCall *call, const char *name) {
#if 0
	return sal_custom_header_find(sal_op_get_recv_custom_header(call->op),name);
#else
	return nullptr;
#endif
}

char *linphone_call_get_remote_address_as_string (const LinphoneCall *call) {
	return ms_strdup(L_GET_CPP_PTR_FROM_C_OBJECT(call)->getRemoteAddressAsString().c_str());
}

const LinphoneAddress *linphone_call_get_diversion_address (const LinphoneCall *call) {
#if 0
	return call->op?(const LinphoneAddress *)sal_op_get_diversion_address(call->op):nullptr;
#else
	return nullptr;
#endif
}

LinphoneCallDir linphone_call_get_dir (const LinphoneCall *call) {
	return linphone_call_log_get_dir(linphone_call_get_log(call));
}

LinphoneCallLog *linphone_call_get_call_log (const LinphoneCall *call) {
	return L_GET_CPP_PTR_FROM_C_OBJECT(call)->getLog();
}

const char *linphone_call_get_refer_to (const LinphoneCall *call) {
#if 0
	return call->refer_to;
#else
	return nullptr;
#endif
}

bool_t linphone_call_has_transfer_pending (const LinphoneCall *call) {
#if 0
	return call->refer_pending;
#else
	return FALSE;
#endif
}

LinphoneCall *linphone_call_get_transferer_call (const LinphoneCall *call) {
#if 0
	return call->referer;
#else
	return nullptr;
#endif
}

LinphoneCall *linphone_call_get_transfer_target_call (const LinphoneCall *call) {
#if 0
	return call->transfer_target;
#else
	return nullptr;
#endif
}

LinphoneCall *linphone_call_get_replaced_call (LinphoneCall *call) {
#if 0
	SalOp *op=sal_call_get_replaces(call->op);
	if (op){
		return (LinphoneCall*)sal_op_get_user_pointer(op);
	}
	return nullptr;
#else
	return nullptr;
#endif
}

int linphone_call_get_duration (const LinphoneCall *call) {
	return L_GET_CPP_PTR_FROM_C_OBJECT(call)->getDuration();
}

const LinphoneCallParams *linphone_call_get_current_params(LinphoneCall *call) {
	L_SET_CPP_PTR_FROM_C_OBJECT(call->currentParamsCache, L_GET_CPP_PTR_FROM_C_OBJECT(call)->getCurrentParams());
	return call->currentParamsCache;
}

const LinphoneCallParams *linphone_call_get_remote_params(LinphoneCall *call) {
	const LinphonePrivate::MediaSessionParams *remoteParams = L_GET_CPP_PTR_FROM_C_OBJECT(call)->getRemoteParams();
	if (!remoteParams)
		return nullptr;
	L_SET_CPP_PTR_FROM_C_OBJECT(call->remoteParamsCache, remoteParams);
	return call->remoteParamsCache;
}

void linphone_call_enable_camera (LinphoneCall *call, bool_t enable) {
	L_GET_CPP_PTR_FROM_C_OBJECT(call)->enableCamera(!!enable);
}

bool_t linphone_call_camera_enabled (const LinphoneCall *call) {
	return L_GET_CPP_PTR_FROM_C_OBJECT(call)->cameraEnabled();
}

LinphoneStatus linphone_call_take_video_snapshot (LinphoneCall *call, const char *file) {
	return L_GET_CPP_PTR_FROM_C_OBJECT(call)->takeVideoSnapshot(L_C_TO_STRING(file));
}

LinphoneStatus linphone_call_take_preview_snapshot (LinphoneCall *call, const char *file) {
	return L_GET_CPP_PTR_FROM_C_OBJECT(call)->takePreviewSnapshot(L_C_TO_STRING(file));
}

LinphoneReason linphone_call_get_reason (const LinphoneCall *call) {
	return L_GET_CPP_PTR_FROM_C_OBJECT(call)->getReason();
}

const LinphoneErrorInfo *linphone_call_get_error_info (const LinphoneCall *call) {
	return L_GET_CPP_PTR_FROM_C_OBJECT(call)->getErrorInfo();
}

const char *linphone_call_get_remote_user_agent (LinphoneCall *call) {
	string ua = L_GET_CPP_PTR_FROM_C_OBJECT(call)->getRemoteUserAgent();
	if (ua.empty())
		return nullptr;
	if (call->remoteUserAgentCache)
		bctbx_free(call->remoteUserAgentCache);
	call->remoteUserAgentCache = bctbx_strdup(ua.c_str());
	return call->remoteUserAgentCache;
}

const char * linphone_call_get_remote_contact (LinphoneCall *call) {
	string contact = L_GET_CPP_PTR_FROM_C_OBJECT(call)->getRemoteContact();
	if (contact.empty())
		return nullptr;
	if (call->remoteContactCache)
		bctbx_free(call->remoteContactCache);
	call->remoteContactCache = bctbx_strdup(contact.c_str());
	return call->remoteContactCache;
}

const char *linphone_call_get_authentication_token (LinphoneCall *call) {
	string token = L_GET_CPP_PTR_FROM_C_OBJECT(call)->getAuthenticationToken();
	if (token.empty())
		return nullptr;
	if (call->authenticationTokenCache)
		bctbx_free(call->authenticationTokenCache);
	call->authenticationTokenCache = bctbx_strdup(token.c_str());
	return call->authenticationTokenCache;
}

bool_t linphone_call_get_authentication_token_verified (const LinphoneCall *call) {
	return L_GET_CPP_PTR_FROM_C_OBJECT(call)->getAuthenticationTokenVerified();
}

void linphone_call_set_authentication_token_verified (LinphoneCall *call, bool_t verified) {
	L_GET_CPP_PTR_FROM_C_OBJECT(call)->setAuthenticationTokenVerified(!!verified);
}

void linphone_call_send_vfu_request (LinphoneCall *call) {
	L_GET_CPP_PTR_FROM_C_OBJECT(call)->sendVfuRequest();
}

void linphone_call_set_next_video_frame_decoded_callback (LinphoneCall *call, LinphoneCallCbFunc cb, void *ud) {
	L_GET_CPP_PTR_FROM_C_OBJECT(call)->setNextVideoFrameDecodedCallback(cb, ud);
}

LinphoneCallState linphone_call_get_transfer_state (LinphoneCall *call) {
#if 0
	return call->transfer_state;
#else
	return LinphoneCallIdle;
#endif
}

void linphone_call_zoom_video (LinphoneCall* call, float zoom_factor, float* cx, float* cy) {
	L_GET_CPP_PTR_FROM_C_OBJECT(call)->zoomVideo(zoom_factor, cx, cy);
}

void linphone_call_zoom (LinphoneCall *call, float zoom_factor, float cx, float cy) {
	L_GET_CPP_PTR_FROM_C_OBJECT(call)->zoomVideo(zoom_factor, cx, cy);
}

LinphoneStatus linphone_call_send_dtmf (LinphoneCall *call, char dtmf) {
#if 0
	if (!call){
		ms_warning("linphone_call_send_dtmf(): invalid call, canceling DTMF.");
		return -1;
	}
	call->dtmf_sequence = &dtmf;
	send_dtmf_handler(call,0);
	call->dtmf_sequence = nullptr;
	return 0;
#else
	return 0;
#endif
}

LinphoneStatus linphone_call_send_dtmfs (LinphoneCall *call, const char *dtmfs) {
#if 0
	if (!call){
		ms_warning("linphone_call_send_dtmfs(): invalid call, canceling DTMF sequence.");
		return -1;
	}
	if (call->dtmfs_timer){
		ms_warning("linphone_call_send_dtmfs(): a DTMF sequence is already in place, canceling DTMF sequence.");
		return -2;
	}
	if (dtmfs) {
		int delay_ms = lp_config_get_int(call->core->config,"net","dtmf_delay_ms",200);
		call->dtmf_sequence = ms_strdup(dtmfs);
		call->dtmfs_timer = sal_create_timer(call->core->sal, send_dtmf_handler, call, delay_ms, "DTMF sequence timer");
	}
	return 0;
#else
	return 0;
#endif
}

void linphone_call_cancel_dtmfs (LinphoneCall *call) {
#if 0
	/*nothing to do*/
	if (!call || !call->dtmfs_timer) return;

	sal_cancel_timer(call->core->sal, call->dtmfs_timer);
	belle_sip_object_unref(call->dtmfs_timer);
	call->dtmfs_timer = nullptr;
	if (call->dtmf_sequence) {
		ms_free(call->dtmf_sequence);
		call->dtmf_sequence = nullptr;
	}
#endif
}

bool_t linphone_call_is_in_conference (const LinphoneCall *call) {
#if 0
	return linphone_call_params_get_in_conference(call->params);
#else
	return FALSE;
#endif
}

LinphoneConference *linphone_call_get_conference (const LinphoneCall *call) {
#if 0
	return call->conf_ref;
#else
	return nullptr;
#endif
}

void linphone_call_set_audio_route (LinphoneCall *call, LinphoneAudioRoute route) {
#if 0
	if (call && call->audiostream){
		audio_stream_set_audio_route(call->audiostream, (MSAudioRoute) route);
	}
#endif
}

int linphone_call_get_stream_count (const LinphoneCall *call) {
	return L_GET_CPP_PTR_FROM_C_OBJECT(call)->getStreamCount();
}

MSFormatType linphone_call_get_stream_type (const LinphoneCall *call, int stream_index) {
	return L_GET_CPP_PTR_FROM_C_OBJECT(call)->getStreamType(stream_index);
}

RtpTransport *linphone_call_get_meta_rtp_transport (const LinphoneCall *call, int stream_index) {
	return L_GET_CPP_PTR_FROM_C_OBJECT(call)->getMetaRtpTransport(stream_index);
}

RtpTransport *linphone_call_get_meta_rtcp_transport (const LinphoneCall *call, int stream_index) {
	return L_GET_CPP_PTR_FROM_C_OBJECT(call)->getMetaRtcpTransport(stream_index);
}

LinphoneStatus linphone_call_pause (LinphoneCall *call) {
	return L_GET_CPP_PTR_FROM_C_OBJECT(call)->pause();
}

LinphoneStatus linphone_call_resume (LinphoneCall *call) {
	return L_GET_CPP_PTR_FROM_C_OBJECT(call)->resume();
}

LinphoneStatus linphone_call_terminate (LinphoneCall *call) {
	return L_GET_CPP_PTR_FROM_C_OBJECT(call)->terminate();
}

LinphoneStatus linphone_call_terminate_with_error_info (LinphoneCall *call , const LinphoneErrorInfo *ei) {
	return L_GET_CPP_PTR_FROM_C_OBJECT(call)->terminate(ei);
}

LinphoneStatus linphone_call_redirect (LinphoneCall *call, const char *redirect_uri) {
	return L_GET_CPP_PTR_FROM_C_OBJECT(call)->redirect(redirect_uri);
}

LinphoneStatus linphone_call_decline (LinphoneCall *call, LinphoneReason reason) {
	return L_GET_CPP_PTR_FROM_C_OBJECT(call)->decline(reason);
}

LinphoneStatus linphone_call_decline_with_error_info (LinphoneCall *call, const LinphoneErrorInfo *ei) {
	return L_GET_CPP_PTR_FROM_C_OBJECT(call)->decline(ei);
}

LinphoneStatus linphone_call_accept (LinphoneCall *call) {
	return L_GET_CPP_PTR_FROM_C_OBJECT(call)->accept(nullptr);
}

LinphoneStatus linphone_call_accept_with_params (LinphoneCall *call, const LinphoneCallParams *params) {
	return L_GET_CPP_PTR_FROM_C_OBJECT(call)->accept(params ? L_GET_CPP_PTR_FROM_C_OBJECT(params) : nullptr);
}

LinphoneStatus linphone_call_accept_early_media (LinphoneCall* call) {
	return L_GET_CPP_PTR_FROM_C_OBJECT(call)->acceptEarlyMedia();
}

LinphoneStatus linphone_call_accept_early_media_with_params (LinphoneCall *call, const LinphoneCallParams *params) {
	return L_GET_CPP_PTR_FROM_C_OBJECT(call)->acceptEarlyMedia(params ? L_GET_CPP_PTR_FROM_C_OBJECT(params) : nullptr);
}

LinphoneStatus linphone_call_update (LinphoneCall *call, const LinphoneCallParams *params) {
	return L_GET_CPP_PTR_FROM_C_OBJECT(call)->update(params ? L_GET_CPP_PTR_FROM_C_OBJECT(params) : nullptr);
}

LinphoneStatus linphone_call_defer_update (LinphoneCall *call) {
#if 0
	if (call->state != LinphoneCallUpdatedByRemote) {
		ms_error("linphone_call_defer_update() not done in state LinphoneCallUpdatedByRemote");
		return -1;
	}

	if (call->expect_media_in_ack) {
		ms_error("linphone_call_defer_update() is not possible during a late offer incoming reINVITE (INVITE without SDP)");
		return -1;
	}

	call->defer_update=TRUE;
	return 0;
#else
	return 0;
#endif
}

LinphoneStatus linphone_call_accept_update (LinphoneCall *call, const LinphoneCallParams *params) {
	return L_GET_CPP_PTR_FROM_C_OBJECT(call)->acceptUpdate(params ? L_GET_CPP_PTR_FROM_C_OBJECT(params) : nullptr);
}

LinphoneStatus linphone_call_transfer (LinphoneCall *call, const char *refer_to) {
#if 0
	char *real_url = nullptr;
	LinphoneCore *lc = linphone_call_get_core(call);
	LinphoneAddress *real_parsed_url = linphone_core_interpret_url(lc, refer_to);

	if (!real_parsed_url) {
		/* bad url */
		return -1;
	}
	//lc->call = nullptr; // Do not do that you will lose the call afterward...
	real_url = linphone_address_as_string(real_parsed_url);
	sal_call_refer(call->op, real_url);
	ms_free(real_url);
	linphone_address_unref(real_parsed_url);
	linphone_call_set_transfer_state(call, LinphoneCallOutgoingInit);
	return 0;
#else
	return 0;
#endif
}

LinphoneStatus linphone_call_transfer_to_another (LinphoneCall *call, LinphoneCall *dest) {
#if 0
	int result = sal_call_refer_with_replaces (call->op, dest->op);
	linphone_call_set_transfer_state(call, LinphoneCallOutgoingInit);
	return result;
#else
	return 0;
#endif
}

void *linphone_call_get_native_video_window_id (const LinphoneCall *call) {
	return L_GET_CPP_PTR_FROM_C_OBJECT(call)->getNativeVideoWindowId();
}

void linphone_call_set_native_video_window_id (LinphoneCall *call, void *id) {
	L_GET_CPP_PTR_FROM_C_OBJECT(call)->setNativeVideoWindowId(id);
}

void linphone_call_enable_echo_cancellation (LinphoneCall *call, bool_t enable) {
	L_GET_CPP_PTR_FROM_C_OBJECT(call)->enableEchoCancellation(!!enable);
}

bool_t linphone_call_echo_cancellation_enabled (const LinphoneCall *call) {
	return L_GET_CPP_PTR_FROM_C_OBJECT(call)->echoCancellationEnabled();
}

void linphone_call_enable_echo_limiter (LinphoneCall *call, bool_t val) {
	L_GET_CPP_PTR_FROM_C_OBJECT(call)->enableEchoLimiter(!!val);
}

bool_t linphone_call_echo_limiter_enabled (const LinphoneCall *call) {
	return L_GET_CPP_PTR_FROM_C_OBJECT(call)->echoLimiterEnabled();
}

LinphoneChatRoom *linphone_call_get_chat_room (LinphoneCall *call) {
#if 0
	if (!call->chat_room){
		if (call->state != LinphoneCallReleased && call->state != LinphoneCallEnd){
			call->chat_room = _linphone_core_create_chat_room_from_call(call);
		}
	}
	return call->chat_room;
#else
	return nullptr;
#endif
}

float linphone_call_get_play_volume (const LinphoneCall *call) {
	return L_GET_CPP_PTR_FROM_C_OBJECT(call)->getPlayVolume();
}

float linphone_call_get_record_volume (const LinphoneCall *call) {
	return L_GET_CPP_PTR_FROM_C_OBJECT(call)->getRecordVolume();
}

float linphone_call_get_speaker_volume_gain (const LinphoneCall *call) {
	return L_GET_CPP_PTR_FROM_C_OBJECT(call)->getSpeakerVolumeGain();
}

void linphone_call_set_speaker_volume_gain( LinphoneCall *call, float volume) {
	L_GET_CPP_PTR_FROM_C_OBJECT(call)->setSpeakerVolumeGain(volume);
}

float linphone_call_get_microphone_volume_gain (const LinphoneCall *call) {
	return L_GET_CPP_PTR_FROM_C_OBJECT(call)->getMicrophoneVolumeGain();
}

void linphone_call_set_microphone_volume_gain (LinphoneCall *call, float volume) {
	L_GET_CPP_PTR_FROM_C_OBJECT(call)->setMicrophoneVolumeGain(volume);
}

float linphone_call_get_current_quality (const LinphoneCall *call) {
	return L_GET_CPP_PTR_FROM_C_OBJECT(call)->getCurrentQuality();
}

float linphone_call_get_average_quality (const LinphoneCall *call) {
	return L_GET_CPP_PTR_FROM_C_OBJECT(call)->getAverageQuality();
}

void linphone_call_start_recording (LinphoneCall *call) {
	L_GET_CPP_PTR_FROM_C_OBJECT(call)->startRecording();
}

void linphone_call_stop_recording (LinphoneCall *call) {
	L_GET_CPP_PTR_FROM_C_OBJECT(call)->stopRecording();
}

LinphonePlayer *linphone_call_get_player (LinphoneCall *call) {
	return L_GET_CPP_PTR_FROM_C_OBJECT(call)->getPlayer();
}

bool_t linphone_call_media_in_progress (const LinphoneCall *call) {
	return L_GET_CPP_PTR_FROM_C_OBJECT(call)->mediaInProgress();
}

void linphone_call_ogl_render (const LinphoneCall *call) {
	L_GET_CPP_PTR_FROM_C_OBJECT(call)->oglRender();
}

LinphoneStatus linphone_call_send_info_message (LinphoneCall *call, const LinphoneInfoMessage *info) {
	SalBodyHandler *body_handler = sal_body_handler_from_content(linphone_info_message_get_content(info));
	linphone_call_get_op(call)->set_sent_custom_header(linphone_info_message_get_headers(info));
	return linphone_call_get_op(call)->send_info(nullptr, nullptr, body_handler);
}

LinphoneCallStats *linphone_call_get_stats (LinphoneCall *call, LinphoneStreamType type) {
	return L_GET_CPP_PTR_FROM_C_OBJECT(call)->getStats(type);
}

LinphoneCallStats *linphone_call_get_audio_stats (LinphoneCall *call) {
	return L_GET_CPP_PTR_FROM_C_OBJECT(call)->getAudioStats();
}

LinphoneCallStats *linphone_call_get_video_stats (LinphoneCall *call) {
	return L_GET_CPP_PTR_FROM_C_OBJECT(call)->getVideoStats();
}

LinphoneCallStats *linphone_call_get_text_stats (LinphoneCall *call) {
	return L_GET_CPP_PTR_FROM_C_OBJECT(call)->getTextStats();
}

void linphone_call_add_callbacks (LinphoneCall *call, LinphoneCallCbs *cbs) {
	call->callbacks = bctbx_list_append(call->callbacks, linphone_call_cbs_ref(cbs));
}

void linphone_call_remove_callbacks (LinphoneCall *call, LinphoneCallCbs *cbs) {
	call->callbacks = bctbx_list_remove(call->callbacks, cbs);
	linphone_call_cbs_unref(cbs);
}

LinphoneCallCbs *linphone_call_get_current_callbacks (const LinphoneCall *call) {
	return call->currentCbs;
}

const bctbx_list_t *linphone_call_get_callbacks_list(const LinphoneCall *call) {
	return call->callbacks;
}

void linphone_call_set_params (LinphoneCall *call, const LinphoneCallParams *params) {
	L_GET_CPP_PTR_FROM_C_OBJECT(call)->setParams(L_GET_CPP_PTR_FROM_C_OBJECT(params));
}

const LinphoneCallParams *linphone_call_get_params (LinphoneCall *call) {
	L_SET_CPP_PTR_FROM_C_OBJECT(call->paramsCache, L_GET_CPP_PTR_FROM_C_OBJECT(call)->getParams());
	return call->paramsCache;
}

// =============================================================================
// Reference and user data handling functions.
// =============================================================================

LinphoneCall *linphone_call_ref (LinphoneCall *call) {
	belle_sip_object_ref(call);
	return call;
}

void linphone_call_unref (LinphoneCall *call) {
	belle_sip_object_unref(call);
}

void *linphone_call_get_user_data (const LinphoneCall *call) {
	return L_GET_USER_DATA_FROM_C_OBJECT(call);
}

void linphone_call_set_user_data (LinphoneCall *call, void *ud) {
	L_SET_USER_DATA_FROM_C_OBJECT(call, ud);
}

// =============================================================================
// Constructor and destructor functions.
// =============================================================================

LinphoneCall *linphone_call_new_outgoing (LinphoneCore *lc, const LinphoneAddress *from, const LinphoneAddress *to, const LinphoneCallParams *params, LinphoneProxyConfig *cfg) {
	LinphoneCall *lcall = L_INIT(Call);
	shared_ptr<LinphonePrivate::Call> call;
	string confType = lp_config_get_string(linphone_core_get_config(lc), "misc", "conference_type", "local");
	if (confType == "remote") {
		call = make_shared<LinphonePrivate::RemoteConferenceCall>(lc->cppCore, LinphoneCallOutgoing,
			*L_GET_CPP_PTR_FROM_C_OBJECT(from), *L_GET_CPP_PTR_FROM_C_OBJECT(to),
			cfg, nullptr, L_GET_CPP_PTR_FROM_C_OBJECT(params));
	} else {
		call = make_shared<LinphonePrivate::LocalConferenceCall>(lc->cppCore, LinphoneCallOutgoing,
			*L_GET_CPP_PTR_FROM_C_OBJECT(from), *L_GET_CPP_PTR_FROM_C_OBJECT(to),
			cfg, nullptr, L_GET_CPP_PTR_FROM_C_OBJECT(params));
	}
	L_SET_CPP_PTR_FROM_C_OBJECT(lcall, call);
	lcall->currentParamsCache = linphone_call_params_new_for_wrapper();
	lcall->paramsCache = linphone_call_params_new_for_wrapper();
	lcall->remoteParamsCache = linphone_call_params_new_for_wrapper();
	lcall->remoteAddressCache = linphone_address_new(nullptr);
	return lcall;
}

LinphoneCall *linphone_call_new_incoming (LinphoneCore *lc, const LinphoneAddress *from, const LinphoneAddress *to, LinphonePrivate::SalCallOp *op) {
	LinphoneCall *lcall = L_INIT(Call);
	shared_ptr<LinphonePrivate::Call> call;
	string confType = lp_config_get_string(linphone_core_get_config(lc), "misc", "conference_type", "local");
	if (confType == "remote") {
		call = make_shared<LinphonePrivate::RemoteConferenceCall>(lc->cppCore, LinphoneCallIncoming,
			*L_GET_CPP_PTR_FROM_C_OBJECT(from), *L_GET_CPP_PTR_FROM_C_OBJECT(to),
			nullptr, op, nullptr);
	} else {
		call = make_shared<LinphonePrivate::LocalConferenceCall>(lc->cppCore, LinphoneCallIncoming,
			*L_GET_CPP_PTR_FROM_C_OBJECT(from), *L_GET_CPP_PTR_FROM_C_OBJECT(to),
			nullptr, op, nullptr);
	}
	L_SET_CPP_PTR_FROM_C_OBJECT(lcall, call);
	lcall->currentParamsCache = linphone_call_params_new_for_wrapper();
	lcall->paramsCache = linphone_call_params_new_for_wrapper();
	lcall->remoteParamsCache = linphone_call_params_new_for_wrapper();
	lcall->remoteAddressCache = linphone_address_new(nullptr);
	L_GET_PRIVATE_FROM_C_OBJECT(lcall)->initiateIncoming();
	return lcall;
}
