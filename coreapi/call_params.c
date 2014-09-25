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
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
*/

#include "private.h"


/*******************************************************************************
 * Internal functions                                                          *
 ******************************************************************************/

SalMediaProto get_proto_from_call_params(const LinphoneCallParams *params) {
	if ((params->media_encryption == LinphoneMediaEncryptionSRTP) && params->avpf_enabled) return SalProtoRtpSavpf;
	if (params->media_encryption == LinphoneMediaEncryptionSRTP) return SalProtoRtpSavp;
	if (params->avpf_enabled) return SalProtoRtpAvpf;
	return SalProtoRtpAvp;
}


/*******************************************************************************
 * Public functions                                                            *
 ******************************************************************************/

void linphone_call_params_add_custom_header(LinphoneCallParams *params, const char *header_name, const char *header_value){
	params->custom_headers=sal_custom_header_append(params->custom_headers,header_name,header_value);
}

LinphoneCallParams * linphone_call_params_copy(const LinphoneCallParams *cp){
	LinphoneCallParams *ncp=linphone_call_params_new();
	memcpy(ncp,cp,sizeof(LinphoneCallParams));
	if (cp->record_file) ncp->record_file=ms_strdup(cp->record_file);
	if (cp->session_name) ncp->session_name=ms_strdup(cp->session_name);
	/*
	 * The management of the custom headers is not optimal. We copy everything while ref counting would be more efficient.
	 */
	if (cp->custom_headers) ncp->custom_headers=sal_custom_header_clone(cp->custom_headers);
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

void linphone_call_params_enable_video(LinphoneCallParams *cp, bool_t enabled){
	cp->has_video=enabled;
}

const char *linphone_call_params_get_custom_header(const LinphoneCallParams *params, const char *header_name){
	return sal_custom_header_find(params->custom_headers,header_name);
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

bool_t linphone_call_params_video_enabled(const LinphoneCallParams *cp){
	return cp->has_video;
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

static void _linphone_call_params_destroy(LinphoneCallParams *cp){
	if (cp->record_file) ms_free(cp->record_file);
	if (cp->custom_headers) sal_custom_header_free(cp->custom_headers);
}

LinphoneCallParams * linphone_call_params_new(void) {
	return belle_sip_object_new(LinphoneCallParams);
}

/* DEPRECATED */
void linphone_call_params_destroy(LinphoneCallParams *cp) {
	linphone_call_params_unref(cp);
}

BELLE_SIP_DECLARE_NO_IMPLEMENTED_INTERFACES(LinphoneCallParams);

BELLE_SIP_INSTANCIATE_VPTR(LinphoneCallParams, belle_sip_object_t,
	(belle_sip_object_destroy_t)_linphone_call_params_destroy,
	NULL, // clone
	NULL, // marshal
	FALSE
);
