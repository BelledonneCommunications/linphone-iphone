/*
 * media-session-params-p.h
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

#ifndef _MEDIA_SESSION_PARAMS_P_H_
#define _MEDIA_SESSION_PARAMS_P_H_

#include "media-session-params.h"

// =============================================================================

LINPHONE_BEGIN_NAMESPACE

class MediaSessionParamsPrivate : public CallSessionParamsPrivate {
public:
	MediaSessionParamsPrivate () = default;
	MediaSessionParamsPrivate (const MediaSessionParamsPrivate &src);
	virtual ~MediaSessionParamsPrivate ();

	void enableImplicitRtcpFb (bool value) { _implicitRtcpFbEnabled = value; }
	bool implicitRtcpFbEnabled () const { return _implicitRtcpFbEnabled; }
	int getDownBandwidth () const { return downBandwidth; }
	void setDownBandwidth (int value) { downBandwidth = value; }
	int getUpBandwidth () const { return upBandwidth; }
	void setUpBandwidth (int value) { upBandwidth = value; }
	int getDownPtime () const { return downPtime; }
	void setDownPtime (int value) { downPtime = value; }
	int getUpPtime () const { return upPtime; }
	void setUpPtime (int value) { upPtime = value; }
	bool getUpdateCallWhenIceCompleted () const { return updateCallWhenIceCompleted; }
	void setUpdateCallWhenIceCompleted (bool value) { updateCallWhenIceCompleted = value; }

	void setReceivedFps (float value) { receivedFps = value; }
	void setReceivedVideoDefinition (LinphoneVideoDefinition *value);
	void setSentFps (float value) { sentFps = value; }
	void setSentVideoDefinition (LinphoneVideoDefinition *value);
	void setUsedAudioCodec (OrtpPayloadType *pt) { usedAudioCodec = pt; }
	void setUsedVideoCodec (OrtpPayloadType *pt) { usedVideoCodec = pt; }
	void setUsedRealtimeTextCodec (OrtpPayloadType *pt) { usedRealtimeTextCodec = pt; }

public:
	bool audioEnabled = true;
	int audioBandwidthLimit = 0;
	LinphoneMediaDirection audioDirection = LinphoneMediaDirectionSendRecv;
	bool audioMulticastEnabled = false;
	PayloadType *usedAudioCodec = nullptr;

	bool videoEnabled = false;
	LinphoneMediaDirection videoDirection = LinphoneMediaDirectionSendRecv;
	bool videoMulticastEnabled = false;
	PayloadType *usedVideoCodec = nullptr;
	float receivedFps = 0.f;
	LinphoneVideoDefinition *receivedVideoDefinition = nullptr;
	float sentFps = 0.f;
	LinphoneVideoDefinition *sentVideoDefinition = nullptr;

	bool realtimeTextEnabled = false;
	PayloadType *usedRealtimeTextCodec = nullptr;

	bool avpfEnabled = false;
	uint16_t avpfRrInterval = 0; /* In milliseconds */

	bool lowBandwidthEnabled = false;

	std::string recordFilePath;

	bool earlyMediaSendingEnabled = false; /* Send real media even during early media (for outgoing calls) */

	LinphoneMediaEncryption encryption = LinphoneMediaEncryptionNone;
	bool mandatoryMediaEncryptionEnabled = false;

private:
	bool _implicitRtcpFbEnabled = false;
	int downBandwidth = 0;
	int upBandwidth = 0;
	int downPtime = 0;
	int upPtime = 0;
	bool updateCallWhenIceCompleted = true;

public:
	L_DECLARE_PUBLIC(MediaSessionParams);
};

LINPHONE_END_NAMESPACE

#endif // ifndef _MEDIA_SESSION_PARAMS_P_H_
