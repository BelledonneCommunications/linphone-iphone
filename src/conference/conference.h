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
#include "conference/params/call-session-params.h"
#include "conference/participant.h"

#include "linphone/types.h"

// =============================================================================

LINPHONE_BEGIN_NAMESPACE

class ConferencePrivate;
class CallSessionPrivate;

class Conference : public Object {
	friend class CallSessionPrivate;

public:
	//Conference (CallListener *listener = nullptr);

	std::shared_ptr<Participant> addParticipant (const Address &addr, const std::shared_ptr<CallSessionParams> params, bool hasMedia);
	std::shared_ptr<Participant> getActiveParticipant () const;
	std::shared_ptr<Participant> getMe () const;

protected:
	explicit Conference (ConferencePrivate &p, LinphoneCore *core, const Address &myAddress, CallListener *listener = nullptr);

private:
	L_DECLARE_PRIVATE(Conference);
	L_DISABLE_COPY(Conference);
};

LINPHONE_END_NAMESPACE

#endif // ifndef _CONFERENCE_H_
