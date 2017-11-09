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
#include "c-wrapper/c-wrapper.h"
#include "client-group-chat-room-p.h"
#include "conference/participant-p.h"
#include "conference/remote-conference-event-handler.h"
#include "conference/remote-conference-p.h"
#include "conference/session/call-session-p.h"
#include "core/core-p.h"
#include "event-log/events.h"
#include "logger/logger.h"
#include "sal/refer-op.h"

// =============================================================================

using namespace std;

LINPHONE_BEGIN_NAMESPACE

// -----------------------------------------------------------------------------

list<Address> ClientGroupChatRoomPrivate::cleanAddressesList (const list<Address> &addresses) const {
	L_Q();
	list<Address> cleanedList(addresses);
	cleanedList.sort();
	cleanedList.unique();
	for (auto it = cleanedList.begin(); it != cleanedList.end();) {
		if (q->findParticipant(*it) || (q->getMe()->getAddress() == SimpleAddress(*it)))
			it = cleanedList.erase(it);
		else
			it++;
	}
	return cleanedList;
}

shared_ptr<CallSession> ClientGroupChatRoomPrivate::createSession () {
	L_Q();
	L_Q_T(RemoteConference, qConference);

	CallSessionParams csp;
	csp.addCustomHeader("Require", "recipient-list-invite");
	csp.addCustomContactParameter("text");

	shared_ptr<Participant> focus = qConference->getPrivate()->focus;
	shared_ptr<CallSession> session = focus->getPrivate()->createSession(*q, &csp, false, q);
	const Address &myAddress = q->getMe()->getAddress();
	session->configure(LinphoneCallOutgoing, nullptr, nullptr, myAddress, focus->getContactAddress());
	session->initiateOutgoing();
	return session;
}

void ClientGroupChatRoomPrivate::notifyReceived (string body) {
	L_Q_T(RemoteConference, qConference);
	qConference->getPrivate()->eventHandler->notifyReceived(body);
}

// =============================================================================

ClientGroupChatRoom::ClientGroupChatRoom (
	const std::shared_ptr<Core> &core,
	const Address &me,
	const std::string &uri,
	const std::string &subject
) : ChatRoom(*new ClientGroupChatRoomPrivate, core, me), RemoteConference(core->getCCore(), me, nullptr) {
	L_D_T(RemoteConference, dConference);
	dConference->focus = make_shared<Participant>(Address(uri));
	RemoteConference::setSubject(subject);
}

int ClientGroupChatRoom::getCapabilities () const {
	return static_cast<int>(Capabilities::Conference);
}

void ClientGroupChatRoom::addParticipant (const Address &addr, const CallSessionParams *params, bool hasMedia) {
	list<Address> addresses;
	addresses.push_back(addr);
	addParticipants(addresses, params, hasMedia);
}

void ClientGroupChatRoom::addParticipants (
	const list<Address> &addresses,
	const CallSessionParams *params,
	bool hasMedia
) {
	L_D();
	L_D_T(RemoteConference, dConference);

	list<Address> addressesList = d->cleanAddressesList(addresses);
	if (addressesList.empty())
		return;

	if ((d->state != ChatRoom::State::Instantiated) && (d->state != ChatRoom::State::Created)) {
		lError() << "Cannot add participants to the ClientGroupChatRoom in a state other than Instantiated or Created";
		return;
	}

	Content content;
	content.setBody(getResourceLists(addressesList));
	content.setContentType("application/resource-lists+xml");
	content.setContentDisposition("recipient-list");

	shared_ptr<CallSession> session = dConference->focus->getPrivate()->getSession();
	if (session)
		session->update(nullptr, getSubject(), &content);
	else {
		session = d->createSession();
		session->startInvite(nullptr, getSubject(), &content);
		if (d->state == ChatRoom::State::Instantiated)
			d->setState(ChatRoom::State::CreationPending);
	}
}

bool ClientGroupChatRoom::canHandleParticipants () const {
	return RemoteConference::canHandleParticipants();
}

