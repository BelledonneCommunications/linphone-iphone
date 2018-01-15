/*
 * c-chat-room-cbs.h
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

#ifndef _L_C_CHAT_ROOM_CBS_H_
#define _L_C_CHAT_ROOM_CBS_H_

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

/**
 * Acquire a reference to the chat room callbacks object.
 * @param[in] cbs The chat room callbacks object
 * @return The same chat room callbacks object
**/
LINPHONE_PUBLIC LinphoneChatRoomCbs * linphone_chat_room_cbs_ref (LinphoneChatRoomCbs *cbs);

/**
 * Release reference to the chat room callbacks object.
 * @param[in] cr The chat room callbacks object
**/
LINPHONE_PUBLIC void linphone_chat_room_cbs_unref (LinphoneChatRoomCbs *cbs);

/**
 * Retrieve the user pointer associated with the chat room callbacks object.
 * @param[in] cr The chat room callbacks object
 * @return The user pointer associated with the chat room callbacks object
**/
LINPHONE_PUBLIC void * linphone_chat_room_cbs_get_user_data (const LinphoneChatRoomCbs *cbs);

/**
 * Assign a user pointer to the chat room callbacks object.
 * @param[in] cr The chat room callbacks object
 * @param[in] ud The user pointer to associate with the chat room callbacks object
**/
LINPHONE_PUBLIC void linphone_chat_room_cbs_set_user_data (LinphoneChatRoomCbs *cbs, void *ud);

/**
 * Get the is-composing received callback.
 * @param[in] cbs LinphoneChatRoomCbs object.
 * @return The current is-composing received callback.
 */
LINPHONE_PUBLIC LinphoneChatRoomCbsIsComposingReceivedCb linphone_chat_room_cbs_get_is_composing_received (const LinphoneChatRoomCbs *cbs);

/**
 * Set the is-composing received callback.
 * @param[in] cbs LinphoneChatRoomCbs object.
 * @param[in] cb The is-composing received callback to be used.
 */
LINPHONE_PUBLIC void linphone_chat_room_cbs_set_is_composing_received (LinphoneChatRoomCbs *cbs, LinphoneChatRoomCbsIsComposingReceivedCb cb);

/**
 * Get the message received callback.
 * @param[in] cbs LinphoneChatRoomCbs object.
 * @return The current message received callback.
 */
LINPHONE_PUBLIC LinphoneChatRoomCbsMessageReceivedCb linphone_chat_room_cbs_get_message_received (const LinphoneChatRoomCbs *cbs);

/**
 * Set the message received callback.
 * @param[in] cbs LinphoneChatRoomCbs object.
 * @param[in] cb The message received callback to be used.
 */
LINPHONE_PUBLIC void linphone_chat_room_cbs_set_message_received (LinphoneChatRoomCbs *cbs, LinphoneChatRoomCbsMessageReceivedCb cb);

/**
 * Get the chat message received callback.
 * @param[in] cbs LinphoneChatRoomCbs object.
 * @return The current chat message received callback.
 */
LINPHONE_PUBLIC LinphoneChatRoomCbsChatMessageReceivedCb linphone_chat_room_cbs_get_chat_message_received (const LinphoneChatRoomCbs *cbs);

/**
 * Set the chat message received callback.
 * @param[in] cbs LinphoneChatRoomCbs object.
 * @param[in] cb The chat message received callback to be used.
 */
LINPHONE_PUBLIC void linphone_chat_room_cbs_set_chat_message_received (LinphoneChatRoomCbs *cbs, LinphoneChatRoomCbsChatMessageReceivedCb cb);

/**
 * Get the chat message sent callback.
 * @param[in] cbs LinphoneChatRoomCbs object.
 * @return The current chat message sent callback.
 */
LINPHONE_PUBLIC LinphoneChatRoomCbsChatMessageSentCb linphone_chat_room_cbs_get_chat_message_sent (const LinphoneChatRoomCbs *cbs);

/**
 * Set the chat message sent callback.
 * @param[in] cbs LinphoneChatRoomCbs object.
 * @param[in] cb The chat message sent callback to be used.
 */
