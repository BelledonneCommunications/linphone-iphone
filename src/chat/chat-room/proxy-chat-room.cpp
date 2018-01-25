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
#include "chat-room.h"
#include "proxy-chat-room-p.h"
#include "c-wrapper/c-wrapper.h"

// =============================================================================

using namespace std;

LINPHONE_BEGIN_NAMESPACE

#define PROXY_CALLBACK(callback, ...) \
	LinphoneChatRoomCbs *proxiedCbs = linphone_chat_room_get_callbacks(cr); \
	ProxyChatRoom *pcr = static_cast<ProxyChatRoom *>(linphone_chat_room_cbs_get_user_data(proxiedCbs)); \
	LinphoneChatRoom *lcr = L_GET_C_BACK_PTR(pcr->getSharedFromThis()); \
	LinphoneChatRoomCbs *cbs = linphone_chat_room_get_callbacks(lcr); \
	if (linphone_chat_room_cbs_get_ ## callback(cbs)) \
		linphone_chat_room_cbs_get_ ## callback(cbs)(lcr, ##__VA_ARGS__)

static void chatMessageReceived (LinphoneChatRoom *cr, const LinphoneEventLog *event_log) {
	PROXY_CALLBACK(chat_message_received, event_log);
}

static void chatMessageSent (LinphoneChatRoom *cr, const LinphoneEventLog *event_log) {
	PROXY_CALLBACK(chat_message_sent, event_log);
}

static void conferenceAddressGeneration (LinphoneChatRoom *cr) {
	PROXY_CALLBACK(conference_address_generation);
}

static void isComposingReceived (LinphoneChatRoom *cr, const LinphoneAddress *remoteAddr, bool_t isComposing) {
	PROXY_CALLBACK(is_composing_received, remoteAddr, isComposing);
}

static void messageReceived (LinphoneChatRoom *cr, LinphoneChatMessage *msg) {
	PROXY_CALLBACK(message_received, msg);
}

static void participantAdded (LinphoneChatRoom *cr, const LinphoneEventLog *event_log) {
	PROXY_CALLBACK(participant_added, event_log);
}

static void participantAdminStatusChanged (LinphoneChatRoom *cr, const LinphoneEventLog *event_log) {
	PROXY_CALLBACK(participant_admin_status_changed, event_log);
}

static void participantDeviceAdded (LinphoneChatRoom *cr, const LinphoneEventLog *event_log) {
	PROXY_CALLBACK(participant_device_added, event_log);
}

static void participantDeviceFetched (LinphoneChatRoom *cr, const LinphoneAddress *participantAddr) {
	PROXY_CALLBACK(participant_device_fetched, participantAddr);
}

static void participantDeviceRemoved (LinphoneChatRoom *cr, const LinphoneEventLog *event_log) {
	PROXY_CALLBACK(participant_device_removed, event_log);
}

static void participantRemoved (LinphoneChatRoom *cr, const LinphoneEventLog *event_log) {
	PROXY_CALLBACK(participant_removed, event_log);
}

static void participantsCapabilitiesChecked (LinphoneChatRoom *cr, const LinphoneAddress *deviceAddr, const bctbx_list_t *participantsAddr) {
	PROXY_CALLBACK(participants_capabilities_checked, deviceAddr, participantsAddr);
}

static void stateChanged (LinphoneChatRoom *cr, LinphoneChatRoomState newState) {
	PROXY_CALLBACK(state_changed, newState);
}

static void subjectChanged (LinphoneChatRoom *cr, const LinphoneEventLog *event_log) {
	PROXY_CALLBACK(subject_changed, event_log);
}

static void undecryptableMessageReceived (LinphoneChatRoom *cr, LinphoneChatMessage *msg) {
	PROXY_CALLBACK(undecryptable_message_received, msg);
}

void ProxyChatRoomPrivate::setupCallbacks () {
	L_Q();
	LinphoneChatRoom *lcr = L_GET_C_BACK_PTR(chatRoom);
	LinphoneChatRoomCbs *cbs = linphone_chat_room_get_callbacks(lcr);
	linphone_chat_room_cbs_set_user_data(cbs, q);
	linphone_chat_room_cbs_set_chat_message_received(cbs, chatMessageReceived);
	linphone_chat_room_cbs_set_chat_message_sent(cbs, chatMessageSent);
	linphone_chat_room_cbs_set_conference_address_generation(cbs, conferenceAddressGeneration);
	linphone_chat_room_cbs_set_is_composing_received(cbs, isComposingReceived);
	linphone_chat_room_cbs_set_message_received(cbs, messageReceived);
	linphone_chat_room_cbs_set_participant_added(cbs, participantAdded);
	linphone_chat_room_cbs_set_participant_admin_status_changed(cbs, participantAdminStatusChanged);
	linphone_chat_room_cbs_set_participant_device_added(cbs, participantDeviceAdded);
	linphone_chat_room_cbs_set_participant_device_fetched(cbs, participantDeviceFetched);
	linphone_chat_room_cbs_set_participant_device_removed(cbs, participantDeviceRemoved);
	linphone_chat_room_cbs_set_participant_removed(cbs, participantRemoved);
	linphone_chat_room_cbs_set_participants_capabilities_checked(cbs, participantsCapabilitiesChecked);
	linphone_chat_room_cbs_set_state_changed(cbs, stateChanged);
	linphone_chat_room_cbs_set_subject_changed(cbs, subjectChanged);
	linphone_chat_room_cbs_set_undecryptable_message_received(cbs, undecryptableMessageReceived);
}

// -----------------------------------------------------------------------------

 ProxyChatRoom::ProxyChatRoom (ProxyChatRoomPrivate &p, const shared_ptr<ChatRoom> &chatRoom) :
	AbstractChatRoom(p, chatRoom->getCore()) {
	L_D();
	d->chatRoom = chatRoom;
	d->setupCallbacks();
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

bool ProxyChatRoom::canHandleParticipants () const {
	L_D();
	return d->chatRoom->canHandleParticipants();
}

bool ProxyChatRoom::canHandleCpim () const {
	return true;
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
