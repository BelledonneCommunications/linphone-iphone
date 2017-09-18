/*
 * local-conference.h
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

#ifndef _LOCAL_CONFERENCE_H_
#define _LOCAL_CONFERENCE_H_

#include "conference.h"
#include "local-conference-event-handler.h"

// =============================================================================

LINPHONE_BEGIN_NAMESPACE

class LocalConference : public Conference {
public:
	LocalConference (LinphoneCore *core, const Address &myAddress, CallListener *listener = nullptr);
	virtual ~LocalConference();

	LocalConferenceEventHandler * getEventHandler() const { return eventHandler; }

public:
	/* ConferenceInterface */
	virtual std::shared_ptr<Participant> addParticipant (const Address &addr, const CallSessionParams *params, bool hasMedia);
	virtual void addParticipants (const std::list<Address> &addresses, const CallSessionParams *params, bool hasMedia);
	virtual bool canHandleParticipants () const;
	virtual const std::string& getId () const;
	virtual int getNbParticipants () const;
	virtual std::list<std::shared_ptr<Participant>> getParticipants () const;
	virtual void removeParticipant (const std::shared_ptr<Participant> participant);
	virtual void removeParticipants (const std::list<std::shared_ptr<Participant>> participants);

private:
	L_DISABLE_COPY(LocalConference);

	LocalConferenceEventHandler *eventHandler = nullptr;
};

LINPHONE_END_NAMESPACE

#endif // ifndef _LOCAL_CONFERENCE_H_