LINPHONE_PUBLIC void linphone_chat_room_cbs_set_chat_message_sent (LinphoneChatRoomCbs *cbs, LinphoneChatRoomCbsChatMessageSentCb cb);

/**
 * Get the participant added callback.
 * @param[in] cbs LinphoneChatRoomCbs object.
 * @return The current participant added callback.
 */
LINPHONE_PUBLIC LinphoneChatRoomCbsParticipantAddedCb linphone_chat_room_cbs_get_participant_added (const LinphoneChatRoomCbs *cbs);

/**
 * Set the participant added callback.
 * @param[in] cbs LinphoneChatRoomCbs object.
 * @param[in] cb The participant added callback to be used.
 */
LINPHONE_PUBLIC void linphone_chat_room_cbs_set_participant_added (LinphoneChatRoomCbs *cbs, LinphoneChatRoomCbsParticipantAddedCb cb);

/**
 * Get the participant removed callback.
 * @param[in] cbs LinphoneChatRoomCbs object.
 * @return The current participant removed callback.
 */
LINPHONE_PUBLIC LinphoneChatRoomCbsParticipantRemovedCb linphone_chat_room_cbs_get_participant_removed (const LinphoneChatRoomCbs *cbs);

/**
 * Set the participant removed callback.
 * @param[in] cbs LinphoneChatRoomCbs object.
 * @param[in] cb The participant removed callback to be used.
 */
LINPHONE_PUBLIC void linphone_chat_room_cbs_set_participant_removed (LinphoneChatRoomCbs *cbs, LinphoneChatRoomCbsParticipantRemovedCb cb);

/**
 * Get the participant admin status changed callback.
 * @param[in] cbs LinphoneChatRoomCbs object.
 * @return The current participant admin status changed callback.
 */
LINPHONE_PUBLIC LinphoneChatRoomCbsParticipantAdminStatusChangedCb linphone_chat_room_cbs_get_participant_admin_status_changed (const LinphoneChatRoomCbs *cbs);

/**
 * Set the participant admin status changed callback.
 * @param[in] cbs LinphoneChatRoomCbs object.
 * @param[in] cb The participant admin status changed callback to be used.
 */
LINPHONE_PUBLIC void linphone_chat_room_cbs_set_participant_admin_status_changed (LinphoneChatRoomCbs *cbs, LinphoneChatRoomCbsParticipantAdminStatusChangedCb cb);

/**
 * Get the state changed callback.
 * @param[in] cbs LinphoneChatRoomCbs object.
 * @return The current state changed callback.
 */
LINPHONE_PUBLIC LinphoneChatRoomCbsStateChangedCb linphone_chat_room_cbs_get_state_changed (const LinphoneChatRoomCbs *cbs);

/**
 * Set the state changed callback.
 * @param[in] cbs LinphoneChatRoomCbs object.
 * @param[in] cb The state changed callback to be used.
 */
LINPHONE_PUBLIC void linphone_chat_room_cbs_set_state_changed (LinphoneChatRoomCbs *cbs, LinphoneChatRoomCbsStateChangedCb cb);

/**
 * Get the subject changed callback.
 * @param[in] cbs LinphoneChatRoomCbs object.
 * @return The current subject changed callback.
 */
LINPHONE_PUBLIC LinphoneChatRoomCbsSubjectChangedCb linphone_chat_room_cbs_get_subject_changed (const LinphoneChatRoomCbs *cbs);

/**
 * Set the subject changed callback.
 * @param[in] cbs LinphoneChatRoomCbs object.
 * @param[in] cb The subject changed callback to be used.
 */
LINPHONE_PUBLIC void linphone_chat_room_cbs_set_subject_changed (LinphoneChatRoomCbs *cbs, LinphoneChatRoomCbsSubjectChangedCb cb);

/**
 * Get the undecryptable message received callback.
 * @param[in] cbs LinphoneChatRoomCbs object.
 * @return The current undecryptable message received callback.
 */
LINPHONE_PUBLIC LinphoneChatRoomCbsUndecryptableMessageReceivedCb linphone_chat_room_cbs_get_undecryptable_message_received (const LinphoneChatRoomCbs *cbs);

/**
 * Set the undecryptable message received callback.
 * @param[in] cbs LinphoneChatRoomCbs object.
 * @param[in] cb The undecryptable message received callback to be used.
 */
