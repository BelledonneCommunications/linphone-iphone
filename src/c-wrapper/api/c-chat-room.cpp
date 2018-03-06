/*
 * c-chat-room.cpp
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

#include <algorithm>

// TODO: Remove me later.
#include "linphone/chat.h"

#include "linphone/api/c-chat-room.h"
#include "linphone/wrapper_utils.h"

#include "address/identity-address.h"
#include "c-wrapper/c-wrapper.h"
#include "call/call.h"
#include "chat/chat-message/chat-message-p.h"
#include "chat/chat-room/real-time-text-chat-room-p.h"
#include "chat/chat-room/server-group-chat-room-p.h"
#include "conference/participant.h"
#include "core/core-p.h"
#include "event-log/event-log.h"

// =============================================================================

using namespace std;

static void _linphone_chat_room_constructor (LinphoneChatRoom *cr);
static void _linphone_chat_room_destructor (LinphoneChatRoom *cr);

L_DECLARE_C_OBJECT_IMPL_WITH_XTORS(
	ChatRoom,
	_linphone_chat_room_constructor, _linphone_chat_room_destructor,
	bctbx_list_t *callbacks; /* A list of LinphoneCallCbs object */
	LinphoneChatRoomCbs *currentCbs; /* The current LinphoneCallCbs object used to call a callback */
	mutable LinphoneAddress *conferenceAddressCache;
	mutable LinphoneAddress *peerAddressCache;
	mutable LinphoneAddress *localAddressCache;
	mutable bctbx_list_t *composingAddresses;
)

static void _linphone_chat_room_constructor (LinphoneChatRoom *cr) {}

static void _linphone_chat_room_destructor (LinphoneChatRoom *cr) {
	if (cr->conferenceAddressCache)
		linphone_address_unref(cr->conferenceAddressCache);
	if (cr->peerAddressCache)
		linphone_address_unref(cr->peerAddressCache);
	if (cr->localAddressCache)
		linphone_address_unref(cr->localAddressCache);
	if (cr->composingAddresses)
		bctbx_list_free_with_data(cr->composingAddresses, (bctbx_list_free_func)linphone_address_unref);
	_linphone_chat_room_clear_callbacks(cr);
}

static list<LinphonePrivate::IdentityAddress> _get_identity_address_list_from_address_list(list<LinphonePrivate::Address> addressList) {
	list<LinphonePrivate::IdentityAddress> lIdent;
	for (const auto &addr : addressList)
		lIdent.push_back(LinphonePrivate::IdentityAddress(addr));
	return lIdent;
}

// =============================================================================
// Public functions.
// =============================================================================

void linphone_chat_room_send_message (LinphoneChatRoom *cr, const char *msg) {
	L_GET_CPP_PTR_FROM_C_OBJECT(cr)->createChatMessage(msg)->send();
}

bool_t linphone_chat_room_is_remote_composing (const LinphoneChatRoom *cr) {
	return L_GET_CPP_PTR_FROM_C_OBJECT(cr)->isRemoteComposing();
}

LinphoneCore *linphone_chat_room_get_lc (const LinphoneChatRoom *cr) {
	return linphone_chat_room_get_core(cr);
}

LinphoneCore *linphone_chat_room_get_core (const LinphoneChatRoom *cr) {
	return L_GET_CPP_PTR_FROM_C_OBJECT(cr)->getCore()->getCCore();
}

const LinphoneAddress *linphone_chat_room_get_peer_address (LinphoneChatRoom *cr) {
	if (cr->peerAddressCache) {
		linphone_address_unref(cr->peerAddressCache);
	}

	cr->peerAddressCache = linphone_address_new(L_GET_CPP_PTR_FROM_C_OBJECT(cr)->getPeerAddress().asString().c_str());
	return cr->peerAddressCache;
}

const LinphoneAddress *linphone_chat_room_get_local_address (LinphoneChatRoom *cr) {
	if (cr->localAddressCache) {
		linphone_address_unref(cr->localAddressCache);
	}

	cr->localAddressCache = linphone_address_new(L_GET_CPP_PTR_FROM_C_OBJECT(cr)->getLocalAddress().asString().c_str());
	return cr->localAddressCache;
}

