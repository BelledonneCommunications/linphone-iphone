/*
 * local-conference-event-handler.h
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

#ifndef _LOCAL_CONFERENCE_EVENT_HANDLER_H_
#define _LOCAL_CONFERENCE_EVENT_HANDLER_H_

#include "linphone/types.h"

#include "address/address.h"
#include "core/core-accessor.h"
#include "object/object.h"

// =============================================================================

LINPHONE_BEGIN_NAMESPACE

class LocalConference;
class LocalConferenceEventHandlerPrivate;

class LocalConferenceEventHandler : public Object {
public:
	LocalConferenceEventHandler (LocalConference *localConference, unsigned int notify = 0);
	~LocalConferenceEventHandler ();

	void subscribeReceived (LinphoneEvent *lev);
	void notifyParticipantAdded (const Address &addr);
	void notifyParticipantRemoved (const Address &addr);
	void notifyParticipantSetAdmin (const Address &addr, bool isAdmin);
	void notifySubjectChanged ();
	void notifyParticipantDeviceAdded (const Address &addr, const Address &gruu);
	void notifyParticipantDeviceRemoved (const Address &addr, const Address &gruu);
	
	void setLastNotify (unsigned int lastNotify);

private:
	L_DECLARE_PRIVATE(LocalConferenceEventHandler);
	L_DISABLE_COPY(LocalConferenceEventHandler);
};

LINPHONE_END_NAMESPACE

#endif // ifndef _LOCAL_CONFERENCE_EVENT_HANDLER_H_
