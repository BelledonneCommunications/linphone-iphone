/*
 * media-session-params.cpp
 * Copyright (C) 2017  Belledonne Communications SARL
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "call-session-params-p.h"
#include "media-session-params-p.h"

#include "media-session-params.h"

#include "logger/logger.h"

#include "private.h"

using namespace std;

LINPHONE_BEGIN_NAMESPACE

// =============================================================================

MediaSessionParamsPrivate::MediaSessionParamsPrivate () {
	memset(customSdpMediaAttributes, 0, sizeof(customSdpMediaAttributes));
}

MediaSessionParamsPrivate::MediaSessionParamsPrivate (const MediaSessionParamsPrivate &src) : CallSessionParamsPrivate(src) {
	clone(src, *this);
}

MediaSessionParamsPrivate::~MediaSessionParamsPrivate () {
	this->clean();
}

MediaSessionParamsPrivate &MediaSessionParamsPrivate::operator= (const MediaSessionParamsPrivate &src) {
	if (this != &src) {
		this->clean();
		clone(src, *this);
	}
	return *this;
}

void MediaSessionParamsPrivate::clone (const MediaSessionParamsPrivate &src, MediaSessionParamsPrivate &dst) {
	dst.audioEnabled = src.audioEnabled;
	dst.audioBandwidthLimit = src.audioBandwidthLimit;
	dst.audioDirection = src.audioDirection;
	dst.audioMulticastEnabled = src.audioMulticastEnabled;
	dst.usedAudioCodec = src.usedAudioCodec;
	dst.videoEnabled = src.videoEnabled;
	dst.videoDirection = src.videoDirection;
	dst.videoMulticastEnabled = src.videoMulticastEnabled;
	dst.usedVideoCodec = src.usedVideoCodec;
	dst.receivedFps = src.receivedFps;
	dst.receivedVideoDefinition = src.receivedVideoDefinition ? linphone_video_definition_ref(src.receivedVideoDefinition) : nullptr;
	dst.sentFps = src.sentFps;
	dst.sentVideoDefinition = src.sentVideoDefinition ? linphone_video_definition_ref(src.sentVideoDefinition) : nullptr;
	dst.realtimeTextEnabled = src.realtimeTextEnabled;
	dst.usedRealtimeTextCodec = src.usedRealtimeTextCodec;
	dst.avpfEnabled = src.avpfEnabled;
	dst.avpfRrInterval = src.avpfRrInterval;
	dst.lowBandwidthEnabled = src.lowBandwidthEnabled;
	dst.recordFilePath = src.recordFilePath;
	dst.earlyMediaSendingEnabled = src.earlyMediaSendingEnabled;
	dst.encryption = src.encryption;
	dst.mandatoryMediaEncryptionEnabled = src.mandatoryMediaEncryptionEnabled;
	dst._implicitRtcpFbEnabled = src._implicitRtcpFbEnabled;
	dst.downBandwidth = src.downBandwidth;
	dst.upBandwidth = src.upBandwidth;
	dst.downPtime = src.downPtime;
	dst.upPtime = src.upPtime;
	dst.updateCallWhenIceCompleted = src.updateCallWhenIceCompleted;
	if (src.customSdpAttributes)
		dst.customSdpAttributes = sal_custom_sdp_attribute_clone(src.customSdpAttributes);
	memset(dst.customSdpMediaAttributes, 0, sizeof(dst.customSdpMediaAttributes));
	for (unsigned int i = 0; i < (unsigned int)LinphoneStreamTypeUnknown; i++) {
		if (src.customSdpMediaAttributes[i])
			dst.customSdpMediaAttributes[i] = sal_custom_sdp_attribute_clone(src.customSdpMediaAttributes[i]);
	}
}

void MediaSessionParamsPrivate::clean () {
	if (receivedVideoDefinition)
		linphone_video_definition_unref(receivedVideoDefinition);
	if (sentVideoDefinition)
		linphone_video_definition_unref(sentVideoDefinition);
	if (customSdpAttributes)
		sal_custom_sdp_attribute_free(customSdpAttributes);
	for (unsigned int i = 0; i < (unsigned int)LinphoneStreamTypeUnknown; i++) {
		if (customSdpMediaAttributes[i])
			sal_custom_sdp_attribute_free(customSdpMediaAttributes[i]);
	}
}

// -----------------------------------------------------------------------------

SalStreamDir MediaSessionParamsPrivate::mediaDirectionToSalStreamDir (LinphoneMediaDirection direction) {
	switch (direction) {
		case LinphoneMediaDirectionInactive:
			return SalStreamInactive;
		case LinphoneMediaDirectionSendOnly:
			return SalStreamSendOnly;
		case LinphoneMediaDirectionRecvOnly:
			return SalStreamRecvOnly;
		case LinphoneMediaDirectionSendRecv:
			return SalStreamSendRecv;
		case LinphoneMediaDirectionInvalid:
			lError() << "LinphoneMediaDirectionInvalid shall not be used";
			return SalStreamInactive;
	}
	return SalStreamSendRecv;
}

LinphoneMediaDirection MediaSessionParamsPrivate::salStreamDirToMediaDirection (SalStreamDir dir) {
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

// -----------------------------------------------------------------------------

void MediaSessionParamsPrivate::adaptToNetwork (LinphoneCore *core, int pingTimeMs) {
	L_Q();
	if ((pingTimeMs > 0) && lp_config_get_int(linphone_core_get_config(core), "net", "activate_edge_workarounds", 0)) {
		lInfo() << "STUN server ping time is " << pingTimeMs << " ms";
		int threshold = lp_config_get_int(linphone_core_get_config(core), "net", "edge_ping_time", 500);
		if (pingTimeMs > threshold) {
			/* We might be in a 2G network */
			q->enableLowBandwidth(true);
		} /* else use default settings */
	}
	if (q->lowBandwidthEnabled()) {
		setUpBandwidth(linphone_core_get_edge_bw(core));
		setDownBandwidth(linphone_core_get_edge_bw(core));
		setUpPtime(linphone_core_get_edge_ptime(core));
		setDownPtime(linphone_core_get_edge_ptime(core));
		q->enableVideo(false);
	}
}

