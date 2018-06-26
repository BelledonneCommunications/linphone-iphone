/*
 * c-chat-message-cbs.h
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

#ifndef _L_C_CHAT_MESSAGE_CBS_H_
#define _L_C_CHAT_MESSAGE_CBS_H_

#include "linphone/api/c-callbacks.h"
#include "linphone/api/c-types.h"

// =============================================================================

#ifdef __cplusplus
	extern "C" {
#endif // ifdef __cplusplus

/**
 * @addtogroup chatroom
 * @{
 */

LinphoneChatMessageCbs *linphone_chat_message_cbs_new (void);

/**
 * Acquire a reference to the chat room callbacks object.
 * @param[in] cbs The chat room callbacks object
 * @return The same chat room callbacks object
**/
LINPHONE_PUBLIC LinphoneChatMessageCbs * linphone_chat_message_cbs_ref (LinphoneChatMessageCbs *cbs);

/**
 * Release reference to the chat room callbacks object.
 * @param[in] cr The chat room callbacks object
**/
LINPHONE_PUBLIC void linphone_chat_message_cbs_unref (LinphoneChatMessageCbs *cbs);

/**
 * Retrieve the user pointer associated with the chat room callbacks object.
 * @param[in] cr The chat room callbacks object
 * @return The user pointer associated with the chat room callbacks object
**/
LINPHONE_PUBLIC void * linphone_chat_message_cbs_get_user_data (const LinphoneChatMessageCbs *cbs);

/**
 * Assign a user pointer to the chat room callbacks object.
 * @param[in] cr The chat room callbacks object
 * @param[in] ud The user pointer to associate with the chat room callbacks object
**/
LINPHONE_PUBLIC void linphone_chat_message_cbs_set_user_data (LinphoneChatMessageCbs *cbs, void *ud);

/**
 * Get the message state changed callback.
 * @param[in] cbs #LinphoneChatMessageCbs object.
 * @return The current message state changed callback.
 */
LINPHONE_PUBLIC LinphoneChatMessageCbsMsgStateChangedCb linphone_chat_message_cbs_get_msg_state_changed (const LinphoneChatMessageCbs *cbs);

/**
 * Set the message state changed callback.
 * @param[in] cbs LinphoneChatMessageCbs object.
 * @param[in] cb The message state changed callback to be used.
 */
LINPHONE_PUBLIC void linphone_chat_message_cbs_set_msg_state_changed (LinphoneChatMessageCbs *cbs, LinphoneChatMessageCbsMsgStateChangedCb cb);

/**
 * Get the file transfer receive callback.
 * @param[in] cbs LinphoneChatMessageCbs object.
 * @return The current file transfer receive callback.
 */
LINPHONE_PUBLIC LinphoneChatMessageCbsFileTransferRecvCb linphone_chat_message_cbs_get_file_transfer_recv (const LinphoneChatMessageCbs *cbs);

/**
 * Set the file transfer receive callback.
 * @param[in] cbs LinphoneChatMessageCbs object.
 * @param[in] cb The file transfer receive callback to be used.
 */
LINPHONE_PUBLIC void linphone_chat_message_cbs_set_file_transfer_recv (LinphoneChatMessageCbs *cbs, LinphoneChatMessageCbsFileTransferRecvCb cb);

/**
  * Get the file transfer send callback.
  * @param[in] cbs LinphoneChatMessageCbs object.
  * @return The current file transfer send callback.
  */
LINPHONE_PUBLIC LinphoneChatMessageCbsFileTransferSendCb linphone_chat_message_cbs_get_file_transfer_send (const LinphoneChatMessageCbs *cbs);

/**
 * Set the file transfer send callback.
 * @param[in] cbs LinphoneChatMessageCbs object.
 * @param[in] cb The file transfer send callback to be used.
 */
LINPHONE_PUBLIC void linphone_chat_message_cbs_set_file_transfer_send (LinphoneChatMessageCbs *cbs, LinphoneChatMessageCbsFileTransferSendCb cb);

/**
 * Get the file transfer progress indication callback.
 * @param[in] cbs LinphoneChatMessageCbs object.
 * @return The current file transfer progress indication callback.
 */
LINPHONE_PUBLIC LinphoneChatMessageCbsFileTransferProgressIndicationCb linphone_chat_message_cbs_get_file_transfer_progress_indication (const LinphoneChatMessageCbs *cbs);

/**
 * Set the file transfer progress indication callback.
 * @param[in] cbs LinphoneChatMessageCbs object.
 * @param[in] cb The file transfer progress indication callback to be used.
 */
LINPHONE_PUBLIC void linphone_chat_message_cbs_set_file_transfer_progress_indication (LinphoneChatMessageCbs *cbs, LinphoneChatMessageCbsFileTransferProgressIndicationCb cb);

/**
 * Get the participant IMDN state changed callback.
 * @param[in] cbs #LinphoneChatMessageCbs object.
 * @return The current participant IMDN state changed callback.
 */
LINPHONE_PUBLIC LinphoneChatMessageCbsParticipantImdnStateChangedCb linphone_chat_message_cbs_get_participant_imdn_state_changed (const LinphoneChatMessageCbs *cbs);

/**
 * Set the participant IMDN state changed callback.
 * @param[in] cbs LinphoneChatMessageCbs object.
 * @param[in] cb The participant IMDN state changed callback to be used.
 */
LINPHONE_PUBLIC void linphone_chat_message_cbs_set_participant_imdn_state_changed (LinphoneChatMessageCbs *cbs, LinphoneChatMessageCbsParticipantImdnStateChangedCb cb);

/**
 * @}
 */

#ifdef __cplusplus
	}
#endif // ifdef __cplusplus

#endif // ifndef _L_C_CHAT_MESSAGE_CBS_H_