LinphoneChatMessage *linphone_chat_room_create_message (LinphoneChatRoom *cr, const char *message) {
	shared_ptr<LinphonePrivate::ChatMessage> cppPtr = L_GET_CPP_PTR_FROM_C_OBJECT(cr)->createChatMessage(L_C_TO_STRING(message));
	LinphoneChatMessage *object = L_INIT(ChatMessage);
	L_SET_CPP_PTR_FROM_C_OBJECT(object, cppPtr);
	return object;
}

LinphoneChatMessage *linphone_chat_room_create_message_2 (
	LinphoneChatRoom *cr,
	const char *message,
	const char *external_body_url,
	LinphoneChatMessageState state,
	time_t time,
	bool_t is_read,
	bool_t is_incoming
) {
	LinphoneChatMessage *msg = linphone_chat_room_create_message(cr, message);

	linphone_chat_message_set_external_body_url(msg, external_body_url ? ms_strdup(external_body_url) : NULL);

	LinphonePrivate::ChatMessagePrivate *dMsg = L_GET_PRIVATE_FROM_C_OBJECT(msg);
	dMsg->setTime(time);
	dMsg->setState(static_cast<LinphonePrivate::ChatMessage::State>(state));

	return msg;
}

void linphone_chat_room_send_chat_message_2 (LinphoneChatRoom *cr, LinphoneChatMessage *msg) {
	linphone_chat_message_ref(msg);
	L_GET_CPP_PTR_FROM_C_OBJECT(msg)->send();
}

void linphone_chat_room_send_chat_message (LinphoneChatRoom *cr, LinphoneChatMessage *msg) {
	L_GET_CPP_PTR_FROM_C_OBJECT(msg)->send();
}

void linphone_chat_room_receive_chat_message (LinphoneChatRoom *cr, LinphoneChatMessage *msg) {
	L_GET_PRIVATE_FROM_C_OBJECT(msg)->receive();
}

uint32_t linphone_chat_room_get_char (const LinphoneChatRoom *cr) {
	if (!(L_GET_CPP_PTR_FROM_C_OBJECT(cr)->getCapabilities() & LinphonePrivate::ChatRoom::Capabilities::RealTimeText))
		return 0;
	return L_GET_CPP_PTR_FROM_C_OBJECT(cr, RealTimeTextChatRoom)->getChar();
}

void linphone_chat_room_compose (LinphoneChatRoom *cr) {
	L_GET_CPP_PTR_FROM_C_OBJECT(cr)->compose();
}

LinphoneCall *linphone_chat_room_get_call (const LinphoneChatRoom *cr) {
	if (!(L_GET_CPP_PTR_FROM_C_OBJECT(cr)->getCapabilities() & LinphonePrivate::ChatRoom::Capabilities::RealTimeText))
		return nullptr;
	return L_GET_C_BACK_PTR(L_GET_CPP_PTR_FROM_C_OBJECT(cr, RealTimeTextChatRoom)->getCall());
}

void linphone_chat_room_set_call (LinphoneChatRoom *cr, LinphoneCall *call) {
	if (!(L_GET_CPP_PTR_FROM_C_OBJECT(cr)->getCapabilities() & LinphonePrivate::ChatRoom::Capabilities::RealTimeText))
		return;
	L_GET_PRIVATE_FROM_C_OBJECT(cr, RealTimeTextChatRoom)->call = L_GET_CPP_PTR_FROM_C_OBJECT(call);
}

void linphone_chat_room_mark_as_read (LinphoneChatRoom *cr) {
	L_GET_CPP_PTR_FROM_C_OBJECT(cr)->markAsRead();
}

int linphone_chat_room_get_unread_messages_count (LinphoneChatRoom *cr) {
	return L_GET_CPP_PTR_FROM_C_OBJECT(cr)->getUnreadChatMessageCount();
}

int linphone_chat_room_get_history_size (LinphoneChatRoom *cr) {
	return L_GET_CPP_PTR_FROM_C_OBJECT(cr)->getChatMessageCount();
}

