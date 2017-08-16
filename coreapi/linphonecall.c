
/*
linphone
Copyright (C) 2010  Belledonne Communications SARL
 (simon.morlat@linphone.org)

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
#ifdef _WIN32
#include <time.h>
#endif
#include "linphone/core.h"
#include "linphone/sipsetup.h"
#include "linphone/lpconfig.h"
#include "private.h"
#include "conference_private.h"

#include <ortp/event.h>
#include <ortp/b64.h>
#include <math.h>

#include "mediastreamer2/mediastream.h"
#include "mediastreamer2/msvolume.h"
#include "mediastreamer2/msequalizer.h"
#include "mediastreamer2/msfileplayer.h"
#include "mediastreamer2/msjpegwriter.h"
#include "mediastreamer2/msogl.h"
#include "mediastreamer2/mseventqueue.h"
#include "mediastreamer2/mssndcard.h"
#include "mediastreamer2/msrtt4103.h"

#include <bctoolbox/defs.h>

// For migration purpose.
#include "address/address-p.h"
#include "c-wrapper/c-private-types.h"
#include "c-wrapper/c-tools.h"
#include "call/call.h"
#include "call/call-p.h"
#include "conference/params/media-session-params-p.h"

struct _LinphoneCall{
	belle_sip_object_t base;
	void *user_data;
	bctbx_list_t *callbacks; /* A list of LinphoneCallCbs object */
	LinphoneCallCbs *current_cbs; /* The current LinphoneCallCbs object used to call a callback */
	std::shared_ptr<LinphonePrivate::Call> call;
	LinphoneCallParams *currentParamsCache;
	LinphoneCallParams *paramsCache;
	LinphoneCallParams *remoteParamsCache;
	LinphoneAddress *remoteAddressCache;
	char *remoteContactCache;

	struct _LinphoneCore *core;
	LinphoneErrorInfo *ei;
	SalMediaDescription *localdesc;
	SalMediaDescription *resultdesc;
	struct _LinphoneCallLog *log;
	SalOp *op;
	SalOp *ping_op;
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
};

BELLE_SIP_DECLARE_VPTR_NO_EXPORT(LinphoneCall);


typedef belle_sip_object_t_vptr_t LinphoneCallCbs_vptr_t;
BELLE_SIP_DECLARE_NO_IMPLEMENTED_INTERFACES(LinphoneCallCbs);
BELLE_SIP_INSTANCIATE_VPTR(LinphoneCallCbs, belle_sip_object_t,
	NULL, // destroy
	NULL, // clone
	NULL, // Marshall
	FALSE
);

LinphoneCallCbs *_linphone_call_cbs_new(void) {
	LinphoneCallCbs *obj = belle_sip_object_new(LinphoneCallCbs);
	return obj;
}

LinphoneCallCbs *linphone_call_cbs_ref(LinphoneCallCbs *cbs) {
	return (LinphoneCallCbs *)belle_sip_object_ref(cbs);
}

void linphone_call_cbs_unref(LinphoneCallCbs *cbs) {
	belle_sip_object_unref(cbs);
}

void *linphone_call_cbs_get_user_data(const LinphoneCallCbs *cbs) {
	return cbs->user_data;
}

void linphone_call_cbs_set_user_data(LinphoneCallCbs *cbs, void *user_data) {
	cbs->user_data = user_data;
}

LinphoneCallCbsDtmfReceivedCb linphone_call_cbs_get_dtmf_received(LinphoneCallCbs *cbs) {
	return cbs->dtmf_received_cb;
}

void linphone_call_cbs_set_dtmf_received(LinphoneCallCbs *cbs, LinphoneCallCbsDtmfReceivedCb cb) {
	cbs->dtmf_received_cb = cb;
}

LinphoneCallCbsEncryptionChangedCb linphone_call_cbs_get_encryption_changed(LinphoneCallCbs *cbs) {
	return cbs->encryption_changed_cb;
}

void linphone_call_cbs_set_encryption_changed(LinphoneCallCbs *cbs, LinphoneCallCbsEncryptionChangedCb cb) {
	cbs->encryption_changed_cb = cb;
}

LinphoneCallCbsInfoMessageReceivedCb linphone_call_cbs_get_info_message_received(LinphoneCallCbs *cbs) {
	return cbs->info_message_received_cb;
}

void linphone_call_cbs_set_info_message_received(LinphoneCallCbs *cbs, LinphoneCallCbsInfoMessageReceivedCb cb) {
	cbs->info_message_received_cb = cb;
}

LinphoneCallCbsStateChangedCb linphone_call_cbs_get_state_changed(LinphoneCallCbs *cbs) {
	return cbs->state_changed_cb;
}

void linphone_call_cbs_set_state_changed(LinphoneCallCbs *cbs, LinphoneCallCbsStateChangedCb cb) {
	cbs->state_changed_cb = cb;
}

LinphoneCallCbsStatsUpdatedCb linphone_call_cbs_get_stats_updated(LinphoneCallCbs *cbs) {
	return cbs->stats_updated_cb;
}

void linphone_call_cbs_set_stats_updated(LinphoneCallCbs *cbs, LinphoneCallCbsStatsUpdatedCb cb) {
	cbs->stats_updated_cb = cb;
}

LinphoneCallCbsTransferStateChangedCb linphone_call_cbs_get_transfer_state_changed(LinphoneCallCbs *cbs) {
	return cbs->transfer_state_changed_cb;
}

void linphone_call_cbs_set_transfer_state_changed(LinphoneCallCbs *cbs, LinphoneCallCbsTransferStateChangedCb cb) {
	cbs->transfer_state_changed_cb = cb;
}

LinphoneCallCbsAckProcessingCb linphone_call_cbs_get_ack_processing(LinphoneCallCbs *cbs){
	return cbs->ack_processing;
}

void linphone_call_cbs_set_ack_processing(LinphoneCallCbs *cbs, LinphoneCallCbsAckProcessingCb cb){
	cbs->ack_processing = cb;
}


bool_t linphone_call_state_is_early(LinphoneCallState state){
	switch (state){
		case LinphoneCallIdle:
		case LinphoneCallOutgoingInit:
		case LinphoneCallOutgoingEarlyMedia:
		case LinphoneCallOutgoingRinging:
		case LinphoneCallOutgoingProgress:
		case LinphoneCallIncomingReceived:
		case LinphoneCallIncomingEarlyMedia:
		case LinphoneCallEarlyUpdatedByRemote:
		case LinphoneCallEarlyUpdating:
			return TRUE;
		case LinphoneCallResuming:
		case LinphoneCallEnd:
		case LinphoneCallUpdating:
		case LinphoneCallRefered:
		case LinphoneCallPausing:
		case LinphoneCallPausedByRemote:
		case LinphoneCallPaused:
		case LinphoneCallConnected:
		case LinphoneCallError:
		case LinphoneCallUpdatedByRemote:
		case LinphoneCallReleased:
		case LinphoneCallStreamsRunning:
		break;
	}
	return FALSE;
}

MSWebCam *get_nowebcam_device(MSFactory* f){
#ifdef VIDEO_ENABLED
	return ms_web_cam_manager_get_cam(ms_factory_get_web_cam_manager(f),"StaticImage: Static picture");
#else
	return NULL;
#endif
}

LinphoneCore *linphone_call_get_core(const LinphoneCall *call) {
	return linphone_call_get_cpp_obj(call)->getCore();
}