// -----------------------------------------------------------------------------

SalStreamDir MediaSessionParamsPrivate::getSalAudioDirection () const {
	L_Q();
	return mediaDirectionToSalStreamDir(q->getAudioDirection());
}

SalStreamDir MediaSessionParamsPrivate::getSalVideoDirection () const {
	L_Q();
	return mediaDirectionToSalStreamDir(q->getVideoDirection());
}

// -----------------------------------------------------------------------------

void MediaSessionParamsPrivate::setReceivedVideoDefinition (LinphoneVideoDefinition *value) {
	if (receivedVideoDefinition)
		linphone_video_definition_unref(receivedVideoDefinition);
	receivedVideoDefinition = linphone_video_definition_ref(value);
}

void MediaSessionParamsPrivate::setSentVideoDefinition (LinphoneVideoDefinition *value) {
	if (sentVideoDefinition)
		linphone_video_definition_unref(sentVideoDefinition);
	sentVideoDefinition = linphone_video_definition_ref(value);
}

// -----------------------------------------------------------------------------

SalCustomSdpAttribute * MediaSessionParamsPrivate::getCustomSdpAttributes () const {
	return customSdpAttributes;
}

void MediaSessionParamsPrivate::setCustomSdpAttributes (const SalCustomSdpAttribute *csa) {
	if (customSdpAttributes) {
		sal_custom_sdp_attribute_free(customSdpAttributes);
		customSdpAttributes = nullptr;
	}
	if (csa)
		customSdpAttributes = sal_custom_sdp_attribute_clone(csa);
}

