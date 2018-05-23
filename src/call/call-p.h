/*
 * call-p.h
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

#ifndef _L_CALL_P_H_
#define _L_CALL_P_H_

#include "call.h"
#include "conference/conference.h"
#include "conference/session/call-session-listener.h"
#include "object/object-p.h"
#include "utils/background-task.h"

// TODO: Remove me later.
#include "private.h"

// =============================================================================

LINPHONE_BEGIN_NAMESPACE

class CallSession;
class RealTimeTextChatRoom;

class CallPrivate :	public ObjectPrivate, public CallSessionListener {
public:
	void initiateIncoming ();
	bool initiateOutgoing ();
	void iterate (time_t currentRealTime, bool oneSecondElapsed);
	void startIncomingNotification ();

	void pauseForTransfer ();
	int startInvite (const Address *destination);
	std::shared_ptr<Call> startReferredCall (const MediaSessionParams *params);

	virtual std::shared_ptr<CallSession> getActiveSession () const { return nullptr; }
	bool getAudioMuted () const;
	std::shared_ptr<RealTimeTextChatRoom> getChatRoom ();

	LinphoneProxyConfig *getDestProxy () const;
	IceSession *getIceSession () const;
	unsigned int getMediaStartCount () const;
	MediaStream *getMediaStream (LinphoneStreamType type) const;
	SalCallOp *getOp () const;
	bool getRingingBeep () const { return ringingBeep; }
	void setAudioMuted (bool value);
	void setRingingBeep (bool value) { ringingBeep = value; }
	LinphoneCallStats *getStats (LinphoneStreamType type) const;

	void createPlayer () const;

	void initializeMediaStreams ();
	void stopMediaStreams ();

private:
	void requestNotifyNextVideoFrameDecoded ();
	void startRemoteRing ();
	void terminateBecauseOfLostMedia ();

	/* CallSessionListener */
	void onAckBeingSent (const std::shared_ptr<CallSession> &session, LinphoneHeaders *headers) override;
	void onAckReceived (const std::shared_ptr<CallSession> &session, LinphoneHeaders *headers) override;
	void onBackgroundTaskToBeStarted (const std::shared_ptr<CallSession> &session) override;
	void onBackgroundTaskToBeStopped (const std::shared_ptr<CallSession> &session) override;
	bool onCallSessionAccepted (const std::shared_ptr<CallSession> &session) override;
	void onCallSessionConferenceStreamStarting (const std::shared_ptr<CallSession> &session, bool mute) override;
	void onCallSessionConferenceStreamStopping (const std::shared_ptr<CallSession> &session) override;
	void onCallSessionEarlyFailed (const std::shared_ptr<CallSession> &session, LinphoneErrorInfo *ei) override;
	void onCallSessionSetReleased (const std::shared_ptr<CallSession> &session) override;
	void onCallSessionSetTerminated (const std::shared_ptr<CallSession> &session) override;
	void onCallSessionStartReferred (const std::shared_ptr<CallSession> &session) override;
	void onCallSessionStateChanged (const std::shared_ptr<CallSession> &session, CallSession::State state, const std::string &message) override;
	void onCallSessionTransferStateChanged (const std::shared_ptr<CallSession> &session, CallSession::State state) override;
	void onCheckForAcceptation (const std::shared_ptr<CallSession> &session) override;
	void onDtmfReceived (const std::shared_ptr<CallSession> &session, char dtmf) override;
	void onIncomingCallSessionNotified (const std::shared_ptr<CallSession> &session) override;
	void onIncomingCallSessionStarted (const std::shared_ptr<CallSession> &session) override;
	void onIncomingCallSessionTimeoutCheck (const std::shared_ptr<CallSession> &session, int elapsed, bool oneSecondElapsed) override;
	void onInfoReceived (const std::shared_ptr<CallSession> &session, const LinphoneInfoMessage *im) override;
	void onNoMediaTimeoutCheck (const std::shared_ptr<CallSession> &session, bool oneSecondElapsed) override;
	void onEncryptionChanged (const std::shared_ptr<CallSession> &session, bool activated, const std::string &authToken) override;
	void onCallSessionStateChangedForReporting (const std::shared_ptr<CallSession> &session) override;
	void onRtcpUpdateForReporting (const std::shared_ptr<CallSession> &session, SalStreamType type) override;
	void onStatsUpdated (const std::shared_ptr<CallSession> &session, const LinphoneCallStats *stats) override;
	void onUpdateMediaInfoForReporting (const std::shared_ptr<CallSession> &session, int statsType) override;
	void onResetCurrentSession (const std::shared_ptr<CallSession> &session) override;
	void onSetCurrentSession (const std::shared_ptr<CallSession> &session) override;
	void onFirstVideoFrameDecoded (const std::shared_ptr<CallSession> &session) override;
	void onResetFirstVideoFrameDecoded (const std::shared_ptr<CallSession> &session) override;
	void onPlayErrorTone (const std::shared_ptr<CallSession> &session, LinphoneReason reason) override;
	void onRingbackToneRequested (const std::shared_ptr<CallSession> &session, bool requested) override;
	void onStartRinging (const std::shared_ptr<CallSession> &session) override;
	void onStopRinging (const std::shared_ptr<CallSession> &session) override;
	void onStopRingingIfInCall (const std::shared_ptr<CallSession> &session) override;
	void onStopRingingIfNeeded (const std::shared_ptr<CallSession> &session) override;
	bool areSoundResourcesAvailable (const std::shared_ptr<CallSession> &session) override;
	bool isPlayingRingbackTone (const std::shared_ptr<CallSession> &session) override;
	void onRealTimeTextCharacterReceived (const std::shared_ptr<CallSession> &session, RealtimeTextReceivedCharacter *character) override;
	void onTmmbrReceived(const std::shared_ptr<CallSession> &session, int streamIndex, int tmmbr) override;
	void onSnapshotTaken(const std::shared_ptr<CallSession> &session, const char *file_path) override;

	mutable LinphonePlayer *player = nullptr;

	CallCallbackObj nextVideoFrameDecoded;

	bool ringingBeep = false;
	bool playingRingbackTone = false;

	BackgroundTask bgTask;

	mutable std::shared_ptr<RealTimeTextChatRoom> chatRoom;

	L_DECLARE_PUBLIC(Call);
};

LINPHONE_END_NAMESPACE

#endif // ifndef _L_CALL_P_H_