const char * linphone_call_get_authentication_token(LinphoneCall *call) {
	std::string token = linphone_call_get_cpp_obj(call)->getAuthenticationToken();
	return token.empty() ? nullptr : token.c_str();
}

bool_t linphone_call_get_authentication_token_verified(const LinphoneCall *call) {
	return linphone_call_get_cpp_obj(call)->getAuthenticationTokenVerified();
}

void linphone_call_set_authentication_token_verified(LinphoneCall *call, bool_t verified) {
	linphone_call_get_cpp_obj(call)->setAuthenticationTokenVerified(verified);
}

bool_t is_payload_type_number_available(const bctbx_list_t *l, int number, const PayloadType *ignore){
	const bctbx_list_t *elem;
	for (elem=l; elem!=NULL; elem=elem->next){
		const PayloadType *pt=(PayloadType*)elem->data;
		if (pt!=ignore && payload_type_get_number(pt)==number) return FALSE;
	}
	return TRUE;
}

void linphone_call_update_local_media_description_from_ice_or_upnp(LinphoneCall *call){
}

void linphone_call_make_local_media_description(LinphoneCall *call) {
}

void linphone_call_create_op(LinphoneCall *call){
#if 0
	if (call->op) sal_op_release(call->op);
	call->op=sal_op_new(call->core->sal);
	sal_op_set_user_pointer(call->op,call);
	if (linphone_call_params_get_referer(call->params))
		sal_call_set_referer(call->op,linphone_call_params_get_referer(call->params)->op);
	linphone_configure_op(call->core,call->op,call->log->to,linphone_call_params_get_custom_headers(call->params),FALSE);
	if (linphone_call_params_get_privacy(call->params) != LinphonePrivacyDefault)
		sal_op_set_privacy(call->op,(SalPrivacyMask)linphone_call_params_get_privacy(call->params));
  /*else privacy might be set by proxy */
#endif
}

static void linphone_call_destroy(LinphoneCall *obj);

BELLE_SIP_DECLARE_NO_IMPLEMENTED_INTERFACES(LinphoneCall);

BELLE_SIP_INSTANCIATE_VPTR(LinphoneCall, belle_sip_object_t,
	(belle_sip_object_destroy_t)linphone_call_destroy,
	NULL, // clone
	NULL, // marshal
	FALSE
);

LinphoneCall * linphone_call_new_outgoing(LinphoneCore *lc, const LinphoneAddress *from, const LinphoneAddress *to, const LinphoneCallParams *params, LinphoneProxyConfig *cfg){
	LinphoneCall *call = belle_sip_object_new(LinphoneCall);
	call->currentParamsCache = linphone_call_params_new_for_wrapper();
	call->paramsCache = linphone_call_params_new_for_wrapper();
	call->remoteParamsCache = linphone_call_params_new_for_wrapper();
	call->remoteAddressCache = linphone_address_new(nullptr);
	call->call = std::make_shared<LinphonePrivate::Call>(call, lc, LinphoneCallOutgoing, *L_GET_CPP_PTR_FROM_C_STRUCT(from, Address), *L_GET_CPP_PTR_FROM_C_STRUCT(to, Address), cfg, nullptr, linphone_call_params_get_cpp_obj(params));
	return call;
}

LinphoneCall * linphone_call_new_incoming(LinphoneCore *lc, const LinphoneAddress *from, const LinphoneAddress *to, SalOp *op) {
	LinphoneCall *call = belle_sip_object_new(LinphoneCall);
	call->currentParamsCache = linphone_call_params_new_for_wrapper();
	call->paramsCache = linphone_call_params_new_for_wrapper();
	call->remoteParamsCache = linphone_call_params_new_for_wrapper();
	call->remoteAddressCache = linphone_address_new(nullptr);
	call->call = std::make_shared<LinphonePrivate::Call>(call, lc, LinphoneCallIncoming, *L_GET_CPP_PTR_FROM_C_STRUCT(from, Address), *L_GET_CPP_PTR_FROM_C_STRUCT(to, Address), nullptr, op, nullptr);
	L_GET_PRIVATE(linphone_call_get_cpp_obj(call).get())->initiateIncoming();
	return call;
}

static void linphone_call_free_media_resources(LinphoneCall *call){
}

const char *linphone_call_state_to_string(LinphoneCallState cs){
	switch (cs){
		case LinphoneCallIdle:
			return "LinphoneCallIdle";
		case LinphoneCallIncomingReceived:
			return "LinphoneCallIncomingReceived";
		case LinphoneCallOutgoingInit:
			return "LinphoneCallOutgoingInit";
		case LinphoneCallOutgoingProgress:
			return "LinphoneCallOutgoingProgress";
		case LinphoneCallOutgoingRinging:
			return "LinphoneCallOutgoingRinging";
		case LinphoneCallOutgoingEarlyMedia:
			return "LinphoneCallOutgoingEarlyMedia";
		case LinphoneCallConnected:
			return "LinphoneCallConnected";
		case LinphoneCallStreamsRunning:
			return "LinphoneCallStreamsRunning";
		case LinphoneCallPausing:
			return "LinphoneCallPausing";
		case LinphoneCallPaused:
			return "LinphoneCallPaused";
		case LinphoneCallResuming:
			return "LinphoneCallResuming";
		case LinphoneCallRefered:
			return "LinphoneCallRefered";
		case LinphoneCallError:
			return "LinphoneCallError";
		case LinphoneCallEnd:
			return "LinphoneCallEnd";
		case LinphoneCallPausedByRemote:
			return "LinphoneCallPausedByRemote";
		case LinphoneCallUpdatedByRemote:
			return "LinphoneCallUpdatedByRemote";
		case LinphoneCallIncomingEarlyMedia:
			return "LinphoneCallIncomingEarlyMedia";
		case LinphoneCallUpdating:
			return "LinphoneCallUpdating";
		case LinphoneCallReleased:
			return "LinphoneCallReleased";
		case LinphoneCallEarlyUpdatedByRemote:
			return "LinphoneCallEarlyUpdatedByRemote";
		case LinphoneCallEarlyUpdating:
			return "LinphoneCallEarlyUpdating";
	}
	return "undefined state";
}

void linphone_call_set_state(LinphoneCall *call, LinphoneCallState cstate, const char *message){
}

