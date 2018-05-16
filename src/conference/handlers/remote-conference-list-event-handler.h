/*
 * remote-conference-list-event-handler.h
 * Copyright (C) 2010-2018 Belledonne Communications SARL
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

#ifndef _L_REMOTE_CONFERENCE_LIST_EVENT_HANDLER_H_
#define _L_REMOTE_CONFERENCE_LIST_EVENT_HANDLER_H_

#include <list>
#include <map>
#include <memory>

#include "linphone/types.h"
#include "linphone/utils/general.h"

#include "chat/chat-room/chat-room-id.h"
#include "core/core-accessor.h"
#include "core/core-listener.h"

// =============================================================================

LINPHONE_BEGIN_NAMESPACE

class Address;
class Content;
class RemoteConferenceEventHandler;

class RemoteConferenceListEventHandler : public CoreAccessor , public CoreListener {
public:
	RemoteConferenceListEventHandler (const std::shared_ptr<Core> &core);
	~RemoteConferenceListEventHandler ();

	void subscribe ();
	void unsubscribe ();
	void notifyReceived (const Content *notifyContent);
	void addHandler (RemoteConferenceEventHandler *handler);
	void removeHandler (RemoteConferenceEventHandler *handler);
	RemoteConferenceEventHandler *findHandler (const ChatRoomId &chatRoomId) const;
	const std::list<RemoteConferenceEventHandler *> &getHandlers () const;

private:
	std::list<RemoteConferenceEventHandler *> handlers;
	LinphoneEvent *lev = nullptr;

	std::map<std::string, IdentityAddress> parseRlmi (const std::string &xmlBody) const;

	// CoreListener
	void onNetworkReachable (bool sipNetworkReachable, bool mediaNetworkReachable) override;
	void onRegistrationStateChanged (LinphoneProxyConfig *cfg, LinphoneRegistrationState state, const std::string &message) override;
	void onEnteringBackground () override;
	void onEnteringForeground () override;
};

LINPHONE_END_NAMESPACE

#endif // ifndef _L_REMOTE_CONFERENCE_LIST_EVENT_HANDLER_H_