shared_ptr<Participant> ClientGroupChatRoom::findParticipant (const Address &addr) const {
	return RemoteConference::findParticipant(addr);
}

const Address &ClientGroupChatRoom::getConferenceAddress () const {
	return RemoteConference::getConferenceAddress();
}

shared_ptr<Participant> ClientGroupChatRoom::getMe () const {
	return RemoteConference::getMe();
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
	L_D_T(RemoteConference, dConference);

	shared_ptr<CallSession> session = dConference->focus->getPrivate()->getSession();
	if (!session && d->state == ChatRoom::State::Instantiated) {
		session = d->createSession();
		session->startInvite(nullptr, "", nullptr);
		d->setState(ChatRoom::State::CreationPending);
	}
}

void ClientGroupChatRoom::leave () {
	L_D();
	L_D_T(RemoteConference, dConference);

	dConference->eventHandler->unsubscribe();

	shared_ptr<CallSession> session = dConference->focus->getPrivate()->getSession();
	if (session)
		session->terminate();
	else {
		session = d->createSession();
		session->startInvite(nullptr, "", nullptr);
	}

	d->setState(ChatRoom::State::TerminationPending);
}

void ClientGroupChatRoom::removeParticipant (const shared_ptr<const Participant> &participant) {
	LinphoneCore *cCore = CoreAccessor::getCore()->getCCore();

	SalReferOp *referOp = new SalReferOp(cCore->sal);
	LinphoneAddress *lAddr = linphone_address_new(getConferenceAddress().asString().c_str());
	linphone_configure_op(cCore, referOp, lAddr, nullptr, false);
	linphone_address_unref(lAddr);
	Address referToAddr = participant->getAddress();
	referToAddr.setParam("text");
	referToAddr.setUriParam("method", "BYE");
	referOp->send_refer(referToAddr.getPrivate()->getInternalAddress());
	referOp->unref();
}

void ClientGroupChatRoom::removeParticipants (const list<shared_ptr<Participant>> &participants) {
	RemoteConference::removeParticipants(participants);
}

void ClientGroupChatRoom::setParticipantAdminStatus (shared_ptr<Participant> &participant, bool isAdmin) {
	if (isAdmin == participant->isAdmin())
		return;

	if (!getMe()->isAdmin()) {
		lError() << "Cannot change the participant admin status because I am not admin";
		return;
	}

	LinphoneCore *cCore = CoreAccessor::getCore()->getCCore();

	SalReferOp *referOp = new SalReferOp(cCore->sal);
	LinphoneAddress *lAddr = linphone_address_new(getConferenceAddress().asString().c_str());
	linphone_configure_op(cCore, referOp, lAddr, nullptr, false);
	linphone_address_unref(lAddr);
	Address referToAddr = participant->getAddress();
	referToAddr.setParam("text");
	referToAddr.setParam("admin", Utils::toString(isAdmin));
	referOp->send_refer(referToAddr.getPrivate()->getInternalAddress());
	referOp->unref();
}

void ClientGroupChatRoom::setSubject (const string &subject) {
	L_D();
	L_D_T(RemoteConference, dConference);

	if (d->state != ChatRoom::State::Created) {
		lError() << "Cannot change the ClientGroupChatRoom subject in a state other than Created";
		return;
	}

	if (!getMe()->isAdmin()) {
		lError() << "Cannot change the ClientGroupChatRoom subject because I am not admin";
		return;
	}

	shared_ptr<CallSession> session = dConference->focus->getPrivate()->getSession();
	if (session)
		session->update(nullptr, subject);
	else {
		session = d->createSession();
		session->startInvite(nullptr, subject, nullptr);
	}
}

// -----------------------------------------------------------------------------

void ClientGroupChatRoom::onChatMessageReceived (const shared_ptr<ChatMessage> &msg) {

}

void ClientGroupChatRoom::onConferenceCreated (const Address &addr) {
	L_D();
	L_D_T(RemoteConference, dConference);
	dConference->conferenceAddress = addr;
	d->peerAddress = addr;
	CoreAccessor::getCore()->getPrivate()->insertChatRoom(getSharedFromThis());
}

