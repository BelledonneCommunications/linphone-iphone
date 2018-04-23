/*
 * c-call-params.cpp
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

#include "c-wrapper/c-wrapper.h"
#include "call/call-p.h"
#include "core/core.h"
#include "conference/params/call-session-params-p.h"
#include "conference/params/media-session-params-p.h"
#include "conference/session/call-session.h"

#include "linphone/call_params.h"

// =============================================================================

L_DECLARE_C_CLONABLE_OBJECT_IMPL(CallParams)

using namespace std;

// =============================================================================
// Internal functions.
// =============================================================================

SalMediaProto get_proto_from_call_params (const LinphoneCallParams *params) {
	return L_GET_CPP_PTR_FROM_C_OBJECT(params)->getMediaProto();
}

SalStreamDir sal_dir_from_call_params_dir (LinphoneMediaDirection cpdir) {
	switch (cpdir) {
		case LinphoneMediaDirectionInactive:
			return SalStreamInactive;
		case LinphoneMediaDirectionSendOnly:
			return SalStreamSendOnly;
		case LinphoneMediaDirectionRecvOnly:
			return SalStreamRecvOnly;
		case LinphoneMediaDirectionSendRecv:
			return SalStreamSendRecv;
		case LinphoneMediaDirectionInvalid:
			ms_error("LinphoneMediaDirectionInvalid shall not be used.");
			return SalStreamInactive;
	}
	return SalStreamSendRecv;
}

LinphoneMediaDirection media_direction_from_sal_stream_dir (SalStreamDir dir) {
	switch (dir) {
		case SalStreamInactive:
			return LinphoneMediaDirectionInactive;
		case SalStreamSendOnly:
			return LinphoneMediaDirectionSendOnly;
		case SalStreamRecvOnly:
			return LinphoneMediaDirectionRecvOnly;
		case SalStreamSendRecv:
			return LinphoneMediaDirectionSendRecv;
	}
	return LinphoneMediaDirectionSendRecv;
}

SalStreamDir get_audio_dir_from_call_params (const LinphoneCallParams *params) {
	return sal_dir_from_call_params_dir(linphone_call_params_get_audio_direction(params));
}

SalStreamDir get_video_dir_from_call_params (const LinphoneCallParams *params) {
	return sal_dir_from_call_params_dir(linphone_call_params_get_video_direction(params));
}

void linphone_call_params_set_custom_headers (LinphoneCallParams *params, const SalCustomHeader *ch) {
	L_GET_PRIVATE_FROM_C_OBJECT(params)->setCustomHeaders(ch);
}

void linphone_call_params_set_custom_sdp_attributes (LinphoneCallParams *params, const SalCustomSdpAttribute *csa) {
	L_GET_PRIVATE_FROM_C_OBJECT(params)->setCustomSdpAttributes(csa);
}

void linphone_call_params_set_custom_sdp_media_attributes (LinphoneCallParams *params, LinphoneStreamType type, const SalCustomSdpAttribute *csa) {
	L_GET_PRIVATE_FROM_C_OBJECT(params)->setCustomSdpMediaAttributes(type, csa);
}

// =============================================================================
// Public functions.
// =============================================================================

void linphone_call_params_add_custom_header (LinphoneCallParams *params, const char *header_name, const char *header_value) {
	L_GET_CPP_PTR_FROM_C_OBJECT(params)->addCustomHeader(header_name, L_C_TO_STRING(header_value));
}

void linphone_call_params_add_custom_sdp_attribute (LinphoneCallParams *params, const char *attribute_name, const char *attribute_value) {
	L_GET_CPP_PTR_FROM_C_OBJECT(params)->addCustomSdpAttribute(attribute_name, L_C_TO_STRING(attribute_value));
}

void linphone_call_params_add_custom_sdp_media_attribute (LinphoneCallParams *params, LinphoneStreamType type, const char *attribute_name, const char *attribute_value) {
	L_GET_CPP_PTR_FROM_C_OBJECT(params)->addCustomSdpMediaAttribute(type, attribute_name, L_C_TO_STRING(attribute_value));
}

void linphone_call_params_clear_custom_sdp_attributes (LinphoneCallParams *params) {
	L_GET_CPP_PTR_FROM_C_OBJECT(params)->clearCustomSdpAttributes();
}

void linphone_call_params_clear_custom_sdp_media_attributes (LinphoneCallParams *params, LinphoneStreamType type) {
	L_GET_CPP_PTR_FROM_C_OBJECT(params)->clearCustomSdpMediaAttributes(type);
}

LinphoneCallParams *linphone_call_params_copy (const LinphoneCallParams *params) {
	return (LinphoneCallParams *)belle_sip_object_clone((const belle_sip_object_t *)params);
}

bool_t linphone_call_params_early_media_sending_enabled (const LinphoneCallParams *params) {
	return L_GET_CPP_PTR_FROM_C_OBJECT(params)->earlyMediaSendingEnabled();
}

void linphone_call_params_enable_early_media_sending (LinphoneCallParams *params, bool_t enabled) {
	L_GET_CPP_PTR_FROM_C_OBJECT(params)->enableEarlyMediaSending(!!enabled);
}

void linphone_call_params_enable_low_bandwidth (LinphoneCallParams *params, bool_t enabled) {
	L_GET_CPP_PTR_FROM_C_OBJECT(params)->enableLowBandwidth(!!enabled);
}

void linphone_call_params_enable_audio (LinphoneCallParams *params, bool_t enabled) {
	L_GET_CPP_PTR_FROM_C_OBJECT(params)->enableAudio(!!enabled);
}

LinphoneStatus linphone_call_params_enable_realtime_text (LinphoneCallParams *params, bool_t yesno) {
	L_GET_CPP_PTR_FROM_C_OBJECT(params)->enableRealtimeText(!!yesno);
	return 0;
}

void linphone_call_params_enable_video (LinphoneCallParams *params, bool_t enabled) {
	L_GET_CPP_PTR_FROM_C_OBJECT(params)->enableVideo(!!enabled);
}

const char *linphone_call_params_get_custom_header (const LinphoneCallParams *params, const char *header_name) {
	return L_GET_CPP_PTR_FROM_C_OBJECT(params)->getCustomHeader(header_name);
}

const char *linphone_call_params_get_custom_sdp_attribute (const LinphoneCallParams *params, const char *attribute_name) {
	return L_GET_CPP_PTR_FROM_C_OBJECT(params)->getCustomSdpAttribute(attribute_name);
}

const char *linphone_call_params_get_custom_sdp_media_attribute (const LinphoneCallParams *params, LinphoneStreamType type, const char *attribute_name) {
	return L_GET_CPP_PTR_FROM_C_OBJECT(params)->getCustomSdpMediaAttribute(type, attribute_name);
}

bool_t linphone_call_params_get_local_conference_mode (const LinphoneCallParams *params) {
	return linphone_call_params_get_in_conference(params);
}

LinphoneMediaEncryption linphone_call_params_get_media_encryption (const LinphoneCallParams *params) {
	return L_GET_CPP_PTR_FROM_C_OBJECT(params)->getMediaEncryption();
}

LinphonePrivacyMask linphone_call_params_get_privacy (const LinphoneCallParams *params) {
	return L_GET_CPP_PTR_FROM_C_OBJECT(params)->getPrivacy();
}

float linphone_call_params_get_received_framerate (const LinphoneCallParams *params) {
	return L_GET_CPP_PTR_FROM_C_OBJECT(params)->getReceivedFps();
}

MSVideoSize linphone_call_params_get_received_video_size (const LinphoneCallParams *params) {
	MSVideoSize vsize;
	LinphoneVideoDefinition *vdef = L_GET_CPP_PTR_FROM_C_OBJECT(params)->getReceivedVideoDefinition();
	if (vdef) {
		vsize.width = static_cast<int>(linphone_video_definition_get_width(vdef));
		vsize.height = static_cast<int>(linphone_video_definition_get_height(vdef));
	} else {
		vsize.width = MS_VIDEO_SIZE_UNKNOWN_W;
		vsize.height = MS_VIDEO_SIZE_UNKNOWN_H;
	}
	return vsize;
}

const LinphoneVideoDefinition *linphone_call_params_get_received_video_definition (const LinphoneCallParams *params) {
	return L_GET_CPP_PTR_FROM_C_OBJECT(params)->getReceivedVideoDefinition();
}

const char *linphone_call_params_get_record_file (const LinphoneCallParams *params) {
	return L_STRING_TO_C(L_GET_CPP_PTR_FROM_C_OBJECT(params)->getRecordFilePath());
}

const char *linphone_call_params_get_rtp_profile (const LinphoneCallParams *params) {
	return L_GET_CPP_PTR_FROM_C_OBJECT(params)->getRtpProfile();
}

float linphone_call_params_get_sent_framerate (const LinphoneCallParams *params) {
	return L_GET_CPP_PTR_FROM_C_OBJECT(params)->getSentFps();
}

MSVideoSize linphone_call_params_get_sent_video_size (const LinphoneCallParams *params) {
	MSVideoSize vsize;
	LinphoneVideoDefinition *vdef = L_GET_CPP_PTR_FROM_C_OBJECT(params)->getSentVideoDefinition();
	if (vdef) {
		vsize.width = static_cast<int>(linphone_video_definition_get_width(vdef));
		vsize.height = static_cast<int>(linphone_video_definition_get_height(vdef));
	} else {
		vsize.width = MS_VIDEO_SIZE_UNKNOWN_W;
		vsize.height = MS_VIDEO_SIZE_UNKNOWN_H;
	}
	return vsize;
}

const LinphoneVideoDefinition *linphone_call_params_get_sent_video_definition (const LinphoneCallParams *params) {
	return L_GET_CPP_PTR_FROM_C_OBJECT(params)->getSentVideoDefinition();
}

const char *linphone_call_params_get_session_name (const LinphoneCallParams *params) {
	return L_STRING_TO_C(L_GET_CPP_PTR_FROM_C_OBJECT(params)->getSessionName());
}

LinphonePayloadType *linphone_call_params_get_used_audio_payload_type (const LinphoneCallParams *params) {
	return L_GET_CPP_PTR_FROM_C_OBJECT(params)->getUsedAudioPayloadType();
}

LinphonePayloadType *linphone_call_params_get_used_video_payload_type (const LinphoneCallParams *params) {
	return L_GET_CPP_PTR_FROM_C_OBJECT(params)->getUsedVideoPayloadType();
}

LinphonePayloadType *linphone_call_params_get_used_text_payload_type (const LinphoneCallParams *params) {
	return L_GET_CPP_PTR_FROM_C_OBJECT(params)->getUsedRealtimeTextPayloadType();
}

const OrtpPayloadType *linphone_call_params_get_used_audio_codec (const LinphoneCallParams *params) {
	return L_GET_CPP_PTR_FROM_C_OBJECT(params)->getUsedAudioCodec();
}

void linphone_call_params_set_used_audio_codec (LinphoneCallParams *params, OrtpPayloadType *codec) {
	L_GET_PRIVATE_FROM_C_OBJECT(params)->setUsedAudioCodec(codec);
}

const OrtpPayloadType *linphone_call_params_get_used_video_codec (const LinphoneCallParams *params) {
	return L_GET_CPP_PTR_FROM_C_OBJECT(params)->getUsedVideoCodec();
}

void linphone_call_params_set_used_video_codec (LinphoneCallParams *params, OrtpPayloadType *codec) {
	L_GET_PRIVATE_FROM_C_OBJECT(params)->setUsedVideoCodec(codec);
}

const OrtpPayloadType *linphone_call_params_get_used_text_codec (const LinphoneCallParams *params) {
	return L_GET_CPP_PTR_FROM_C_OBJECT(params)->getUsedRealtimeTextCodec();
}

void linphone_call_params_set_used_text_codec (LinphoneCallParams *params, OrtpPayloadType *codec) {
	L_GET_PRIVATE_FROM_C_OBJECT(params)->setUsedRealtimeTextCodec(codec);
}

bool_t linphone_call_params_low_bandwidth_enabled (const LinphoneCallParams *params) {
	return L_GET_CPP_PTR_FROM_C_OBJECT(params)->lowBandwidthEnabled();
}

int linphone_call_params_get_audio_bandwidth_limit (const LinphoneCallParams *params) {
	return L_GET_CPP_PTR_FROM_C_OBJECT(params)->getAudioBandwidthLimit();
}

void linphone_call_params_set_audio_bandwidth_limit (LinphoneCallParams *params, int bandwidth) {
	L_GET_CPP_PTR_FROM_C_OBJECT(params)->setAudioBandwidthLimit(bandwidth);
}

void linphone_call_params_set_media_encryption (LinphoneCallParams *params, LinphoneMediaEncryption encryption) {
	L_GET_CPP_PTR_FROM_C_OBJECT(params)->setMediaEncryption(encryption);
}

void linphone_call_params_set_privacy (LinphoneCallParams *params, LinphonePrivacyMask privacy) {
	L_GET_CPP_PTR_FROM_C_OBJECT(params)->setPrivacy(privacy);
}

void linphone_call_params_set_record_file (LinphoneCallParams *params, const char *path) {
	L_GET_CPP_PTR_FROM_C_OBJECT(params)->setRecordFilePath(L_C_TO_STRING(path));
}

void linphone_call_params_set_session_name (LinphoneCallParams *params, const char *name) {
	L_GET_CPP_PTR_FROM_C_OBJECT(params)->setSessionName(L_C_TO_STRING(name));
}

bool_t linphone_call_params_audio_enabled (const LinphoneCallParams *params) {
	return L_GET_CPP_PTR_FROM_C_OBJECT(params)->audioEnabled();
}

bool_t linphone_call_params_realtime_text_enabled (const LinphoneCallParams *params) {
	return L_GET_CPP_PTR_FROM_C_OBJECT(params)->realtimeTextEnabled();
}

bool_t linphone_call_params_video_enabled (const LinphoneCallParams *params) {
	return L_GET_CPP_PTR_FROM_C_OBJECT(params)->videoEnabled();
}

LinphoneMediaDirection linphone_call_params_get_audio_direction (const LinphoneCallParams *params) {
	return L_GET_CPP_PTR_FROM_C_OBJECT(params)->getAudioDirection();
}

LinphoneMediaDirection linphone_call_params_get_video_direction (const LinphoneCallParams *params) {
	return L_GET_CPP_PTR_FROM_C_OBJECT(params)->getVideoDirection();
}

void linphone_call_params_set_audio_direction (LinphoneCallParams *params, LinphoneMediaDirection dir) {
	L_GET_CPP_PTR_FROM_C_OBJECT(params)->setAudioDirection(dir);
}

void linphone_call_params_set_video_direction (LinphoneCallParams *params, LinphoneMediaDirection dir) {
	L_GET_CPP_PTR_FROM_C_OBJECT(params)->setVideoDirection(dir);
}

void linphone_call_params_enable_audio_multicast (LinphoneCallParams *params, bool_t yesno) {
	L_GET_CPP_PTR_FROM_C_OBJECT(params)->enableAudioMulticast(!!yesno);
}

bool_t linphone_call_params_audio_multicast_enabled (const LinphoneCallParams *params) {
	return L_GET_CPP_PTR_FROM_C_OBJECT(params)->audioMulticastEnabled();
}

void linphone_call_params_enable_video_multicast (LinphoneCallParams *params, bool_t yesno) {
	L_GET_CPP_PTR_FROM_C_OBJECT(params)->enableVideoMulticast(!!yesno);
}

bool_t linphone_call_params_video_multicast_enabled (const LinphoneCallParams *params) {
	return L_GET_CPP_PTR_FROM_C_OBJECT(params)->videoMulticastEnabled();
}

bool_t linphone_call_params_real_early_media_enabled (const LinphoneCallParams *params) {
	return L_GET_CPP_PTR_FROM_C_OBJECT(params)->earlyMediaSendingEnabled();
}

bool_t linphone_call_params_avpf_enabled (const LinphoneCallParams *params) {
	return L_GET_CPP_PTR_FROM_C_OBJECT(params)->avpfEnabled();
}

void linphone_call_params_enable_avpf (LinphoneCallParams *params, bool_t enable) {
	L_GET_CPP_PTR_FROM_C_OBJECT(params)->enableAvpf(!!enable);
}

bool_t linphone_call_params_mandatory_media_encryption_enabled (const LinphoneCallParams *params) {
	return L_GET_CPP_PTR_FROM_C_OBJECT(params)->mandatoryMediaEncryptionEnabled();
}

void linphone_call_params_enable_mandatory_media_encryption (LinphoneCallParams *params, bool_t value) {
	L_GET_CPP_PTR_FROM_C_OBJECT(params)->enableMandatoryMediaEncryption(!!value);
}

uint16_t linphone_call_params_get_avpf_rr_interval (const LinphoneCallParams *params) {
	return L_GET_CPP_PTR_FROM_C_OBJECT(params)->getAvpfRrInterval();
}

void linphone_call_params_set_avpf_rr_interval (LinphoneCallParams *params, uint16_t value) {
	L_GET_CPP_PTR_FROM_C_OBJECT(params)->setAvpfRrInterval(value);
}

void linphone_call_params_set_sent_fps (LinphoneCallParams *params, float value) {
	L_GET_PRIVATE_FROM_C_OBJECT(params)->setSentFps(value);
}

void linphone_call_params_set_received_fps (LinphoneCallParams *params, float value) {
	L_GET_PRIVATE_FROM_C_OBJECT(params)->setReceivedFps(value);
}

// =============================================================================
// Private functions.
// =============================================================================

bool_t linphone_call_params_get_in_conference (const LinphoneCallParams *params) {
	return L_GET_PRIVATE_FROM_C_OBJECT(params)->getInConference();
}

void linphone_call_params_set_in_conference (LinphoneCallParams *params, bool_t value) {
	L_GET_PRIVATE_FROM_C_OBJECT(params)->setInConference(!!value);
}

bool_t linphone_call_params_get_internal_call_update (const LinphoneCallParams *params) {
	return L_GET_PRIVATE_FROM_C_OBJECT(params)->getInternalCallUpdate();
}

void linphone_call_params_set_internal_call_update (LinphoneCallParams *params, bool_t value) {
	L_GET_PRIVATE_FROM_C_OBJECT(params)->setInternalCallUpdate(!!value);
}

bool_t linphone_call_params_implicit_rtcp_fb_enabled (const LinphoneCallParams *params) {
	return L_GET_PRIVATE_FROM_C_OBJECT(params)->implicitRtcpFbEnabled();
}

void linphone_call_params_enable_implicit_rtcp_fb (LinphoneCallParams *params, bool_t value) {
	L_GET_PRIVATE_FROM_C_OBJECT(params)->enableImplicitRtcpFb(!!value);
}

int linphone_call_params_get_down_bandwidth (const LinphoneCallParams *params) {
	return L_GET_PRIVATE_FROM_C_OBJECT(params)->getDownBandwidth();
}

void linphone_call_params_set_down_bandwidth (LinphoneCallParams *params, int value) {
	L_GET_PRIVATE_FROM_C_OBJECT(params)->setDownBandwidth(value);
}

int linphone_call_params_get_up_bandwidth (const LinphoneCallParams *params) {
	return L_GET_PRIVATE_FROM_C_OBJECT(params)->getUpBandwidth();
}

void linphone_call_params_set_up_bandwidth (LinphoneCallParams *params, int value) {
	L_GET_PRIVATE_FROM_C_OBJECT(params)->setUpBandwidth(value);
}

int linphone_call_params_get_down_ptime (const LinphoneCallParams *params) {
	return L_GET_PRIVATE_FROM_C_OBJECT(params)->getDownPtime();
}

void linphone_call_params_set_down_ptime (LinphoneCallParams *params, int value) {
	L_GET_PRIVATE_FROM_C_OBJECT(params)->setDownPtime(value);
}

int linphone_call_params_get_up_ptime (const LinphoneCallParams *params) {
	return L_GET_PRIVATE_FROM_C_OBJECT(params)->getUpPtime();
}

void linphone_call_params_set_up_ptime (LinphoneCallParams *params, int value) {
	L_GET_PRIVATE_FROM_C_OBJECT(params)->setUpPtime(value);
}

SalCustomHeader *linphone_call_params_get_custom_headers (const LinphoneCallParams *params) {
	return L_GET_PRIVATE_FROM_C_OBJECT(params)->getCustomHeaders();
}

SalCustomSdpAttribute *linphone_call_params_get_custom_sdp_attributes (const LinphoneCallParams *params) {
	return L_GET_PRIVATE_FROM_C_OBJECT(params)->getCustomSdpAttributes();
}

SalCustomSdpAttribute *linphone_call_params_get_custom_sdp_media_attributes (const LinphoneCallParams *params, LinphoneStreamType type) {
	return L_GET_PRIVATE_FROM_C_OBJECT(params)->getCustomSdpMediaAttributes(type);
}

LinphoneCall *linphone_call_params_get_referer (const LinphoneCallParams *params) {
	shared_ptr<LinphonePrivate::CallSession> session = L_GET_PRIVATE_FROM_C_OBJECT(params)->getReferer();
	if (!session)
		return nullptr;
	for (const auto &call : session->getCore()->getCalls()) {
		if (L_GET_PRIVATE(call)->getActiveSession() == session)
			return L_GET_C_BACK_PTR(call);
	}
	return nullptr;
}

void linphone_call_params_set_referer (LinphoneCallParams *params, LinphoneCall *referer) {
	L_GET_PRIVATE_FROM_C_OBJECT(params)->setReferer(L_GET_PRIVATE_FROM_C_OBJECT(referer)->getActiveSession());
}

bool_t linphone_call_params_get_update_call_when_ice_completed (const LinphoneCallParams *params) {
	return L_GET_PRIVATE_FROM_C_OBJECT(params)->getUpdateCallWhenIceCompleted();
}

void linphone_call_params_set_update_call_when_ice_completed (LinphoneCallParams *params, bool_t value) {
	L_GET_PRIVATE_FROM_C_OBJECT(params)->setUpdateCallWhenIceCompleted(!!value);
}

void linphone_call_params_set_sent_vsize (LinphoneCallParams *params, MSVideoSize vsize) {
	L_GET_PRIVATE_FROM_C_OBJECT(params)->setSentVideoDefinition(linphone_video_definition_new(static_cast<unsigned int>(vsize.width), static_cast<unsigned int>(vsize.height), nullptr));
}

void linphone_call_params_set_recv_vsize (LinphoneCallParams *params, MSVideoSize vsize) {
	L_GET_PRIVATE_FROM_C_OBJECT(params)->setReceivedVideoDefinition(linphone_video_definition_new(static_cast<unsigned int>(vsize.width), static_cast<unsigned int>(vsize.height), nullptr));
}

void linphone_call_params_set_sent_video_definition (LinphoneCallParams *params, LinphoneVideoDefinition *vdef) {
	L_GET_PRIVATE_FROM_C_OBJECT(params)->setSentVideoDefinition(vdef);
}

void linphone_call_params_set_received_video_definition (LinphoneCallParams *params, LinphoneVideoDefinition *vdef) {
	L_GET_PRIVATE_FROM_C_OBJECT(params)->setReceivedVideoDefinition(vdef);
}

bool_t linphone_call_params_get_no_user_consent (const LinphoneCallParams *params) {
	return L_GET_PRIVATE_FROM_C_OBJECT(params)->getNoUserConsent();
}

void linphone_call_params_set_no_user_consent (LinphoneCallParams *params, bool_t value) {
	L_GET_PRIVATE_FROM_C_OBJECT(params)->setNoUserConsent(!!value);
}

// =============================================================================
// Reference and user data handling functions.
// =============================================================================

void *linphone_call_params_get_user_data (const LinphoneCallParams *cp) {
	return L_GET_USER_DATA_FROM_C_OBJECT(cp);
}

void linphone_call_params_set_user_data (LinphoneCallParams *cp, void *ud) {
	L_SET_USER_DATA_FROM_C_OBJECT(cp, ud);
}

LinphoneCallParams *linphone_call_params_ref (LinphoneCallParams *cp) {
	belle_sip_object_ref(cp);
	return cp;
}

void linphone_call_params_unref (LinphoneCallParams *cp) {
	belle_sip_object_unref(cp);
}

// =============================================================================
// Constructor and destructor functions.
// =============================================================================

LinphoneCallParams *linphone_call_params_new (LinphoneCore *core) {
	LinphoneCallParams *params = L_INIT(CallParams);
	L_SET_CPP_PTR_FROM_C_OBJECT(params, new LinphonePrivate::MediaSessionParams());
	L_GET_CPP_PTR_FROM_C_OBJECT(params)->initDefault(L_GET_CPP_PTR_FROM_C_OBJECT(core));
	return params;
}

LinphoneCallParams *linphone_call_params_new_for_wrapper (void) {
	return _linphone_CallParams_init();
}

/* DEPRECATED */
void linphone_call_params_destroy (LinphoneCallParams *cp) {
	linphone_call_params_unref(cp);
}