// -----------------------------------------------------------------------------

SalCustomSdpAttribute * MediaSessionParamsPrivate::getCustomSdpMediaAttributes (LinphoneStreamType lst) const {
	return customSdpMediaAttributes[lst];
}

void MediaSessionParamsPrivate::setCustomSdpMediaAttributes (LinphoneStreamType lst, const SalCustomSdpAttribute *csa) {
	if (customSdpMediaAttributes[lst]) {
		sal_custom_sdp_attribute_free(customSdpMediaAttributes[lst]);
		customSdpMediaAttributes[lst] = nullptr;
	}
	if (csa)
		customSdpMediaAttributes[lst] = sal_custom_sdp_attribute_clone(csa);
}

// =============================================================================

MediaSessionParams::MediaSessionParams () : CallSessionParams(*new MediaSessionParamsPrivate) {}

MediaSessionParams::MediaSessionParams (const MediaSessionParams &src)
	: CallSessionParams(*new MediaSessionParamsPrivate(*src.getPrivate())) {}

MediaSessionParams &MediaSessionParams::operator= (const MediaSessionParams &src) {
	L_D();
	if (this != &src) {
		CallSessionParams::operator=(src);
		*d = *src.getPrivate();
	}
	return *this;
}

// -----------------------------------------------------------------------------

void MediaSessionParams::initDefault (LinphoneCore *core) {
	L_D();
	CallSessionParams::initDefault(core);
	d->audioEnabled = true;
	d->videoEnabled = linphone_core_video_enabled(core) && core->video_policy.automatically_initiate;
	if (!linphone_core_video_enabled(core) && core->video_policy.automatically_initiate) {
		lError() << "LinphoneCore has video disabled for both capture and display, but video policy is to start the call with video. "
			"This is a possible mis-use of the API. In this case, video is disabled in default LinphoneCallParams";
	}
	d->realtimeTextEnabled = linphone_core_realtime_text_enabled(core);
	d->encryption = linphone_core_get_media_encryption(core);
	d->avpfEnabled = (linphone_core_get_avpf_mode(core) == LinphoneAVPFEnabled);
	d->_implicitRtcpFbEnabled = lp_config_get_int(linphone_core_get_config(core), "rtp", "rtcp_fb_implicit_rtcp_fb", true);
	d->avpfRrInterval = static_cast<uint16_t>(linphone_core_get_avpf_rr_interval(core));
	d->audioDirection = LinphoneMediaDirectionSendRecv;
	d->videoDirection = LinphoneMediaDirectionSendRecv;
	d->earlyMediaSendingEnabled = lp_config_get_int(linphone_core_get_config(core), "misc", "real_early_media", false);
	d->audioMulticastEnabled = linphone_core_audio_multicast_enabled(core);
	d->videoMulticastEnabled = linphone_core_video_multicast_enabled(core);
	d->updateCallWhenIceCompleted = lp_config_get_int(linphone_core_get_config(core), "sip", "update_call_when_ice_completed", true);
	d->mandatoryMediaEncryptionEnabled = linphone_core_is_media_encryption_mandatory(core);
}

// -----------------------------------------------------------------------------

bool MediaSessionParams::audioEnabled () const {
	L_D();
	return d->audioEnabled;
}

bool MediaSessionParams::audioMulticastEnabled () const {
	L_D();
	return d->audioMulticastEnabled;
}

void MediaSessionParams::enableAudio (bool value) {
	L_D();
	d->audioEnabled = value;
	if (d->audioEnabled && (getAudioDirection() == LinphoneMediaDirectionInactive))
		setAudioDirection(LinphoneMediaDirectionSendRecv);
}

void MediaSessionParams::enableAudioMulticast (bool value) {
	L_D();
	d->audioMulticastEnabled = value;
}

int MediaSessionParams::getAudioBandwidthLimit () const {
	L_D();
	return d->audioBandwidthLimit;
}

