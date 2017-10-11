/*
 * client-group-chat-room.cpp
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

#include "linphone/utils/utils.h"

#include "address/address-p.h"
#include "client-group-chat-room-p.h"
#include "c-wrapper/c-wrapper.h"
#include "conference/params/call-session-params-p.h"
#include "conference/session/call-session-p.h"
#include "conference/participant-p.h"
#include "content/content.h"
#include "hacks/hacks.h"
#include "logger/logger.h"
#include "sal/refer-op.h"

// =============================================================================

using namespace std;

LINPHONE_BEGIN_NAMESPACE

ClientGroupChatRoomPrivate::ClientGroupChatRoomPrivate (LinphoneCore *core)
	: ChatRoomPrivate(core) {}

// -----------------------------------------------------------------------------

shared_ptr<CallSession> ClientGroupChatRoomPrivate::createSession () {
	L_Q();
	CallSessionParams csp;
	csp.addCustomHeader("Require", "recipient-list-invite");
	shared_ptr<CallSession> session = q->focus->getPrivate()->createSession(*q, &csp, false, q);
	session->configure(LinphoneCallOutgoing, nullptr, nullptr, q->me->getAddress(), q->focus->getAddress());
	session->initiateOutgoing();
	Address addr = q->me->getAddress();
	addr.setParam("text");
	session->getPrivate()->getOp()->set_contact_address(addr.getPrivate()->getInternalAddress());
	return session;
}

void ClientGroupChatRoomPrivate::notifyReceived (string body) {
	L_Q();
	q->eventHandler->notifyReceived(body);
}

// =============================================================================

ClientGroupChatRoom::ClientGroupChatRoom (LinphoneCore *core, const Address &me, const string &uri, const string &subject)
	: ChatRoom(*new ClientGroupChatRoomPrivate(core)), RemoteConference(core, me, nullptr) {
	focus = ObjectFactory::create<Participant>(Address(uri));
	this->subject = subject;
}

// -----------------------------------------------------------------------------

void ClientGroupChatRoom::addParticipant (const Address &addr, const CallSessionParams *params, bool hasMedia) {
	list<Address> addresses;
	addresses.push_back(addr);
	addParticipants(addresses, params, hasMedia);
}

void ClientGroupChatRoom::addParticipants (const list<Address> &addresses, const CallSessionParams *params, bool hasMedia) {
	L_D();
	if (addresses.empty())
		return;
	if ((d->state != ChatRoom::State::Instantiated) && (d->state != ChatRoom::State::Created)) {
		lError() << "Cannot add participants to the ClientGroupChatRoom in a state other than Instantiated or Created";
		return;
	}
	list<Address> sortedAddresses(addresses);
	sortedAddresses.sort();
	sortedAddresses.unique();
	Content content;
	content.setBody(getResourceLists(sortedAddresses));
	content.setContentType("application/resource-lists+xml");
	content.setContentDisposition("recipient-list");
	shared_ptr<CallSession> session = focus->getPrivate()->getSession();
	if (session)
		session->update(nullptr, subject, &content);
	else {
		session = d->createSession();
		session->startInvite(nullptr, subject, &content);
		if (d->state == ChatRoom::State::Instantiated) {
			d->setState(ChatRoom::State::CreationPending);
		}
	}
}

bool ClientGroupChatRoom::canHandleParticipants () const {
	return RemoteConference::canHandleParticipants();
}

shared_ptr<Participant> ClientGroupChatRoom::findParticipant (const Address &addr) const {
	return RemoteConference::findParticipant(addr);
}

const Address *ClientGroupChatRoom::getConferenceAddress () const {
	return RemoteConference::getConferenceAddress();
}

int ClientGroupChatRoom::getNbParticipants () const {
	return RemoteConference::getNbParticipants();
}

list<shared_ptr<Participant>> ClientGroupChatRoom::getParticipants () const {
	return RemoteConference::getParticipants();
}

const string &ClientGroupChatRoom::getSubject () const {
	return RemoteConference::getSubject();
}

void ClientGroupChatRoom::join () {
	L_D();
	shared_ptr<CallSession> session = focus->getPrivate()->getSession();
	if (!session && (d->state == ChatRoom::State::Instantiated)) {
		session = d->createSession();
		session->startInvite(nullptr, "", nullptr);
		d->setState(ChatRoom::State::CreationPending);
	}
}

void ClientGroupChatRoom::leave () {
	L_D();
	eventHandler->unsubscribe();
	shared_ptr<CallSession> session = focus->getPrivate()->getSession();
	if (session)
		session->terminate();
	else {
		session = d->createSession();
		session->startInvite(nullptr, "", nullptr);
	}
	d->setState(ChatRoom::State::TerminationPending);
}

void ClientGroupChatRoom::removeParticipant (const shared_ptr<const Participant> &participant) {
	// TODO
}

void ClientGroupChatRoom::removeParticipants (const list<shared_ptr<Participant>> &participants) {
	// TODO
}

void ClientGroupChatRoom::setParticipantAdminStatus (shared_ptr<Participant> &participant, bool isAdmin) {
	L_D();
	if (isAdmin == participant->isAdmin())
		return;
	if (!me->isAdmin()) {
		lError() << "Cannot change the participant admin status because I am not admin";
		return;
	}
	SalReferOp *referOp = new SalReferOp(d->core->sal);
	LinphoneAddress *lAddr = linphone_address_new(conferenceAddress.asString().c_str());
	linphone_configure_op(d->core, referOp, lAddr, nullptr, false);
	linphone_address_unref(lAddr);
	Address referToAddr = participant->getAddress();
	referToAddr.setParam("text");
	referToAddr.setParam("admin", Utils::toString(isAdmin));
	referToAddr.setDomain("");
	referToAddr.setPort(-1);
	referOp->send_refer(referToAddr.getPrivate()->getInternalAddress());
	referOp->unref();
}

void ClientGroupChatRoom::setSubject (const string &subject) {
	L_D();
	if (d->state != ChatRoom::State::Created) {
		lError() << "Cannot change the ClientGroupChatRoom subject in a state other than Created";
		return;
	}
	if (!me->isAdmin()) {
		lError() << "Cannot change the ClientGroupChatRoom subject because I am not admin";
		return;
	}
	shared_ptr<CallSession> session = focus->getPrivate()->getSession();
	if (session)
		session->update(nullptr, subject);
	else {
		session = d->createSession();
		session->startInvite(nullptr, subject, nullptr);
	}
}

// -----------------------------------------------------------------------------

void ClientGroupChatRoom::onConferenceCreated (const Address &addr) {
	L_D();
	conferenceAddress = addr;
	d->setState(ChatRoom::State::Created);
	_linphone_core_add_group_chat_room(d->core, addr, L_GET_C_BACK_PTR(this));
}

void ClientGroupChatRoom::onConferenceTerminated (const Address &addr) {
	L_D();
	d->setState(ChatRoom::State::Terminated);
}

void ClientGroupChatRoom::onParticipantAdded (const Address &addr) {
	if (isMe(addr))
		return;
	shared_ptr<Participant> participant = findParticipant(addr);
	if (participant) {
		lWarning() << "Participant " << participant << " added but already in the list of participants!";
		return;
	}
	participant = ObjectFactory::create<Participant>(addr);
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
		lWarning() << "Participant " << participant << " removed but is not in the list of participants!";
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
	shared_ptr<Participant> participant = nullptr;
	if (isMe(addr))
		participant = me;
	else
		participant = findParticipant(addr);
	if (!participant) {
		lWarning() << "Participant " << participant << " admin status has been changed but is not in the list of participants!";
		return;
	}
	participant->getPrivate()->setAdmin(isAdmin);
	LinphoneChatRoom *cr = L_GET_C_BACK_PTR(this);
	LinphoneChatRoomCbs *cbs = linphone_chat_room_get_callbacks(cr);
	LinphoneChatRoomCbsParticipantAdminStatusChangedCb cb = linphone_chat_room_cbs_get_participant_admin_status_changed(cbs);
	if (cb)
		cb(cr, L_GET_C_BACK_PTR(participant), isAdmin);
}

void ClientGroupChatRoom::onSubjectChanged (const std::string &subject) {
	this->subject = subject;
	LinphoneChatRoom *cr = L_GET_C_BACK_PTR(this);
	LinphoneChatRoomCbs *cbs = linphone_chat_room_get_callbacks(cr);
	LinphoneChatRoomCbsSubjectChangedCb cb = linphone_chat_room_cbs_get_subject_changed(cbs);
	if (cb)
		cb(cr, subject.c_str());
}

void ClientGroupChatRoom::onParticipantDeviceAdded (const Address &addr, const Address &gruu) {
	shared_ptr<Participant> participant = nullptr;
	if (isMe(addr))
		participant = me;
	else
		participant = findParticipant(addr);
	if (!participant) {
		lWarning() << "Participant " << participant << " added a device but is not in the list of participants!";
		return;
	}
	participant->getPrivate()->addDevice(gruu);
}

void ClientGroupChatRoom::onParticipantDeviceRemoved (const Address &addr, const Address &gruu) {
	shared_ptr<Participant> participant = nullptr;
	if (isMe(addr))
		participant = me;
	else
		participant = findParticipant(addr);
	if (!participant) {
		lWarning() << "Participant " << participant << " removed a device but is not in the list of participants!";
		return;
	}
	participant->getPrivate()->removeDevice(gruu);
}

// -----------------------------------------------------------------------------

void ClientGroupChatRoom::onCallSessionSetReleased (const std::shared_ptr<const CallSession> &session) {
	if (session == focus->getPrivate()->getSession())
		focus->getPrivate()->removeSession();
}

void ClientGroupChatRoom::onCallSessionStateChanged (const std::shared_ptr<const CallSession> &session, LinphoneCallState state, const string &message) {
	L_D();
	if (state == LinphoneCallConnected) {
		if (d->state == ChatRoom::State::CreationPending) {
			Address addr(session->getRemoteContact());
			addr.clean();
			onConferenceCreated(addr);
			if (session->getRemoteContactAddress()->hasParam("isfocus"))
				eventHandler->subscribe(conferenceAddress);
		} else if (d->state == ChatRoom::State::TerminationPending) {
			focus->getPrivate()->getSession()->terminate();
		}
	} else {
		if ((state == LinphoneCallReleased) && (d->state == ChatRoom::State::TerminationPending)) {
			onConferenceTerminated(conferenceAddress);
		}
	}
}

LINPHONE_END_NAMESPACE
