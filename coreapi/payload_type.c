/*
payload_type.c
Copyright (C) 2017 Belledonne Communications SARL

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

#include <string.h>
#include <ortp/payloadtype.h>
#include "linphone/payload_type.h"
#include "private.h"

struct _LinphonePayloadType {
	belle_sip_object_t base;
	OrtpPayloadType *pt;
	LinphoneCore *lc;
};

BELLE_SIP_DECLARE_NO_IMPLEMENTED_INTERFACES(LinphonePayloadType);
BELLE_SIP_DECLARE_VPTR_NO_EXPORT(LinphonePayloadType);

LinphonePayloadType *linphone_payload_type_new(LinphoneCore *lc, OrtpPayloadType *ortp_pt) {
	LinphonePayloadType *pt;
	if (ortp_pt == NULL) return NULL;
	pt = belle_sip_object_new(LinphonePayloadType);
	pt->pt = ortp_pt;
	pt->lc = lc;
	return pt;
}

LinphonePayloadType *linphone_payload_type_ref(LinphonePayloadType *pt) {
	return (LinphonePayloadType *)belle_sip_object_ref(pt);
}

void linphone_payload_type_unref(LinphonePayloadType *pt) {
	belle_sip_object_unref(pt);
}

int linphone_payload_type_get_type(const LinphonePayloadType *pt) {
	return pt->pt->type;
}

static bool_t _payload_type_is_in_core(const OrtpPayloadType *pt, const LinphoneCore *lc) {
	return (bctbx_list_find(lc->codecs_conf.audio_codecs, pt) != NULL)
		|| (bctbx_list_find(lc->codecs_conf.video_codecs, pt) != NULL)
		|| (bctbx_list_find(lc->codecs_conf.text_codecs, pt) != NULL);
}

static char *_payload_type_get_description(const OrtpPayloadType *pt) {
	return bctbx_strdup_printf("%s/%d/%d", pt->mime_type, pt->clock_rate, pt->channels);
}

static int _linphone_core_enable_payload_type(LinphoneCore *lc, OrtpPayloadType *pt, bool_t enabled) {
	payload_type_set_enable(pt,enabled);
	_linphone_core_codec_config_write(lc);
	linphone_core_update_allocated_audio_bandwidth(lc);
	return 0;
}

LinphoneStatus linphone_core_enable_payload_type(LinphoneCore *lc, OrtpPayloadType *pt, bool_t enabled){
	if (!_payload_type_is_in_core(pt, lc)) {
		char *desc = _payload_type_get_description(pt);
		ms_error("cannot enable '%s' payload type: not in the core", desc);
		bctbx_free(desc);
		return -1;
	}
	return _linphone_core_enable_payload_type(lc, pt, enabled);
}

int linphone_payload_type_enable(LinphonePayloadType *pt, bool_t enabled) {
	if (pt->lc == NULL) {
		char *desc = linphone_payload_type_get_description(pt);
		ms_error("cannot enable '%s' payload type: no core associated", desc);
		bctbx_free(desc);
		return -1;
	}
	return _linphone_core_enable_payload_type(pt->lc, pt->pt, enabled);
}

bool_t linphone_core_payload_type_enabled(const LinphoneCore *lc, const OrtpPayloadType *pt){
	return payload_type_enabled(pt);
}

bool_t linphone_payload_type_enabled(const LinphonePayloadType *pt) {
	return payload_type_enabled(pt->pt);
}

char *linphone_payload_type_get_description(const LinphonePayloadType *pt) {
	return _payload_type_get_description(pt->pt);
}

static const char *_linphone_core_get_payload_type_codec_description(const LinphoneCore *lc, const OrtpPayloadType *pt) {
	if (ms_factory_codec_supported(lc->factory, pt->mime_type)){
		MSFilterDesc *desc=ms_factory_get_encoder(lc->factory, pt->mime_type);
#ifdef ENABLE_NLS
		return dgettext("mediastreamer",desc->text);
#else
		return desc->text;
#endif
	}
	return NULL;
}

const char *linphone_core_get_payload_type_description(LinphoneCore *lc, const OrtpPayloadType *pt){
	if (!_payload_type_is_in_core(pt, lc)) {
		char *desc = _payload_type_get_description(pt);
		ms_error("cannot get codec description for '%s' payload type: not in the core", desc);
		bctbx_free(desc);
		return NULL;
	}
	return _linphone_core_get_payload_type_codec_description(lc, pt);
}

const char *linphone_payload_type_get_encoder_description(const LinphonePayloadType *pt) {
	if (pt->lc == NULL) {
		char *desc = linphone_payload_type_get_description(pt);
		ms_error("cannot get codec description for '%s' payload type: no associated core", desc);
		bctbx_free(desc);
		return NULL;
	}
	return _linphone_core_get_payload_type_codec_description(pt->lc, pt->pt);
}

static int _linphone_core_get_payload_type_normal_bitrate(const LinphoneCore *lc, const OrtpPayloadType *pt) {
	int maxbw = get_min_bandwidth(linphone_core_get_download_bandwidth(lc),
					linphone_core_get_upload_bandwidth(lc));
	if (pt->type==PAYLOAD_AUDIO_CONTINUOUS || pt->type==PAYLOAD_AUDIO_PACKETIZED){
		return get_audio_payload_bandwidth(lc, pt, maxbw);
	}else if (pt->type==PAYLOAD_VIDEO){
		int video_bw;
		if (maxbw<=0) {
			video_bw=1500; /*default bitrate for video stream when no bandwidth limit is set, around 1.5 Mbit/s*/
		}else{
			video_bw=get_remaining_bandwidth_for_video(maxbw,lc->audio_bw);
		}
		return video_bw;
	}
	return 0;
}

