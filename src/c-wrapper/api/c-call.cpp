/*
 * c-call.cpp
 * Copyright (C) 2010-2018 Belledonne Communications SARL
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
#include "chat/chat-room/real-time-text-chat-room.h"
#include "conference/params/media-session-params-p.h"
#include "core/core-p.h"

// =============================================================================

using namespace std;

static void _linphone_call_constructor (LinphoneCall *call);
static void _linphone_call_destructor (LinphoneCall *call);

L_DECLARE_C_OBJECT_IMPL_WITH_XTORS(Call,
	_linphone_call_constructor, _linphone_call_destructor,
	bctbx_list_t *callbacks; /* A list of LinphoneCallCbs object */
	LinphoneCallCbs *currentCbs; /* The current LinphoneCallCbs object used to call a callback */
	char *authenticationTokenCache;
	mutable char *referToCache;
	char *remoteContactCache;
	char *remoteUserAgentCache;
	mutable char *toHeaderCache;
	/* TODO: all the fields need to be removed */
	LinphoneConference *confRef; /**> Point on the associated conference if this call is part of a conference. NULL instead. */
	MSAudioEndpoint *endpoint; /*used for conferencing*/
	LinphoneChatRoom *chat_room;
)

static void _linphone_call_constructor (LinphoneCall *call) {}

static void _linphone_call_destructor (LinphoneCall *call) {
	if (call->referToCache)
		bctbx_free(call->referToCache);
	if (call->remoteContactCache)
		bctbx_free(call->remoteContactCache);
	if (call->remoteUserAgentCache)
		bctbx_free(call->remoteUserAgentCache);
	if (call->toHeaderCache)
		bctbx_free(call->toHeaderCache);
	bctbx_list_free_with_data(call->callbacks, (bctbx_list_free_func)linphone_call_cbs_unref);
}

// =============================================================================
// TODO: To remove!
// =============================================================================

void linphone_call_init_media_streams (LinphoneCall *call) {
	L_GET_PRIVATE_FROM_C_OBJECT(call)->initializeMediaStreams();
}

/*This function is not static because used internally in linphone-daemon project*/
void _post_configure_audio_stream (AudioStream *st, LinphoneCore *lc, bool_t muted) {
	L_GET_PRIVATE_FROM_C_OBJECT(lc)->postConfigureAudioStream(st, !!muted);
}

void linphone_call_stop_media_streams (LinphoneCall *call) {
	L_GET_PRIVATE_FROM_C_OBJECT(call)->stopMediaStreams();
}

/* Internal version that does not play tone indication*/
int _linphone_call_pause (LinphoneCall *call) {
	return L_GET_CPP_PTR_FROM_C_OBJECT(call)->pause();
}


// =============================================================================
// Private functions.
// =============================================================================

void _linphone_call_set_conf_ref (LinphoneCall *call, LinphoneConference *ref) {
	call->confRef = ref;
}

MSAudioEndpoint *_linphone_call_get_endpoint (const LinphoneCall *call) {
	return call->endpoint;
}

void _linphone_call_set_endpoint (LinphoneCall *call, MSAudioEndpoint *endpoint) {
	call->endpoint = endpoint;
}

