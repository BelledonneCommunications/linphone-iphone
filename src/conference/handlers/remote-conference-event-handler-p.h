/*
 * remote-conference-event-handler-p.h
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

#ifndef _L_REMOTE_CONFERENCE_EVENT_HANDLER_P_H_
#define _L_REMOTE_CONFERENCE_EVENT_HANDLER_P_H_

#include "linphone/types.h"

#include "chat/chat-room/chat-room-id.h"
#include "core/core-listener.h"
#include "object/object-p.h"
#include "remote-conference-event-handler.h"

// =============================================================================

LINPHONE_BEGIN_NAMESPACE

class RemoteConferenceEventHandlerPrivate : public ObjectPrivate, public CoreListener {
	friend class ClientGroupChatRoom;
private:
	void simpleNotifyReceived (const std::string &xmlBody);
	void subscribe ();
	void unsubscribe ();

	// CoreListener
	void onNetworkReachable (bool sipNetworkReachable, bool mediaNetworkReachable) override;
	void onRegistrationStateChanged (LinphoneProxyConfig *cfg, LinphoneRegistrationState state, const std::string &message) override;
	void onEnteringBackground () override;
	void onEnteringForeground () override;

	ChatRoomId chatRoomId;

	RemoteConference *conf = nullptr;
	LinphoneEvent *lev = nullptr;

	unsigned int lastNotify = 0;
	bool subscriptionWanted = false;

	L_DECLARE_PUBLIC(RemoteConferenceEventHandler);
};

LINPHONE_END_NAMESPACE

#endif // ifndef _L_REMOTE_CONFERENCE_EVENT_HANDLER_P_H_