LINPHONE_PUBLIC void linphone_chat_room_cbs_set_undecryptable_message_received (LinphoneChatRoomCbs *cbs, LinphoneChatRoomCbsUndecryptableMessageReceivedCb cb);

/**
 * Get the participant device added callback.
 * @param[in] cbs LinphoneChatRoomCbs object.
 * @return The current participant device added callback.
 */
LINPHONE_PUBLIC LinphoneChatRoomCbsParticipantDeviceAddedCb linphone_chat_room_cbs_get_participant_device_added (const LinphoneChatRoomCbs *cbs);

/**
 * Set the participant device added callback.
 * @param[in] cbs LinphoneChatRoomCbs object.
 * @param[in] cb The participant device added callback to be used.
 */
LINPHONE_PUBLIC void linphone_chat_room_cbs_set_participant_device_added (LinphoneChatRoomCbs *cbs, LinphoneChatRoomCbsParticipantDeviceAddedCb cb);

/**
 * Get the participant device removed callback.
 * @param[in] cbs LinphoneChatRoomCbs object.
 * @return The current participant device removed callback.
 */
LINPHONE_PUBLIC LinphoneChatRoomCbsParticipantDeviceRemovedCb linphone_chat_room_cbs_get_participant_device_removed (const LinphoneChatRoomCbs *cbs);

/**
 * Set the participant device removed callback.
 * @param[in] cbs LinphoneChatRoomCbs object.
 * @param[in] cb The participant device removed callback to be used.
 */
LINPHONE_PUBLIC void linphone_chat_room_cbs_set_participant_device_removed (LinphoneChatRoomCbs *cbs, LinphoneChatRoomCbsParticipantDeviceRemovedCb cb);

/**
 * Get the conference address generation callback.
 * @param[in] cbs LinphoneChatRoomCbs object
 * @return The current conference address generation callback
 */
LINPHONE_PUBLIC LinphoneChatRoomCbsConferenceAddressGenerationCb linphone_chat_room_cbs_get_conference_address_generation (const LinphoneChatRoomCbs *cbs);

/**
 * Set the conference address generation callback.
 * @param[in] cbs LinphoneChatRoomCbs object
 * @param[in] cb The conference address generation callback to be used
 */
LINPHONE_PUBLIC void linphone_chat_room_cbs_set_conference_address_generation (LinphoneChatRoomCbs *cbs, LinphoneChatRoomCbsConferenceAddressGenerationCb cb);

/**
 * Get the participant device getting callback.
 * @param[in] cbs LinphoneChatRoomCbs object
 * @return The participant device getting callback
 */
LINPHONE_PUBLIC LinphoneChatRoomCbsParticipantDeviceFetchedCb linphone_chat_room_cbs_get_participant_device_fetched (const LinphoneChatRoomCbs *cbs);

/**
 * Set the participant device getting callback.
 * @param[in] cbs LinphoneChatRoomCbs object
 * @param[in] cb The participant device getting callback to be used
 */
LINPHONE_PUBLIC void linphone_chat_room_cbs_set_participant_device_fetched (LinphoneChatRoomCbs *cbs, LinphoneChatRoomCbsParticipantDeviceFetchedCb cb);

/**
 * Get the participants capabilities callback.
 * @param[in] cbs LinphoneChatRoomCbs object
 * @return The participants capabilities getting callback
 */
LINPHONE_PUBLIC LinphoneChatRoomCbsParticipantsCapabilitiesCheckedCb linphone_chat_room_cbs_get_participants_capabilities_checked (const LinphoneChatRoomCbs *cbs);

/**
 * Set the participants capabilities callback.
 * @param[in] cbs LinphoneChatRoomCbs object
 * @param[in] cb The participants capabilities callback to be used
 */
LINPHONE_PUBLIC void linphone_chat_room_cbs_set_participants_capabilities_checked (LinphoneChatRoomCbs *cbs, LinphoneChatRoomCbsParticipantsCapabilitiesCheckedCb cb);

/**
 * @}
 */

#ifdef __cplusplus
	}
#endif // ifdef __cplusplus

#endif // ifndef _L_C_CHAT_ROOM_CBS_H_
