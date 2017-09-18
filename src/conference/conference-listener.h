/*
 * conference-listener.h
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

#ifndef _CONFERENCE_LISTENER_H_
#define _CONFERENCE_LISTENER_H_

#include "address/address.h"

// =============================================================================

LINPHONE_BEGIN_NAMESPACE

class ConferenceListener {
public:
	virtual void onConferenceCreated (const Address &addr) = 0;
	virtual void onConferenceTerminated (const Address &addr) = 0;
	virtual void onParticipantAdded (const Address &addr) = 0;
	virtual void onParticipantRemoved (const Address &addr) = 0;
	virtual void onParticipantSetAdmin (const Address &addr, bool isAdmin) = 0;
};

LINPHONE_END_NAMESPACE

#endif // ifndef _CONFERENCE_LISTENER_H_