LinphoneMediaDirection MediaSessionParams::getAudioDirection () const {
	L_D();
	return d->audioDirection;
}

const OrtpPayloadType * MediaSessionParams::getUsedAudioCodec () const {
	L_D();
	return d->usedAudioCodec;
}

LinphonePayloadType * MediaSessionParams::getUsedAudioPayloadType () const {
	L_D();
	return d->usedAudioCodec ? linphone_payload_type_new(nullptr, d->usedAudioCodec) : nullptr;
}

void MediaSessionParams::setAudioBandwidthLimit (int value) {
	L_D();
	d->audioBandwidthLimit = value;
}

void MediaSessionParams::setAudioDirection (LinphoneMediaDirection direction) {
	L_D();
	d->audioDirection = direction;
}

// -----------------------------------------------------------------------------

void MediaSessionParams::enableVideo (bool value) {
	L_D();
	d->videoEnabled = value;
	if (d->videoEnabled && (getVideoDirection() == LinphoneMediaDirectionInactive))
		setVideoDirection(LinphoneMediaDirectionSendRecv);
}

void MediaSessionParams::enableVideoMulticast (bool value) {
	L_D();
	d->videoMulticastEnabled = value;
}

float MediaSessionParams::getReceivedFps () const {
	L_D();
	return d->receivedFps;
}

LinphoneVideoDefinition * MediaSessionParams::getReceivedVideoDefinition () const {
	L_D();
	return d->receivedVideoDefinition;
}

float MediaSessionParams::getSentFps () const {
	L_D();
	return d->sentFps;
}

LinphoneVideoDefinition * MediaSessionParams::getSentVideoDefinition () const {
	L_D();
	return d->sentVideoDefinition;
}

const OrtpPayloadType * MediaSessionParams::getUsedVideoCodec () const {
	L_D();
	return d->usedVideoCodec;
}

LinphonePayloadType * MediaSessionParams::getUsedVideoPayloadType () const {
	L_D();
	return d->usedVideoCodec ? linphone_payload_type_new(nullptr, d->usedVideoCodec) : nullptr;
}

LinphoneMediaDirection MediaSessionParams::getVideoDirection () const {
	L_D();
	return d->videoDirection;
}

void MediaSessionParams::setVideoDirection (LinphoneMediaDirection direction) {
	L_D();
	d->videoDirection = direction;
}

bool MediaSessionParams::videoEnabled () const {
	L_D();
	return d->videoEnabled;
}

bool MediaSessionParams::videoMulticastEnabled () const {
	L_D();
	return d->videoMulticastEnabled;
}

// -----------------------------------------------------------------------------

void MediaSessionParams::enableRealtimeText (bool value) {
	L_D();
	d->realtimeTextEnabled = value;
}

const OrtpPayloadType * MediaSessionParams::getUsedRealtimeTextCodec () const {
	L_D();
	return d->usedRealtimeTextCodec;
}

LinphonePayloadType * MediaSessionParams::getUsedRealtimeTextPayloadType () const {
	L_D();
	return d->usedRealtimeTextCodec ? linphone_payload_type_new(nullptr, d->usedRealtimeTextCodec) : nullptr;
}

bool MediaSessionParams::realtimeTextEnabled () const {
	L_D();
	return d->realtimeTextEnabled;
}

// -----------------------------------------------------------------------------

bool MediaSessionParams::avpfEnabled () const {
	L_D();
	return d->avpfEnabled;
}

void MediaSessionParams::enableAvpf (bool value) {
	L_D();
	d->avpfEnabled = value;
}

uint16_t MediaSessionParams::getAvpfRrInterval () const {
	L_D();
	return d->avpfRrInterval;
}

void MediaSessionParams::setAvpfRrInterval (uint16_t value) {
	L_D();
	d->avpfRrInterval = value;
}

// -----------------------------------------------------------------------------