void ClientGroupChatRoom::onConferenceTerminated (const Address &addr) {
	L_D();
	d->setState(ChatRoom::State::Terminated);
}

void ClientGroupChatRoom::onFirstNotifyReceived (const Address &addr) {
	L_D();
	d->setState(ChatRoom::State::Created);
	CoreAccessor::getCore()->getPrivate()->insertChatRoomWithDb(getSharedFromThis());
}

void ClientGroupChatRoom::onParticipantAdded (const shared_ptr<ConferenceParticipantEvent> &event, bool isFullState) {
	L_D_T(RemoteConference, dConference);

	const Address &addr = event->getParticipantAddress();
	if (isMe(addr))
		return;

	shared_ptr<Participant> participant = findParticipant(addr);
	if (participant) {
		lWarning() << "Participant " << participant << " added but already in the list of participants!";
		return;
	}

	participant = make_shared<Participant>(addr);
	dConference->participants.push_back(participant);

	if (isFullState)
		return;

	LinphoneChatRoom *cr = L_GET_C_BACK_PTR(this);
	LinphoneChatRoomCbs *cbs = linphone_chat_room_get_callbacks(cr);
	LinphoneChatRoomCbsParticipantAddedCb cb = linphone_chat_room_cbs_get_participant_added(cbs);
	Conference::getCore()->cppCore->getPrivate()->mainDb->addEvent(event);

	if (cb)
		cb(cr, L_GET_C_BACK_PTR(event));
}

void ClientGroupChatRoom::onParticipantRemoved (const shared_ptr<ConferenceParticipantEvent> &event, bool isFullState) {
	(void)isFullState;

	L_D_T(RemoteConference, dConference);

	const Address &addr = event->getParticipantAddress();
	shared_ptr<Participant> participant = findParticipant(addr);
	if (!participant) {
		lWarning() << "Participant " << addr.asString() << " removed but not in the list of participants!";
		return;
	}

	LinphoneChatRoom *cr = L_GET_C_BACK_PTR(this);
	LinphoneChatRoomCbs *cbs = linphone_chat_room_get_callbacks(cr);
	LinphoneChatRoomCbsParticipantRemovedCb cb = linphone_chat_room_cbs_get_participant_removed(cbs);
	Conference::getCore()->cppCore->getPrivate()->mainDb->addEvent(event);

	if (cb)
		cb(cr, L_GET_C_BACK_PTR(event));

	dConference->participants.remove(participant);
}

void ClientGroupChatRoom::onParticipantSetAdmin (const shared_ptr<ConferenceParticipantEvent> &event, bool isFullState) {
	const Address &addr = event->getParticipantAddress();
	shared_ptr<Participant> participant;
	if (isMe(addr))
		participant = getMe();
	else
		participant = findParticipant(addr);
	if (!participant) {
		lWarning() << "Participant " << participant << " admin status has been changed but is not in the list of participants!";
		return;
	}

	bool isAdmin = event->getType() == EventLog::Type::ConferenceParticipantSetAdmin;
	if (participant->isAdmin() == isAdmin)
		return; // No change in the local admin status, do not notify
	participant->getPrivate()->setAdmin(isAdmin);

	if (isFullState)
		return;

	LinphoneChatRoom *cr = L_GET_C_BACK_PTR(this);
	LinphoneChatRoomCbs *cbs = linphone_chat_room_get_callbacks(cr);
	LinphoneChatRoomCbsParticipantAdminStatusChangedCb cb = linphone_chat_room_cbs_get_participant_admin_status_changed(cbs);
	Conference::getCore()->cppCore->getPrivate()->mainDb->addEvent(event);

	if (cb)
		cb(cr, L_GET_C_BACK_PTR(event));
}

