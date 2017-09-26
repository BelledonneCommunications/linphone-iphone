/*
 * participant-p.h
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

#ifndef _PARTICIPANT_P_H_
#define _PARTICIPANT_P_H_

#include <memory>

#include "object/object-p.h"

#include "conference/participant.h"
#include "conference/session/call-session.h"
#include "conference/session/call-session-listener.h"
#include "conference/params/call-session-params.h"

#include "linphone/types.h"

// =============================================================================

LINPHONE_BEGIN_NAMESPACE

class ParticipantPrivate : public ObjectPrivate {
public:
	virtual ~ParticipantPrivate () = default;

	std::shared_ptr<CallSession> createSession (const Conference &conference, const CallSessionParams *params, bool hasMedia, CallSessionListener *listener);
	std::shared_ptr<CallSession> getSession () const;

private:
	Address addr;
	bool isAdmin = false;
	std::shared_ptr<CallSession> session;

	L_DECLARE_PUBLIC(Participant);
};

LINPHONE_END_NAMESPACE

#endif // ifndef _PARTICIPANT_P_H_
