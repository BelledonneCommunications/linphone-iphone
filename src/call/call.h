/*
 * call.h
 * Copyright (C) 2010-2018 Belledonne Communications SARL
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

#ifndef _L_CALL_CALL_H_
#define _L_CALL_CALL_H_

#include "conference/params/media-session-params.h"
#include "conference/session/call-session.h"
#include "core/core-accessor.h"
#include "object/object.h"

// =============================================================================

LINPHONE_BEGIN_NAMESPACE

class Address;
class CallPrivate;
class CallSessionPrivate;
class MediaSessionPrivate;

class LINPHONE_PUBLIC Call : public Object, public CoreAccessor {
	friend class CallSessionPrivate;
	friend class ChatMessage;
	friend class ChatMessagePrivate;
	friend class CorePrivate;
	friend class MediaSessionPrivate;

public:
	L_OVERRIDE_SHARED_FROM_THIS(Call);

	LinphoneStatus accept (const MediaSessionParams *msp = nullptr);
	LinphoneStatus acceptEarlyMedia (const MediaSessionParams *msp = nullptr);
	LinphoneStatus acceptUpdate (const MediaSessionParams *msp);
	void cancelDtmfs ();
	LinphoneStatus decline (LinphoneReason reason);
	LinphoneStatus decline (const LinphoneErrorInfo *ei);
	LinphoneStatus deferUpdate ();
	bool hasTransferPending () const;
	void oglRender () const;
	LinphoneStatus pause ();
	LinphoneStatus redirect (const std::string &redirectUri);
	LinphoneStatus resume ();
	LinphoneStatus sendDtmf (char dtmf);
	LinphoneStatus sendDtmfs (const std::string &dtmfs);
	void sendVfuRequest ();
	void startRecording ();
	void stopRecording ();
	LinphoneStatus takePreviewSnapshot (const std::string &file);
	LinphoneStatus takeVideoSnapshot (const std::string &file);
	LinphoneStatus terminate (const LinphoneErrorInfo *ei = nullptr);
	LinphoneStatus transfer (const std::shared_ptr<Call> &dest);
	LinphoneStatus transfer (const std::string &dest);
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
	const MediaSessionParams *getCurrentParams () const;
	float getCurrentQuality () const;
	LinphoneCallDir getDirection () const;
	const Address &getDiversionAddress () const;
	int getDuration () const;
	const LinphoneErrorInfo *getErrorInfo () const;
	const Address &getLocalAddress () const;
	LinphoneCallLog *getLog () const;
	RtpTransport *getMetaRtcpTransport (int streamIndex) const;
	RtpTransport *getMetaRtpTransport (int streamIndex) const;
	float getMicrophoneVolumeGain () const;
	void *getNativeVideoWindowId () const;
	const MediaSessionParams *getParams () const;
	LinphonePlayer *getPlayer () const;
	float getPlayVolume () const;
	LinphoneReason getReason () const;
	float getRecordVolume () const;
	std::shared_ptr<Call> getReferer () const;
	std::string getReferTo () const;
	const Address &getRemoteAddress () const;
	std::string getRemoteContact () const;
	const MediaSessionParams *getRemoteParams () const;
	std::string getRemoteUserAgent () const;
	std::shared_ptr<Call> getReplacedCall () const;
	float getSpeakerVolumeGain () const;
	CallSession::State getState () const;
	LinphoneCallStats *getStats (LinphoneStreamType type) const;
	int getStreamCount () const;
	MSFormatType getStreamType (int streamIndex) const;
	LinphoneCallStats *getTextStats () const;
	const Address &getToAddress () const;
	std::string getToHeader (const std::string &name) const;
	CallSession::State getTransferState () const;
	std::shared_ptr<Call> getTransferTarget () const;
	LinphoneCallStats *getVideoStats () const;
	bool isInConference () const;
	bool mediaInProgress () const;
	void setAudioRoute (LinphoneAudioRoute route);
	void setAuthenticationTokenVerified (bool value);
	void setMicrophoneVolumeGain (float value);
	void setNativeVideoWindowId (void *id);
	void setNextVideoFrameDecodedCallback (LinphoneCallCbFunc cb, void *user_data);
	void requestNotifyNextVideoFrameDecoded();
	void setParams (const MediaSessionParams *msp);
	void setSpeakerVolumeGain (float value);

protected:
	Call (CallPrivate &p, std::shared_ptr<Core> core);

private:
	L_DECLARE_PRIVATE(Call);
	L_DISABLE_COPY(Call);
};

LINPHONE_END_NAMESPACE

#endif // ifndef _L_CALL_CALL_H_