static void linphone_call_destroy(LinphoneCall *obj) {
	ms_message("Call [%p] freed.", obj);
	obj->call = nullptr;
	if (obj->currentParamsCache) {
		linphone_call_params_unref(obj->currentParamsCache);
		obj->currentParamsCache = nullptr;
	}
	if (obj->paramsCache) {
		linphone_call_params_unref(obj->paramsCache);
		obj->paramsCache = nullptr;
	}
	if (obj->remoteParamsCache) {
		linphone_call_params_unref(obj->remoteParamsCache);
		obj->remoteParamsCache = nullptr;
	}
	if (obj->remoteAddressCache) {
		linphone_address_unref(obj->remoteAddressCache);
		obj->remoteAddressCache = nullptr;
	}
	bctbx_list_free_with_data(obj->callbacks, (bctbx_list_free_func)linphone_call_cbs_unref);
	if (obj->audiostream || obj->videostream){
		linphone_call_free_media_resources(obj);
	}
	if (obj->audio_stats) {
		linphone_call_stats_unref(obj->audio_stats);
		obj->audio_stats = NULL;
	}
	if (obj->video_stats) {
		linphone_call_stats_unref(obj->video_stats);
		obj->video_stats = NULL;
	}
	if (obj->text_stats) {
		linphone_call_stats_unref(obj->text_stats);
		obj->text_stats = NULL;
	}
	if (obj->op!=NULL) {
		sal_op_release(obj->op);
		obj->op=NULL;
	}
	if (obj->resultdesc!=NULL) {
		sal_media_description_unref(obj->resultdesc);
		obj->resultdesc=NULL;
	}
	if (obj->localdesc!=NULL) {
		sal_media_description_unref(obj->localdesc);
		obj->localdesc=NULL;
	}
	if (obj->ping_op) {
		sal_op_release(obj->ping_op);
		obj->ping_op=NULL;
	}
	if (obj->refer_to){
		ms_free(obj->refer_to);
		obj->refer_to=NULL;
	}
	if (obj->referer){
		linphone_call_unref(obj->referer);
		obj->referer=NULL;
	}
	if (obj->transfer_target){
		linphone_call_unref(obj->transfer_target);
		obj->transfer_target=NULL;
	}
	if (obj->log) {
		linphone_call_log_unref(obj->log);
		obj->log=NULL;
	}
	if (obj->dtmfs_timer) {
		linphone_call_cancel_dtmfs(obj);
	}
	if (obj->params){
		linphone_call_params_unref(obj->params);
		obj->params=NULL;
	}
	if (obj->current_params){
		linphone_call_params_unref(obj->current_params);
		obj->current_params=NULL;
	}
	if (obj->remote_params != NULL) {
		linphone_call_params_unref(obj->remote_params);
		obj->remote_params=NULL;
	}
	if (obj->ei) linphone_error_info_unref(obj->ei);
}

LinphoneCall * linphone_call_ref(LinphoneCall *obj){
	belle_sip_object_ref(obj);
	return obj;
}

void linphone_call_unref(LinphoneCall *obj){
	belle_sip_object_unref(obj);
}

const LinphoneCallParams * linphone_call_get_current_params(LinphoneCall *call){
	call->currentParamsCache->msp = linphone_call_get_cpp_obj(call)->getCurrentParams();
	return call->currentParamsCache;
}

const LinphoneCallParams * linphone_call_get_remote_params(LinphoneCall *call) {
	call->remoteParamsCache->msp = linphone_call_get_cpp_obj(call)->getRemoteParams();
	if (call->remoteParamsCache->msp)
		return call->remoteParamsCache;
	return nullptr;
}

const LinphoneAddress * linphone_call_get_remote_address(const LinphoneCall *call) {
	std::shared_ptr<LinphonePrivate::Address> addr = std::make_shared<LinphonePrivate::Address>(linphone_call_get_cpp_obj(call)->getRemoteAddress());
	L_SET_CPP_PTR_FROM_C_STRUCT(call->remoteAddressCache, addr);
	return call->remoteAddressCache;
}

const LinphoneAddress * linphone_call_get_to_address(const LinphoneCall *call){
#if 0
  return (const LinphoneAddress *)sal_op_get_to_address(call->op);
#else
	return nullptr;
#endif
}

const char *linphone_call_get_to_header(const LinphoneCall *call, const char *name){
#if 0
	return sal_custom_header_find(sal_op_get_recv_custom_header(call->op),name);
#else
	return nullptr;
#endif
}

char * linphone_call_get_remote_address_as_string(const LinphoneCall *call) {
	return ms_strdup(linphone_call_get_cpp_obj(call)->getRemoteAddressAsString().c_str());
}

const LinphoneAddress * linphone_call_get_diversion_address(const LinphoneCall *call){
#if 0
	return call->op?(const LinphoneAddress *)sal_op_get_diversion_address(call->op):NULL;
#else
	return nullptr;
#endif
}

LinphoneCallState linphone_call_get_state(const LinphoneCall *call) {
	return linphone_call_get_cpp_obj(call)->getState();
}

LinphoneReason linphone_call_get_reason(const LinphoneCall *call) {
	return linphone_call_get_cpp_obj(call)->getReason();
}

const LinphoneErrorInfo * linphone_call_get_error_info(const LinphoneCall *call) {
	return linphone_call_get_cpp_obj(call)->getErrorInfo();
}

void *linphone_call_get_user_data(const LinphoneCall *call) {
	return call->user_data;
}

void linphone_call_set_user_data(LinphoneCall *call, void *user_pointer) {
	call->user_data = user_pointer;
}

LinphoneCallLog *linphone_call_get_call_log(const LinphoneCall *call){
	return linphone_call_get_cpp_obj(call)->getLog();
}

const char *linphone_call_get_refer_to(const LinphoneCall *call){
#if 0
	return call->refer_to;
#else
	return nullptr;
#endif
}

LinphoneCall *linphone_call_get_transferer_call(const LinphoneCall *call){
#if 0
	return call->referer;
#else
	return nullptr;
#endif
}

LinphoneCall *linphone_call_get_transfer_target_call(const LinphoneCall *call){
#if 0
	return call->transfer_target;
#else
	return nullptr;
#endif
}

LinphoneCallDir linphone_call_get_dir(const LinphoneCall *call) {
	return linphone_call_log_get_dir(linphone_call_get_log(call));
}

const char *linphone_call_get_remote_user_agent(LinphoneCall *call){
#if 0
	if (call->op){
		return sal_op_get_remote_ua (call->op);
	}
	return NULL;
#else
	return nullptr;
#endif
}

const char * linphone_call_get_remote_contact(LinphoneCall *call) {
	std::string contact = linphone_call_get_cpp_obj(call)->getRemoteContact();
	if (contact.empty())
		return nullptr;
	if (call->remoteContactCache)
		bctbx_free(call->remoteContactCache);
	call->remoteContactCache = bctbx_strdup(contact.c_str());
	return call->remoteContactCache;
}

bool_t linphone_call_has_transfer_pending(const LinphoneCall *call){
#if 0
	return call->refer_pending;
#else
	return FALSE;
#endif
}

int linphone_call_get_duration(const LinphoneCall *call) {
	return linphone_call_get_cpp_obj(call)->getDuration();
}

LinphoneCall *linphone_call_get_replaced_call(LinphoneCall *call){
#if 0
	SalOp *op=sal_call_get_replaces(call->op);
	if (op){
		return (LinphoneCall*)sal_op_get_user_pointer(op);
	}
	return NULL;
#else
	return nullptr;
#endif
}

void linphone_call_enable_camera(LinphoneCall *call, bool_t enable) {
	linphone_call_get_cpp_obj(call)->enableCamera(enable);
}

void linphone_call_send_vfu_request(LinphoneCall *call) {
	linphone_call_get_cpp_obj(call)->sendVfuRequest();
}

LinphoneStatus linphone_call_take_video_snapshot(LinphoneCall *call, const char *file) {
	return linphone_call_get_cpp_obj(call)->takeVideoSnapshot(file ? file : "");
}

LinphoneStatus linphone_call_take_preview_snapshot(LinphoneCall *call, const char *file) {
	return linphone_call_get_cpp_obj(call)->takePreviewSnapshot(file ? file : "");
}

bool_t linphone_call_camera_enabled (const LinphoneCall *call) {
	return linphone_call_get_cpp_obj(call)->cameraEnabled();
}

/**
 * @ingroup call_control
 * @return string value of LinphonePrivacy enum
 **/
