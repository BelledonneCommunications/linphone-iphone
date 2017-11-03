/*
 * cenference-listener.h
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

#ifndef _CONFERENCE_LISTENER_H_
#define _CONFERENCE_LISTENER_H_

#include <ctime>
#include <string>

#include "linphone/utils/general.h"

// =============================================================================

LINPHONE_BEGIN_NAMESPACE

class Address;

class ConferenceListener {
public:
	virtual void onConferenceCreated (const Address &addr) = 0;
	virtual void onConferenceTerminated (const Address &addr) = 0;
	virtual void onParticipantAdded (time_t tm, bool isFullState, const Address &addr) = 0;
	virtual void onParticipantRemoved (time_t tm, bool isFullState, const Address &addr) = 0;
	virtual void onParticipantSetAdmin (time_t tm, bool isFullState, const Address &addr, bool isAdmin) = 0;
	virtual void onSubjectChanged (time_t tm, bool isFullState, const std::string &subject) = 0;
	virtual void onParticipantDeviceAdded (time_t tm, bool isFullState, const Address &addr, const Address &gruu) = 0;
	virtual void onParticipantDeviceRemoved (time_t tm, bool isFullState, const Address &addr, const Address &gruu) = 0;
};

LINPHONE_END_NAMESPACE

#endif // ifndef _CONFERENCE_LISTENER_H_
