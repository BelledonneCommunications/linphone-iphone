/*
 * call.h
 * Copyright (C) 2010-2017 Belledonne Communications SARL
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

#ifndef _CALL_CALL_H_
#define _CALL_CALL_H_

#include "conference/params/media-session-params.h"
#include "object/object.h"

// =============================================================================

LINPHONE_BEGIN_NAMESPACE

class Address;
class CallPrivate;
class CallSessionPrivate;
class MediaSessionPrivate;

class Call : public Object {
	friend class CallSessionPrivate;
	friend class MediaSessionPrivate;

public:
	Call (
		LinphoneCall *call,
		LinphoneCore *core,
		LinphoneCallDir direction,
		const Address &from,
		const Address &to,
		LinphoneProxyConfig *cfg,
		SalCallOp *op,
		const MediaSessionParams *msp
	);

	LinphoneStatus accept (const MediaSessionParams *msp = nullptr);
	LinphoneStatus acceptEarlyMedia (const MediaSessionParams *msp = nullptr);
	LinphoneStatus acceptUpdate (const MediaSessionParams *msp);
	LinphoneStatus decline (LinphoneReason reason);
	LinphoneStatus decline (const LinphoneErrorInfo *ei);
	LinphoneStatus pause ();
	LinphoneStatus redirect (const std::string &redirectUri);
	LinphoneStatus resume ();
	void sendVfuRequest ();
	void startRecording ();
	void stopRecording ();
	LinphoneStatus takePreviewSnapshot (const std::string &file);
	LinphoneStatus takeVideoSnapshot (const std::string &file);
	LinphoneStatus terminate (const LinphoneErrorInfo *ei = nullptr);
	LinphoneStatus update (const MediaSessionParams *msp = nullptr);
	void zoomVideo (float zoomFactor, float *cx, float *cy);
	void zoomVideo (float zoomFactor, float cx, float cy);

	bool cameraEnabled () const;
	bool echoCancellationEnabled () const;
	bool echoLimiterEnabled () const;
	void enableCamera (bool value);
	void enableEchoCancellation (bool value);
	void enableEchoLimiter (bool value);
	bool getAllMuted () const;
	LinphoneCallStats *getAudioStats () const;
	std::string getAuthenticationToken () const;
	bool getAuthenticationTokenVerified () const;
	float getAverageQuality () const;
	LinphoneCore *getCore () const;
	const MediaSessionParams *getCurrentParams () const;
	float getCurrentQuality () const;
	LinphoneCallDir getDirection () const;
	int getDuration () const;
	const LinphoneErrorInfo *getErrorInfo () const;
	LinphoneCallLog *getLog () const;
	RtpTransport *getMetaRtcpTransport (int streamIndex) const;
	RtpTransport *getMetaRtpTransport (int streamIndex) const;
	float getMicrophoneVolumeGain () const;
	void *getNativeVideoWindowId () const;
	const MediaSessionParams *getParams () const;
	float getPlayVolume () const;
	LinphoneReason getReason () const;
	float getRecordVolume () const;
	const Address &getRemoteAddress () const;
	std::string getRemoteAddressAsString () const;
	std::string getRemoteContact () const;
	const MediaSessionParams *getRemoteParams () const;
	std::string getRemoteUserAgent () const;
	float getSpeakerVolumeGain () const;
	LinphoneCallState getState () const;
	LinphoneCallStats *getStats (LinphoneStreamType type) const;
	int getStreamCount () const;
	MSFormatType getStreamType (int streamIndex) const;
	LinphoneCallStats *getTextStats () const;
	LinphoneCallStats *getVideoStats () const;
	bool mediaInProgress () const;
	void setAuthenticationTokenVerified (bool value);
	void setMicrophoneVolumeGain (float value);
	void setNativeVideoWindowId (void *id);
	void setNextVideoFrameDecodedCallback (LinphoneCallCbFunc cb, void *user_data);
	void setSpeakerVolumeGain (float value);

private:
	L_DECLARE_PRIVATE(Call);
	L_DISABLE_COPY(Call);
};

LINPHONE_END_NAMESPACE

#endif // ifndef _CALL_CALL_H_