const char* linphone_privacy_to_string(LinphonePrivacy privacy) {
	switch(privacy) {
	case LinphonePrivacyDefault: return "LinphonePrivacyDefault";
	case LinphonePrivacyUser: return "LinphonePrivacyUser";
	case LinphonePrivacyHeader: return "LinphonePrivacyHeader";
	case LinphonePrivacySession: return "LinphonePrivacySession";
	case LinphonePrivacyId: return "LinphonePrivacyId";
	case LinphonePrivacyNone: return "LinphonePrivacyNone";
	case LinphonePrivacyCritical: return "LinphonePrivacyCritical";
	default: return "Unknown privacy mode";
	}
}

void linphone_call_set_next_video_frame_decoded_callback(LinphoneCall *call, LinphoneCallCbFunc cb, void* user_data) {
	linphone_call_get_cpp_obj(call)->setNextVideoFrameDecodedCallback(cb, user_data);
}

void linphone_call_init_media_streams(LinphoneCall *call){
}

static int dtmf_tab[16]={'0','1','2','3','4','5','6','7','8','9','*','#','A','B','C','D'};

static void linphone_core_dtmf_received(LinphoneCall *call, int dtmf){
	if (dtmf<0 || dtmf>15){
		ms_warning("Bad dtmf value %i",dtmf);
		return;
	}
	linphone_call_notify_dtmf_received(call, dtmf_tab[dtmf]);
}

void set_playback_gain_db(AudioStream *st, float gain){
	if (st->volrecv){
		ms_filter_call_method(st->volrecv,MS_VOLUME_SET_DB_GAIN,&gain);
	}else ms_warning("Could not apply playback gain: gain control wasn't activated.");
}

/*This function is not static because used internally in linphone-daemon project*/
void _post_configure_audio_stream(AudioStream *st, LinphoneCore *lc, bool_t muted){
}

static void setup_ring_player(LinphoneCore *lc, LinphoneCall *call){
	int pause_time=3000;
	audio_stream_play(call->audiostream,lc->sound_conf.ringback_tone);
	ms_filter_call_method(call->audiostream->soundread,MS_FILE_PLAYER_LOOP,&pause_time);
}

static bool_t linphone_call_sound_resources_available(LinphoneCall *call){
	LinphoneCore *lc=call->core;
	LinphoneCall *current=linphone_core_get_current_call(lc);
	return !linphone_core_is_in_conference(lc) &&
		(current==NULL || current==call);
}

void linphone_call_delete_upnp_session(LinphoneCall *call){
}

void linphone_call_stop_media_streams(LinphoneCall *call){
}

void linphone_call_enable_echo_cancellation(LinphoneCall *call, bool_t enable) {
	linphone_call_get_cpp_obj(call)->enableEchoCancellation(enable);
}

bool_t linphone_call_echo_cancellation_enabled(const LinphoneCall *call) {
	return linphone_call_get_cpp_obj(call)->echoCancellationEnabled();
}

void linphone_call_enable_echo_limiter(LinphoneCall *call, bool_t val) {
	linphone_call_get_cpp_obj(call)->enableEchoLimiter(val);
}

bool_t linphone_call_echo_limiter_enabled(const LinphoneCall *call) {
	return linphone_call_get_cpp_obj(call)->echoLimiterEnabled();
}

float linphone_call_get_play_volume(const LinphoneCall *call) {
	return linphone_call_get_cpp_obj(call)->getPlayVolume();
}

float linphone_call_get_record_volume(const LinphoneCall *call) {
	return linphone_call_get_cpp_obj(call)->getRecordVolume();
}

float linphone_call_get_speaker_volume_gain(const LinphoneCall *call) {
	return linphone_call_get_cpp_obj(call)->getSpeakerVolumeGain();
}

void linphone_call_set_speaker_volume_gain(LinphoneCall *call, float volume) {
	linphone_call_get_cpp_obj(call)->setSpeakerVolumeGain(volume);
}

float linphone_call_get_microphone_volume_gain(const LinphoneCall *call) {
	return linphone_call_get_cpp_obj(call)->getMicrophoneVolumeGain();
}

void linphone_call_set_microphone_volume_gain(LinphoneCall *call, float volume) {
	linphone_call_get_cpp_obj(call)->setMicrophoneVolumeGain(volume);
}

float linphone_call_get_current_quality(const LinphoneCall *call) {
	return linphone_call_get_cpp_obj(call)->getCurrentQuality();
}

float linphone_call_get_average_quality(const LinphoneCall *call) {
	return linphone_call_get_cpp_obj(call)->getAverageQuality();
}

void linphone_call_stats_update(LinphoneCallStats *stats, MediaStream *stream) {
	PayloadType *pt;
	RtpSession *session = stream->sessions.rtp_session;
	const MSQualityIndicator *qi = media_stream_get_quality_indicator(stream);
	if (qi) {
		stats->local_late_rate=ms_quality_indicator_get_local_late_rate(qi);
		stats->local_loss_rate=ms_quality_indicator_get_local_loss_rate(qi);
	}
	media_stream_get_local_rtp_stats(stream, &stats->rtp_stats);
	pt = rtp_profile_get_payload(rtp_session_get_profile(session), rtp_session_get_send_payload_type(session));
	stats->clockrate = pt ? pt->clock_rate : 8000;
}

MediaStream * linphone_call_get_stream(LinphoneCall *call, LinphoneStreamType type) {
	return L_GET_PRIVATE(linphone_call_get_cpp_obj(call).get())->getMediaStream(type);
}

void _linphone_call_stats_clone(LinphoneCallStats *dst, const LinphoneCallStats *src) {
	/*
	 * Save the belle_sip_object_t part, copy the entire structure and restore the belle_sip_object_t part
	 */
	belle_sip_object_t tmp = dst->base;
	memcpy(dst, src, sizeof(LinphoneCallStats));
	dst->base = tmp;

	dst->received_rtcp = NULL;
	dst->sent_rtcp = NULL;
}

LinphoneCallStats *linphone_call_get_stats(LinphoneCall *call, LinphoneStreamType type) {
	return linphone_call_get_cpp_obj(call)->getStats(type);
}

LinphoneCallStats *linphone_call_get_audio_stats(LinphoneCall *call) {
	return linphone_call_get_cpp_obj(call)->getAudioStats();
}

LinphoneCallStats *linphone_call_get_video_stats(LinphoneCall *call) {
	return linphone_call_get_cpp_obj(call)->getVideoStats();
}

LinphoneCallStats *linphone_call_get_text_stats(LinphoneCall *call) {
	return linphone_call_get_cpp_obj(call)->getTextStats();
}

bool_t linphone_call_media_in_progress(const LinphoneCall *call) {
	return linphone_call_get_cpp_obj(call)->mediaInProgress();
}

BELLE_SIP_DECLARE_NO_IMPLEMENTED_INTERFACES(LinphoneCallStats);

BELLE_SIP_INSTANCIATE_VPTR(LinphoneCallStats, belle_sip_object_t,
	NULL, // destroy
	_linphone_call_stats_clone, // clone
	NULL, // marshal
	TRUE
);

LinphoneCallStats *linphone_call_stats_new() {
	LinphoneCallStats *stats = belle_sip_object_new(LinphoneCallStats);
	return stats;
}

LinphoneCallStats* linphone_call_stats_ref(LinphoneCallStats* stats) {
	return (LinphoneCallStats*) belle_sip_object_ref(stats);
}

void linphone_call_stats_unref(LinphoneCallStats* stats) {
	belle_sip_object_unref(stats);
}

