/*
 * remote-conference-event-handler.h
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

#ifndef _REMOTE_CONFERENCE_EVENT_HANDLER_H_
#define _REMOTE_CONFERENCE_EVENT_HANDLER_H_

#include <string>

#include "linphone/types.h"

#include "object/object.h"
#include "conference-listener.h"

LINPHONE_BEGIN_NAMESPACE

class RemoteConferenceEventHandlerPrivate;

class RemoteConferenceEventHandler : public Object {
	public:
		RemoteConferenceEventHandler(LinphoneCore *core, ConferenceListener *listener);
		~RemoteConferenceEventHandler();

		void subscribe(std::string confId);
		void notifyReceived(std::string xmlBody);
		void unsubscribe();

		std::string getConfId();
		void setConferenceAddress (const Address &addr);

	private:
		L_DECLARE_PRIVATE(RemoteConferenceEventHandler);
		L_DISABLE_COPY(RemoteConferenceEventHandler);
};

LINPHONE_END_NAMESPACE

#endif // ifndef _REMOTE_CONFERENCE_EVENT_HANDLER_H_
