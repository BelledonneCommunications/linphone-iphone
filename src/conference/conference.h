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

#include <memory>

#include "object/object.h"
#include "address/address.h"
#include "call/call-listener.h"
#include "conference/conference-interface.h"
#include "conference/params/call-session-params.h"
#include "conference/participant.h"
#include "conference/session/call-session-listener.h"

#include "linphone/types.h"

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
	virtual std::shared_ptr<Participant> addParticipant (const Address &addr, const std::shared_ptr<CallSessionParams> params, bool hasMedia);
	virtual void addParticipants (const std::list<Address> &addresses, const std::shared_ptr<CallSessionParams> params, bool hasMedia);
	virtual const std::string& getId () const;
	virtual int getNbParticipants () const;
	virtual std::list<std::shared_ptr<Participant>> getParticipants () const;
	virtual void removeParticipant (const std::shared_ptr<Participant> participant);
	virtual void removeParticipants (const std::list<std::shared_ptr<Participant>> participants);

private:
	/* CallSessionListener */
	virtual void onAckBeingSent (const CallSession &session, LinphoneHeaders *headers);
	virtual void onAckReceived (const CallSession &session, LinphoneHeaders *headers);
	virtual void onCallSessionAccepted (const CallSession &session);
	virtual void onCallSessionSetReleased (const CallSession &session);
	virtual void onCallSessionSetTerminated (const CallSession &session);
	virtual void onCallSessionStateChanged (const CallSession &session, LinphoneCallState state, const std::string &message);
	virtual void onIncomingCallSessionStarted (const CallSession &session);
	virtual void onEncryptionChanged (const CallSession &session, bool activated, const std::string &authToken);
	virtual void onStatsUpdated (const LinphoneCallStats *stats);
	virtual void onResetCurrentSession (const CallSession &session);
	virtual void onSetCurrentSession (const CallSession &session);
	virtual void onFirstVideoFrameDecoded (const CallSession &session);
	virtual void onResetFirstVideoFrameDecoded (const CallSession &session);

protected:
	explicit Conference (LinphoneCore *core, const Address &myAddress, CallListener *listener = nullptr);

protected:
	LinphoneCore *core = nullptr;
	CallListener *callListener = nullptr;

	std::shared_ptr<Participant> activeParticipant = nullptr;
	std::string id;
	std::shared_ptr<Participant> me = nullptr;
	std::list<std::shared_ptr<Participant>> participants;

private:
	L_DISABLE_COPY(Conference);
};

LINPHONE_END_NAMESPACE

#endif // ifndef _CONFERENCE_H_
