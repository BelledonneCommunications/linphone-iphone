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


#include "linphone/chat.h"
#include "linphone/wrapper_utils.h"
#include "private.h"

#include "c-wrapper/c-tools.h"
#include "chat/basic-chat-room.h"
#include "chat/chat-room.h"
#include "chat/chat-room-p.h"
#include "chat/client-group-chat-room.h"
#include "chat/real-time-text-chat-room.h"
#include "chat/real-time-text-chat-room-p.h"

using namespace std;

#define GET_CPP_PTR(obj) L_GET_CPP_PTR_FROM_C_STRUCT(obj, ChatRoom, ChatRoom)
#define GET_CPP_PRIVATE_PTR(obj) L_GET_PRIVATE_FROM_C_STRUCT(obj, ChatRoom, ChatRoom)


static void _linphone_chat_room_constructor(LinphoneChatRoom *cr);
static void _linphone_chat_room_destructor(LinphoneChatRoom *cr);

L_DECLARE_C_STRUCT_IMPL_WITH_XTORS(ChatRoom, ChatRoom, chat_room,
	_linphone_chat_room_constructor, _linphone_chat_room_destructor,
	LinphoneAddress *peerAddressCache;
)

static void _linphone_chat_room_constructor(LinphoneChatRoom *cr) {}

static void _linphone_chat_room_destructor(LinphoneChatRoom *cr) {
	if (cr->peerAddressCache) {
		linphone_address_unref(cr->peerAddressCache);
		cr->peerAddressCache = nullptr;
	}
}


/*******************************************************************************
 * Public functions                                                            *
 ******************************************************************************/

void linphone_chat_room_release(LinphoneChatRoom *cr) {
	GET_CPP_PRIVATE_PTR(cr)->release();
}

void linphone_chat_room_remove_transient_message(LinphoneChatRoom *cr, LinphoneChatMessage *msg) {
	GET_CPP_PRIVATE_PTR(cr)->removeTransientMessage(msg);
}

void linphone_chat_room_send_message(LinphoneChatRoom *cr, const char *msg) {
	GET_CPP_PTR(cr)->sendMessage(GET_CPP_PTR(cr)->createMessage(msg));
}

bool_t linphone_chat_room_is_remote_composing(const LinphoneChatRoom *cr) {
	return GET_CPP_PTR(cr)->isRemoteComposing();
}

LinphoneCore *linphone_chat_room_get_lc(const LinphoneChatRoom *cr) {
	return linphone_chat_room_get_core(cr);
}

LinphoneCore *linphone_chat_room_get_core(const LinphoneChatRoom *cr) {
	return GET_CPP_PTR(cr)->getCore();
}

const LinphoneAddress *linphone_chat_room_get_peer_address(LinphoneChatRoom *cr) {
	if (cr->peerAddressCache) {
		linphone_address_unref(cr->peerAddressCache);
	}
	cr->peerAddressCache = linphone_address_new(GET_CPP_PTR(cr)->getPeerAddress().asString().c_str());
	return cr->peerAddressCache;
}

LinphoneChatMessage *linphone_chat_room_create_message(LinphoneChatRoom *cr, const char *message) {
	return GET_CPP_PTR(cr)->createMessage(message ? message : "");
}

LinphoneChatMessage *linphone_chat_room_create_message_2(LinphoneChatRoom *cr, const char *message,
	const char *external_body_url, LinphoneChatMessageState state,
	time_t time, bool_t is_read, bool_t is_incoming) {
	LinphoneChatMessage *msg = linphone_chat_room_create_message(cr, message);
	LinphoneCore *lc = linphone_chat_room_get_core(cr);
	msg->external_body_url = external_body_url ? ms_strdup(external_body_url) : NULL;
	msg->time = time;
	msg->is_secured = FALSE;
	linphone_chat_message_set_state(msg, state);
	if (is_incoming) {
		msg->dir = LinphoneChatMessageIncoming;
		linphone_chat_message_set_from(msg, linphone_chat_room_get_peer_address(cr));
		msg->to = linphone_address_new(linphone_core_get_identity(lc)); /*direct assignment*/
	} else {
		msg->dir = LinphoneChatMessageOutgoing;
		linphone_chat_message_set_to(msg, linphone_chat_room_get_peer_address(cr));
		msg->from = linphone_address_new(linphone_core_get_identity(lc));/*direct assignment*/
	}
	return msg;
}

void linphone_chat_room_send_message2(LinphoneChatRoom *cr, LinphoneChatMessage *msg,
									  LinphoneChatMessageStateChangedCb status_cb, void *ud) {
	msg->message_state_changed_cb = status_cb;
	msg->message_state_changed_user_data = ud;
	GET_CPP_PTR(cr)->sendMessage(msg);
}

void linphone_chat_room_send_chat_message_2(LinphoneChatRoom *cr, LinphoneChatMessage *msg) {
	linphone_chat_message_ref(msg);
	GET_CPP_PTR(cr)->sendMessage(msg);
}

void linphone_chat_room_send_chat_message(LinphoneChatRoom *cr, LinphoneChatMessage *msg) {
	GET_CPP_PTR(cr)->sendMessage(msg);
}

uint32_t linphone_chat_room_get_char(const LinphoneChatRoom *cr) {
	if (linphone_core_realtime_text_enabled(linphone_chat_room_get_core(cr)))
		return static_cast<const LinphonePrivate::RealTimeTextChatRoom *>(GET_CPP_PTR(cr).get())->getChar();
	return 0;
}

void linphone_chat_room_compose(LinphoneChatRoom *cr) {
	GET_CPP_PTR(cr)->compose();
}