int linphone_core_get_payload_type_bitrate(LinphoneCore *lc, const OrtpPayloadType *pt){
	if (_payload_type_is_in_core(pt, lc)) {
		return _linphone_core_get_payload_type_normal_bitrate(lc, pt);
	} else {
		char *desc = _payload_type_get_description(pt);
		ms_error("cannot get normal bitrate of payload type '%s': not in the core", desc);
		bctbx_free(desc);
		return -1;
	}
}

int linphone_payload_type_get_normal_bitrate(const LinphonePayloadType *pt) {
	if (pt->lc == NULL) {
		char *desc = linphone_payload_type_get_description(pt);
		ms_error("cannot get normal bitrate of codec '%s': no associated core", desc);
		bctbx_free(desc);
		return -1;
	}
	return _linphone_core_get_payload_type_normal_bitrate(pt->lc, pt->pt);
}

static void _linphone_core_set_payload_type_normal_bitrate(LinphoneCore *lc, OrtpPayloadType *pt, int bitrate) {
	if (pt->type==PAYLOAD_VIDEO || pt->flags & PAYLOAD_TYPE_IS_VBR){
		pt->normal_bitrate=bitrate*1000;
		pt->flags|=PAYLOAD_TYPE_BITRATE_OVERRIDE;
		linphone_core_update_allocated_audio_bandwidth(lc);
	}else{
		char *desc = _payload_type_get_description(pt);
		ms_error("Cannot set an explicit bitrate for codec '%s', because it is not VBR.",desc);
		bctbx_free(desc);
	}
}

void linphone_core_set_payload_type_bitrate(LinphoneCore *lc, OrtpPayloadType *pt, int bitrate) {
	if (_payload_type_is_in_core(pt, lc)) {
		_linphone_core_set_payload_type_normal_bitrate(lc, pt, bitrate);
	} else {
		char *desc = _payload_type_get_description(pt);
		ms_error("cannot set normal bitrate of codec '%s': not in the core", desc);
		bctbx_free(desc);
	}
}

void linphone_payload_type_set_normal_bitrate(LinphonePayloadType *pt, int bitrate) {
	if (pt->lc == NULL) {
		ms_error("cannot set bitrate of codec %s/%d: no associated core", pt->pt->mime_type, pt->pt->clock_rate);
		return;
	}
	_linphone_core_set_payload_type_normal_bitrate(pt->lc, pt->pt, bitrate);
}