void ClientGroupChatRoom::onSubjectChanged (const shared_ptr<ConferenceSubjectEvent> &event, bool isFullState) {
	if (getSubject() == event->getSubject())
		return; // No change in the local subject, do not notify
	RemoteConference::setSubject(event->getSubject());

	if (isFullState)
		return;

	LinphoneChatRoom *cr = L_GET_C_BACK_PTR(this);
	LinphoneChatRoomCbs *cbs = linphone_chat_room_get_callbacks(cr);
	LinphoneChatRoomCbsSubjectChangedCb cb = linphone_chat_room_cbs_get_subject_changed(cbs);
	Conference::getCore()->cppCore->getPrivate()->mainDb->addEvent(event);

	if (cb)
		cb(cr, L_GET_C_BACK_PTR(event));
}

void ClientGroupChatRoom::onParticipantDeviceAdded (const shared_ptr<ConferenceParticipantDeviceEvent> &event, bool isFullState) {
	const Address &addr = event->getParticipantAddress();
	shared_ptr<Participant> participant;
	if (isMe(addr))
		participant = getMe();
	else
		participant = findParticipant(addr);
	if (!participant) {
		lWarning() << "Participant " << participant << " added a device but is not in the list of participants!";
		return;
	}
	participant->getPrivate()->addDevice(event->getGruuAddress());

	if (isFullState)
		return;

	LinphoneChatRoom *cr = L_GET_C_BACK_PTR(this);
	LinphoneChatRoomCbs *cbs = linphone_chat_room_get_callbacks(cr);
	LinphoneChatRoomCbsParticipantDeviceAddedCb cb = linphone_chat_room_cbs_get_participant_device_added(cbs);
	Conference::getCore()->cppCore->getPrivate()->mainDb->addEvent(event);

	if (cb)
		cb(cr, L_GET_C_BACK_PTR(event));
}

void ClientGroupChatRoom::onParticipantDeviceRemoved (const shared_ptr<ConferenceParticipantDeviceEvent> &event, bool isFullState) {
	(void)isFullState;

	const Address &addr = event->getParticipantAddress();
	shared_ptr<Participant> participant;
	if (isMe(addr))
		participant = getMe();
	else
		participant = findParticipant(addr);
	if (!participant) {
		lWarning() << "Participant " << participant << " removed a device but is not in the list of participants!";
		return;
	}
	participant->getPrivate()->removeDevice(event->getGruuAddress());
	LinphoneChatRoom *cr = L_GET_C_BACK_PTR(this);
	LinphoneChatRoomCbs *cbs = linphone_chat_room_get_callbacks(cr);
	LinphoneChatRoomCbsParticipantDeviceRemovedCb cb = linphone_chat_room_cbs_get_participant_device_removed(cbs);
	Conference::getCore()->cppCore->getPrivate()->mainDb->addEvent(event);

	if (cb)
		cb(cr, L_GET_C_BACK_PTR(event));
}

// -----------------------------------------------------------------------------

void ClientGroupChatRoom::onCallSessionSetReleased (const std::shared_ptr<const CallSession> &session) {
	L_D_T(RemoteConference, dConference);

	ParticipantPrivate *participantPrivate = dConference->focus->getPrivate();
	if (session == participantPrivate->getSession())
		participantPrivate->removeSession();
}

void ClientGroupChatRoom::onCallSessionStateChanged (
	const std::shared_ptr<const CallSession> &session,
	LinphoneCallState state,
	const string &message
) {
	L_D();
	L_D_T(RemoteConference, dConference);

	if (state == LinphoneCallConnected) {
		if (d->state == ChatRoom::State::CreationPending) {
			Address addr(session->getRemoteContactAddress()->asStringUriOnly());
			onConferenceCreated(addr);
			if (session->getRemoteContactAddress()->hasParam("isfocus"))
				dConference->eventHandler->subscribe(getConferenceAddress());
		} else if (d->state == ChatRoom::State::TerminationPending)
			dConference->focus->getPrivate()->getSession()->terminate();
	} else if (state == LinphoneCallReleased && d->state == ChatRoom::State::TerminationPending) {
		onConferenceTerminated(getConferenceAddress());
	} else if (state == LinphoneCallError && d->state == ChatRoom::State::CreationPending) {
		d->setState(ChatRoom::State::CreationFailed);
	}
}

LINPHONE_END_NAMESPACE