void *linphone_call_stats_get_user_data(const LinphoneCallStats *stats) {
	return stats->user_data;
}

void linphone_call_stats_set_user_data(LinphoneCallStats *stats, void *data) {
	stats->user_data = data;
}

LinphoneStreamType linphone_call_stats_get_type(const LinphoneCallStats *stats) {
	return stats->type;
}

float linphone_call_stats_get_sender_loss_rate(const LinphoneCallStats *stats) {
	const report_block_t *srb = NULL;

	if (!stats || !stats->sent_rtcp)
		return 0.0;
	/* Perform msgpullup() to prevent crashes in rtcp_is_SR() or rtcp_is_RR() if the RTCP packet is composed of several mblk_t structure */
	if (stats->sent_rtcp->b_cont != NULL)
		msgpullup(stats->sent_rtcp, -1);

	do{
		if (rtcp_is_SR(stats->sent_rtcp))
			srb = rtcp_SR_get_report_block(stats->sent_rtcp, 0);
		else if (rtcp_is_RR(stats->sent_rtcp))
			srb = rtcp_RR_get_report_block(stats->sent_rtcp, 0);
		if (srb) break;
	}while (rtcp_next_packet(stats->sent_rtcp));
	rtcp_rewind(stats->sent_rtcp);
	if (!srb)
		return 0.0;
	return 100.0f * report_block_get_fraction_lost(srb) / 256.0f;
}

float linphone_call_stats_get_receiver_loss_rate(const LinphoneCallStats *stats) {
	const report_block_t *rrb = NULL;

	if (!stats || !stats->received_rtcp)
		return 0.0;
	/* Perform msgpullup() to prevent crashes in rtcp_is_SR() or rtcp_is_RR() if the RTCP packet is composed of several mblk_t structure */
	if (stats->received_rtcp->b_cont != NULL)
		msgpullup(stats->received_rtcp, -1);

	do{
		if (rtcp_is_RR(stats->received_rtcp))
			rrb = rtcp_RR_get_report_block(stats->received_rtcp, 0);
		else if (rtcp_is_SR(stats->received_rtcp))
			rrb = rtcp_SR_get_report_block(stats->received_rtcp, 0);
		if (rrb) break;
	}while (rtcp_next_packet(stats->received_rtcp));
	rtcp_rewind(stats->received_rtcp);
	if (!rrb)
		return 0.0;
	return 100.0f * report_block_get_fraction_lost(rrb) / 256.0f;
}

float linphone_call_stats_get_local_loss_rate(const LinphoneCallStats *stats) {
	return stats->local_loss_rate;
}

float linphone_call_stats_get_local_late_rate(const LinphoneCallStats *stats) {
	return stats->local_late_rate;
}

float linphone_call_stats_get_sender_interarrival_jitter(const LinphoneCallStats *stats) {
	const report_block_t *srb = NULL;

	if (!stats || !stats->sent_rtcp)
		return 0.0;
	/* Perform msgpullup() to prevent crashes in rtcp_is_SR() or rtcp_is_RR() if the RTCP packet is composed of several mblk_t structure */
	if (stats->sent_rtcp->b_cont != NULL)
		msgpullup(stats->sent_rtcp, -1);
	if (rtcp_is_SR(stats->sent_rtcp))
		srb = rtcp_SR_get_report_block(stats->sent_rtcp, 0);
	else if (rtcp_is_RR(stats->sent_rtcp))
		srb = rtcp_RR_get_report_block(stats->sent_rtcp, 0);
	if (!srb)
		return 0.0;
	if (stats->clockrate == 0)
		return 0.0;
	return (float)report_block_get_interarrival_jitter(srb) / (float)stats->clockrate;
}

float linphone_call_stats_get_receiver_interarrival_jitter(const LinphoneCallStats *stats) {
	const report_block_t *rrb = NULL;

	if (!stats || !stats->received_rtcp)
		return 0.0;
	/* Perform msgpullup() to prevent crashes in rtcp_is_SR() or rtcp_is_RR() if the RTCP packet is composed of several mblk_t structure */
	if (stats->received_rtcp->b_cont != NULL)
		msgpullup(stats->received_rtcp, -1);
	if (rtcp_is_SR(stats->received_rtcp))
		rrb = rtcp_SR_get_report_block(stats->received_rtcp, 0);
	else if (rtcp_is_RR(stats->received_rtcp))
		rrb = rtcp_RR_get_report_block(stats->received_rtcp, 0);
	if (!rrb)
		return 0.0;
	if (stats->clockrate == 0)
		return 0.0;
	return (float)report_block_get_interarrival_jitter(rrb) / (float)stats->clockrate;
}

const rtp_stats_t *linphone_call_stats_get_rtp_stats(const LinphoneCallStats *stats) {
	return &stats->rtp_stats;
}

uint64_t linphone_call_stats_get_late_packets_cumulative_number(const LinphoneCallStats *stats) {
	return linphone_call_stats_get_rtp_stats(stats)->outoftime;
}

float linphone_call_stats_get_download_bandwidth(const LinphoneCallStats *stats) {
	return stats->download_bandwidth;
}

float linphone_call_stats_get_upload_bandwidth(const LinphoneCallStats *stats) {
	return stats->upload_bandwidth;
}

LinphoneIceState linphone_call_stats_get_ice_state(const LinphoneCallStats *stats) {
	return stats->ice_state;
}

LinphoneUpnpState linphone_call_stats_get_upnp_state(const LinphoneCallStats *stats) {
	return stats->upnp_state;
}

LinphoneAddressFamily linphone_call_stats_get_ip_family_of_remote(const LinphoneCallStats *stats) {
	return (LinphoneAddressFamily)stats->rtp_remote_family;
}

float linphone_call_stats_get_jitter_buffer_size_ms(const LinphoneCallStats *stats) {
	return stats->jitter_stats.jitter_buffer_size_ms;
}

float linphone_call_stats_get_round_trip_delay(const LinphoneCallStats *stats) {
	return stats->round_trip_delay;
}

void linphone_call_start_recording(LinphoneCall *call) {
	linphone_call_get_cpp_obj(call)->startRecording();
}

void linphone_call_stop_recording(LinphoneCall *call) {
	linphone_call_get_cpp_obj(call)->stopRecording();
}

static void linphone_call_lost(LinphoneCall *call){
	LinphoneCore *lc = call->core;
	char *temp = NULL;
	char *from = NULL;

	from = linphone_call_get_remote_address_as_string(call);
	temp = ms_strdup_printf("Media connectivity with %s is lost, call is going to be closed.", from ? from : "?");
	if (from) ms_free(from);
	ms_message("LinphoneCall [%p]: %s", call, temp);
	linphone_core_notify_display_warning(lc, temp);
	call->non_op_error = TRUE;
	linphone_error_info_set(call->ei,NULL, LinphoneReasonIOError, 503, "Media lost", NULL);
	linphone_call_terminate(call);
	linphone_core_play_named_tone(lc, LinphoneToneCallLost);
	ms_free(temp);
}