LinphoneCall *linphone_chat_room_get_call(const LinphoneChatRoom *cr) {
	if (linphone_core_realtime_text_enabled(linphone_chat_room_get_core(cr)))
		return static_cast<const LinphonePrivate::RealTimeTextChatRoom *>(GET_CPP_PTR(cr).get())->getCall();
	return nullptr;
}

void linphone_chat_room_set_call(LinphoneChatRoom *cr, LinphoneCall *call) {
	if (linphone_core_realtime_text_enabled(linphone_chat_room_get_core(cr)))
		static_cast<LinphonePrivate::RealTimeTextChatRoomPrivate *>(GET_CPP_PRIVATE_PTR(cr))->setCall(call);
}

bctbx_list_t * linphone_chat_room_get_transient_messages(const LinphoneChatRoom *cr) {
	return L_GET_C_LIST_FROM_CPP_LIST(GET_CPP_PRIVATE_PTR(cr)->getTransientMessages(), LinphoneChatMessage);
}

void linphone_chat_room_mark_as_read(LinphoneChatRoom *cr) {
	GET_CPP_PTR(cr)->markAsRead();
}

int linphone_chat_room_get_unread_messages_count(LinphoneChatRoom *cr) {
	return GET_CPP_PTR(cr)->getUnreadMessagesCount();
}

int linphone_chat_room_get_history_size(LinphoneChatRoom *cr) {
	return GET_CPP_PTR(cr)->getHistorySize();
}

void linphone_chat_room_delete_message(LinphoneChatRoom *cr, LinphoneChatMessage *msg) {
	GET_CPP_PTR(cr)->deleteMessage(msg);
}

void linphone_chat_room_delete_history(LinphoneChatRoom *cr) {
	GET_CPP_PTR(cr)->deleteHistory();
}

bctbx_list_t *linphone_chat_room_get_history_range(LinphoneChatRoom *cr, int startm, int endm) {
	return L_GET_C_LIST_FROM_CPP_LIST(GET_CPP_PTR(cr)->getHistoryRange(startm, endm), LinphoneChatMessage);
}

bctbx_list_t *linphone_chat_room_get_history(LinphoneChatRoom *cr, int nb_message) {
	return L_GET_C_LIST_FROM_CPP_LIST(GET_CPP_PTR(cr)->getHistory(nb_message), LinphoneChatMessage);
}

LinphoneChatMessage * linphone_chat_room_find_message(LinphoneChatRoom *cr, const char *message_id) {
	return GET_CPP_PTR(cr)->findMessage(message_id);
}


/*******************************************************************************
 * Reference and user data handling functions                                  *
 ******************************************************************************/

LinphoneChatRoom *linphone_chat_room_ref(LinphoneChatRoom *cr) {
	belle_sip_object_ref(cr);
	return cr;
}

void linphone_chat_room_unref(LinphoneChatRoom *cr) {
	belle_sip_object_unref(cr);
}

void * linphone_chat_room_get_user_data(const LinphoneChatRoom *cr) {
	return L_GET_USER_DATA_FROM_C_STRUCT(cr, ChatRoom, ChatRoom);
}

void linphone_chat_room_set_user_data(LinphoneChatRoom *cr, void *ud) {
	L_SET_USER_DATA_FROM_C_STRUCT(cr, ud, ChatRoom, ChatRoom);
}


/*******************************************************************************
 * Constructor and destructor functions                                        *
 ******************************************************************************/

LinphoneChatRoom * linphone_chat_room_new(LinphoneCore *core, const LinphoneAddress *addr) {
	LinphoneChatRoom *cr = _linphone_chat_room_init();
	if (linphone_core_realtime_text_enabled(core))
		L_SET_CPP_PTR_FROM_C_STRUCT(cr, std::make_shared<LinphonePrivate::RealTimeTextChatRoom>(core, *L_GET_CPP_PTR_FROM_C_STRUCT(addr, Address, Address)));
	else
		L_SET_CPP_PTR_FROM_C_STRUCT(cr, std::make_shared<LinphonePrivate::BasicChatRoom>(core, *L_GET_CPP_PTR_FROM_C_STRUCT(addr, Address, Address)));
	return cr;
}

LinphoneChatRoom * linphone_client_group_chat_room_new(LinphoneCore *lc, const bctbx_list_t *addresses) {
	const char *factoryUri = linphone_core_get_chat_conference_factory_uri(lc);
	if (!factoryUri)
		return nullptr;
	LinphoneAddress *factoryAddr = linphone_address_new(factoryUri);
	LinphoneProxyConfig *proxy = linphone_core_lookup_known_proxy(lc, factoryAddr);
	linphone_address_unref(factoryAddr);
	std::string from;
	if (proxy)
		from = L_GET_CPP_PTR_FROM_C_STRUCT(linphone_proxy_config_get_identity_address(proxy), Address, Address)->asString();
	if (from.empty())
		from = linphone_core_get_primary_contact(lc);
	LinphonePrivate::Address me(from);
	std::list<LinphonePrivate::Address> l = L_GET_CPP_LIST_OF_CPP_OBJ_FROM_C_LIST_OF_STRUCT_PTR(addresses, Address, Address);
	LinphoneChatRoom *cr = _linphone_chat_room_init();
	L_SET_CPP_PTR_FROM_C_STRUCT(cr, make_shared<LinphonePrivate::ClientGroupChatRoom>(lc, me, l));
	return cr;
}

/* DEPRECATED */
void linphone_chat_room_destroy(LinphoneChatRoom *cr) {
	linphone_chat_room_unref(cr);
}
