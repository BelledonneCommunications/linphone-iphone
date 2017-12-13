/*
 * call-session-listener.h
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

#ifndef _CALL_SESSION_LISTENER_H_
#define _CALL_SESSION_LISTENER_H_

// =============================================================================

LINPHONE_BEGIN_NAMESPACE

class CallSession;

class LINPHONE_PUBLIC CallSessionListener {
public:
	virtual ~CallSessionListener() = default;

	virtual void onAckBeingSent (const std::shared_ptr<const CallSession> &session, LinphoneHeaders *headers) {}
	virtual void onAckReceived (const std::shared_ptr<const CallSession> &session, LinphoneHeaders *headers) {}
	virtual void onBackgroundTaskToBeStarted (const std::shared_ptr<const CallSession> &session) {}
	virtual void onBackgroundTaskToBeStopped (const std::shared_ptr<const CallSession> &session) {}
	virtual bool onCallSessionAccepted (const std::shared_ptr<const CallSession> &session) { return false; }
	virtual void onCallSessionConferenceStreamStarting (const std::shared_ptr<const CallSession> &session, bool mute) {}
	virtual void onCallSessionConferenceStreamStopping (const std::shared_ptr<const CallSession> &session) {}
	virtual void onCallSessionSetReleased (const std::shared_ptr<const CallSession> &session) {}
	virtual void onCallSessionSetTerminated (const std::shared_ptr<const CallSession> &session) {}
	virtual void onCallSessionStartReferred (const std::shared_ptr<const CallSession> &session) {}
	virtual void onCallSessionStateChanged (const std::shared_ptr<const CallSession> &session, LinphoneCallState state, const std::string &message) {}
	virtual void onCallSessionTransferStateChanged (const std::shared_ptr<const CallSession> &session, LinphoneCallState state) {}
	virtual void onCheckForAcceptation (const std::shared_ptr<const CallSession> &session) {}
	virtual void onDtmfReceived (const std::shared_ptr<const CallSession> &session, char dtmf) {}
	virtual void onIncomingCallSessionNotified (const std::shared_ptr<const CallSession> &session) {}
	virtual void onIncomingCallSessionStarted (const std::shared_ptr<const CallSession> &session) {}
	virtual void onIncomingCallSessionTimeoutCheck (const std::shared_ptr<const CallSession> &session, int elapsed, bool oneSecondElapsed) {}
	virtual void onInfoReceived (const std::shared_ptr<const CallSession> &session, const LinphoneInfoMessage *im) {}
	virtual void onNoMediaTimeoutCheck (const std::shared_ptr<const CallSession> &session, bool oneSecondElapsed) {}

	virtual void onEncryptionChanged (const std::shared_ptr<const CallSession> &session, bool activated, const std::string &authToken) {}

	virtual void onStatsUpdated (const std::shared_ptr<const CallSession> &session, const LinphoneCallStats *stats) {}

	virtual void onResetCurrentSession (const std::shared_ptr<const CallSession> &session) {}
	virtual void onSetCurrentSession (const std::shared_ptr<const CallSession> &session) {}

	virtual void onFirstVideoFrameDecoded (const std::shared_ptr<const CallSession> &session) {}
	virtual void onResetFirstVideoFrameDecoded (const std::shared_ptr<const CallSession> &session) {}

	virtual void onPlayErrorTone (const std::shared_ptr<const CallSession> &session, LinphoneReason reason) {}
	virtual void onRingbackToneRequested (const std::shared_ptr<const CallSession> &session, bool requested) {}
	virtual void onStartRinging (const std::shared_ptr<const CallSession> &session) {}
	virtual void onStopRinging (const std::shared_ptr<const CallSession> &session) {}
	virtual void onStopRingingIfInCall (const std::shared_ptr<const CallSession> &session) {}
	virtual void onStopRingingIfNeeded (const std::shared_ptr<const CallSession> &session) {}

	virtual bool areSoundResourcesAvailable (const std::shared_ptr<const CallSession> &session) { return true; }
	virtual bool isPlayingRingbackTone (const std::shared_ptr<const CallSession> &session) { return false; }
};

LINPHONE_END_NAMESPACE

#endif // ifndef _CALL_SESSION_LISTENER_H_
