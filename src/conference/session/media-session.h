/*
 * media-session.h
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

#ifndef _MEDIA_SESSION_H_
#define _MEDIA_SESSION_H_

#include "call-session.h"
#include "conference/params/media-session-params.h"

// =============================================================================

LINPHONE_BEGIN_NAMESPACE

class CallPrivate;
class IceAgent;
class MediaSessionPrivate;

class MediaSession : public CallSession {
	friend class CallPrivate;
	friend class IceAgent;

public:
	MediaSession (const Conference &conference, const CallSessionParams *params, CallSessionListener *listener);

	LinphoneStatus accept (const MediaSessionParams *msp = nullptr);
	LinphoneStatus acceptEarlyMedia (const MediaSessionParams *msp = nullptr);
	LinphoneStatus acceptUpdate (const MediaSessionParams *msp);
	void configure (LinphoneCallDir direction, LinphoneProxyConfig *cfg, SalCallOp *op, const Address &from, const Address &to) override;
	void initiateIncoming () override;
	bool initiateOutgoing () override;
	void iterate (time_t currentRealTime, bool oneSecondElapsed) override;
	LinphoneStatus pause ();
	LinphoneStatus resume ();
	void sendVfuRequest ();
	void startIncomingNotification () override;
	int startInvite (const Address *destination, const std::string &subject) override;
	void startRecording ();
	void stopRecording ();
	LinphoneStatus update (const MediaSessionParams *msp);

	void resetFirstVideoFrameDecoded ();
	LinphoneStatus takePreviewSnapshot (const std::string& file);
	LinphoneStatus takeVideoSnapshot (const std::string& file);
	void zoomVideo (float zoomFactor, float *cx, float *cy);

	bool cameraEnabled () const;
	bool echoCancellationEnabled () const;
	bool echoLimiterEnabled () const;
	void enableCamera (bool value);
	void enableEchoCancellation (bool value);
	void enableEchoLimiter (bool value);
	bool getAllMuted () const;
	LinphoneCallStats * getAudioStats () const;
	std::string getAuthenticationToken () const;
	bool getAuthenticationTokenVerified () const;
	float getAverageQuality () const;
	MediaSessionParams *getCurrentParams () const;
	float getCurrentQuality () const;
	const MediaSessionParams *getMediaParams () const;
	RtpTransport * getMetaRtcpTransport (int streamIndex) const;
	RtpTransport * getMetaRtpTransport (int streamIndex) const;
	float getMicrophoneVolumeGain () const;
	void * getNativeVideoWindowId () const;
	const CallSessionParams *getParams () const override;
	float getPlayVolume () const;
	float getRecordVolume () const;
	const MediaSessionParams *getRemoteParams ();
	float getSpeakerVolumeGain () const;
	LinphoneCallStats * getStats (LinphoneStreamType type) const;
	int getStreamCount () const;
	MSFormatType getStreamType (int streamIndex) const;
	LinphoneCallStats * getTextStats () const;
	LinphoneCallStats * getVideoStats () const;
	bool mediaInProgress () const;
	void setAuthenticationTokenVerified (bool value);
	void setMicrophoneVolumeGain (float value);
	void setNativeVideoWindowId (void *id);
	void setSpeakerVolumeGain (float value);

private:
	L_DECLARE_PRIVATE(MediaSession);
	L_DISABLE_COPY(MediaSession);
};

LINPHONE_END_NAMESPACE

#endif // ifndef _MEDIA_SESSION_H_