/*do not change the prototype of this function, it is also used internally in linphone-daemon.*/
void linphone_call_stats_fill(LinphoneCallStats *stats, MediaStream *ms, OrtpEvent *ev){
	OrtpEventType evt=ortp_event_get_type(ev);
	OrtpEventData *evd=ortp_event_get_data(ev);

	if (evt == ORTP_EVENT_RTCP_PACKET_RECEIVED) {
		stats->round_trip_delay = rtp_session_get_round_trip_propagation(ms->sessions.rtp_session);
		if(stats->received_rtcp != NULL)
			freemsg(stats->received_rtcp);
		stats->received_rtcp = evd->packet;
		stats->rtcp_received_via_mux = evd->info.socket_type == OrtpRTPSocket;
		evd->packet = NULL;
		stats->updated = LINPHONE_CALL_STATS_RECEIVED_RTCP_UPDATE;
		linphone_call_stats_update(stats,ms);
	} else if (evt == ORTP_EVENT_RTCP_PACKET_EMITTED) {
		memcpy(&stats->jitter_stats, rtp_session_get_jitter_stats(ms->sessions.rtp_session), sizeof(jitter_stats_t));
		if (stats->sent_rtcp != NULL)
			freemsg(stats->sent_rtcp);
		stats->sent_rtcp = evd->packet;
		evd->packet = NULL;
		stats->updated = LINPHONE_CALL_STATS_SENT_RTCP_UPDATE;
		linphone_call_stats_update(stats,ms);
	}
}

void linphone_call_stats_uninit(LinphoneCallStats *stats){
	if (stats->received_rtcp) {
		freemsg(stats->received_rtcp);
		stats->received_rtcp=NULL;
	}
	if (stats->sent_rtcp){
		freemsg(stats->sent_rtcp);
		stats->sent_rtcp=NULL;
	}
}

LinphoneCallState linphone_call_get_transfer_state(LinphoneCall *call) {
#if 0
	return call->transfer_state;
#else
	return LinphoneCallIdle;
#endif
}