void linphone_chat_room_delete_message (LinphoneChatRoom *cr, LinphoneChatMessage *msg) {
	shared_ptr<LinphonePrivate::EventLog> event = LinphonePrivate::MainDb::getEventFromKey(
		L_GET_PRIVATE_FROM_C_OBJECT(msg)->dbKey
	);
	if (event)
		LinphonePrivate::EventLog::deleteFromDatabase(event);
}

void linphone_chat_room_delete_history (LinphoneChatRoom *cr) {
	L_GET_CPP_PTR_FROM_C_OBJECT(cr)->deleteHistory();
}

bctbx_list_t *linphone_chat_room_get_history_range (LinphoneChatRoom *cr, int startm, int endm) {
	list<shared_ptr<LinphonePrivate::ChatMessage>> chatMessages;
	for (auto &event : L_GET_CPP_PTR_FROM_C_OBJECT(cr)->getMessageHistoryRange(startm, endm))
		chatMessages.push_back(static_pointer_cast<LinphonePrivate::ConferenceChatMessageEvent>(event)->getChatMessage());

	return L_GET_RESOLVED_C_LIST_FROM_CPP_LIST(chatMessages);
}

bctbx_list_t *linphone_chat_room_get_history (LinphoneChatRoom *cr, int nb_message) {
	return linphone_chat_room_get_history_range(cr, 0, nb_message);
}

bctbx_list_t *linphone_chat_room_get_history_range_message_events (LinphoneChatRoom *cr, int startm, int endm) {
	return L_GET_RESOLVED_C_LIST_FROM_CPP_LIST(L_GET_CPP_PTR_FROM_C_OBJECT(cr)->getMessageHistoryRange(startm, endm));
}

bctbx_list_t *linphone_chat_room_get_history_message_events (LinphoneChatRoom *cr, int nb_events) {
	return L_GET_RESOLVED_C_LIST_FROM_CPP_LIST(L_GET_CPP_PTR_FROM_C_OBJECT(cr)->getMessageHistory(nb_events));
}

bctbx_list_t *linphone_chat_room_get_history_events (LinphoneChatRoom *cr, int nb_events) {
	return L_GET_RESOLVED_C_LIST_FROM_CPP_LIST(L_GET_CPP_PTR_FROM_C_OBJECT(cr)->getHistory(nb_events));
}

bctbx_list_t *linphone_chat_room_get_history_range_events (LinphoneChatRoom *cr, int begin, int end) {
	return L_GET_RESOLVED_C_LIST_FROM_CPP_LIST(L_GET_CPP_PTR_FROM_C_OBJECT(cr)->getHistoryRange(begin, end));
}

int linphone_chat_room_get_history_events_size(LinphoneChatRoom *cr) {
	return L_GET_CPP_PTR_FROM_C_OBJECT(cr)->getHistorySize();
}

LinphoneChatMessage *linphone_chat_room_get_last_message_in_history(LinphoneChatRoom *cr) {
	shared_ptr<LinphonePrivate::ChatMessage> cppPtr = L_GET_CPP_PTR_FROM_C_OBJECT(cr)->getLastChatMessageInHistory();
	if (!cppPtr)
		return nullptr;

	LinphoneChatMessage *object = L_INIT(ChatMessage);
	L_SET_CPP_PTR_FROM_C_OBJECT(object, cppPtr);
	return object;
}

LinphoneChatMessage *linphone_chat_room_find_message (LinphoneChatRoom *cr, const char *message_id) {
	return L_GET_C_BACK_PTR(L_GET_CPP_PTR_FROM_C_OBJECT(cr)->findChatMessage(message_id));
}

LinphoneChatRoomState linphone_chat_room_get_state (const LinphoneChatRoom *cr) {
	return (LinphoneChatRoomState)L_GET_CPP_PTR_FROM_C_OBJECT(cr)->getState();
}

bool_t linphone_chat_room_has_been_left (const LinphoneChatRoom *cr) {
	return (bool_t)L_GET_CPP_PTR_FROM_C_OBJECT(cr)->hasBeenLeft();
}

