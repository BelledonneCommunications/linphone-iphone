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

#include "private.h"


/*******************************************************************************
 * Internal functions                                                          *
 ******************************************************************************/

SalMediaProto get_proto_from_call_params(const LinphoneCallParams *params) {
	if ((params->media_encryption == LinphoneMediaEncryptionSRTP) && params->avpf_enabled) return SalProtoRtpSavpf;
	if (params->media_encryption == LinphoneMediaEncryptionSRTP) return SalProtoRtpSavp;
	if ((params->media_encryption == LinphoneMediaEncryptionDTLS) && params->avpf_enabled) return SalProtoUdpTlsRtpSavpf;
	if (params->media_encryption == LinphoneMediaEncryptionDTLS) return SalProtoUdpTlsRtpSavp;
	if (params->avpf_enabled) return SalProtoRtpAvpf;
	return SalProtoRtpAvp;
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

void linphone_call_params_set_custom_headers(LinphoneCallParams *params, const SalCustomHeader *ch){
	if (params->custom_headers){
		sal_custom_header_free(params->custom_headers);
		params->custom_headers = NULL;
	}
	if (ch){
		params->custom_headers = sal_custom_header_clone(ch);
	}
}

void linphone_call_params_set_custom_sdp_attributes(LinphoneCallParams *params, const SalCustomSdpAttribute *csa) {
	if (params->custom_sdp_attributes) {
		sal_custom_sdp_attribute_free(params->custom_sdp_attributes);
		params->custom_sdp_attributes = NULL;
	}
	if (csa) {
		params->custom_sdp_attributes = sal_custom_sdp_attribute_clone(csa);
	}
}

void linphone_call_params_set_custom_sdp_media_attributes(LinphoneCallParams *params, LinphoneStreamType type, const SalCustomSdpAttribute *csa) {
	if (params->custom_sdp_media_attributes[type]) {
		sal_custom_sdp_attribute_free(params->custom_sdp_media_attributes[type]);
		params->custom_sdp_media_attributes[type] = NULL;
	}
	if (csa) {
		params->custom_sdp_media_attributes[type] = sal_custom_sdp_attribute_clone(csa);
	}
}


/*******************************************************************************
 * Public functions                                                            *
 ******************************************************************************/

void linphone_call_params_add_custom_header(LinphoneCallParams *params, const char *header_name, const char *header_value){
	params->custom_headers=sal_custom_header_append(params->custom_headers,header_name,header_value);
}

void linphone_call_params_add_custom_sdp_attribute(LinphoneCallParams *params, const char *attribute_name, const char *attribute_value) {
	params->custom_sdp_attributes = sal_custom_sdp_attribute_append(params->custom_sdp_attributes, attribute_name, attribute_value);
}

void linphone_call_params_add_custom_sdp_media_attribute(LinphoneCallParams *params, LinphoneStreamType type, const char *attribute_name, const char *attribute_value) {
	params->custom_sdp_media_attributes[type] = sal_custom_sdp_attribute_append(params->custom_sdp_media_attributes[type], attribute_name, attribute_value);
}

void linphone_call_params_clear_custom_sdp_attributes(LinphoneCallParams *params) {
	linphone_call_params_set_custom_sdp_attributes(params, NULL);
}

void linphone_call_params_clear_custom_sdp_media_attributes(LinphoneCallParams *params, LinphoneStreamType type) {
	linphone_call_params_set_custom_sdp_media_attributes(params, type, NULL);
}

LinphoneCallParams * linphone_call_params_copy(const LinphoneCallParams *cp){
	unsigned int i;
	LinphoneCallParams *ncp=linphone_call_params_new();
	memcpy(ncp,cp,sizeof(LinphoneCallParams));
	if (cp->record_file) ncp->record_file=ms_strdup(cp->record_file);
	if (cp->session_name) ncp->session_name=ms_strdup(cp->session_name);
	/*
	 * The management of the custom headers is not optimal. We copy everything while ref counting would be more efficient.
	 */
	if (cp->custom_headers) ncp->custom_headers=sal_custom_header_clone(cp->custom_headers);
	if (cp->custom_sdp_attributes) ncp->custom_sdp_attributes = sal_custom_sdp_attribute_clone(cp->custom_sdp_attributes);
	for (i = 0; i < (unsigned int)LinphoneStreamTypeUnknown; i++) {
		if (cp->custom_sdp_media_attributes[i]) ncp->custom_sdp_media_attributes[i] = sal_custom_sdp_attribute_clone(cp->custom_sdp_media_attributes[i]);
	}

	return ncp;
}

bool_t linphone_call_params_early_media_sending_enabled(const LinphoneCallParams *cp){
	return cp->real_early_media;
}

void linphone_call_params_enable_early_media_sending(LinphoneCallParams *cp, bool_t enabled){
	cp->real_early_media=enabled;
}

void linphone_call_params_enable_low_bandwidth(LinphoneCallParams *cp, bool_t enabled){
	cp->low_bandwidth=enabled;
}

void linphone_call_params_enable_audio(LinphoneCallParams *cp, bool_t enabled){
	cp->has_audio=enabled;
	if (enabled && cp->audio_dir==LinphoneMediaDirectionInactive)
		cp->audio_dir=LinphoneMediaDirectionSendRecv;
}

int linphone_call_params_enable_realtime_text(LinphoneCallParams *params, bool_t yesno) {
	params->realtimetext_enabled=yesno;
	return 0;
}

void linphone_call_params_enable_video(LinphoneCallParams *cp, bool_t enabled){
	cp->has_video=enabled;
	if (enabled && cp->video_dir==LinphoneMediaDirectionInactive)
		cp->video_dir=LinphoneMediaDirectionSendRecv;
}

const char *linphone_call_params_get_custom_header(const LinphoneCallParams *params, const char *header_name){
	return sal_custom_header_find(params->custom_headers,header_name);
}

const char * linphone_call_params_get_custom_sdp_attribute(const LinphoneCallParams *params, const char *attribute_name) {
	return sal_custom_sdp_attribute_find(params->custom_sdp_attributes, attribute_name);
}

const char * linphone_call_params_get_custom_sdp_media_attribute(const LinphoneCallParams *params, LinphoneStreamType type, const char *attribute_name) {
	return sal_custom_sdp_attribute_find(params->custom_sdp_media_attributes[type], attribute_name);
}

bool_t linphone_call_params_get_local_conference_mode(const LinphoneCallParams *cp){
	return cp->in_conference;
}

LinphoneMediaEncryption linphone_call_params_get_media_encryption(const LinphoneCallParams *cp) {
	return cp->media_encryption;
}

LinphonePrivacyMask linphone_call_params_get_privacy(const LinphoneCallParams *params) {
	return params->privacy;
}

float linphone_call_params_get_received_framerate(const LinphoneCallParams *cp){
	return cp->received_fps;
}

MSVideoSize linphone_call_params_get_received_video_size(const LinphoneCallParams *cp) {
	return cp->recv_vsize;
}

const char *linphone_call_params_get_record_file(const LinphoneCallParams *cp){
	return cp->record_file;
}

const char * linphone_call_params_get_rtp_profile(const LinphoneCallParams *cp) {
	return sal_media_proto_to_string(get_proto_from_call_params(cp));
}

float linphone_call_params_get_sent_framerate(const LinphoneCallParams *cp){
	return cp->sent_fps;
}

MSVideoSize linphone_call_params_get_sent_video_size(const LinphoneCallParams *cp) {
	return cp->sent_vsize;
}

const char *linphone_call_params_get_session_name(const LinphoneCallParams *cp){
	return cp->session_name;
}

const LinphonePayloadType* linphone_call_params_get_used_audio_codec(const LinphoneCallParams *cp) {
	return cp->audio_codec;
}

const LinphonePayloadType* linphone_call_params_get_used_video_codec(const LinphoneCallParams *cp) {
	return cp->video_codec;
}

const LinphonePayloadType* linphone_call_params_get_used_text_codec(const LinphoneCallParams *cp) {
	return cp->text_codec;
}


bool_t linphone_call_params_low_bandwidth_enabled(const LinphoneCallParams *cp) {
	return cp->low_bandwidth;
}

void linphone_call_params_set_audio_bandwidth_limit(LinphoneCallParams *cp, int bandwidth){
	cp->audio_bw=bandwidth;
}

void linphone_call_params_set_media_encryption(LinphoneCallParams *cp, LinphoneMediaEncryption e) {
	cp->media_encryption = e;
}

void linphone_call_params_set_privacy(LinphoneCallParams *params, LinphonePrivacyMask privacy) {
	params->privacy=privacy;
}

void linphone_call_params_set_record_file(LinphoneCallParams *cp, const char *path){
	if (cp->record_file){
		ms_free(cp->record_file);
		cp->record_file=NULL;
	}
	if (path) cp->record_file=ms_strdup(path);
}

void linphone_call_params_set_session_name(LinphoneCallParams *cp, const char *name){
	if (cp->session_name){
		ms_free(cp->session_name);
		cp->session_name=NULL;
	}
	if (name) cp->session_name=ms_strdup(name);
}

bool_t linphone_call_params_audio_enabled(const LinphoneCallParams *cp){
	return cp->has_audio;
}

bool_t linphone_call_params_realtime_text_enabled(const LinphoneCallParams *params) {
	return params->realtimetext_enabled;
}

bool_t linphone_call_params_video_enabled(const LinphoneCallParams *cp){
	return cp->has_video;
}

LinphoneMediaDirection linphone_call_params_get_audio_direction(const LinphoneCallParams *cp) {
	return cp->audio_dir;
}

LinphoneMediaDirection linphone_call_params_get_video_direction(const LinphoneCallParams *cp) {
	return cp->video_dir;
}

void linphone_call_params_set_audio_direction(LinphoneCallParams *cp,LinphoneMediaDirection dir) {
	cp->audio_dir=dir;
}

void linphone_call_params_set_video_direction(LinphoneCallParams *cp,LinphoneMediaDirection dir) {
	cp->video_dir=dir;
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

void linphone_call_params_enable_audio_multicast(LinphoneCallParams *params, bool_t yesno) {
	params->audio_multicast_enabled=yesno;
}

bool_t linphone_call_params_audio_multicast_enabled(const LinphoneCallParams *params) {
	return params->audio_multicast_enabled;
}

void linphone_call_params_enable_video_multicast(LinphoneCallParams *params, bool_t yesno) {
	params->video_multicast_enabled=yesno;
}
bool_t linphone_call_params_video_multicast_enabled(const LinphoneCallParams *params) {
	return params->video_multicast_enabled;
}

/*******************************************************************************
 * Constructor and destructor functions                                        *
 ******************************************************************************/

static void _linphone_call_params_unref(LinphoneCallParams *cp){
	unsigned int i;
	if (cp->record_file) ms_free(cp->record_file);
	if (cp->custom_headers) sal_custom_header_free(cp->custom_headers);
	if (cp->custom_sdp_attributes) sal_custom_sdp_attribute_free(cp->custom_sdp_attributes);
	for (i = 0; i < (unsigned int)LinphoneStreamTypeUnknown; i++) {
		if (cp->custom_sdp_media_attributes[i]) sal_custom_sdp_attribute_free(cp->custom_sdp_media_attributes[i]);
	}
	if (cp->session_name) ms_free(cp->session_name);
}

LinphoneCallParams * linphone_call_params_new(void) {
	LinphoneCallParams *cp=belle_sip_object_new(LinphoneCallParams);
	cp->audio_dir=LinphoneMediaDirectionSendRecv;
	cp->video_dir=LinphoneMediaDirectionSendRecv;
	cp->has_audio=TRUE;
	cp->realtimetext_enabled = FALSE;
	return cp;
}

/* DEPRECATED */
void linphone_call_params_destroy(LinphoneCallParams *cp) {
	linphone_call_params_unref(cp);
}

BELLE_SIP_DECLARE_NO_IMPLEMENTED_INTERFACES(LinphoneCallParams);

BELLE_SIP_INSTANCIATE_VPTR(LinphoneCallParams, belle_sip_object_t,
	(belle_sip_object_destroy_t)_linphone_call_params_unref,
	NULL, // clone
	NULL, // marshal
	FALSE
);