MediaStream *linphone_call_get_stream (LinphoneCall *call, LinphoneStreamType type) {
	return L_GET_PRIVATE_FROM_C_OBJECT(call)->getMediaStream(type);
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

void linphone_call_notify_tmmbr_received (LinphoneCall *call, int stream_index, int tmmbr) {
	NOTIFY_IF_EXIST(TmmbrReceived, tmmbr_received, call, stream_index, tmmbr)
}

void linphone_call_notify_snapshot_taken(LinphoneCall *call, const char *file_path) {
	NOTIFY_IF_EXIST(SnapshotTaken, snapshot_taken, call, file_path)
}

void linphone_call_notify_next_video_frame_decoded(LinphoneCall *call) {
	NOTIFY_IF_EXIST(NextVideoFrameDecoded, next_video_frame_decoded, call)
}

// =============================================================================
// Public functions.
// =============================================================================

LinphoneCore *linphone_call_get_core (const LinphoneCall *call) {
	return L_GET_CPP_PTR_FROM_C_OBJECT(call)->getCore()->getCCore();
}

LinphoneCallState linphone_call_get_state (const LinphoneCall *call) {
	return static_cast<LinphoneCallState>(L_GET_CPP_PTR_FROM_C_OBJECT(call)->getState());
}

bool_t linphone_call_asked_to_autoanswer (LinphoneCall *call) {
	//return TRUE if the unique(for the moment) incoming call asked to be autoanswered
	if (call)
		return linphone_call_get_op(call)->autoAnswerAsked();
	return FALSE;
}

const LinphoneAddress *linphone_call_get_remote_address (const LinphoneCall *call) {
	return L_GET_C_BACK_PTR(&L_GET_CPP_PTR_FROM_C_OBJECT(call)->getRemoteAddress());
}

const LinphoneAddress *linphone_call_get_to_address (const LinphoneCall *call) {
	return L_GET_C_BACK_PTR(&L_GET_CPP_PTR_FROM_C_OBJECT(call)->getToAddress());
}

const char *linphone_call_get_to_header (const LinphoneCall *call, const char *name) {
	string header = L_GET_CPP_PTR_FROM_C_OBJECT(call)->getToHeader(name);
	if (header.empty())
		return nullptr;
	if (call->toHeaderCache)
		bctbx_free(call->toHeaderCache);
	call->toHeaderCache = bctbx_strdup(header.c_str());
	return call->toHeaderCache;
}

char *linphone_call_get_remote_address_as_string (const LinphoneCall *call) {
	return ms_strdup(L_GET_CPP_PTR_FROM_C_OBJECT(call)->getRemoteAddress().asString().c_str());
}

const LinphoneAddress *linphone_call_get_diversion_address (const LinphoneCall *call) {
	const LinphonePrivate::Address &diversionAddress = L_GET_CPP_PTR_FROM_C_OBJECT(call)->getDiversionAddress();
	return diversionAddress.isValid() ? L_GET_C_BACK_PTR(&diversionAddress) : nullptr;
}

LinphoneCallDir linphone_call_get_dir (const LinphoneCall *call) {
	return linphone_call_log_get_dir(linphone_call_get_log(call));
}

LinphoneCallLog *linphone_call_get_call_log (const LinphoneCall *call) {
	return L_GET_CPP_PTR_FROM_C_OBJECT(call)->getLog();
}

const char *linphone_call_get_refer_to (const LinphoneCall *call) {
	string referTo = L_GET_CPP_PTR_FROM_C_OBJECT(call)->getReferTo();
	if (referTo.empty())
		return nullptr;
	if (call->referToCache)
		bctbx_free(call->referToCache);
	call->referToCache = bctbx_strdup(referTo.c_str());
	return call->referToCache;
}

bool_t linphone_call_has_transfer_pending (const LinphoneCall *call) {
	return L_GET_CPP_PTR_FROM_C_OBJECT(call)->hasTransferPending();
}

LinphoneCall *linphone_call_get_transferer_call (const LinphoneCall *call) {
	shared_ptr<LinphonePrivate::Call> referer = L_GET_CPP_PTR_FROM_C_OBJECT(call)->getReferer();
	if (!referer)
		return nullptr;
	return L_GET_C_BACK_PTR(referer);
}

LinphoneCall *linphone_call_get_transfer_target_call (const LinphoneCall *call) {
	shared_ptr<LinphonePrivate::Call> transferTarget = L_GET_CPP_PTR_FROM_C_OBJECT(call)->getTransferTarget();
	if (!transferTarget)
		return nullptr;
	return L_GET_C_BACK_PTR(transferTarget);
}

LinphoneCall *linphone_call_get_replaced_call (LinphoneCall *call) {
	shared_ptr<LinphonePrivate::Call> replacedCall = L_GET_CPP_PTR_FROM_C_OBJECT(call)->getReplacedCall();
	if (!replacedCall)
		return nullptr;
	return L_GET_C_BACK_PTR(replacedCall);
}

int linphone_call_get_duration (const LinphoneCall *call) {
	return L_GET_CPP_PTR_FROM_C_OBJECT(call)->getDuration();
}

const LinphoneCallParams *linphone_call_get_current_params (LinphoneCall *call) {
	return L_GET_C_BACK_PTR(L_GET_CPP_PTR_FROM_C_OBJECT(call)->getCurrentParams());
}

const LinphoneCallParams *linphone_call_get_remote_params(LinphoneCall *call) {
	const LinphonePrivate::MediaSessionParams *remoteParams = L_GET_CPP_PTR_FROM_C_OBJECT(call)->getRemoteParams();
	return remoteParams ? L_GET_C_BACK_PTR(remoteParams) : nullptr;
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

void linphone_call_request_notify_next_video_frame_decoded(LinphoneCall *call){
	L_GET_CPP_PTR_FROM_C_OBJECT(call)->requestNotifyNextVideoFrameDecoded();
}

LinphoneCallState linphone_call_get_transfer_state (LinphoneCall *call) {
	return static_cast<LinphoneCallState>(L_GET_CPP_PTR_FROM_C_OBJECT(call)->getTransferState());
}

void linphone_call_zoom_video (LinphoneCall* call, float zoom_factor, float* cx, float* cy) {
	L_GET_CPP_PTR_FROM_C_OBJECT(call)->zoomVideo(zoom_factor, cx, cy);
}

void linphone_call_zoom (LinphoneCall *call, float zoom_factor, float cx, float cy) {
	L_GET_CPP_PTR_FROM_C_OBJECT(call)->zoomVideo(zoom_factor, cx, cy);
}

LinphoneStatus linphone_call_send_dtmf (LinphoneCall *call, char dtmf) {
	if (!call) {
		ms_warning("linphone_call_send_dtmf(): invalid call, canceling DTMF");
		return -1;
	}
	return L_GET_CPP_PTR_FROM_C_OBJECT(call)->sendDtmf(dtmf);
}

LinphoneStatus linphone_call_send_dtmfs (LinphoneCall *call, const char *dtmfs) {
	if (!call) {
		ms_warning("linphone_call_send_dtmfs(): invalid call, canceling DTMF sequence");
		return -1;
	}
	return L_GET_CPP_PTR_FROM_C_OBJECT(call)->sendDtmfs(dtmfs);
}

void linphone_call_cancel_dtmfs (LinphoneCall *call) {
	if (!call)
		return;
	L_GET_CPP_PTR_FROM_C_OBJECT(call)->cancelDtmfs();
}

bool_t linphone_call_is_in_conference (const LinphoneCall *call) {
	return !!L_GET_CPP_PTR_FROM_C_OBJECT(call)->isInConference();
}

LinphoneConference *linphone_call_get_conference (const LinphoneCall *call) {
	return call->confRef;
}

void linphone_call_set_audio_route (LinphoneCall *call, LinphoneAudioRoute route) {
	L_GET_CPP_PTR_FROM_C_OBJECT(call)->setAudioRoute(route);
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
	return L_GET_CPP_PTR_FROM_C_OBJECT(call)->deferUpdate();
}

LinphoneStatus linphone_call_accept_update (LinphoneCall *call, const LinphoneCallParams *params) {
	return L_GET_CPP_PTR_FROM_C_OBJECT(call)->acceptUpdate(params ? L_GET_CPP_PTR_FROM_C_OBJECT(params) : nullptr);
}

LinphoneStatus linphone_call_transfer (LinphoneCall *call, const char *referTo) {
	return L_GET_CPP_PTR_FROM_C_OBJECT(call)->transfer(referTo);
}

LinphoneStatus linphone_call_transfer_to_another (LinphoneCall *call, LinphoneCall *dest) {
	return L_GET_CPP_PTR_FROM_C_OBJECT(call)->transfer(L_GET_CPP_PTR_FROM_C_OBJECT(dest));
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
	shared_ptr<LinphonePrivate::RealTimeTextChatRoom> acr = L_GET_PRIVATE_FROM_C_OBJECT(call)->getChatRoom();
	if (acr)
		return L_GET_C_BACK_PTR(acr);
	return nullptr;
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
	linphone_call_get_op(call)->setSentCustomHeaders(linphone_info_message_get_headers(info));
	return linphone_call_get_op(call)->sendInfo(body_handler);
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
	return L_GET_C_BACK_PTR(L_GET_CPP_PTR_FROM_C_OBJECT(call)->getParams());
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
		call = make_shared<LinphonePrivate::RemoteConferenceCall>(L_GET_CPP_PTR_FROM_C_OBJECT(lc), LinphoneCallOutgoing,
			*L_GET_CPP_PTR_FROM_C_OBJECT(from), *L_GET_CPP_PTR_FROM_C_OBJECT(to),
			cfg, nullptr, L_GET_CPP_PTR_FROM_C_OBJECT(params));
	} else {
		call = make_shared<LinphonePrivate::LocalConferenceCall>(L_GET_CPP_PTR_FROM_C_OBJECT(lc), LinphoneCallOutgoing,
			*L_GET_CPP_PTR_FROM_C_OBJECT(from), *L_GET_CPP_PTR_FROM_C_OBJECT(to),
			cfg, nullptr, L_GET_CPP_PTR_FROM_C_OBJECT(params));
	}
	L_SET_CPP_PTR_FROM_C_OBJECT(lcall, call);
	return lcall;
}

LinphoneCall *linphone_call_new_incoming (LinphoneCore *lc, const LinphoneAddress *from, const LinphoneAddress *to, LinphonePrivate::SalCallOp *op) {
	LinphoneCall *lcall = L_INIT(Call);
	shared_ptr<LinphonePrivate::Call> call;
	string confType = lp_config_get_string(linphone_core_get_config(lc), "misc", "conference_type", "local");
	if (confType == "remote") {
		call = make_shared<LinphonePrivate::RemoteConferenceCall>(L_GET_CPP_PTR_FROM_C_OBJECT(lc), LinphoneCallIncoming,
			*L_GET_CPP_PTR_FROM_C_OBJECT(from), *L_GET_CPP_PTR_FROM_C_OBJECT(to),
			nullptr, op, nullptr);
	} else {
		call = make_shared<LinphonePrivate::LocalConferenceCall>(L_GET_CPP_PTR_FROM_C_OBJECT(lc), LinphoneCallIncoming,
			*L_GET_CPP_PTR_FROM_C_OBJECT(from), *L_GET_CPP_PTR_FROM_C_OBJECT(to),
			nullptr, op, nullptr);
	}
	L_SET_CPP_PTR_FROM_C_OBJECT(lcall, call);
	L_GET_PRIVATE_FROM_C_OBJECT(lcall)->initiateIncoming();
	return lcall;
}