time_t linphone_chat_room_get_last_update_time(const LinphoneChatRoom *cr) {
	return L_GET_CPP_PTR_FROM_C_OBJECT(cr)->getLastUpdateTime();
}

void linphone_chat_room_add_participant (LinphoneChatRoom *cr, const LinphoneAddress *addr) {
	L_GET_CPP_PTR_FROM_C_OBJECT(cr)->addParticipant(
		LinphonePrivate::IdentityAddress(*L_GET_CPP_PTR_FROM_C_OBJECT(addr)), nullptr, false
	);
}

void linphone_chat_room_add_participants (LinphoneChatRoom *cr, const bctbx_list_t *addresses) {
	list<LinphonePrivate::Address> lAddr = L_GET_RESOLVED_CPP_LIST_FROM_C_LIST(addresses, Address);
	list<LinphonePrivate::IdentityAddress> lIdentAddr;
	for (const auto &addr : lAddr)
		lIdentAddr.push_back(LinphonePrivate::IdentityAddress(addr));
	L_GET_CPP_PTR_FROM_C_OBJECT(cr)->addParticipants(lIdentAddr, nullptr, false);
}

bool_t linphone_chat_room_can_handle_participants (const LinphoneChatRoom *cr) {
	return L_GET_CPP_PTR_FROM_C_OBJECT(cr)->canHandleParticipants();
}

LinphoneParticipant *linphone_chat_room_find_participant (const LinphoneChatRoom *cr, const LinphoneAddress *addr) {
	return L_GET_C_BACK_PTR(L_GET_CPP_PTR_FROM_C_OBJECT(cr)->findParticipant(
		LinphonePrivate::IdentityAddress(*L_GET_CPP_PTR_FROM_C_OBJECT(addr))
	));
}

LinphoneChatRoomCapabilitiesMask linphone_chat_room_get_capabilities (const LinphoneChatRoom *cr) {
	return L_GET_CPP_PTR_FROM_C_OBJECT(cr)->getCapabilities();
}

bool_t linphone_chat_room_has_capability(const LinphoneChatRoom *cr, int mask) {
	if (L_GET_CPP_PTR_FROM_C_OBJECT(cr)->getCapabilities() & mask) return true;
	return false;
}

const LinphoneAddress *linphone_chat_room_get_conference_address (const LinphoneChatRoom *cr) {
	if (cr->conferenceAddressCache)
		linphone_address_unref(cr->conferenceAddressCache);

	const LinphonePrivate::IdentityAddress &address = L_GET_CPP_PTR_FROM_C_OBJECT(cr)->getConferenceAddress();
	if (address.isValid())
		cr->conferenceAddressCache = linphone_address_new(address.asString().c_str());
	else
		cr->conferenceAddressCache = nullptr;
	return cr->conferenceAddressCache;
}

LinphoneParticipant *linphone_chat_room_get_me (const LinphoneChatRoom *cr) {
	return L_GET_C_BACK_PTR(L_GET_CPP_PTR_FROM_C_OBJECT(cr)->getMe());
}

int linphone_chat_room_get_nb_participants (const LinphoneChatRoom *cr) {
	return L_GET_CPP_PTR_FROM_C_OBJECT(cr)->getParticipantCount();
}

bctbx_list_t *linphone_chat_room_get_participants (const LinphoneChatRoom *cr) {
	return L_GET_RESOLVED_C_LIST_FROM_CPP_LIST(L_GET_CPP_PTR_FROM_C_OBJECT(cr)->getParticipants());
}

const char * linphone_chat_room_get_subject (const LinphoneChatRoom *cr) {
	return L_STRING_TO_C(L_GET_CPP_PTR_FROM_C_OBJECT(cr)->getSubject());
}

void linphone_chat_room_leave (LinphoneChatRoom *cr) {
	L_GET_CPP_PTR_FROM_C_OBJECT(cr)->leave();
}

void linphone_chat_room_remove_participant (LinphoneChatRoom *cr, LinphoneParticipant *participant) {
	L_GET_CPP_PTR_FROM_C_OBJECT(cr)->removeParticipant(L_GET_CPP_PTR_FROM_C_OBJECT(participant));
}