const char *linphone_payload_type_get_mime_type(const LinphonePayloadType *pt) {
	return pt->pt->mime_type;
}

int linphone_payload_type_get_channels(const LinphonePayloadType *pt) {
	return pt->pt->channels;
}

int linphone_core_get_payload_type_number(LinphoneCore *lc, const OrtpPayloadType *pt) {
	return payload_type_get_number(pt);
}

int linphone_payload_type_get_number(const LinphonePayloadType *pt) {
	return payload_type_get_number(pt->pt);
}

void linphone_core_set_payload_type_number(LinphoneCore *lc, OrtpPayloadType *pt, int number) {
	payload_type_set_number(pt, number);
}

void linphone_payload_type_set_number(LinphonePayloadType *pt, int number) {
	payload_type_set_number(pt->pt, number);
}

const char *linphone_payload_type_get_recv_fmtp(const LinphonePayloadType *pt) {
	return pt->pt->recv_fmtp;
}

void linphone_payload_type_set_recv_fmtp(LinphonePayloadType *pt, const char *recv_fmtp) {
	if (pt->pt->recv_fmtp != NULL) bctbx_free(pt->pt->recv_fmtp);
	if (recv_fmtp != NULL) pt->pt->recv_fmtp = bctbx_strdup(recv_fmtp);
	else recv_fmtp = NULL;
}

const char *linphone_payload_type_get_send_fmtp(const LinphonePayloadType *pt) {
	return pt->pt->send_fmtp;
}

void linphone_payload_type_set_send_fmtp(LinphonePayloadType *pt, const char *send_fmtp) {
	if (pt->pt->send_fmtp != NULL) bctbx_free(pt->pt->send_fmtp);
	if (send_fmtp != NULL) pt->pt->recv_fmtp = bctbx_strdup(send_fmtp);
	else send_fmtp = NULL;
}

int linphone_payload_type_get_clock_rate(const LinphonePayloadType *pt) {
	return pt->pt->clock_rate;
}

bool_t linphone_core_payload_type_is_vbr(const LinphoneCore *lc, const OrtpPayloadType *pt) {
	return payload_type_is_vbr(pt);
}

bool_t linphone_payload_type_is_vbr(const LinphonePayloadType *pt) {
	return payload_type_is_vbr(pt->pt);
}

bool_t _linphone_core_check_payload_type_usability(const LinphoneCore *lc, const OrtpPayloadType *pt) {
	int maxbw=get_min_bandwidth(linphone_core_get_download_bandwidth(lc),
					linphone_core_get_upload_bandwidth(lc));
	return linphone_core_is_payload_type_usable_for_bandwidth(lc, pt, maxbw);
}

bool_t linphone_core_check_payload_type_usability(LinphoneCore *lc, const OrtpPayloadType *pt) {
	if (!_payload_type_is_in_core(pt, lc)) {
		char *desc = _payload_type_get_description(pt);
		ms_error("cannot check usability of '%s' payload type: not in the core", desc);
		bctbx_free(desc);
		return FALSE;
	}
	return _linphone_core_check_payload_type_usability(lc, pt);
}

bool_t linphone_payload_type_is_usable(const LinphonePayloadType *pt) {
	if (pt->lc == NULL) {
		char *desc = linphone_payload_type_get_description(pt);
		ms_error("cannot check usability of '%s' payload type: no associated core", desc);
		bctbx_free(desc);
		return FALSE;
	}
	return _linphone_core_check_payload_type_usability(pt->lc, pt->pt);
}

OrtpPayloadType *linphone_payload_type_get_ortp_pt(const LinphonePayloadType *pt) {
	return pt->pt;
}

BELLE_SIP_INSTANCIATE_VPTR(LinphonePayloadType, belle_sip_object_t,
	NULL, // uninit
	NULL, // clone
	NULL, // marshale
	TRUE // unown
);
