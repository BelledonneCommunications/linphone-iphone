/*
 * client-group-to-basic-chat-room.cpp
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

#include "client-group-chat-room-p.h"
#include "client-group-to-basic-chat-room.h"
#include "proxy-chat-room-p.h"
#include "c-wrapper/c-wrapper.h"
#include "core/core-p.h"

// =============================================================================

using namespace std;

LINPHONE_BEGIN_NAMESPACE

// -----------------------------------------------------------------------------

class ClientGroupToBasicChatRoomPrivate : public ProxyChatRoomPrivate {
public:
	void onChatRoomInsertRequested (const shared_ptr<AbstractChatRoom> &chatRoom) override {
		L_Q();
		// Insert the proxy chat room instead of the real one
		q->getCore()->getPrivate()->insertChatRoom(q->getSharedFromThis());
	}

	void onChatRoomInsertInDatabaseRequested (const shared_ptr<AbstractChatRoom> &chatRoom) override {
		L_Q();
		// Insert the proxy chat room instead of the real one
		q->getCore()->getPrivate()->insertChatRoomWithDb(q->getSharedFromThis());
	}

	void onCallSessionSetReleased (const std::shared_ptr<const CallSession> &session) override {
		if (!(chatRoom->getCapabilities() & ChatRoom::Capabilities::Conference))
			return;
		static_pointer_cast<ClientGroupChatRoom>(chatRoom)->getPrivate()->onCallSessionSetReleased(session);
	}

	void onCallSessionStateChanged (
		const shared_ptr<const CallSession> &session,
		LinphoneCallState newState,
		const string &message
	) override {
		L_Q();
		shared_ptr<ClientGroupChatRoom> cgcr = dynamic_pointer_cast<ClientGroupChatRoom>(chatRoom);
		if (!cgcr)
			return;
		if ((newState == LinphoneCallError) && (cgcr->getState() == ChatRoom::State::CreationPending)
			&& (invitedAddresses.size() == 1)) {
			cgcr->getPrivate()->setCallSessionListener(cgcr->getPrivate());
			cgcr->getPrivate()->setChatRoomListener(cgcr->getPrivate());
			cgcr->getPrivate()->setState(ChatRoom::State::CreationFailed);
			Core::deleteChatRoom(q->getSharedFromThis());
			LinphoneChatRoom *lcr = L_GET_C_BACK_PTR(q);
			L_SET_CPP_PTR_FROM_C_OBJECT(lcr, cgcr->getCore()->getOrCreateBasicChatRoom(invitedAddresses.front()));
			return;
		}
		cgcr->getPrivate()->onCallSessionStateChanged(session, newState, message);
	}

private:
	list<IdentityAddress> invitedAddresses;

	L_DECLARE_PUBLIC(ClientGroupToBasicChatRoom);
};

// =============================================================================

ClientGroupToBasicChatRoom::ClientGroupToBasicChatRoom (const shared_ptr<ChatRoom> &chatRoom) :
	ProxyChatRoom(*new ClientGroupToBasicChatRoomPrivate, chatRoom) {}

void ClientGroupToBasicChatRoom::addParticipant (
	const IdentityAddress &participantAddress,
	const CallSessionParams *params,
	bool hasMedia
) {
	L_D();
	if (getState() == ChatRoom::State::Instantiated) {
		d->invitedAddresses.clear();
		d->invitedAddresses.push_back(participantAddress);
	}
	ProxyChatRoom::addParticipant(participantAddress, params, hasMedia);
}
void ClientGroupToBasicChatRoom::addParticipants (
	const std::list<IdentityAddress> &addresses,
	const CallSessionParams *params,
	bool hasMedia
) {
	L_D();
	if ((getState() == ChatRoom::State::Instantiated) && (addresses.size() == 1))
		d->invitedAddresses = addresses;
	ProxyChatRoom::addParticipants(addresses, params, hasMedia);
}

LINPHONE_END_NAMESPACE