void linphone_chat_room_remove_participants (LinphoneChatRoom *cr, const bctbx_list_t *participants) {
	L_GET_CPP_PTR_FROM_C_OBJECT(cr)->removeParticipants(L_GET_RESOLVED_CPP_LIST_FROM_C_LIST(participants, Participant));
}

void linphone_chat_room_set_participant_admin_status (LinphoneChatRoom *cr, LinphoneParticipant *participant, bool_t isAdmin) {
	shared_ptr<LinphonePrivate::Participant> p = L_GET_CPP_PTR_FROM_C_OBJECT(participant);
	L_GET_CPP_PTR_FROM_C_OBJECT(cr)->setParticipantAdminStatus(p, !!isAdmin);
}

void linphone_chat_room_set_subject (LinphoneChatRoom *cr, const char *subject) {
	L_GET_CPP_PTR_FROM_C_OBJECT(cr)->setSubject(L_C_TO_STRING(subject));
}

const bctbx_list_t *linphone_chat_room_get_composing_addresses (LinphoneChatRoom *cr) {
	bctbx_list_free_with_data(cr->composingAddresses, (bctbx_list_free_func)linphone_address_unref);
	list<LinphonePrivate::Address> composingAddresses;
	// TODO: Improve perf or algorithm?
	{
		list<LinphonePrivate::IdentityAddress> addresses = L_GET_CPP_PTR_FROM_C_OBJECT(cr)->getComposingAddresses();
		transform(
			addresses.cbegin(), addresses.cend(),
			back_inserter(composingAddresses), [](const LinphonePrivate::Address &address) {
				return LinphonePrivate::IdentityAddress(address);
			}
		);
	}
	cr->composingAddresses = L_GET_RESOLVED_C_LIST_FROM_CPP_LIST(composingAddresses);
	return cr->composingAddresses;
}

LinphoneChatMessage *linphone_chat_room_create_file_transfer_message(LinphoneChatRoom *cr, const LinphoneContent *initial_content) {
	shared_ptr<LinphonePrivate::ChatMessage> cppPtr = L_GET_CPP_PTR_FROM_C_OBJECT(cr)->createFileTransferMessage(initial_content);
	LinphoneChatMessage *object = L_INIT(ChatMessage);
	L_SET_CPP_PTR_FROM_C_OBJECT(object, cppPtr);
	return object;
}

void linphone_chat_room_set_conference_address (LinphoneChatRoom *cr, const LinphoneAddress *confAddr) {
	char *addrStr = linphone_address_as_string(confAddr);
	LinphonePrivate::ServerGroupChatRoomPrivate *sgcr = dynamic_cast<LinphonePrivate::ServerGroupChatRoomPrivate *>(L_GET_PRIVATE_FROM_C_OBJECT(cr));
	if (sgcr)
		sgcr->setConferenceAddress(LinphonePrivate::IdentityAddress(addrStr));
	bctbx_free(addrStr);
}

void linphone_chat_room_set_participant_devices (LinphoneChatRoom *cr, const LinphoneAddress *partAddr, const bctbx_list_t *partDevices) {
	char *addrStr = linphone_address_as_string(partAddr);
	list<LinphonePrivate::Address> lDevices = L_GET_RESOLVED_CPP_LIST_FROM_C_LIST(partDevices, Address);
	list<LinphonePrivate::IdentityAddress> lIdentAddr;
	for (const auto &addr : lDevices)
		lIdentAddr.push_back(LinphonePrivate::IdentityAddress(addr));
	LinphonePrivate::ServerGroupChatRoomPrivate *sgcr = dynamic_cast<LinphonePrivate::ServerGroupChatRoomPrivate *>(L_GET_PRIVATE_FROM_C_OBJECT(cr));
	if (sgcr)
		sgcr->setParticipantDevices(LinphonePrivate::IdentityAddress(addrStr), lIdentAddr);
	bctbx_free(addrStr);
}

