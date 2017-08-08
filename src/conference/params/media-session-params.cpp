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

#include "private.h"

using namespace std;

LINPHONE_BEGIN_NAMESPACE

// =============================================================================

MediaSessionParamsPrivate::MediaSessionParamsPrivate (const MediaSessionParamsPrivate &src) : CallSessionParamsPrivate(src) {
	audioEnabled = src.audioEnabled;
	audioBandwidthLimit = src.audioBandwidthLimit;
	audioDirection = src.audioDirection;
	audioMulticastEnabled = src.audioMulticastEnabled;
	usedAudioCodec = src.usedAudioCodec;
	videoEnabled = src.videoEnabled;
	videoDirection = src.videoDirection;
	videoMulticastEnabled = src.videoMulticastEnabled;
	usedVideoCodec = src.usedVideoCodec;
	receivedFps = src.receivedFps;
	receivedVideoDefinition = src.receivedVideoDefinition ? linphone_video_definition_ref(src.receivedVideoDefinition) : nullptr;
	sentFps = src.sentFps;
	sentVideoDefinition = src.sentVideoDefinition ? linphone_video_definition_ref(src.sentVideoDefinition) : nullptr;
	realtimeTextEnabled = src.realtimeTextEnabled;
	usedRealtimeTextCodec = src.usedRealtimeTextCodec;
	avpfEnabled = src.avpfEnabled;
	avpfRrInterval = src.avpfRrInterval;
	lowBandwidthEnabled = src.lowBandwidthEnabled;
	recordFilePath = src.recordFilePath;
	earlyMediaSendingEnabled = src.earlyMediaSendingEnabled;
	encryption = src.encryption;
	mandatoryMediaEncryptionEnabled = src.mandatoryMediaEncryptionEnabled;
	_implicitRtcpFbEnabled = src._implicitRtcpFbEnabled;
	downBandwidth = src.downBandwidth;
	upBandwidth = src.upBandwidth;
	downPtime = src.downPtime;
	upPtime = src.upPtime;
	updateCallWhenIceCompleted = src.updateCallWhenIceCompleted;
}

