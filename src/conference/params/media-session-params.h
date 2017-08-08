/*
 * media-session-params.h
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

#ifndef _MEDIA_SESSION_PARAMS_H_
#define _MEDIA_SESSION_PARAMS_H_

#include "call-session-params.h"

#include <ortp/payloadtype.h>

extern "C" {
	bool_t linphone_call_params_implicit_rtcp_fb_enabled(const LinphoneCallParams *params);
	void linphone_call_params_enable_implicit_rtcp_fb(LinphoneCallParams *params, bool_t value);
	int linphone_call_params_get_down_bandwidth(const LinphoneCallParams *params);
	void linphone_call_params_set_down_bandwidth(LinphoneCallParams *params, int value);
	int linphone_call_params_get_up_bandwidth(const LinphoneCallParams *params);
	void linphone_call_params_set_up_bandwidth(LinphoneCallParams *params, int value);
	int linphone_call_params_get_down_ptime(const LinphoneCallParams *params);
	void linphone_call_params_set_down_ptime(LinphoneCallParams *params, int value);
	int linphone_call_params_get_up_ptime(const LinphoneCallParams *params);
	void linphone_call_params_set_up_ptime(LinphoneCallParams *params, int value);
	bool_t linphone_call_params_get_update_call_when_ice_completed(const LinphoneCallParams *params);
	void linphone_call_params_set_update_call_when_ice_completed(LinphoneCallParams *params, bool_t value);
	void linphone_call_params_set_received_fps(LinphoneCallParams *params, float value);
	void linphone_call_params_set_received_video_definition(LinphoneCallParams *params, LinphoneVideoDefinition *vdef);
	void linphone_call_params_set_recv_vsize(LinphoneCallParams *params, MSVideoSize vsize);
	void linphone_call_params_set_sent_fps(LinphoneCallParams *params, float value);
	void linphone_call_params_set_sent_video_definition(LinphoneCallParams *params, LinphoneVideoDefinition *vdef);
	void linphone_call_params_set_sent_vsize(LinphoneCallParams *params, MSVideoSize vsize);
	void linphone_call_params_set_used_audio_codec(LinphoneCallParams *params, OrtpPayloadType *codec);
	void linphone_call_params_set_used_video_codec(LinphoneCallParams *params, OrtpPayloadType *codec);
	void linphone_call_params_set_used_text_codec(LinphoneCallParams *params, OrtpPayloadType *codec);
}

// =============================================================================

LINPHONE_BEGIN_NAMESPACE

class MediaSessionParamsPrivate;

class MediaSessionParams : public CallSessionParams {
	friend unsigned char ::linphone_call_params_implicit_rtcp_fb_enabled(const LinphoneCallParams *params);
	friend void ::linphone_call_params_enable_implicit_rtcp_fb(LinphoneCallParams *params, unsigned char value);
	friend int ::linphone_call_params_get_down_bandwidth(const LinphoneCallParams *params);
	friend void ::linphone_call_params_set_down_bandwidth(LinphoneCallParams *params, int value);
	friend int ::linphone_call_params_get_up_bandwidth(const LinphoneCallParams *params);
	friend void ::linphone_call_params_set_up_bandwidth(LinphoneCallParams *params, int value);
	friend int ::linphone_call_params_get_down_ptime(const LinphoneCallParams *params);
	friend void ::linphone_call_params_set_down_ptime(LinphoneCallParams *params, int value);
	friend int ::linphone_call_params_get_up_ptime(const LinphoneCallParams *params);
	friend void ::linphone_call_params_set_up_ptime(LinphoneCallParams *params, int value);
	friend unsigned char ::linphone_call_params_get_update_call_when_ice_completed(const LinphoneCallParams *params);
	friend void ::linphone_call_params_set_update_call_when_ice_completed(LinphoneCallParams *params, unsigned char value);
	friend void ::linphone_call_params_set_received_fps(LinphoneCallParams *params, float value);
	friend void ::linphone_call_params_set_received_video_definition(LinphoneCallParams *params, LinphoneVideoDefinition *vdef);
	friend void ::linphone_call_params_set_recv_vsize(LinphoneCallParams *params, MSVideoSize vsize);
	friend void ::linphone_call_params_set_sent_fps(LinphoneCallParams *params, float value);
	friend void ::linphone_call_params_set_sent_video_definition(LinphoneCallParams *params, LinphoneVideoDefinition *vdef);
	friend void ::linphone_call_params_set_sent_vsize(LinphoneCallParams *params, MSVideoSize vsize);
	friend void ::linphone_call_params_set_used_audio_codec(LinphoneCallParams *params, OrtpPayloadType *codec);
	friend void ::linphone_call_params_set_used_video_codec(LinphoneCallParams *params, OrtpPayloadType *codec);
	friend void ::linphone_call_params_set_used_text_codec(LinphoneCallParams *params, OrtpPayloadType *codec);

public:
	MediaSessionParams ();
	MediaSessionParams (const MediaSessionParams &src);
	virtual ~MediaSessionParams () = default;

	bool audioEnabled () const;
	bool audioMulticastEnabled () const;
	void enableAudio (bool value);
	void enableAudioMulticast (bool value);
	int getAudioBandwidthLimit () const;
	LinphoneMediaDirection getAudioDirection () const;
	const OrtpPayloadType * getUsedAudioCodec () const;
	LinphonePayloadType * getUsedAudioPayloadType () const;
	void setAudioBandwidthLimit (int value);
	void setAudioDirection (LinphoneMediaDirection direction);

	void enableVideo (bool value);
	void enableVideoMulticast (bool value);
	float getReceivedFps () const;
	LinphoneVideoDefinition * getReceivedVideoDefinition () const;
	float getSentFps () const;
	LinphoneVideoDefinition * getSentVideoDefinition () const;
	const OrtpPayloadType * getUsedVideoCodec () const;
	LinphonePayloadType * getUsedVideoPayloadType () const;
	LinphoneMediaDirection getVideoDirection () const;
	void setVideoDirection (LinphoneMediaDirection direction);
	bool videoEnabled () const;
	bool videoMulticastEnabled () const;

	void enableRealtimeText (bool value);
	const OrtpPayloadType * getUsedRealtimeTextCodec () const;
	LinphonePayloadType * getUsedRealtimeTextPayloadType () const;
	bool realtimeTextEnabled () const;

	bool avpfEnabled () const;
	void enableAvpf (bool value);
	uint16_t getAvpfRrInterval () const;
	void setAvpfRrInterval (uint16_t value);

	bool lowBandwidthEnabled () const;
	void enableLowBandwidth (bool value);

	const std::string& getRecordFilePath () const;
	void setRecordFilePath (const std::string &path);

	bool earlyMediaSendingEnabled () const;
	void enableEarlyMediaSending (bool value);

	void enableMandatoryMediaEncryption (bool value);
	LinphoneMediaEncryption getMediaEncryption () const;
	bool mandatoryMediaEncryptionEnabled () const;
	void setMediaEncryption (LinphoneMediaEncryption encryption);

	SalMediaProto getMediaProto () const;
	const char * getRtpProfile () const;

private:
	L_DECLARE_PRIVATE(MediaSessionParams);
};

LINPHONE_END_NAMESPACE

#endif // ifndef _MEDIA_SESSION_PARAMS_H_
