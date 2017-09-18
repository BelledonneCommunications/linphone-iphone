/*
 * c-chat-message.cpp
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
 #include "chat/chat-message.h"
 #include "chat/chat-message-p.h"
 
 using namespace std;
 
 #define GET_CPP_PTR(obj) L_GET_CPP_PTR_FROM_C_STRUCT(obj, ChatMessage, ChatMessage)
 #define GET_CPP_PRIVATE_PTR(obj) L_GET_PRIVATE_FROM_C_STRUCT(obj, ChatMessage, ChatMessage)

 /*******************************************************************************
 * Reference and user data handling functions                                  *
 ******************************************************************************/

LinphoneChatMessage *linphone_chat_message_ref(LinphoneChatMessage *msg) {
	belle_sip_object_ref(msg);
	return msg;
}

void linphone_chat_message_unref(LinphoneChatMessage *msg) {
	belle_sip_object_unref(msg);
}

void * linphone_chat_message_get_user_data(const LinphoneChatMessage *msg) {
	return L_GET_USER_DATA_FROM_C_STRUCT(msg, ChatMessage, ChatMessage);
}

void linphone_chat_message_set_user_data(LinphoneChatMessage *msg, void *ud) {
	L_SET_USER_DATA_FROM_C_STRUCT(msg, ud, ChatMessage, ChatMessage);
}