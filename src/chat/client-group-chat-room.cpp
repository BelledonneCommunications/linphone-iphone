/*
 * client-group-chat-room.cpp
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

#include "client-group-chat-room-p.h"
#include "c-wrapper/c-wrapper.h"
#include "conference/participant-p.h"
#include "logger/logger.h"

// =============================================================================

using namespace std;

LINPHONE_BEGIN_NAMESPACE

ClientGroupChatRoomPrivate::ClientGroupChatRoomPrivate (LinphoneCore *core) : ChatRoomPrivate(core) {}

// =============================================================================

ClientGroupChatRoom::ClientGroupChatRoom (LinphoneCore *core, const Address &me)
	: ChatRoom(*new ChatRoomPrivate(core)), RemoteConference(core, me, nullptr) {
	string factoryUri = linphone_core_get_conference_factory_uri(core);
	focus = make_shared<Participant>(factoryUri);
	CallSessionParams csp;
	shared_ptr<CallSession> session = focus->getPrivate()->createSession(*this, &csp, false, this);
	session->configure(LinphoneCallOutgoing, nullptr, nullptr, me, focus->getAddress());
	// TODO
}

// -----------------------------------------------------------------------------

shared_ptr<Participant> ClientGroupChatRoom::addParticipant (const Address &addr, const CallSessionParams *params, bool hasMedia) {
	activeParticipant = make_shared<Participant>(addr);
	activeParticipant->getPrivate()->createSession(*this, params, hasMedia, this);
	return activeParticipant;
}

void ClientGroupChatRoom::addParticipants (const list<Address> &addresses, const CallSessionParams *params, bool hasMedia) {
	L_D(ClientGroupChatRoom);
	if (d->state == ChatRoom::State::Instantiated) {
		shared_ptr<CallSession> session = focus->getPrivate()->getSession();
		session->initiateOutgoing();
		session->startInvite(nullptr);
		d->setState(ChatRoom::State::CreationPending);
	}
	// TODO
}

bool ClientGroupChatRoom::canHandleParticipants () const {
	return RemoteConference::canHandleParticipants();
}

const string& ClientGroupChatRoom::getId () const {
	return RemoteConference::getId();
}

int ClientGroupChatRoom::getNbParticipants () const {
	return RemoteConference::getNbParticipants();
}

list<shared_ptr<Participant>> ClientGroupChatRoom::getParticipants () const {
	return RemoteConference::getParticipants();
}

void ClientGroupChatRoom::removeParticipant (const shared_ptr<const Participant> &participant) {
	// TODO
}

void ClientGroupChatRoom::removeParticipants (const list<shared_ptr<Participant>> &participants) {
	// TODO
}

// -----------------------------------------------------------------------------

void ClientGroupChatRoom::onConferenceCreated (const Address &addr) {
	// TODO
}

void ClientGroupChatRoom::onConferenceTerminated (const Address &addr) {
	// TODO
}

void ClientGroupChatRoom::onParticipantAdded (const Address &addr) {
	shared_ptr<Participant> participant = findParticipant(addr);
	if (participant) {
		lWarning() << "Participant " << participant << " added but already in the list of participants!";
		return;
	}
	participant = make_shared<Participant>(addr);
	participants.push_back(participant);
	LinphoneChatRoom *cr = L_GET_C_BACK_PTR(this);
	LinphoneChatRoomCbs *cbs = linphone_chat_room_get_callbacks(cr);
	LinphoneChatRoomCbsParticipantAddedCb cb = linphone_chat_room_cbs_get_participant_added(cbs);
	if (cb)
		cb(cr, L_GET_C_BACK_PTR(participant));
}

void ClientGroupChatRoom::onParticipantRemoved (const Address &addr) {
	shared_ptr<Participant> participant = findParticipant(addr);
	if (!participant) {
		lWarning() << "Participant " << participant << " removed but not in the list of participants!";
		return;
	}
	LinphoneChatRoom *cr = L_GET_C_BACK_PTR(this);
	LinphoneChatRoomCbs *cbs = linphone_chat_room_get_callbacks(cr);
	LinphoneChatRoomCbsParticipantRemovedCb cb = linphone_chat_room_cbs_get_participant_removed(cbs);
	if (cb)
		cb(cr, L_GET_C_BACK_PTR(participant));
	participants.remove(participant);
}

void ClientGroupChatRoom::onParticipantSetAdmin (const Address &addr, bool isAdmin) {
	shared_ptr<Participant> participant = findParticipant(addr);
	if (!participant) {
		lWarning() << "Participant " << participant << " admin status has been changed but is not in the list of participants!";
		return;
	}
	participant->setAdmin(isAdmin);
	LinphoneChatRoom *cr = L_GET_C_BACK_PTR(this);
	LinphoneChatRoomCbs *cbs = linphone_chat_room_get_callbacks(cr);
	LinphoneChatRoomCbsParticipantAdminStatusChangedCb cb = linphone_chat_room_cbs_get_participant_admin_status_changed(cbs);
	if (cb)
		cb(cr, L_GET_C_BACK_PTR(participant), isAdmin);
}

LINPHONE_END_NAMESPACE
