/*
 * remote-conference-event-handler.h
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
		RemoteConferenceEventHandler (LinphoneCore *core, ConferenceListener *listener);
		~RemoteConferenceEventHandler ();

		void subscribe (const Address &confAddress);
		void notifyReceived (std::string xmlBody);
		void unsubscribe ();

		const Address &getConfAddress ();

	private:
		L_DECLARE_PRIVATE(RemoteConferenceEventHandler);
		L_DISABLE_COPY(RemoteConferenceEventHandler);
};

LINPHONE_END_NAMESPACE

#endif // ifndef _REMOTE_CONFERENCE_EVENT_HANDLER_H_
