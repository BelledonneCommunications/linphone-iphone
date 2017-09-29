/*
 * call-p.h
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

#ifndef _CALL_P_H_
#define _CALL_P_H_

#include "call.h"
#include "conference/conference.h"
#include "object/object-p.h"

#include "private.h"

// =============================================================================

LINPHONE_BEGIN_NAMESPACE

class CallPrivate :
	public ObjectPrivate,
	CallListener {
public:
	CallPrivate (
		LinphoneCall *call,
		LinphoneCore *core,
		LinphoneCallDir direction,
		const Address &from,
		const Address &to,
		LinphoneProxyConfig *cfg,
		SalOp *op,
		const MediaSessionParams *msp
	);
	virtual ~CallPrivate ();

	void initiateIncoming ();
	bool initiateOutgoing ();
	void iterate (time_t currentRealTime, bool oneSecondElapsed);
	void startIncomingNotification ();

	int startInvite (const Address *destination);

	std::shared_ptr<CallSession> getActiveSession () const;
	bool getAudioMuted () const;
	Conference *getConference () const {
		return conference;
	}

	LinphoneProxyConfig *getDestProxy () const;
	IceSession *getIceSession () const;
	MediaStream *getMediaStream (LinphoneStreamType type) const;
		SalCallOp *getOp () const;
	void setAudioMuted (bool value);

private:
	/* CallListener */
	void onAckBeingSent (LinphoneHeaders *headers) override;
	void onAckReceived (LinphoneHeaders *headers) override;
	void onCallSetReleased () override;
	void onCallSetTerminated () override;
	void onCallStateChanged (LinphoneCallState state, const std::string &message) override;
	void onCheckForAcceptation () override;
	void onIncomingCallStarted () override;
	void onIncomingCallToBeAdded () override;
	void onEncryptionChanged (bool activated, const std::string &authToken) override;
	void onStatsUpdated (const LinphoneCallStats *stats) override;
	void onResetCurrentCall () override;
	void onSetCurrentCall () override;
	void onFirstVideoFrameDecoded () override;
	void onResetFirstVideoFrameDecoded () override;

	LinphoneCall *lcall = nullptr;

	LinphoneCore *core = nullptr;
	Conference *conference = nullptr;

	CallCallbackObj nextVideoFrameDecoded;

	L_DECLARE_PUBLIC(Call);
};

LINPHONE_END_NAMESPACE

#endif // ifndef _CALL_P_H_