void linphone_chat_room_add_compatible_participants (LinphoneChatRoom *cr, const LinphoneAddress *deviceAddr, const bctbx_list_t *participantsCompatible) {
	list<LinphonePrivate::Address> lPartsComp = L_GET_RESOLVED_CPP_LIST_FROM_C_LIST(participantsCompatible, Address);
	LinphonePrivate::ServerGroupChatRoomPrivate *sgcr = dynamic_cast<LinphonePrivate::ServerGroupChatRoomPrivate *>(L_GET_PRIVATE_FROM_C_OBJECT(cr));
	if (sgcr) {
		char *deviceAddrStr = linphone_address_as_string_uri_only(deviceAddr);
		sgcr->addCompatibleParticipants(LinphonePrivate::IdentityAddress(deviceAddrStr),
										_get_identity_address_list_from_address_list(lPartsComp)
		);
		bctbx_free(deviceAddrStr);
	}
}

// =============================================================================
// Callbacks
// =============================================================================

void _linphone_chat_room_clear_callbacks (LinphoneChatRoom *cr) {
	bctbx_list_free_with_data(cr->callbacks, (bctbx_list_free_func)linphone_chat_room_cbs_unref);
	cr->callbacks = nullptr;
}

void linphone_chat_room_add_callbacks (LinphoneChatRoom *cr, LinphoneChatRoomCbs *cbs) {
	cr->callbacks = bctbx_list_append(cr->callbacks, linphone_chat_room_cbs_ref(cbs));
}

void linphone_chat_room_remove_callbacks (LinphoneChatRoom *cr, LinphoneChatRoomCbs *cbs) {
	cr->callbacks = bctbx_list_remove(cr->callbacks, cbs);
	linphone_chat_room_cbs_unref(cbs);
}

LinphoneChatRoomCbs *linphone_chat_room_get_current_callbacks (const LinphoneChatRoom *cr) {
	return cr->currentCbs;
}

void linphone_chat_room_set_current_callbacks(LinphoneChatRoom *cr, LinphoneChatRoomCbs *cbs) {
	cr->currentCbs = cbs;
}

const bctbx_list_t *linphone_chat_room_get_callbacks_list(const LinphoneChatRoom *cr) {
	return cr->callbacks;
}

#define NOTIFY_IF_EXIST(cbName, functionName, ...) \
	bctbx_list_t *callbacksCopy = bctbx_list_copy(linphone_chat_room_get_callbacks_list(cr)); \
	for (bctbx_list_t *it = callbacksCopy; it; it = bctbx_list_next(it)) { \
		linphone_chat_room_set_current_callbacks(cr, reinterpret_cast<LinphoneChatRoomCbs *>(bctbx_list_get_data(it))); \
		LinphoneChatRoomCbs ## cbName ## Cb cb = linphone_chat_room_cbs_get_ ## functionName (linphone_chat_room_get_current_callbacks(cr)); \
		if (cb) \
			cb(__VA_ARGS__); \
	} \
	bctbx_list_free(callbacksCopy);

void _linphone_chat_room_notify_is_composing_received(LinphoneChatRoom *cr, const LinphoneAddress *remoteAddr, bool_t isComposing) {
	NOTIFY_IF_EXIST(IsComposingReceived, is_composing_received, cr, remoteAddr, isComposing)
}

void _linphone_chat_room_notify_message_received(LinphoneChatRoom *cr, LinphoneChatMessage *msg) {
	NOTIFY_IF_EXIST(MessageReceived, message_received, cr, msg)
}

void _linphone_chat_room_notify_participant_added(LinphoneChatRoom *cr, const LinphoneEventLog *event_log) {
	NOTIFY_IF_EXIST(ParticipantAdded, participant_added, cr, event_log)
}

void _linphone_chat_room_notify_participant_removed(LinphoneChatRoom *cr, const LinphoneEventLog *event_log) {
	NOTIFY_IF_EXIST(ParticipantRemoved, participant_removed, cr, event_log)
}

void _linphone_chat_room_notify_participant_device_added(LinphoneChatRoom *cr, const LinphoneEventLog *event_log) {
	NOTIFY_IF_EXIST(ParticipantDeviceAdded, participant_device_added, cr, event_log)
}

void _linphone_chat_room_notify_participant_device_removed(LinphoneChatRoom *cr, const LinphoneEventLog *event_log) {
	NOTIFY_IF_EXIST(ParticipantDeviceRemoved, participant_device_removed, cr, event_log)
}

