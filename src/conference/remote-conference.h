/*
 * remote-conference.h
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

#ifndef _REMOTE_CONFERENCE_H_
#define _REMOTE_CONFERENCE_H_

#include "conference.h"
#include "remote-conference-event-handler.h"

// =============================================================================

LINPHONE_BEGIN_NAMESPACE

class RemoteConference : public Conference, public ConferenceListener {
public:
	RemoteConference (LinphoneCore *core, const Address &myAddress, CallListener *listener = nullptr);
	virtual ~RemoteConference();

protected:
	std::shared_ptr<Participant> focus;

public:
	/* ConferenceInterface */
	std::shared_ptr<Participant> addParticipant (const Address &addr, const CallSessionParams *params, bool hasMedia) override;
	void removeParticipant (const std::shared_ptr<const Participant> &participant) override;

	std::string getResourceLists (const std::list<Address> &addresses);

protected:
	/* ConferenceListener */
	void onConferenceCreated (const Address &addr) override;
	void onConferenceTerminated (const Address &addr) override;
	void onParticipantAdded (const Address &addr) override;
	void onParticipantRemoved (const Address &addr) override;
	void onParticipantSetAdmin (const Address &addr, bool isAdmin) override;

private:
	L_DISABLE_COPY(RemoteConference);

	RemoteConferenceEventHandler *eventHandler = nullptr;
};

LINPHONE_END_NAMESPACE

#endif // ifndef _REMOTE_CONFERENCE_H_
