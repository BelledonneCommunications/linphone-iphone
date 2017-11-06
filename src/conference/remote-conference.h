/*
 * remote-conference.h
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

#ifndef _REMOTE_CONFERENCE_H_
#define _REMOTE_CONFERENCE_H_

#include "conference-listener.h"
#include "conference.h"

// =============================================================================

LINPHONE_BEGIN_NAMESPACE

class RemoteConferencePrivate;

class LINPHONE_PUBLIC RemoteConference : public Conference, public ConferenceListener {
	friend class ClientGroupChatRoomPrivate;

public:
	RemoteConference (LinphoneCore *core, const Address &myAddress, CallListener *listener = nullptr);
	virtual ~RemoteConference();

	/* ConferenceInterface */
	void addParticipant (const Address &addr, const CallSessionParams *params, bool hasMedia) override;
	void removeParticipant (const std::shared_ptr<const Participant> &participant) override;

	std::string getResourceLists (const std::list<Address> &addresses) const;

protected:
	/* ConferenceListener */
	void onConferenceCreated (const Address &addr) override;
	void onConferenceTerminated (const Address &addr) override;
	void onParticipantAdded (std::shared_ptr<ConferenceParticipantEvent> event) override;
	void onParticipantRemoved (std::shared_ptr<ConferenceParticipantEvent> event) override;
	void onParticipantSetAdmin (std::shared_ptr<ConferenceParticipantEvent> event) override;
	void onSubjectChanged (std::shared_ptr<ConferenceSubjectEvent> event) override;
	void onParticipantDeviceAdded (std::shared_ptr<ConferenceParticipantDeviceEvent> event) override;
	void onParticipantDeviceRemoved (std::shared_ptr<ConferenceParticipantDeviceEvent> event) override;

private:
	L_DECLARE_PRIVATE(RemoteConference);
	L_DISABLE_COPY(RemoteConference);
};

LINPHONE_END_NAMESPACE

#endif // ifndef _REMOTE_CONFERENCE_H_
