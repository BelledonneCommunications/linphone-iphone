/*
 * proxy-chat-room.cpp
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

#include "basic-to-client-group-chat-room.h"
#include "chat-room-p.h"
#include "proxy-chat-room-p.h"
#include "c-wrapper/c-wrapper.h"

// =============================================================================

using namespace std;

LINPHONE_BEGIN_NAMESPACE

void ProxyChatRoomPrivate::setupProxy () {
	L_Q();
	static_pointer_cast<ChatRoom>(chatRoom)->getPrivate()->setProxyChatRoom(q);
}

void ProxyChatRoomPrivate::teardownProxy () {
	static_pointer_cast<ChatRoom>(chatRoom)->getPrivate()->setProxyChatRoom(nullptr);
}

// -----------------------------------------------------------------------------

 ProxyChatRoom::ProxyChatRoom (ProxyChatRoomPrivate &p, const shared_ptr<ChatRoom> &chatRoom) :
	AbstractChatRoom(p, chatRoom->getCore()) {
	L_D();
	d->chatRoom = chatRoom;
	d->setupProxy();
}

// -----------------------------------------------------------------------------

const ChatRoomId &ProxyChatRoom::getChatRoomId () const {
	L_D();
	return d->chatRoom->getChatRoomId();
}

const IdentityAddress &ProxyChatRoom::getPeerAddress () const {
	L_D();
	return d->chatRoom->getPeerAddress();
}

const IdentityAddress &ProxyChatRoom::getLocalAddress () const {
	L_D();
	return d->chatRoom->getLocalAddress();
}

// -----------------------------------------------------------------------------

time_t ProxyChatRoom::getCreationTime () const {
	L_D();
	return d->chatRoom->getCreationTime();
}

time_t ProxyChatRoom::getLastUpdateTime () const {
	L_D();
	return d->chatRoom->getLastUpdateTime();
}

// -----------------------------------------------------------------------------

ProxyChatRoom::CapabilitiesMask ProxyChatRoom::getCapabilities () const {
	L_D();
	return d->chatRoom->getCapabilities() | ProxyChatRoom::Capabilities::Proxy;
}

ProxyChatRoom::State ProxyChatRoom::getState () const {
	L_D();
	return d->chatRoom->getState();
}

bool ProxyChatRoom::hasBeenLeft () const {
	L_D();
	return d->chatRoom->hasBeenLeft();
}

// -----------------------------------------------------------------------------

list<shared_ptr<EventLog>> ProxyChatRoom::getMessageHistory (int nLast) const {
	L_D();
	return d->chatRoom->getMessageHistory(nLast);
}

list<shared_ptr<EventLog>> ProxyChatRoom::getMessageHistoryRange (int begin, int end) const {
	L_D();
	return d->chatRoom->getMessageHistoryRange(begin, end);
}

list<shared_ptr<EventLog>> ProxyChatRoom::getHistory (int nLast) const {
	L_D();
	return d->chatRoom->getHistory(nLast);
}

list<shared_ptr<EventLog>> ProxyChatRoom::getHistoryRange (int begin, int end) const {
	L_D();
	return d->chatRoom->getHistoryRange(begin, end);
}

int ProxyChatRoom::getHistorySize () const {
	L_D();
	return d->chatRoom->getHistorySize();
}

void ProxyChatRoom::deleteFromDb () {
	L_D();
	d->chatRoom->deleteFromDb();
}

void ProxyChatRoom::deleteHistory () {
	L_D();
	d->chatRoom->deleteHistory();
}

shared_ptr<ChatMessage> ProxyChatRoom::getLastChatMessageInHistory () const {
	L_D();
	return d->chatRoom->getLastChatMessageInHistory();
}

int ProxyChatRoom::getChatMessageCount () const {
	L_D();
	return d->chatRoom->getChatMessageCount();
}

int ProxyChatRoom::getUnreadChatMessageCount () const {
	L_D();
	return d->chatRoom->getUnreadChatMessageCount();
}

// -----------------------------------------------------------------------------

void ProxyChatRoom::compose () {
	L_D();
	return d->chatRoom->compose();
}

bool ProxyChatRoom::isRemoteComposing () const {
	L_D();
	return d->chatRoom->isRemoteComposing();
}

list<IdentityAddress> ProxyChatRoom::getComposingAddresses () const {
	L_D();
	return d->chatRoom->getComposingAddresses();
}

// -----------------------------------------------------------------------------

shared_ptr<ChatMessage> ProxyChatRoom::createChatMessage () {
	L_D();
	return d->chatRoom->createChatMessage();
}

shared_ptr<ChatMessage> ProxyChatRoom::createChatMessage (const string &text) {
	L_D();
	return d->chatRoom->createChatMessage(text);
}

shared_ptr<ChatMessage> ProxyChatRoom::createFileTransferMessage (const LinphoneContent *initialContent) {
	L_D();
	return d->chatRoom->createFileTransferMessage(initialContent);
}

// -----------------------------------------------------------------------------

shared_ptr<ChatMessage> ProxyChatRoom::findChatMessage (const string &messageId) const {
	L_D();
	return d->chatRoom->findChatMessage(messageId);
}

shared_ptr<ChatMessage> ProxyChatRoom::findChatMessage (
	const string &messageId,
	ChatMessage::Direction direction
) const {
	L_D();
	return d->chatRoom->findChatMessage(messageId, direction);
}

void ProxyChatRoom::markAsRead () {
	L_D();
	d->chatRoom->markAsRead();
}

// -----------------------------------------------------------------------------

const IdentityAddress &ProxyChatRoom::getConferenceAddress () const {
	L_D();
	return d->chatRoom->getConferenceAddress();
}

// -----------------------------------------------------------------------------

void ProxyChatRoom::allowCpim (bool value) {
	
}

void ProxyChatRoom::allowMultipart (bool value) {
	
}

bool ProxyChatRoom::canHandleCpim () const {
	L_D();
	return d->chatRoom->canHandleCpim();
}

bool ProxyChatRoom::canHandleMultipart () const {
	L_D();
	return d->chatRoom->canHandleMultipart();
}

bool ProxyChatRoom::canHandleParticipants () const {
	L_D();
	return d->chatRoom->canHandleParticipants();
}

void ProxyChatRoom::addParticipant (
	const IdentityAddress &participantAddress,
	const CallSessionParams *params,
	bool hasMedia
) {
	L_D();
	return d->chatRoom->addParticipant(participantAddress, params, hasMedia);
}

void ProxyChatRoom::addParticipants (
	const list<IdentityAddress> &addresses,
	const CallSessionParams *params,
	bool hasMedia
) {
	L_D();
	return d->chatRoom->addParticipants(addresses, params, hasMedia);
}

void ProxyChatRoom::removeParticipant (const shared_ptr<const Participant> &participant) {
	L_D();
	d->chatRoom->removeParticipant(participant);
}

void ProxyChatRoom::removeParticipants (const list<shared_ptr<Participant>> &participants) {
	L_D();
	d->chatRoom->removeParticipants(participants);
}

shared_ptr<Participant> ProxyChatRoom::findParticipant (const IdentityAddress &participantAddress) const {
	L_D();
	return d->chatRoom->findParticipant(participantAddress);
}

shared_ptr<Participant> ProxyChatRoom::getMe () const {
	L_D();
	return d->chatRoom->getMe();
}

int ProxyChatRoom::getParticipantCount () const {
	L_D();
	return d->chatRoom->getParticipantCount();
}

const list<shared_ptr<Participant>> &ProxyChatRoom::getParticipants () const {
	L_D();
	return d->chatRoom->getParticipants();
}

void ProxyChatRoom::setParticipantAdminStatus (const shared_ptr<Participant> &participant, bool isAdmin) {
	L_D();
	d->chatRoom->setParticipantAdminStatus(participant, isAdmin);
}

// -----------------------------------------------------------------------------

const string &ProxyChatRoom::getSubject () const {
	L_D();
	return d->chatRoom->getSubject();
}

void ProxyChatRoom::setSubject (const string &subject) {
	L_D();
	d->chatRoom->setSubject(subject);
}

// -----------------------------------------------------------------------------

void ProxyChatRoom::join () {
	L_D();
	d->chatRoom->join();
}

void ProxyChatRoom::leave () {
	L_D();
	d->chatRoom->leave();
}

// -----------------------------------------------------------------------------

const shared_ptr<AbstractChatRoom> &ProxyChatRoom::getProxiedChatRoom () const {
	L_D();
	return d->chatRoom;
}

LINPHONE_END_NAMESPACE
