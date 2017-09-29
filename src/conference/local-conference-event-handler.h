/*
 * local-conference-event-handler.h
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

#ifndef _LOCAL_CONFERENCE_EVENT_HANDLER_H_
#define _LOCAL_CONFERENCE_EVENT_HANDLER_H_

#include <string>

#include "linphone/types.h"

#include "address/address.h"
#include "object/object.h"

LINPHONE_BEGIN_NAMESPACE

class LocalConference;
class LocalConferenceEventHandlerPrivate;

class LocalConferenceEventHandler : public Object {
	public:
		LocalConferenceEventHandler(LinphoneCore *core, LocalConference *localConf);
		~LocalConferenceEventHandler();

		void subscribeReceived(LinphoneEvent *lev);
		void notifyParticipantAdded(const Address &addr);
		void notifyParticipantRemoved(const Address &addr);
		void notifyParticipantSetAdmin(const Address &addr, bool isAdmin);

	private:
		L_DECLARE_PRIVATE(LocalConferenceEventHandler);
		L_DISABLE_COPY(LocalConferenceEventHandler);
};

LINPHONE_END_NAMESPACE

#endif // ifndef _LOCAL_CONFERENCE_EVENT_HANDLER_H_
