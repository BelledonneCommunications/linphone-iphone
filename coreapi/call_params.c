/*
linphone
Copyright (C) 2010-2014  Belledonne Communications SARL

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

#include "linphone/call_params.h"
#include "private.h"

#include "conference/params/media-session-params.h"
#include "conference/params/call-session-params-p.h"
#include "conference/params/media-session-params-p.h"


struct _LinphoneCallParams{
	belle_sip_object_t base;
	void *user_data;
	LinphonePrivate::MediaSessionParams *msp;
};

BELLE_SIP_DECLARE_VPTR_NO_EXPORT(LinphoneCallParams);


/*******************************************************************************
 * Internal functions                                                          *
 ******************************************************************************/

SalMediaProto get_proto_from_call_params(const LinphoneCallParams *params) {
	return params->msp->getMediaProto();
}

SalStreamDir sal_dir_from_call_params_dir(LinphoneMediaDirection cpdir) {
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

LinphoneMediaDirection media_direction_from_sal_stream_dir(SalStreamDir dir){
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

SalStreamDir get_audio_dir_from_call_params(const LinphoneCallParams *params) {
	return sal_dir_from_call_params_dir(linphone_call_params_get_audio_direction(params));
}

SalStreamDir get_video_dir_from_call_params(const LinphoneCallParams *params) {
	return sal_dir_from_call_params_dir(linphone_call_params_get_video_direction(params));
}

void linphone_call_params_set_custom_headers(LinphoneCallParams *params, const SalCustomHeader *ch) {
	static_cast<LinphonePrivate::CallSessionParams *>(params->msp)->getPrivate()->setCustomHeaders(ch);
}

void linphone_call_params_set_custom_sdp_attributes(LinphoneCallParams *params, const SalCustomSdpAttribute *csa) {
	static_cast<LinphonePrivate::CallSessionParams *>(params->msp)->getPrivate()->setCustomSdpAttributes(csa);
}

void linphone_call_params_set_custom_sdp_media_attributes(LinphoneCallParams *params, LinphoneStreamType type, const SalCustomSdpAttribute *csa) {
	static_cast<LinphonePrivate::CallSessionParams *>(params->msp)->getPrivate()->setCustomSdpMediaAttributes(type, csa);
}


/*******************************************************************************
 * Public functions                                                            *
 ******************************************************************************/

void linphone_call_params_add_custom_header(LinphoneCallParams *params, const char *header_name, const char *header_value) {
	params->msp->addCustomHeader(header_name, header_value);
}

void linphone_call_params_add_custom_sdp_attribute(LinphoneCallParams *params, const char *attribute_name, const char *attribute_value) {
	params->msp->addCustomSdpAttribute(attribute_name, attribute_value);
}

void linphone_call_params_add_custom_sdp_media_attribute(LinphoneCallParams *params, LinphoneStreamType type, const char *attribute_name, const char *attribute_value) {
	params->msp->addCustomSdpMediaAttribute(type, attribute_name, attribute_value);
}

void linphone_call_params_clear_custom_sdp_attributes(LinphoneCallParams *params) {
	params->msp->clearCustomSdpAttributes();
}

void linphone_call_params_clear_custom_sdp_media_attributes(LinphoneCallParams *params, LinphoneStreamType type) {
	params->msp->clearCustomSdpMediaAttributes(type);
}

LinphoneCallParams * linphone_call_params_copy(const LinphoneCallParams *params) {
	return (LinphoneCallParams *)belle_sip_object_clone((const belle_sip_object_t *)params);
}

bool_t linphone_call_params_early_media_sending_enabled(const LinphoneCallParams *params) {
	return params->msp->earlyMediaSendingEnabled();
}

void linphone_call_params_enable_early_media_sending(LinphoneCallParams *params, bool_t enabled) {
	params->msp->enableEarlyMediaSending(enabled);
}

void linphone_call_params_enable_low_bandwidth(LinphoneCallParams *params, bool_t enabled) {
	params->msp->enableLowBandwidth(enabled);
}

void linphone_call_params_enable_audio(LinphoneCallParams *params, bool_t enabled) {
	params->msp->enableAudio(enabled);
}

LinphoneStatus linphone_call_params_enable_realtime_text(LinphoneCallParams *params, bool_t yesno) {
	params->msp->enableRealtimeText(yesno);
	return 0;
}

void linphone_call_params_enable_video(LinphoneCallParams *params, bool_t enabled) {
	params->msp->enableVideo(enabled);
}

const char *linphone_call_params_get_custom_header(const LinphoneCallParams *params, const char *header_name) {
	std::string value = params->msp->getCustomHeader(header_name);
	return value.empty() ? nullptr : value.c_str();
}

const char * linphone_call_params_get_custom_sdp_attribute(const LinphoneCallParams *params, const char *attribute_name) {
	std::string value = params->msp->getCustomSdpAttribute(attribute_name);
	return value.empty() ? nullptr : value.c_str();
}

const char * linphone_call_params_get_custom_sdp_media_attribute(const LinphoneCallParams *params, LinphoneStreamType type, const char *attribute_name) {
	std::string value = params->msp->getCustomSdpMediaAttribute(type, attribute_name);
	return value.empty() ? nullptr : value.c_str();
}

bool_t linphone_call_params_get_local_conference_mode(const LinphoneCallParams *params) {
	return linphone_call_params_get_in_conference(params);
}

LinphoneMediaEncryption linphone_call_params_get_media_encryption(const LinphoneCallParams *params) {
	return params->msp->getMediaEncryption();
}

LinphonePrivacyMask linphone_call_params_get_privacy(const LinphoneCallParams *params) {
	return params->msp->getPrivacy();
}

float linphone_call_params_get_received_framerate(const LinphoneCallParams *params) {
	return params->msp->getReceivedFps();
}

MSVideoSize linphone_call_params_get_received_video_size(const LinphoneCallParams *params) {
	MSVideoSize vsize;
	LinphoneVideoDefinition *vdef = params->msp->getReceivedVideoDefinition();
	if (vdef) {
		vsize.width = linphone_video_definition_get_width(vdef);
		vsize.height = linphone_video_definition_get_height(vdef);
	} else {
		vsize.width = MS_VIDEO_SIZE_UNKNOWN_W;
		vsize.height = MS_VIDEO_SIZE_UNKNOWN_H;
	}
	return vsize;
}

const LinphoneVideoDefinition * linphone_call_params_get_received_video_definition(const LinphoneCallParams *params) {
	return params->msp->getReceivedVideoDefinition();
}

const char *linphone_call_params_get_record_file(const LinphoneCallParams *params) {
	const std::string &value = params->msp->getRecordFilePath();
	return value.empty() ? nullptr : value.c_str();
}

const char * linphone_call_params_get_rtp_profile(const LinphoneCallParams *params) {
	return params->msp->getRtpProfile();
}

float linphone_call_params_get_sent_framerate(const LinphoneCallParams *params) {
	return params->msp->getSentFps();
}

MSVideoSize linphone_call_params_get_sent_video_size(const LinphoneCallParams *params) {
	MSVideoSize vsize;
	LinphoneVideoDefinition *vdef = params->msp->getSentVideoDefinition();
	if (vdef) {
		vsize.width = linphone_video_definition_get_width(vdef);
		vsize.height = linphone_video_definition_get_height(vdef);
	} else {
		vsize.width = MS_VIDEO_SIZE_UNKNOWN_W;
		vsize.height = MS_VIDEO_SIZE_UNKNOWN_H;
	}
	return vsize;
}

const LinphoneVideoDefinition * linphone_call_params_get_sent_video_definition(const LinphoneCallParams *params) {
	return params->msp->getSentVideoDefinition();
}

const char *linphone_call_params_get_session_name(const LinphoneCallParams *params) {
	const std::string &value = params->msp->getSessionName();
	return value.empty() ? nullptr : value.c_str();
}

LinphonePayloadType *linphone_call_params_get_used_audio_payload_type(const LinphoneCallParams *params) {
	return params->msp->getUsedAudioPayloadType();
}

LinphonePayloadType *linphone_call_params_get_used_video_payload_type(const LinphoneCallParams *params) {
	return params->msp->getUsedVideoPayloadType();
}

LinphonePayloadType *linphone_call_params_get_used_text_payload_type(const LinphoneCallParams *params) {
	return params->msp->getUsedRealtimeTextPayloadType();
}

const OrtpPayloadType *linphone_call_params_get_used_audio_codec(const LinphoneCallParams *params) {
	return params->msp->getUsedAudioCodec();
}

void linphone_call_params_set_used_audio_codec(LinphoneCallParams *params, OrtpPayloadType *codec) {
	params->msp->getPrivate()->setUsedAudioCodec(codec);
}

const OrtpPayloadType *linphone_call_params_get_used_video_codec(const LinphoneCallParams *params) {
	return params->msp->getUsedVideoCodec();
}

void linphone_call_params_set_used_video_codec(LinphoneCallParams *params, OrtpPayloadType *codec) {
	params->msp->getPrivate()->setUsedVideoCodec(codec);
}

const OrtpPayloadType *linphone_call_params_get_used_text_codec(const LinphoneCallParams *params) {
	return params->msp->getUsedRealtimeTextCodec();
}

void linphone_call_params_set_used_text_codec(LinphoneCallParams *params, OrtpPayloadType *codec) {
	params->msp->getPrivate()->setUsedRealtimeTextCodec(codec);
}

bool_t linphone_call_params_low_bandwidth_enabled(const LinphoneCallParams *params) {
	return params->msp->lowBandwidthEnabled();
}

int linphone_call_params_get_audio_bandwidth_limit(const LinphoneCallParams *params) {
	return params->msp->getAudioBandwidthLimit();
}

void linphone_call_params_set_audio_bandwidth_limit(LinphoneCallParams *params, int bandwidth) {
	params->msp->setAudioBandwidthLimit(bandwidth);
}

void linphone_call_params_set_media_encryption(LinphoneCallParams *params, LinphoneMediaEncryption encryption) {
	params->msp->setMediaEncryption(encryption);
}

void linphone_call_params_set_privacy(LinphoneCallParams *params, LinphonePrivacyMask privacy) {
	params->msp->setPrivacy(privacy);
}

void linphone_call_params_set_record_file(LinphoneCallParams *params, const char *path) {
	params->msp->setRecordFilePath(path ? path : "");
}

void linphone_call_params_set_session_name(LinphoneCallParams *params, const char *name) {
	params->msp->setSessionName(name ? name : "");
}

bool_t linphone_call_params_audio_enabled(const LinphoneCallParams *params) {
	return params->msp->audioEnabled();
}

bool_t linphone_call_params_realtime_text_enabled(const LinphoneCallParams *params) {
	return params->msp->realtimeTextEnabled();
}

bool_t linphone_call_params_video_enabled(const LinphoneCallParams *params) {
	return params->msp->videoEnabled();
}

LinphoneMediaDirection linphone_call_params_get_audio_direction(const LinphoneCallParams *params) {
	return params->msp->getAudioDirection();
}

LinphoneMediaDirection linphone_call_params_get_video_direction(const LinphoneCallParams *params) {
	return params->msp->getVideoDirection();
}

void linphone_call_params_set_audio_direction(LinphoneCallParams *params, LinphoneMediaDirection dir) {
	params->msp->setAudioDirection(dir);
}

void linphone_call_params_set_video_direction(LinphoneCallParams *params, LinphoneMediaDirection dir) {
	params->msp->setVideoDirection(dir);
}

void linphone_call_params_enable_audio_multicast(LinphoneCallParams *params, bool_t yesno) {
	params->msp->enableAudioMulticast(yesno);
}

bool_t linphone_call_params_audio_multicast_enabled(const LinphoneCallParams *params) {
	return params->msp->audioMulticastEnabled();
}

void linphone_call_params_enable_video_multicast(LinphoneCallParams *params, bool_t yesno) {
	params->msp->enableVideoMulticast(yesno);
}
bool_t linphone_call_params_video_multicast_enabled(const LinphoneCallParams *params) {
	return params->msp->videoMulticastEnabled();
}

bool_t linphone_call_params_real_early_media_enabled(const LinphoneCallParams *params) {
	return params->msp->earlyMediaSendingEnabled();
}

bool_t linphone_call_params_avpf_enabled(const LinphoneCallParams *params) {
	return params->msp->avpfEnabled();
}

void linphone_call_params_enable_avpf(LinphoneCallParams *params, bool_t enable) {
	params->msp->enableAvpf(enable);
}

bool_t linphone_call_params_mandatory_media_encryption_enabled(const LinphoneCallParams *params) {
	return params->msp->mandatoryMediaEncryptionEnabled();
}

void linphone_call_params_enable_mandatory_media_encryption(LinphoneCallParams *params, bool_t value) {
	params->msp->enableMandatoryMediaEncryption(value);
}

uint16_t linphone_call_params_get_avpf_rr_interval(const LinphoneCallParams *params) {
	return params->msp->getAvpfRrInterval();
}

void linphone_call_params_set_avpf_rr_interval(LinphoneCallParams *params, uint16_t value) {
	params->msp->setAvpfRrInterval(value);
}

void linphone_call_params_set_sent_fps(LinphoneCallParams *params, float value) {
	params->msp->getPrivate()->setSentFps(value);
}

void linphone_call_params_set_received_fps(LinphoneCallParams *params, float value) {
	params->msp->getPrivate()->setReceivedFps(value);
}


/*******************************************************************************
 * Private functions                                                           *
 ******************************************************************************/

bool_t linphone_call_params_get_in_conference(const LinphoneCallParams *params) {
	return static_cast<const LinphonePrivate::CallSessionParams *>(params->msp)->getPrivate()->getInConference();
}

void linphone_call_params_set_in_conference(LinphoneCallParams *params, bool_t value) {
	static_cast<LinphonePrivate::CallSessionParams *>(params->msp)->getPrivate()->setInConference(value);
}

bool_t linphone_call_params_get_internal_call_update(const LinphoneCallParams *params) {
	return static_cast<const LinphonePrivate::CallSessionParams *>(params->msp)->getPrivate()->getInternalCallUpdate();
}

void linphone_call_params_set_internal_call_update(LinphoneCallParams *params, bool_t value) {
	static_cast<LinphonePrivate::CallSessionParams *>(params->msp)->getPrivate()->setInternalCallUpdate(value);
}

bool_t linphone_call_params_implicit_rtcp_fb_enabled(const LinphoneCallParams *params) {
	return params->msp->getPrivate()->implicitRtcpFbEnabled();
}

void linphone_call_params_enable_implicit_rtcp_fb(LinphoneCallParams *params, bool_t value) {
	params->msp->getPrivate()->enableImplicitRtcpFb(value);
}

int linphone_call_params_get_down_bandwidth(const LinphoneCallParams *params) {
	return params->msp->getPrivate()->getDownBandwidth();
}

void linphone_call_params_set_down_bandwidth(LinphoneCallParams *params, int value) {
	params->msp->getPrivate()->setDownBandwidth(value);
}

int linphone_call_params_get_up_bandwidth(const LinphoneCallParams *params) {
	return params->msp->getPrivate()->getUpBandwidth();
}

void linphone_call_params_set_up_bandwidth(LinphoneCallParams *params, int value) {
	params->msp->getPrivate()->setUpBandwidth(value);
}

int linphone_call_params_get_down_ptime(const LinphoneCallParams *params) {
	return params->msp->getPrivate()->getDownPtime();
}

void linphone_call_params_set_down_ptime(LinphoneCallParams *params, int value) {
	params->msp->getPrivate()->setDownPtime(value);
}

int linphone_call_params_get_up_ptime(const LinphoneCallParams *params) {
	return params->msp->getPrivate()->getUpPtime();
}

void linphone_call_params_set_up_ptime(LinphoneCallParams *params, int value) {
	params->msp->getPrivate()->setUpPtime(value);
}

SalCustomHeader * linphone_call_params_get_custom_headers(const LinphoneCallParams *params) {
	return static_cast<const LinphonePrivate::CallSessionParams *>(params->msp)->getPrivate()->getCustomHeaders();
}

SalCustomSdpAttribute * linphone_call_params_get_custom_sdp_attributes(const LinphoneCallParams *params) {
	return static_cast<const LinphonePrivate::CallSessionParams *>(params->msp)->getPrivate()->getCustomSdpAttributes();
}

SalCustomSdpAttribute * linphone_call_params_get_custom_sdp_media_attributes(const LinphoneCallParams *params, LinphoneStreamType type) {
	return static_cast<const LinphonePrivate::CallSessionParams *>(params->msp)->getPrivate()->getCustomSdpMediaAttributes(type);
}

LinphoneCall * linphone_call_params_get_referer(const LinphoneCallParams *params) {
	return static_cast<const LinphonePrivate::CallSessionParams *>(params->msp)->getPrivate()->getReferer();
}

void linphone_call_params_set_referer(LinphoneCallParams *params, LinphoneCall *referer) {
	static_cast<LinphonePrivate::CallSessionParams *>(params->msp)->getPrivate()->setReferer(referer);
}

bool_t linphone_call_params_get_update_call_when_ice_completed(const LinphoneCallParams *params) {
	return params->msp->getPrivate()->getUpdateCallWhenIceCompleted();
}

void linphone_call_params_set_update_call_when_ice_completed(LinphoneCallParams *params, bool_t value) {
	params->msp->getPrivate()->setUpdateCallWhenIceCompleted(value);
}

void linphone_call_params_set_sent_vsize(LinphoneCallParams *params, MSVideoSize vsize) {
	params->msp->getPrivate()->setSentVideoDefinition(linphone_video_definition_new(vsize.width, vsize.height, nullptr));
}

void linphone_call_params_set_recv_vsize(LinphoneCallParams *params, MSVideoSize vsize) {
	params->msp->getPrivate()->setReceivedVideoDefinition(linphone_video_definition_new(vsize.width, vsize.height, nullptr));
}

void linphone_call_params_set_sent_video_definition(LinphoneCallParams *params, LinphoneVideoDefinition *vdef) {
	params->msp->getPrivate()->setSentVideoDefinition(vdef);
}

void linphone_call_params_set_received_video_definition(LinphoneCallParams *params, LinphoneVideoDefinition *vdef) {
	params->msp->getPrivate()->setReceivedVideoDefinition(vdef);
}

bool_t linphone_call_params_get_no_user_consent(const LinphoneCallParams *params) {
	return static_cast<const LinphonePrivate::CallSessionParams *>(params->msp)->getPrivate()->getNoUserConsent();
}

void linphone_call_params_set_no_user_consent(LinphoneCallParams *params, bool_t value) {
	static_cast<LinphonePrivate::CallSessionParams *>(params->msp)->getPrivate()->setNoUserConsent(value);
}


/*******************************************************************************
 * Reference and user data handling functions                                  *
 ******************************************************************************/

void *linphone_call_params_get_user_data(const LinphoneCallParams *cp) {
	return cp->user_data;
}

void linphone_call_params_set_user_data(LinphoneCallParams *cp, void *ud) {
	cp->user_data = ud;
}

LinphoneCallParams * linphone_call_params_ref(LinphoneCallParams *cp) {
	belle_sip_object_ref(cp);
	return cp;
}

void linphone_call_params_unref(LinphoneCallParams *cp) {
	belle_sip_object_unref(cp);
}


/*******************************************************************************
 * Constructor and destructor functions                                        *
 ******************************************************************************/

static void _linphone_call_params_destroy(LinphoneCallParams *params) {
	delete params->msp;
}

static void _linphone_call_params_clone(LinphoneCallParams *dst, const LinphoneCallParams *src) {
	dst->msp = new LinphonePrivate::MediaSessionParams(*src->msp);
}

LinphoneCallParams * linphone_call_params_new(void) {
	LinphoneCallParams *params = belle_sip_object_new(LinphoneCallParams);
	params->msp = new LinphonePrivate::MediaSessionParams();
	return params;
}

/* DEPRECATED */
void linphone_call_params_destroy(LinphoneCallParams *cp) {
	linphone_call_params_unref(cp);
}

BELLE_SIP_DECLARE_NO_IMPLEMENTED_INTERFACES(LinphoneCallParams);

BELLE_SIP_INSTANCIATE_VPTR(LinphoneCallParams, belle_sip_object_t,
	(belle_sip_object_destroy_t)_linphone_call_params_destroy,
	(belle_sip_object_clone_t)_linphone_call_params_clone, // clone
	NULL, // marshal
	FALSE
);
