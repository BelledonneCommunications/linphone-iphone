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
	void onParticipantAdded (time_t tm, bool isFullState, const Address &addr) override;
	void onParticipantRemoved (time_t tm, bool isFullState, const Address &addr) override;
	void onParticipantSetAdmin (time_t tm, bool isFullState, const Address &addr, bool isAdmin) override;
	void onSubjectChanged (time_t tm, bool isFullState, const std::string &subject) override;
	void onParticipantDeviceAdded (time_t tm, bool isFullState, const Address &addr, const Address &gruu) override;
	void onParticipantDeviceRemoved (time_t tm, bool isFullState, const Address &addr, const Address &gruu) override;

private:
	L_DECLARE_PRIVATE(RemoteConference);
	L_DISABLE_COPY(RemoteConference);
};

LINPHONE_END_NAMESPACE

#endif // ifndef _REMOTE_CONFERENCE_H_
