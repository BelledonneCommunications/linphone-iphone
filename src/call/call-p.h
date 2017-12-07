/*
 * call-p.h
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

#ifndef _CALL_P_H_
#define _CALL_P_H_

#include "call-listener.h"
#include "call.h"
#include "conference/conference.h"
#include "object/object-p.h"

// TODO: Remove me later.
#include "private.h"

// =============================================================================

LINPHONE_BEGIN_NAMESPACE

class CallPrivate :	public ObjectPrivate, public CallListener {
public:
	CallPrivate () = default;
	virtual ~CallPrivate () = default;

	void initiateIncoming ();
	bool initiateOutgoing ();
	void iterate (time_t currentRealTime, bool oneSecondElapsed);
	void startIncomingNotification ();

	int startInvite (const Address *destination);

	virtual std::shared_ptr<CallSession> getActiveSession () const { return nullptr; }
	bool getAudioMuted () const;

	LinphoneProxyConfig *getDestProxy () const;
	IceSession *getIceSession () const;
	unsigned int getMediaStartCount () const;
	MediaStream *getMediaStream (LinphoneStreamType type) const;
	SalCallOp *getOp () const;
	void setAudioMuted (bool value);

	void createPlayer () const;

private:
	void startRemoteRing ();
	void terminateBecauseOfLostMedia ();

	/* CallListener */
	void onAckBeingSent (LinphoneHeaders *headers) override;
	void onAckReceived (LinphoneHeaders *headers) override;
	void onBackgroundTaskToBeStarted () override;
	void onBackgroundTaskToBeStopped () override;
	bool onCallAccepted () override;
	void onCallSetReleased () override;
	void onCallSetTerminated () override;
	void onCallStateChanged (LinphoneCallState state, const std::string &message) override;
	void onCheckForAcceptation () override;
	void onDtmfReceived (char dtmf) override;
	void onIncomingCallNotified () override;
	void onIncomingCallStarted () override;
	void onIncomingCallTimeoutCheck (int elapsed, bool oneSecondElapsed) override;
	void onInfoReceived (const LinphoneInfoMessage *im) override;
	void onNoMediaTimeoutCheck (bool oneSecondElapsed) override;
	void onEncryptionChanged (bool activated, const std::string &authToken) override;
	void onStatsUpdated (const LinphoneCallStats *stats) override;
	void onResetCurrentCall () override;
	void onSetCurrentCall () override;
	void onFirstVideoFrameDecoded () override;
	void onResetFirstVideoFrameDecoded () override;
	void onPlayErrorTone (LinphoneReason reason) override;
	void onRingbackToneRequested (bool requested) override;
	void onStartRinging () override;
	void onStopRinging () override;
	void onStopRingingIfInCall () override;
	void onStopRingingIfNeeded () override;
	bool isPlayingRingbackTone () override;

	mutable LinphonePlayer *player = nullptr;

	CallCallbackObj nextVideoFrameDecoded;

	unsigned long backgroundTaskId = 0;

	bool ringingBeep = false;
	bool playingRingbackTone = false;

	L_DECLARE_PUBLIC(Call);
};

LINPHONE_END_NAMESPACE

#endif // ifndef _CALL_P_H_
