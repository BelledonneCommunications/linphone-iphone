/*
 * conference-p.h
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

#ifndef _CONFERENCE_P_H_
#define _CONFERENCE_P_H_

#include <memory>

#include "object/object-p.h"

#include "conference/conference.h"
#include "conference/participant.h"
#include "conference/session/call-session-listener.h"

#include <string>

// =============================================================================

LINPHONE_BEGIN_NAMESPACE

class ConferencePrivate : public ObjectPrivate, CallSessionListener {
public:
	virtual ~ConferencePrivate () = default;

	LinphoneCore * getCore () const { return core; }

	virtual void ackBeingSent (const CallSession &session, LinphoneHeaders *headers);
	virtual void ackReceived (const CallSession &session, LinphoneHeaders *headers);
	virtual void callSessionAccepted (const CallSession &session);
	virtual void callSessionSetReleased (const CallSession &session);
	virtual void callSessionSetTerminated (const CallSession &session);
	virtual void callSessionStateChanged (const CallSession &session, LinphoneCallState state, const std::string &message);
	virtual void incomingCallSessionStarted (const CallSession &session);

	virtual void encryptionChanged (const CallSession &session, bool activated, const std::string &authToken);

	virtual void statsUpdated (const LinphoneCallStats *stats);

	virtual void resetCurrentSession (const CallSession &session);
	virtual void setCurrentSession (const CallSession &session);

	virtual void firstVideoFrameDecoded (const CallSession &session);
	virtual void resetFirstVideoFrameDecoded (const CallSession &session);

private:
	LinphoneCore *core = nullptr;
	CallListener *callListener = nullptr;

	std::shared_ptr<Participant> activeParticipant = nullptr;
	std::string id;
	std::shared_ptr<Participant> me = nullptr;
	std::list<std::shared_ptr<Participant>> participants;

	L_DECLARE_PUBLIC(Conference);
};

LINPHONE_END_NAMESPACE

#endif // ifndef _CONFERENCE_P_H_