MediaSessionParamsPrivate::~MediaSessionParamsPrivate () {
	if (receivedVideoDefinition)
		linphone_video_definition_unref(receivedVideoDefinition);
	if (sentVideoDefinition)
		linphone_video_definition_unref(sentVideoDefinition);
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

// =============================================================================

MediaSessionParams::MediaSessionParams () : CallSessionParams(*new MediaSessionParamsPrivate) {}

MediaSessionParams::MediaSessionParams (const MediaSessionParams &src)
	: CallSessionParams(*new MediaSessionParamsPrivate(*src.getPrivate())) {}

// -----------------------------------------------------------------------------

bool MediaSessionParams::audioEnabled () const {
	L_D(const MediaSessionParams);
	return d->audioEnabled;
}

bool MediaSessionParams::audioMulticastEnabled () const {
	L_D(const MediaSessionParams);
	return d->audioMulticastEnabled;
}

void MediaSessionParams::enableAudio (bool value) {
	L_D(MediaSessionParams);
	d->audioEnabled = value;
	if (d->audioEnabled && (getAudioDirection() == LinphoneMediaDirectionInactive))
		setAudioDirection(LinphoneMediaDirectionSendRecv);
}

void MediaSessionParams::enableAudioMulticast (bool value) {
	L_D(MediaSessionParams);
	d->audioMulticastEnabled = value;
}

int MediaSessionParams::getAudioBandwidthLimit () const {
	L_D(const MediaSessionParams);
	return d->audioBandwidthLimit;
}

LinphoneMediaDirection MediaSessionParams::getAudioDirection () const {
	L_D(const MediaSessionParams);
	return d->audioDirection;
}

const OrtpPayloadType * MediaSessionParams::getUsedAudioCodec () const {
	L_D(const MediaSessionParams);
	return d->usedAudioCodec;
}

LinphonePayloadType * MediaSessionParams::getUsedAudioPayloadType () const {
	L_D(const MediaSessionParams);
	return d->usedAudioCodec ? linphone_payload_type_new(nullptr, d->usedAudioCodec) : nullptr;
}

void MediaSessionParams::setAudioBandwidthLimit (int value) {
	L_D(MediaSessionParams);
	d->audioBandwidthLimit = value;
}

void MediaSessionParams::setAudioDirection (LinphoneMediaDirection direction) {
	L_D(MediaSessionParams);
	d->audioDirection = direction;
}

// -----------------------------------------------------------------------------

void MediaSessionParams::enableVideo (bool value) {
	L_D(MediaSessionParams);
	d->videoEnabled = value;
	if (d->videoEnabled && (getVideoDirection() == LinphoneMediaDirectionInactive))
		setVideoDirection(LinphoneMediaDirectionSendRecv);
}

void MediaSessionParams::enableVideoMulticast (bool value) {
	L_D(MediaSessionParams);
	d->videoMulticastEnabled = value;
}

float MediaSessionParams::getReceivedFps () const {
	L_D(const MediaSessionParams);
	return d->receivedFps;
}

LinphoneVideoDefinition * MediaSessionParams::getReceivedVideoDefinition () const {
	L_D(const MediaSessionParams);
	return d->receivedVideoDefinition;
}

float MediaSessionParams::getSentFps () const {
	L_D(const MediaSessionParams);
	return d->sentFps;
}

LinphoneVideoDefinition * MediaSessionParams::getSentVideoDefinition () const {
	L_D(const MediaSessionParams);
	return d->sentVideoDefinition;
}

const OrtpPayloadType * MediaSessionParams::getUsedVideoCodec () const {
	L_D(const MediaSessionParams);
	return d->usedVideoCodec;
}

LinphonePayloadType * MediaSessionParams::getUsedVideoPayloadType () const {
	L_D(const MediaSessionParams);
	return d->usedVideoCodec ? linphone_payload_type_new(nullptr, d->usedVideoCodec) : nullptr;
}

LinphoneMediaDirection MediaSessionParams::getVideoDirection () const {
	L_D(const MediaSessionParams);
	return d->videoDirection;
}

void MediaSessionParams::setVideoDirection (LinphoneMediaDirection direction) {
	L_D(MediaSessionParams);
	d->videoDirection = direction;
}

bool MediaSessionParams::videoEnabled () const {
	L_D(const MediaSessionParams);
	return d->videoEnabled;
}

bool MediaSessionParams::videoMulticastEnabled () const {
	L_D(const MediaSessionParams);
	return d->videoMulticastEnabled;
}

// -----------------------------------------------------------------------------

void MediaSessionParams::enableRealtimeText (bool value) {
	L_D(MediaSessionParams);
	d->realtimeTextEnabled = value;
}

const OrtpPayloadType * MediaSessionParams::getUsedRealtimeTextCodec () const {
	L_D(const MediaSessionParams);
	return d->usedRealtimeTextCodec;
}

LinphonePayloadType * MediaSessionParams::getUsedRealtimeTextPayloadType () const {
	L_D(const MediaSessionParams);
	return d->usedRealtimeTextCodec ? linphone_payload_type_new(nullptr, d->usedRealtimeTextCodec) : nullptr;
}

bool MediaSessionParams::realtimeTextEnabled () const {
	L_D(const MediaSessionParams);
	return d->realtimeTextEnabled;
}

// -----------------------------------------------------------------------------

bool MediaSessionParams::avpfEnabled () const {
	L_D(const MediaSessionParams);
	return d->avpfEnabled;
}

void MediaSessionParams::enableAvpf (bool value) {
	L_D(MediaSessionParams);
	d->avpfEnabled = value;
}

uint16_t MediaSessionParams::getAvpfRrInterval () const {
	L_D(const MediaSessionParams);
	return d->avpfRrInterval;
}

void MediaSessionParams::setAvpfRrInterval (uint16_t value) {
	L_D(MediaSessionParams);
	d->avpfRrInterval = value;
}

// -----------------------------------------------------------------------------

bool MediaSessionParams::lowBandwidthEnabled () const {
	L_D(const MediaSessionParams);
	return d->lowBandwidthEnabled;
}

void MediaSessionParams::enableLowBandwidth (bool value) {
	L_D(MediaSessionParams);
	d->lowBandwidthEnabled = value;
}

// -----------------------------------------------------------------------------

const string& MediaSessionParams::getRecordFilePath () const {
	L_D(const MediaSessionParams);
	return d->recordFilePath;
}

void MediaSessionParams::setRecordFilePath (const string &path) {
	L_D(MediaSessionParams);
	d->recordFilePath = path;
}

// -----------------------------------------------------------------------------

bool MediaSessionParams::earlyMediaSendingEnabled () const {
	L_D(const MediaSessionParams);
	return d->earlyMediaSendingEnabled;
}

void MediaSessionParams::enableEarlyMediaSending (bool value) {
	L_D(MediaSessionParams);
	d->earlyMediaSendingEnabled = value;
}

// -----------------------------------------------------------------------------

void MediaSessionParams::enableMandatoryMediaEncryption (bool value) {
	L_D(MediaSessionParams);
	d->mandatoryMediaEncryptionEnabled = value;
}

LinphoneMediaEncryption MediaSessionParams::getMediaEncryption () const {
	L_D(const MediaSessionParams);
	return d->encryption;
}

bool MediaSessionParams::mandatoryMediaEncryptionEnabled () const {
	L_D(const MediaSessionParams);
	return d->mandatoryMediaEncryptionEnabled;
}

void MediaSessionParams::setMediaEncryption (LinphoneMediaEncryption encryption) {
	L_D(MediaSessionParams);
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

LINPHONE_END_NAMESPACE