bool MediaSessionParams::lowBandwidthEnabled () const {
	L_D();
	return d->lowBandwidthEnabled;
}

void MediaSessionParams::enableLowBandwidth (bool value) {
	L_D();
	d->lowBandwidthEnabled = value;
}

// -----------------------------------------------------------------------------

const string& MediaSessionParams::getRecordFilePath () const {
	L_D();
	return d->recordFilePath;
}

void MediaSessionParams::setRecordFilePath (const string &path) {
	L_D();
	d->recordFilePath = path;
}

// -----------------------------------------------------------------------------

bool MediaSessionParams::earlyMediaSendingEnabled () const {
	L_D();
	return d->earlyMediaSendingEnabled;
}

void MediaSessionParams::enableEarlyMediaSending (bool value) {
	L_D();
	d->earlyMediaSendingEnabled = value;
}

// -----------------------------------------------------------------------------

void MediaSessionParams::enableMandatoryMediaEncryption (bool value) {
	L_D();
	d->mandatoryMediaEncryptionEnabled = value;
}

LinphoneMediaEncryption MediaSessionParams::getMediaEncryption () const {
	L_D();
	return d->encryption;
}

bool MediaSessionParams::mandatoryMediaEncryptionEnabled () const {
	L_D();
	return d->mandatoryMediaEncryptionEnabled;
}

void MediaSessionParams::setMediaEncryption (LinphoneMediaEncryption encryption) {
	L_D();
	d->encryption = encryption;
}

// -----------------------------------------------------------------------------

SalMediaProto MediaSessionParams::getMediaProto () const {
	if ((getMediaEncryption() == LinphoneMediaEncryptionSRTP) && avpfEnabled()) return SalProtoRtpSavpf;
	if (getMediaEncryption() == LinphoneMediaEncryptionSRTP) return SalProtoRtpSavp;
	if ((getMediaEncryption() == LinphoneMediaEncryptionDTLS) && avpfEnabled()) return SalProtoUdpTlsRtpSavpf;
	if (getMediaEncryption() == LinphoneMediaEncryptionDTLS) return SalProtoUdpTlsRtpSavp;
	if (avpfEnabled()) return SalProtoRtpAvpf;
	return SalProtoRtpAvp;
}

const char * MediaSessionParams::getRtpProfile () const {
	return sal_media_proto_to_string(getMediaProto());
}

// -----------------------------------------------------------------------------

void MediaSessionParams::addCustomSdpAttribute (const string &attributeName, const string &attributeValue) {
	L_D();
	d->customSdpAttributes = sal_custom_sdp_attribute_append(d->customSdpAttributes, attributeName.c_str(), attributeValue.c_str());
}

void MediaSessionParams::clearCustomSdpAttributes () {
	L_D();
	d->setCustomSdpAttributes(nullptr);
}

const char * MediaSessionParams::getCustomSdpAttribute (const string &attributeName) const {
	L_D();
	return sal_custom_sdp_attribute_find(d->customSdpAttributes, attributeName.c_str());
}

// -----------------------------------------------------------------------------

void MediaSessionParams::addCustomSdpMediaAttribute (LinphoneStreamType lst, const string &attributeName, const string &attributeValue) {
	L_D();
	d->customSdpMediaAttributes[lst] = sal_custom_sdp_attribute_append(d->customSdpMediaAttributes[lst], attributeName.c_str(), attributeValue.c_str());
}

void MediaSessionParams::clearCustomSdpMediaAttributes (LinphoneStreamType lst) {
	L_D();
	d->setCustomSdpMediaAttributes(lst, nullptr);
}

const char * MediaSessionParams::getCustomSdpMediaAttribute (LinphoneStreamType lst, const string &attributeName) const {
	L_D();
	return sal_custom_sdp_attribute_find(d->customSdpMediaAttributes[lst], attributeName.c_str());
}

LINPHONE_END_NAMESPACE
