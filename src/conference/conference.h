/*
 * conference.h
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

#ifndef _CONFERENCE_H_
#define _CONFERENCE_H_

#include "linphone/types.h"

#include "address/address.h"
#include "call/call-listener.h"
#include "conference/conference-interface.h"
#include "conference/params/call-session-params.h"
#include "conference/participant.h"
#include "conference/session/call-session-listener.h"

// =============================================================================

LINPHONE_BEGIN_NAMESPACE

class CallSessionPrivate;

class Conference : public ConferenceInterface, public CallSessionListener {
	friend class CallSessionPrivate;

public:
	virtual ~Conference() = default;

	std::shared_ptr<Participant> getActiveParticipant () const;
	std::shared_ptr<Participant> getMe () const { return me; }

	LinphoneCore * getCore () const { return core; }

public:
	/* ConferenceInterface */
	std::shared_ptr<Participant> addParticipant (const Address &addr, const CallSessionParams *params, bool hasMedia) override;
	void addParticipants (const std::list<Address> &addresses, const CallSessionParams *params, bool hasMedia) override;
	bool canHandleParticipants () const override;
	const Address *getConferenceAddress () const override;
	int getNbParticipants () const override;
	std::list<std::shared_ptr<Participant>> getParticipants () const override;
	void removeParticipant (const std::shared_ptr<const Participant> &participant) override;
	void removeParticipants (const std::list<std::shared_ptr<Participant>> &participants) override;

private:
	/* CallSessionListener */
	void onAckBeingSent (const CallSession &session, LinphoneHeaders *headers) override;
	void onAckReceived (const CallSession &session, LinphoneHeaders *headers) override;
	void onCallSessionAccepted (const CallSession &session) override;
	void onCallSessionSetReleased (const CallSession &session) override;
	void onCallSessionSetTerminated (const CallSession &session) override;
	void onCallSessionStateChanged (const CallSession &session, LinphoneCallState state, const std::string &message) override;
	void onCheckForAcceptation (const CallSession &session) override;
	void onIncomingCallSessionStarted (const CallSession &session) override;
	void onEncryptionChanged (const CallSession &session, bool activated, const std::string &authToken) override;
	void onStatsUpdated (const LinphoneCallStats *stats) override;
	void onResetCurrentSession (const CallSession &session) override;
	void onSetCurrentSession (const CallSession &session) override;
	void onFirstVideoFrameDecoded (const CallSession &session) override;
	void onResetFirstVideoFrameDecoded (const CallSession &session) override;

protected:
	explicit Conference (LinphoneCore *core, const Address &myAddress, CallListener *listener = nullptr);

	std::shared_ptr<Participant> findParticipant (const Address &addr);

protected:
	LinphoneCore *core = nullptr;
	CallListener *callListener = nullptr;

	std::shared_ptr<Participant> activeParticipant;
	std::shared_ptr<Participant> me;
	std::list<std::shared_ptr<Participant>> participants;
	Address conferenceAddress;

private:
	L_DISABLE_COPY(Conference);
};

LINPHONE_END_NAMESPACE

#endif // ifndef _CONFERENCE_H_
