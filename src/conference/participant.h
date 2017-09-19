/*
 * participant.h
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

#ifndef _PARTICIPANT_H_
#define _PARTICIPANT_H_

#include "address/address.h"

#include "object/object.h"
#include "conference/params/call-session-params.h"

// =============================================================================

LINPHONE_BEGIN_NAMESPACE

class ClientGroupChatRoom;
class ParticipantPrivate;

class Participant : public Object {
	friend class Call;
	friend class CallPrivate;
	friend class ClientGroupChatRoom;
	friend class LocalConference;
	friend class MediaSessionPrivate;
	friend class RemoteConference;

public:
	Participant (const Address &addr);

	const Address& getAddress () const;

	bool isAdmin () const;
	void setAdmin (bool isAdmin);

private:
	L_DECLARE_PRIVATE(Participant);
	L_DISABLE_COPY(Participant);
};

std::ostream & operator<< (std::ostream &strm, const std::shared_ptr<Participant> &participant);

LINPHONE_END_NAMESPACE

#endif // ifndef _PARTICIPANT_H_
