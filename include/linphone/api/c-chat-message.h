/*
 * c-chat-message.h
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

#ifndef _C_CHAT_MESSAGE_H_
#define _C_CHAT_MESSAGE_H_
 
#include "linphone/api/c-types.h"
 
// =============================================================================
 
#ifdef __cplusplus
    extern "C" {
#endif // ifdef __cplusplus
 
/**
 * @addtogroup chatmessage
 * @{
 */

/**
 * Acquire a reference to the chat message.
 * @param[in] cr The chat message.
 * @return The same chat message.
**/
LINPHONE_PUBLIC LinphoneChatMessage *linphone_chat_message_ref(LinphoneChatMessage *msg);

/**
 * Release reference to the chat message.
 * @param[in] cr The chat message.
**/
LINPHONE_PUBLIC void linphone_chat_message_unref(LinphoneChatMessage *msg);

/**
 * Retrieve the user pointer associated with the chat message.
 * @param[in] cr The chat message.
 * @return The user pointer associated with the chat message.
**/
LINPHONE_PUBLIC void *linphone_chat_message_get_user_data(const LinphoneChatMessage *msg);

/**
 * Assign a user pointer to the chat message.
 * @param[in] cr The chat message.
 * @param[in] ud The user pointer to associate with the chat message.
**/
LINPHONE_PUBLIC void linphone_chat_message_set_user_data(LinphoneChatMessage *msg, void *ud);

/**
 * @}
 */

#ifdef __cplusplus
}
#endif // ifdef __cplusplus

#endif // ifndef _C_CHAT_MESSAGE_H_