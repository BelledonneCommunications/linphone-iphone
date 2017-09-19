/*
 * c-chat-room-cbs.cpp
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


#include "linphone/api/c-chat-room-cbs.h"
#include "private.h"


struct _LinphoneChatRoomCbs {
	belle_sip_object_t base;
	void *userData;
	LinphoneChatRoomCbsIsComposingReceivedCb isComposingReceivedCb;
	LinphoneChatRoomCbsMessageReceivedCb messageReceivedCb;
	LinphoneChatRoomCbsStateChangedCb stateChangedCb;
	LinphoneChatRoomCbsUndecryptableMessageReceivedCb undecryptableMessageReceivedCb;
};

BELLE_SIP_DECLARE_VPTR_NO_EXPORT(LinphoneChatRoomCbs);

BELLE_SIP_DECLARE_NO_IMPLEMENTED_INTERFACES(LinphoneChatRoomCbs);

BELLE_SIP_INSTANCIATE_VPTR(LinphoneChatRoomCbs, belle_sip_object_t,
	NULL, // destroy
	NULL, // clone
	NULL, // marshal
	FALSE
);

LinphoneChatRoomCbs * linphone_chat_room_cbs_new (void) {
	return belle_sip_object_new(LinphoneChatRoomCbs);
}

LinphoneChatRoomCbs * linphone_chat_room_cbs_ref (LinphoneChatRoomCbs *cbs) {
	belle_sip_object_ref(cbs);
	return cbs;
}

void linphone_chat_room_cbs_unref (LinphoneChatRoomCbs *cbs) {
	belle_sip_object_unref(cbs);
}

void * linphone_chat_room_cbs_get_user_data (const LinphoneChatRoomCbs *cbs) {
	return cbs->userData;
}

void linphone_chat_room_cbs_set_user_data (LinphoneChatRoomCbs *cbs, void *ud) {
	cbs->userData = ud;
}

LinphoneChatRoomCbsIsComposingReceivedCb linphone_chat_room_cbs_get_is_composing_received (const LinphoneChatRoomCbs *cbs) {
	return cbs->isComposingReceivedCb;
}

void linphone_chat_room_cbs_set_is_composing_received (LinphoneChatRoomCbs *cbs, LinphoneChatRoomCbsIsComposingReceivedCb cb) {
	cbs->isComposingReceivedCb = cb;
}

LinphoneChatRoomCbsMessageReceivedCb linphone_chat_room_cbs_get_message_received (const LinphoneChatRoomCbs *cbs) {
	return cbs->messageReceivedCb;
}

void linphone_chat_room_cbs_set_message_received (LinphoneChatRoomCbs *cbs, LinphoneChatRoomCbsMessageReceivedCb cb) {
	cbs->messageReceivedCb = cb;
}

LinphoneChatRoomCbsStateChangedCb linphone_chat_room_cbs_get_state_changed (const LinphoneChatRoomCbs *cbs) {
	return cbs->stateChangedCb;
}

void linphone_chat_room_cbs_set_state_changed (LinphoneChatRoomCbs *cbs, LinphoneChatRoomCbsStateChangedCb cb) {
	cbs->stateChangedCb = cb;
}

LinphoneChatRoomCbsUndecryptableMessageReceivedCb linphone_chat_room_cbs_get_undecryptable_message_received (const LinphoneChatRoomCbs *cbs) {
	return cbs->undecryptableMessageReceivedCb;
}

void linphone_chat_room_cbs_set_undecryptable_message_received (LinphoneChatRoomCbs *cbs, LinphoneChatRoomCbsUndecryptableMessageReceivedCb cb) {
	cbs->undecryptableMessageReceivedCb = cb;
}