void linphone_call_set_transfer_state(LinphoneCall* call, LinphoneCallState state) {
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

bool_t linphone_call_is_in_conference(const LinphoneCall *call) {
#if 0
	return linphone_call_params_get_in_conference(call->params);
#else
	return FALSE;
#endif
}

LinphoneConference *linphone_call_get_conference(const LinphoneCall *call) {
#if 0
	return call->conf_ref;
#else
	return nullptr;
#endif
}

void linphone_call_zoom_video(LinphoneCall* call, float zoom_factor, float* cx, float* cy) {
	linphone_call_get_cpp_obj(call)->zoomVideo(zoom_factor, cx, cy);
}

LinphonePlayer *linphone_call_get_player(LinphoneCall *call){
#if 0
	if (call->player==NULL)
		call->player=linphone_call_build_player(call);
	return call->player;
#else
	return nullptr;
#endif
}


void linphone_call_set_params(LinphoneCall *call, const LinphoneCallParams *params){
#if 0
	if ( call->state == LinphoneCallOutgoingInit || call->state == LinphoneCallIncomingReceived){
		_linphone_call_set_new_params(call, params);
	}
	else {
		ms_error("linphone_call_set_params() invalid state %s to call this function", linphone_call_state_to_string(call->state));
	}
#endif
}


void _linphone_call_set_new_params(LinphoneCall *call, const LinphoneCallParams *params){
}

const LinphoneCallParams * linphone_call_get_params(LinphoneCall *call) {
	call->paramsCache->msp = linphone_call_get_cpp_obj(call)->getParams();
	return call->paramsCache;
}


static int send_dtmf_handler(void *data, unsigned int revents){
	LinphoneCall *call = (LinphoneCall*)data;
	/*By default we send DTMF RFC2833 if we do not have enabled SIP_INFO but we can also send RFC2833 and SIP_INFO*/
	if (linphone_core_get_use_rfc2833_for_dtmf(call->core)!=0 || linphone_core_get_use_info_for_dtmf(call->core)==0)
	{
		/* In Band DTMF */
		if (call->audiostream!=NULL){
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
	if (call->dtmfs_timer!=NULL) {
		memmove(call->dtmf_sequence, call->dtmf_sequence+1, strlen(call->dtmf_sequence));
	}
	/* continue only if the dtmf sequence is not empty*/
	if (call->dtmf_sequence!=NULL&&*call->dtmf_sequence!='\0') {
		return TRUE;
	} else {
		linphone_call_cancel_dtmfs(call);
		return FALSE;
	}
}

LinphoneStatus linphone_call_send_dtmf(LinphoneCall *call, char dtmf) {
#if 0
	if (call==NULL){
		ms_warning("linphone_call_send_dtmf(): invalid call, canceling DTMF.");
		return -1;
	}
	call->dtmf_sequence = &dtmf;
	send_dtmf_handler(call,0);
	call->dtmf_sequence = NULL;
	return 0;
#else
	return 0;
#endif
}

LinphoneStatus linphone_call_send_dtmfs(LinphoneCall *call,const char *dtmfs) {
#if 0
	if (call==NULL){
		ms_warning("linphone_call_send_dtmfs(): invalid call, canceling DTMF sequence.");
		return -1;
	}
	if (call->dtmfs_timer!=NULL){
		ms_warning("linphone_call_send_dtmfs(): a DTMF sequence is already in place, canceling DTMF sequence.");
		return -2;
	}
	if (dtmfs != NULL) {
		int delay_ms = lp_config_get_int(call->core->config,"net","dtmf_delay_ms",200);
		call->dtmf_sequence = ms_strdup(dtmfs);
		call->dtmfs_timer = sal_create_timer(call->core->sal, send_dtmf_handler, call, delay_ms, "DTMF sequence timer");
	}
	return 0;
#else
	return 0;
#endif
}

void linphone_call_cancel_dtmfs(LinphoneCall *call) {
#if 0
	/*nothing to do*/
	if (!call || !call->dtmfs_timer) return;

	sal_cancel_timer(call->core->sal, call->dtmfs_timer);
	belle_sip_object_unref(call->dtmfs_timer);
	call->dtmfs_timer = NULL;
	if (call->dtmf_sequence != NULL) {
		ms_free(call->dtmf_sequence);
		call->dtmf_sequence = NULL;
	}
#endif
}

void * linphone_call_get_native_video_window_id(const LinphoneCall *call) {
	return linphone_call_get_cpp_obj(call)->getNativeVideoWindowId();
}

void linphone_call_set_native_video_window_id(LinphoneCall *call, void *id) {
	linphone_call_get_cpp_obj(call)->setNativeVideoWindowId(id);
}

void linphone_call_set_audio_route(LinphoneCall *call, LinphoneAudioRoute route) {
#if 0
	if (call != NULL && call->audiostream != NULL){
		audio_stream_set_audio_route(call->audiostream, (MSAudioRoute) route);
	}
#endif
}

LinphoneChatRoom * linphone_call_get_chat_room(LinphoneCall *call) {
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

int linphone_call_get_stream_count(const LinphoneCall *call) {
	return linphone_call_get_cpp_obj(call)->getStreamCount();
}

MSFormatType linphone_call_get_stream_type(const LinphoneCall *call, int stream_index) {
	return linphone_call_get_cpp_obj(call)->getStreamType(stream_index);
}

RtpTransport * linphone_call_get_meta_rtp_transport(const LinphoneCall *call, int stream_index) {
	return linphone_call_get_cpp_obj(call)->getMetaRtpTransport(stream_index);
}

RtpTransport * linphone_call_get_meta_rtcp_transport(const LinphoneCall *call, int stream_index) {
	return linphone_call_get_cpp_obj(call)->getMetaRtcpTransport(stream_index);
}

LinphoneStatus linphone_call_pause(LinphoneCall *call) {
#if 0
	int err = _linphone_call_pause(call);
	if (err == 0) call->paused_by_app = TRUE;
	return err;
#else
	return 0;
#endif
}

/* Internal version that does not play tone indication*/
int _linphone_call_pause(LinphoneCall *call) {
#if 0
	LinphoneCore *lc;
	const char *subject = NULL;

	if ((call->state != LinphoneCallStreamsRunning) && (call->state != LinphoneCallPausedByRemote)) {
		ms_warning("Cannot pause this call, it is not active.");
		return -1;
	}
	if (sal_media_description_has_dir(call->resultdesc, SalStreamSendRecv)) {
		subject = "Call on hold";
	} else if (sal_media_description_has_dir(call->resultdesc, SalStreamRecvOnly)) {
		subject = "Call on hold for me too";
	} else {
		ms_error("No reason to pause this call, it is already paused or inactive.");
		return -1;
	}

	lc = linphone_call_get_core(call);
	call->broken = FALSE;
	linphone_call_set_state(call, LinphoneCallPausing, "Pausing call");
	linphone_call_make_local_media_description(call);
	sal_call_set_local_media_description(call->op, call->localdesc);
	if (sal_call_update(call->op, subject, FALSE) != 0) {
		linphone_core_notify_display_warning(lc, _("Could not pause the call"));
	}
	lc->current_call = NULL;
	linphone_core_notify_display_status(lc, _("Pausing the current call..."));
	if (call->audiostream || call->videostream || call->textstream)
		linphone_call_stop_media_streams(call);
	call->paused_by_app = FALSE;
	return 0;
#else
	return 0;
#endif
}

LinphoneStatus linphone_call_resume(LinphoneCall *call) {
#if 0
	LinphoneCore *lc;
	const char *subject = "Call resuming";
	char *remote_address;
	char *display_status;

	if (call->state != LinphoneCallPaused) {
		ms_warning("we cannot resume a call that has not been established and paused before");
		return -1;
	}
	lc = linphone_call_get_core(call);
	if (linphone_call_params_get_in_conference(call->params) == FALSE) {
		if (linphone_core_sound_resources_locked(lc)) {
			ms_warning("Cannot resume call %p because another call is locking the sound resources.", call);
			return -1;
		}
		linphone_core_preempt_sound_resources(lc);
		ms_message("Resuming call %p", call);
	}

	call->was_automatically_paused = FALSE;
	call->broken = FALSE;

	/* Stop playing music immediately. If remote side is a conference it
	 prevents the participants to hear it while the 200OK comes back. */
	if (call->audiostream) audio_stream_play(call->audiostream, NULL);

	linphone_call_make_local_media_description(call);
	if (!lc->sip_conf.sdp_200_ack) {
		sal_call_set_local_media_description(call->op, call->localdesc);
	} else {
		sal_call_set_local_media_description(call->op, NULL);
	}
	sal_media_description_set_dir(call->localdesc, SalStreamSendRecv);
	if (linphone_call_params_get_in_conference(call->params) && !linphone_call_params_get_in_conference(call->current_params)) subject = "Conference";
	if (sal_call_update(call->op, subject, FALSE) != 0) {
		return -1;
	}
	linphone_call_set_state(call, LinphoneCallResuming,"Resuming");
	if (linphone_call_params_get_in_conference(call->params) == FALSE)
		lc->current_call = call;
	remote_address = linphone_call_get_remote_address_as_string(call);
	display_status = ms_strdup_printf("Resuming the call with with %s", remote_address);
	ms_free(remote_address);
	linphone_core_notify_display_status(lc, display_status);
	ms_free(display_status);

	if (lc->sip_conf.sdp_200_ack) {
		/* We are NOT offering, set local media description after sending the call so that we are ready to
		 process the remote offer when it will arrive. */
		sal_call_set_local_media_description(call->op, call->localdesc);
	}
	return 0;
#else
	return 0;
#endif
}

static void terminate_call(LinphoneCall *call) {
}

LinphoneStatus linphone_call_terminate(LinphoneCall *call) {
	return linphone_call_get_cpp_obj(call)->terminate();
}

LinphoneStatus linphone_call_terminate_with_error_info(LinphoneCall *call , const LinphoneErrorInfo *ei) {
	return linphone_call_get_cpp_obj(call)->terminate(ei);
}

LinphoneStatus linphone_call_redirect(LinphoneCall *call, const char *redirect_uri) {
#if 0
	char *real_url = NULL;
	LinphoneCore *lc;
	LinphoneAddress *real_parsed_url;
	SalErrorInfo sei;

	if (call->state != LinphoneCallIncomingReceived) {
		ms_error("Bad state for call redirection.");
		return -1;
	}

	lc = linphone_call_get_core(call);
	real_parsed_url = linphone_core_interpret_url(lc, redirect_uri);
	if (!real_parsed_url) {
		/* Bad url */
		ms_error("Bad redirect URI: %s", redirect_uri ? redirect_uri : "NULL");
		return -1;
	}

	memset(&sei, 0, sizeof(sei));
	real_url = linphone_address_as_string(real_parsed_url);
	sal_error_info_set(&sei,SalReasonRedirect, "SIP", 0, NULL, NULL);
	sal_call_decline_with_error_info(call->op, &sei, real_url);
	ms_free(real_url);
	linphone_error_info_set(call->ei, NULL, LinphoneReasonMovedPermanently, 302, "Call redirected", NULL);
	call->non_op_error = TRUE;
	terminate_call(call);
	linphone_address_unref(real_parsed_url);
	sal_error_info_reset(&sei);
	return 0;
#else
	return 0;
#endif
}

LinphoneStatus linphone_call_decline(LinphoneCall *call, LinphoneReason reason) {
	return linphone_call_get_cpp_obj(call)->decline(reason);
}

LinphoneStatus linphone_call_decline_with_error_info(LinphoneCall *call, const LinphoneErrorInfo *ei) {
	return linphone_call_get_cpp_obj(call)->decline(ei);
}

LinphoneStatus linphone_call_accept(LinphoneCall *call) {
	return linphone_call_get_cpp_obj(call)->accept(nullptr);
}

LinphoneStatus linphone_call_accept_with_params(LinphoneCall *call, const LinphoneCallParams *params) {
	return linphone_call_get_cpp_obj(call)->accept(params ? linphone_call_params_get_cpp_obj(params) : nullptr);
}

LinphoneStatus linphone_call_accept_early_media(LinphoneCall* call) {
	return linphone_call_get_cpp_obj(call)->acceptEarlyMedia();
}

LinphoneStatus linphone_call_accept_early_media_with_params(LinphoneCall *call, const LinphoneCallParams *params) {
	return linphone_call_get_cpp_obj(call)->acceptEarlyMedia(params ? linphone_call_params_get_cpp_obj(params) : nullptr);
}

LinphoneStatus linphone_call_update(LinphoneCall *call, const LinphoneCallParams *params) {
	return linphone_call_get_cpp_obj(call)->update(params ? linphone_call_params_get_cpp_obj(params) : nullptr);
}

int linphone_call_start_update(LinphoneCall *call) {
	return 0;
}

LinphoneStatus linphone_call_defer_update(LinphoneCall *call) {
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

int linphone_call_start_accept_update(LinphoneCall *call, LinphoneCallState next_state, const char *state_info) {
	return 0;
}

LinphoneStatus linphone_call_accept_update(LinphoneCall *call, const LinphoneCallParams *params) {
	return linphone_call_get_cpp_obj(call)->acceptUpdate(params ? linphone_call_params_get_cpp_obj(params) : nullptr);
}

LinphoneStatus linphone_call_transfer(LinphoneCall *call, const char *refer_to) {
#if 0
	char *real_url = NULL;
	LinphoneCore *lc = linphone_call_get_core(call);
	LinphoneAddress *real_parsed_url = linphone_core_interpret_url(lc, refer_to);

	if (!real_parsed_url) {
		/* bad url */
		return -1;
	}
	//lc->call = NULL; // Do not do that you will lose the call afterward...
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

LinphoneStatus linphone_call_transfer_to_another(LinphoneCall *call, LinphoneCall *dest) {
#if 0
	int result = sal_call_refer_with_replaces (call->op, dest->op);
	linphone_call_set_transfer_state(call, LinphoneCallOutgoingInit);
	return result;
#else
	return 0;
#endif
}

int linphone_call_abort(LinphoneCall *call, const char *error) {
#if 0
	LinphoneCore *lc = linphone_call_get_core(call);

	sal_call_terminate(call->op);

	/* Stop ringing */
	linphone_core_stop_ringing(lc);
	linphone_call_stop_media_streams(call);

	linphone_core_notify_display_status(lc, _("Call aborted"));
	linphone_call_set_state(call, LinphoneCallError, error);
	return 0;
#else
	return 0;
#endif
}

int linphone_call_proceed_with_invite_if_ready(LinphoneCall *call, LinphoneProxyConfig *dest_proxy) {
	return 0;
}

int linphone_call_start_invite(LinphoneCall *call, const LinphoneAddress *destination /* = NULL if to be taken from the call log */) {
	return 0;
}

void linphone_call_set_broken(LinphoneCall *call){
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

static void linphone_call_repair_by_invite_with_replaces(LinphoneCall *call) {
	const char *call_id = sal_op_get_call_id(call->op);
	const char *from_tag = sal_call_get_local_tag(call->op);
	const char *to_tag = sal_call_get_remote_tag(call->op);
	sal_op_kill_dialog(call->op);
	linphone_call_create_op(call);
	sal_call_set_replaces(call->op, call_id, from_tag, to_tag);
	linphone_call_start_invite(call, NULL);
}

void linphone_call_reinvite_to_recover_from_connection_loss(LinphoneCall *call) {
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

void linphone_call_repair_if_broken(LinphoneCall *call){
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
				sal_error_info_set(&sei, SalReasonServiceUnavailable,"SIP", 0, NULL, NULL);
				sal_call_decline_with_error_info(call->op, &sei,NULL);
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

void linphone_call_refresh_sockets(LinphoneCall *call){
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

void linphone_call_replace_op(LinphoneCall *call, SalOp *op) {
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
			sal_op_set_user_pointer(oldop, NULL); /* To make the call does not get terminated by terminating this op. */
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

void linphone_call_ogl_render(const LinphoneCall *call) {
#if 0
	#ifdef VIDEO_ENABLED

	VideoStream *stream = call->videostream;
	if (stream && stream->output && ms_filter_get_id(stream->output) == MS_OGL_ID)
		ms_filter_call_method(stream->output, MS_OGL_RENDER, NULL);

	#endif
#endif
}

void linphone_call_add_callbacks(LinphoneCall *call, LinphoneCallCbs *cbs) {
	call->callbacks = bctbx_list_append(call->callbacks, linphone_call_cbs_ref(cbs));
}

void linphone_call_remove_callbacks(LinphoneCall *call, LinphoneCallCbs *cbs) {
	call->callbacks = bctbx_list_remove(call->callbacks, cbs);
	linphone_call_cbs_unref(cbs);
}

LinphoneCallCbs *linphone_call_get_current_callbacks(const LinphoneCall *call) {
	return call->current_cbs;
}

#define NOTIFY_IF_EXIST(function_name, ...) \
	bctbx_list_t* iterator; \
	for (iterator = call->callbacks; iterator != NULL; iterator = bctbx_list_next(iterator)) { \
		call->current_cbs = (LinphoneCallCbs *)bctbx_list_get_data(iterator); \
		if (call->current_cbs->function_name != NULL) { \
			call->current_cbs->function_name(__VA_ARGS__); \
		} \
	}

void linphone_call_notify_state_changed(LinphoneCall *call, LinphoneCallState cstate, const char *message) {
	NOTIFY_IF_EXIST(state_changed_cb, call, cstate, message)
	linphone_core_notify_call_state_changed(linphone_call_get_core(call), call, cstate, message);
}

void linphone_call_notify_dtmf_received(LinphoneCall *call, int dtmf) {
	NOTIFY_IF_EXIST(dtmf_received_cb, call, dtmf)
	linphone_core_notify_dtmf_received(linphone_call_get_core(call), call, dtmf);
}

void linphone_call_notify_encryption_changed(LinphoneCall *call, bool_t on, const char *authentication_token) {
	NOTIFY_IF_EXIST(encryption_changed_cb, call, on, authentication_token)
	linphone_core_notify_call_encryption_changed(linphone_call_get_core(call), call, on, authentication_token);
}

void linphone_call_notify_transfer_state_changed(LinphoneCall *call, LinphoneCallState cstate) {
	NOTIFY_IF_EXIST(transfer_state_changed_cb, call, cstate)
	linphone_core_notify_transfer_state_changed(linphone_call_get_core(call), call, cstate);
}

void linphone_call_notify_stats_updated(LinphoneCall *call, const LinphoneCallStats *stats) {
	NOTIFY_IF_EXIST(stats_updated_cb, call, stats)
	linphone_core_notify_call_stats_updated(linphone_call_get_core(call), call, stats);
}

void linphone_call_notify_info_message_received(LinphoneCall *call, const LinphoneInfoMessage *msg) {
	NOTIFY_IF_EXIST(info_message_received_cb, call, msg)
	linphone_core_notify_info_received(linphone_call_get_core(call), call, msg);
}

void linphone_call_notify_ack_processing(LinphoneCall *call, LinphoneHeaders *msg, bool_t is_received) {
	NOTIFY_IF_EXIST(ack_processing, call, msg, is_received)
}

SalOp * linphone_call_get_op(const LinphoneCall *call) {
	return L_GET_PRIVATE(linphone_call_get_cpp_obj(call).get())->getOp();
}

LinphoneProxyConfig * linphone_call_get_dest_proxy(const LinphoneCall *call) {
	return L_GET_PRIVATE(linphone_call_get_cpp_obj(call).get())->getDestProxy();
}

LinphoneCallLog * linphone_call_get_log(const LinphoneCall *call) {
	return linphone_call_get_call_log(call);
}

IceSession * linphone_call_get_ice_session(const LinphoneCall *call) {
	return L_GET_PRIVATE(linphone_call_get_cpp_obj(call).get())->getIceSession();
}

bool_t linphone_call_get_audio_muted(const LinphoneCall *call) {
	return L_GET_PRIVATE(linphone_call_get_cpp_obj(call).get())->getAudioMuted();
}

void linphone_call_set_audio_muted(LinphoneCall *call, bool_t value) {
	L_GET_PRIVATE(linphone_call_get_cpp_obj(call).get())->setAudioMuted(value);
}

bool_t linphone_call_get_all_muted(const LinphoneCall *call) {
	return linphone_call_get_cpp_obj(call)->getAllMuted();
}

std::shared_ptr<LinphonePrivate::Call> linphone_call_get_cpp_obj(const LinphoneCall *call) {
	return call->call;
}