void _linphone_chat_room_notify_participant_admin_status_changed(LinphoneChatRoom *cr, const LinphoneEventLog *event_log) {
	NOTIFY_IF_EXIST(ParticipantAdminStatusChanged, participant_admin_status_changed, cr, event_log)
}

void _linphone_chat_room_notify_state_changed(LinphoneChatRoom *cr, LinphoneChatRoomState newState) {
	NOTIFY_IF_EXIST(StateChanged, state_changed, cr, newState)
}

void _linphone_chat_room_notify_subject_changed(LinphoneChatRoom *cr, const LinphoneEventLog *event_log) {
	NOTIFY_IF_EXIST(SubjectChanged, subject_changed, cr, event_log)
}

void _linphone_chat_room_notify_undecryptable_message_received(LinphoneChatRoom *cr, LinphoneChatMessage *msg) {
	NOTIFY_IF_EXIST(UndecryptableMessageReceived, undecryptable_message_received, cr, msg)
}

void _linphone_chat_room_notify_chat_message_received(LinphoneChatRoom *cr, const LinphoneEventLog *event_log) {
	NOTIFY_IF_EXIST(ChatMessageReceived, chat_message_received, cr, event_log)
}

void _linphone_chat_room_notify_chat_message_sent(LinphoneChatRoom *cr, const LinphoneEventLog *event_log) {
	NOTIFY_IF_EXIST(ChatMessageSent, chat_message_sent, cr, event_log)
}

void _linphone_chat_room_notify_conference_address_generation(LinphoneChatRoom *cr) {
	NOTIFY_IF_EXIST(ConferenceAddressGeneration, conference_address_generation, cr)
}

void _linphone_chat_room_notify_participant_device_fetched(LinphoneChatRoom *cr, const LinphoneAddress *participantAddr) {
	NOTIFY_IF_EXIST(ParticipantDeviceFetched, participant_device_fetched, cr, participantAddr)
}

void _linphone_chat_room_notify_participants_capabilities_checked(LinphoneChatRoom *cr, const LinphoneAddress *deviceAddr, const bctbx_list_t *participantsAddr) {
	NOTIFY_IF_EXIST(ParticipantsCapabilitiesChecked, participants_capabilities_checked, cr, deviceAddr, participantsAddr)
}

void _linphone_chat_room_notify_chat_message_should_be_stored(LinphoneChatRoom *cr, LinphoneChatMessage *msg) {
	NOTIFY_IF_EXIST(ShouldChatMessageBeStored, chat_message_should_be_stored, cr, msg)
}

// =============================================================================
// Reference and user data handling functions.
// =============================================================================

LinphoneChatRoom *linphone_chat_room_ref (LinphoneChatRoom *cr) {
	belle_sip_object_ref(cr);
	return cr;
}

void linphone_chat_room_unref (LinphoneChatRoom *cr) {
	belle_sip_object_unref(cr);
}

void *linphone_chat_room_get_user_data (const LinphoneChatRoom *cr) {
	return L_GET_USER_DATA_FROM_C_OBJECT(cr);
}

void linphone_chat_room_set_user_data (LinphoneChatRoom *cr, void *ud) {
	L_SET_USER_DATA_FROM_C_OBJECT(cr, ud);
}

// =============================================================================
// Constructor and destructor functions.
// =============================================================================

LinphoneChatRoom *_linphone_server_group_chat_room_new (LinphoneCore *core, LinphonePrivate::SalCallOp *op) {
	LinphoneChatRoom *cr = L_INIT(ChatRoom);
	L_SET_CPP_PTR_FROM_C_OBJECT(cr, make_shared<LinphonePrivate::ServerGroupChatRoom>(
		L_GET_CPP_PTR_FROM_C_OBJECT(core),
		op
	));
	L_GET_PRIVATE_FROM_C_OBJECT(cr)->setState(LinphonePrivate::ChatRoom::State::Instantiated);
	L_GET_PRIVATE_FROM_C_OBJECT(cr, ServerGroupChatRoom)->confirmCreation();
	return cr;
}
