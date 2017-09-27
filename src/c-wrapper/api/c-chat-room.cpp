/*
 * c-chat-room.cpp
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

// TODO: Remove me later.
#include "linphone/chat.h"

#include "linphone/wrapper_utils.h"
#include "linphone/api/c-chat-room.h"

#include "c-wrapper/c-wrapper.h"
#include "chat/basic-chat-room.h"
#include "chat/client-group-chat-room.h"
#include "chat/real-time-text-chat-room-p.h"

// =============================================================================

using namespace std;

static void _linphone_chat_room_constructor (LinphoneChatRoom *cr);
static void _linphone_chat_room_destructor (LinphoneChatRoom *cr);

L_DECLARE_C_OBJECT_IMPL_WITH_XTORS(
	ChatRoom,
	_linphone_chat_room_constructor, _linphone_chat_room_destructor,
	LinphoneChatRoomCbs *cbs;
	mutable LinphoneAddress *conferenceAddressCache;
	LinphoneAddress *peerAddressCache;
)

static void _linphone_chat_room_constructor (LinphoneChatRoom *cr) {
	cr->cbs = linphone_chat_room_cbs_new();
}

static void _linphone_chat_room_destructor (LinphoneChatRoom *cr) {
	linphone_chat_room_cbs_unref(cr->cbs);
	cr->cbs = nullptr;
	if (cr->conferenceAddressCache) {
		linphone_address_unref(cr->conferenceAddressCache);
		cr->conferenceAddressCache = nullptr;
	}
	if (cr->peerAddressCache) {
		linphone_address_unref(cr->peerAddressCache);
		cr->peerAddressCache = nullptr;
	}
}

// =============================================================================
// Public functions.
// =============================================================================

void linphone_chat_room_release (LinphoneChatRoom *cr) {
	L_GET_PRIVATE_FROM_C_OBJECT(cr)->release();
}

void linphone_chat_room_remove_transient_message (LinphoneChatRoom *cr, LinphoneChatMessage *msg) {
	L_GET_PRIVATE_FROM_C_OBJECT(cr)->removeTransientMessage(msg);
}

void linphone_chat_room_send_message (LinphoneChatRoom *cr, const char *msg) {
	L_GET_CPP_PTR_FROM_C_OBJECT(cr)->sendMessage(L_GET_CPP_PTR_FROM_C_OBJECT(cr)->createMessage(msg));
}

bool_t linphone_chat_room_is_remote_composing (const LinphoneChatRoom *cr) {
	return L_GET_CPP_PTR_FROM_C_OBJECT(cr)->isRemoteComposing();
}

LinphoneCore *linphone_chat_room_get_lc (const LinphoneChatRoom *cr) {
	return linphone_chat_room_get_core(cr);
}

LinphoneCore *linphone_chat_room_get_core (const LinphoneChatRoom *cr) {
	return L_GET_CPP_PTR_FROM_C_OBJECT(cr)->getCore();
}

const LinphoneAddress *linphone_chat_room_get_peer_address (LinphoneChatRoom *cr) {
	if (cr->peerAddressCache) {
		linphone_address_unref(cr->peerAddressCache);
	}
	cr->peerAddressCache = linphone_address_new(L_GET_CPP_PTR_FROM_C_OBJECT(cr)->getPeerAddress().asString().c_str());
	return cr->peerAddressCache;
}

LinphoneChatMessage *linphone_chat_room_create_message (LinphoneChatRoom *cr, const char *message) {
	return L_GET_CPP_PTR_FROM_C_OBJECT(cr)->createMessage(message ? message : "");
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
	LinphoneCore *lc = linphone_chat_room_get_core(cr);
	linphone_chat_message_set_external_body_url(msg, external_body_url ? ms_strdup(external_body_url) : NULL);
	linphone_chat_message_set_time(msg, time);
	linphone_chat_message_set_is_secured(msg, FALSE);
	linphone_chat_message_set_state(msg, state);
	if (is_incoming) {
		linphone_chat_message_set_incoming(msg);
		linphone_chat_message_set_from_address(msg, linphone_chat_room_get_peer_address(cr));
		linphone_chat_message_set_to_address(msg, linphone_address_new(linphone_core_get_identity(lc)));
	} else {
		linphone_chat_message_set_outgoing(msg);
		linphone_chat_message_set_to_address(msg, linphone_chat_room_get_peer_address(cr));
		linphone_chat_message_set_from_address(msg, linphone_address_new(linphone_core_get_identity(lc)));
	}
	return msg;
}

void linphone_chat_room_send_message2 (
	LinphoneChatRoom *cr,
	LinphoneChatMessage *msg,
	LinphoneChatMessageStateChangedCb status_cb,
	void *ud
) {
	linphone_chat_message_set_message_state_changed_cb(msg, status_cb);
	linphone_chat_message_set_message_state_changed_cb_user_data(msg, ud);
	L_GET_CPP_PTR_FROM_C_OBJECT(cr)->sendMessage(msg);
}

void linphone_chat_room_send_chat_message_2 (LinphoneChatRoom *cr, LinphoneChatMessage *msg) {
	linphone_chat_message_ref(msg);
	L_GET_CPP_PTR_FROM_C_OBJECT(cr)->sendMessage(msg);
}

void linphone_chat_room_send_chat_message (LinphoneChatRoom *cr, LinphoneChatMessage *msg) {
	L_GET_CPP_PTR_FROM_C_OBJECT(cr)->sendMessage(msg);
}

uint32_t linphone_chat_room_get_char (const LinphoneChatRoom *cr) {
	if (linphone_core_realtime_text_enabled(linphone_chat_room_get_core(cr)))
		return static_cast<const LinphonePrivate::RealTimeTextChatRoom *>(L_GET_CPP_PTR_FROM_C_OBJECT(cr).get())->getChar();
	return 0;
}

void linphone_chat_room_compose (LinphoneChatRoom *cr) {
	L_GET_CPP_PTR_FROM_C_OBJECT(cr)->compose();
}

LinphoneCall *linphone_chat_room_get_call (const LinphoneChatRoom *cr) {
	if (linphone_core_realtime_text_enabled(linphone_chat_room_get_core(cr)))
		return static_cast<const LinphonePrivate::RealTimeTextChatRoom *>(L_GET_CPP_PTR_FROM_C_OBJECT(cr).get())->getCall();
	return nullptr;
}

void linphone_chat_room_set_call (LinphoneChatRoom *cr, LinphoneCall *call) {
	if (linphone_core_realtime_text_enabled(linphone_chat_room_get_core(cr)))
		L_GET_PRIVATE_FROM_C_OBJECT(cr, RealTimeTextChatRoom)->setCall(call);
}

bctbx_list_t *linphone_chat_room_get_transient_messages (const LinphoneChatRoom *cr) {
	return L_GET_C_LIST_FROM_CPP_LIST(L_GET_PRIVATE_FROM_C_OBJECT(cr)->getTransientMessages());
}

void linphone_chat_room_mark_as_read (LinphoneChatRoom *cr) {
	L_GET_CPP_PTR_FROM_C_OBJECT(cr)->markAsRead();
}

int linphone_chat_room_get_unread_messages_count (LinphoneChatRoom *cr) {
	return L_GET_CPP_PTR_FROM_C_OBJECT(cr)->getUnreadMessagesCount();
}

int linphone_chat_room_get_history_size (LinphoneChatRoom *cr) {
	return L_GET_CPP_PTR_FROM_C_OBJECT(cr)->getHistorySize();
}

void linphone_chat_room_delete_message (LinphoneChatRoom *cr, LinphoneChatMessage *msg) {
	L_GET_CPP_PTR_FROM_C_OBJECT(cr)->deleteMessage(msg);
}

void linphone_chat_room_delete_history (LinphoneChatRoom *cr) {
	L_GET_CPP_PTR_FROM_C_OBJECT(cr)->deleteHistory();
}

bctbx_list_t *linphone_chat_room_get_history_range (LinphoneChatRoom *cr, int startm, int endm) {
	return L_GET_C_LIST_FROM_CPP_LIST(L_GET_CPP_PTR_FROM_C_OBJECT(cr)->getHistoryRange(startm, endm));
}

bctbx_list_t *linphone_chat_room_get_history (LinphoneChatRoom *cr, int nb_message) {
	return L_GET_C_LIST_FROM_CPP_LIST(L_GET_CPP_PTR_FROM_C_OBJECT(cr)->getHistory(nb_message));
}

LinphoneChatMessage *linphone_chat_room_find_message (LinphoneChatRoom *cr, const char *message_id) {
	return L_GET_CPP_PTR_FROM_C_OBJECT(cr)->findMessage(message_id);
}

LinphoneChatRoomCbs *linphone_chat_room_get_callbacks (const LinphoneChatRoom *cr) {
	return cr->cbs;
}

LinphoneChatRoomState linphone_chat_room_get_state (const LinphoneChatRoom *cr) {
	return (LinphoneChatRoomState)L_GET_CPP_PTR_FROM_C_OBJECT(cr)->getState();
}

LinphoneParticipant *linphone_chat_room_add_participant (LinphoneChatRoom *cr, const LinphoneAddress *addr) {
	return L_GET_C_BACK_PTR(L_GET_CPP_PTR_FROM_C_OBJECT(cr)->addParticipant(
		*L_GET_CPP_PTR_FROM_C_OBJECT(addr), nullptr, false)
	);
}

void linphone_chat_room_add_participants (LinphoneChatRoom *cr, const bctbx_list_t *addresses) {
	L_GET_CPP_PTR_FROM_C_OBJECT(cr)->addParticipants(L_GET_RESOLVED_CPP_LIST_FROM_C_LIST(addresses, Address), nullptr, false);
}

bool_t linphone_chat_room_can_handle_participants (const LinphoneChatRoom *cr) {
	return L_GET_CPP_PTR_FROM_C_OBJECT(cr)->canHandleParticipants();
}

const LinphoneAddress *linphone_chat_room_get_conference_address (const LinphoneChatRoom *cr) {
	if (cr->conferenceAddressCache) {
		linphone_address_unref(cr->conferenceAddressCache);
	}
	auto addr = L_GET_CPP_PTR_FROM_C_OBJECT(cr)->getConferenceAddress();
	if (addr)
		cr->conferenceAddressCache = linphone_address_new(addr->asString().c_str());
	else
		cr->conferenceAddressCache = nullptr;
	return cr->conferenceAddressCache;
}

int linphone_chat_room_get_nb_participants (const LinphoneChatRoom *cr) {
	return L_GET_CPP_PTR_FROM_C_OBJECT(cr)->getNbParticipants();
}

bctbx_list_t *linphone_chat_room_get_participants (const LinphoneChatRoom *cr) {
	return L_GET_RESOLVED_C_LIST_FROM_CPP_LIST(L_GET_CPP_PTR_FROM_C_OBJECT(cr)->getParticipants());
}

void linphone_chat_room_remove_participant (LinphoneChatRoom *cr, LinphoneParticipant *participant) {
	L_GET_CPP_PTR_FROM_C_OBJECT(cr)->removeParticipant(L_GET_CPP_PTR_FROM_C_OBJECT(participant));
}

void linphone_chat_room_remove_participants (LinphoneChatRoom *cr, const bctbx_list_t *participants) {
	L_GET_CPP_PTR_FROM_C_OBJECT(cr)->removeParticipants(L_GET_RESOLVED_CPP_LIST_FROM_C_LIST(participants, Participant));
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

LinphoneChatRoom *linphone_chat_room_new (LinphoneCore *core, const LinphoneAddress *addr) {
	LinphoneChatRoom *cr = L_INIT(ChatRoom);
	if (linphone_core_realtime_text_enabled(core))
		L_SET_CPP_PTR_FROM_C_OBJECT(cr, make_shared<LinphonePrivate::RealTimeTextChatRoom>(core, *L_GET_CPP_PTR_FROM_C_OBJECT(addr)));
	else
		L_SET_CPP_PTR_FROM_C_OBJECT(cr, make_shared<LinphonePrivate::BasicChatRoom>(core, *L_GET_CPP_PTR_FROM_C_OBJECT(addr)));
	L_GET_PRIVATE_FROM_C_OBJECT(cr)->setState(LinphonePrivate::ChatRoom::State::Instantiated);
	L_GET_PRIVATE_FROM_C_OBJECT(cr)->setState(LinphonePrivate::ChatRoom::State::Created);
	return cr;
}

LinphoneChatRoom *_linphone_client_group_chat_room_new (LinphoneCore *core, const char *subject) {
	const char *factoryUri = linphone_core_get_conference_factory_uri(core);
	if (!factoryUri)
		return nullptr;
	LinphoneAddress *factoryAddr = linphone_address_new(factoryUri);
	LinphoneProxyConfig *proxy = linphone_core_lookup_known_proxy(core, factoryAddr);
	linphone_address_unref(factoryAddr);
	string from;
	if (proxy)
		from = L_GET_CPP_PTR_FROM_C_OBJECT(linphone_proxy_config_get_identity_address(proxy))->asString();
	if (from.empty())
		from = linphone_core_get_primary_contact(core);
	LinphonePrivate::Address me(from);
	LinphoneChatRoom *cr = L_INIT(ChatRoom);
	L_SET_CPP_PTR_FROM_C_OBJECT(cr, make_shared<LinphonePrivate::ClientGroupChatRoom>(core, me, subject ? subject : ""));
	L_GET_PRIVATE_FROM_C_OBJECT(cr)->setState(LinphonePrivate::ChatRoom::State::Instantiated);
	return cr;
}

/* DEPRECATED */
void linphone_chat_room_destroy (LinphoneChatRoom *cr) {
	linphone_chat_room_unref(cr);
}
