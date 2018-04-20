/*
 * c-chat-room.h
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

#ifndef _L_C_CHAT_ROOM_H_
#define _L_C_CHAT_ROOM_H_

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
 * Acquire a reference to the chat room.
 * @param[in] cr The chat room.
 * @return The same chat room.
**/
LINPHONE_PUBLIC LinphoneChatRoom *linphone_chat_room_ref(LinphoneChatRoom *cr);

/**
 * Release reference to the chat room.
 * @param[in] cr The chat room.
**/
LINPHONE_PUBLIC void linphone_chat_room_unref(LinphoneChatRoom *cr);

/**
 * Retrieve the user pointer associated with the chat room.
 * @param[in] cr The chat room.
 * @return The user pointer associated with the chat room.
**/
LINPHONE_PUBLIC void *linphone_chat_room_get_user_data(const LinphoneChatRoom *cr);

/**
 * Assign a user pointer to the chat room.
 * @param[in] cr The chat room.
 * @param[in] ud The user pointer to associate with the chat room.
**/
LINPHONE_PUBLIC void linphone_chat_room_set_user_data(LinphoneChatRoom *cr, void *ud);

/**
 * Create a message attached to a dedicated chat room;
 * @param cr the chat room.
 * @param message text message, NULL if absent.
 * @return a new #LinphoneChatMessage
 */
LINPHONE_PUBLIC LinphoneChatMessage* linphone_chat_room_create_message(LinphoneChatRoom *cr,const char* message);

/**
 * Create a message attached to a dedicated chat room;
 * @param cr the chat room.
 * @param message text message, NULL if absent.
 * @param external_body_url the URL given in external body or NULL.
 * @param state the LinphoneChatMessage.State of the message.
 * @param time the time_t at which the message has been received/sent.
 * @param is_read TRUE if the message should be flagged as read, FALSE otherwise.
 * @param is_incoming TRUE if the message has been received, FALSE otherwise.
 * @return a new #LinphoneChatMessage
 * @deprecated Use #linphone_chat_room_create_message() instead. Deprecated since 2017-11-14.
 * @donotwrap
 */
LINPHONE_PUBLIC LinphoneChatMessage* linphone_chat_room_create_message_2(LinphoneChatRoom *cr, const char* message, const char* external_body_url, LinphoneChatMessageState state, time_t time, bool_t is_read, bool_t is_incoming);

 /**
 * Create a message attached to a dedicated chat room with a particular content.
 * Use #linphone_chat_room_send_message to initiate the transfer
 * @param cr the chat room.
 * @param initial_content #LinphoneContent initial content. #LinphoneCoreVTable.file_transfer_send is invoked later to notify file transfer progress and collect next chunk of the message if LinphoneContent.data is NULL.
 * @return a new #LinphoneChatMessage
 */
LINPHONE_PUBLIC LinphoneChatMessage* linphone_chat_room_create_file_transfer_message(LinphoneChatRoom *cr, LinphoneContent* initial_content);

/**
 * get peer address \link linphone_core_get_chat_room() associated to \endlink this #LinphoneChatRoom
 * @param cr #LinphoneChatRoom object
 * @return #LinphoneAddress peer address
 */
LINPHONE_PUBLIC const LinphoneAddress* linphone_chat_room_get_peer_address(LinphoneChatRoom *cr);

/**
 * get local address \link linphone_core_get_chat_room() associated to \endlink this #LinphoneChatRoom
 * @param cr #LinphoneChatRoom object
 * @return #LinphoneAddress local address
 */
LINPHONE_PUBLIC const LinphoneAddress* linphone_chat_room_get_local_address(LinphoneChatRoom *cr);


/**
 * Send a message to peer member of this chat room.
 * @deprecated Use linphone_chat_message_send() instead.
 * @param cr #LinphoneChatRoom object
 * @param msg message to be sent
 * @donotwrap
 */
LINPHONE_PUBLIC LINPHONE_DEPRECATED void linphone_chat_room_send_message(LinphoneChatRoom *cr, const char *msg);

/**
 * Send a message to peer member of this chat room.
 * @param[in] cr #LinphoneChatRoom object
 * @param[in] msg #LinphoneChatMessage object
 * The state of the message sending will be notified via the callbacks defined in the #LinphoneChatMessageCbs object that can be obtained
 * by calling linphone_chat_message_get_callbacks().
 * The #LinphoneChatMessage reference is transfered to the function and thus doesn't need to be unref'd by the application.
 * @deprecated Use linphone_chat_message_send() instead.
 * @donotwrap
 */
LINPHONE_PUBLIC LINPHONE_DEPRECATED void linphone_chat_room_send_chat_message(LinphoneChatRoom *cr, LinphoneChatMessage *msg);

/**
 * Used to receive a chat message when using async mechanism with IM encryption engine
 * @param[in] cr #LinphoneChatRoom object
 * @param[in] msg #LinphoneChatMessage object
 */
LINPHONE_PUBLIC void linphone_chat_room_receive_chat_message (LinphoneChatRoom *cr, LinphoneChatMessage *msg);

/**
 * Mark all messages of the conversation as read
 * @param[in] cr The #LinphoneChatRoom object corresponding to the conversation.
 */
LINPHONE_PUBLIC void linphone_chat_room_mark_as_read(LinphoneChatRoom *cr);

/**
 * Delete a message from the chat room history.
 * @param[in] cr The #LinphoneChatRoom object corresponding to the conversation.
 * @param[in] msg The #LinphoneChatMessage object to remove.
 */

LINPHONE_PUBLIC void linphone_chat_room_delete_message(LinphoneChatRoom *cr, LinphoneChatMessage *msg);

/**
 * Delete all messages from the history
 * @param[in] cr The #LinphoneChatRoom object corresponding to the conversation.
 */
LINPHONE_PUBLIC void linphone_chat_room_delete_history(LinphoneChatRoom *cr);

/**
 * Gets the number of messages in a chat room.
 * @param[in] cr The #LinphoneChatRoom object corresponding to the conversation for which size has to be computed
 * @return the number of messages.
 */
LINPHONE_PUBLIC int linphone_chat_room_get_history_size(LinphoneChatRoom *cr);

/**
 * Gets nb_message most recent messages from cr chat room, sorted from oldest to most recent.
 * @param[in] cr The #LinphoneChatRoom object corresponding to the conversation for which messages should be retrieved
 * @param[in] nb_message Number of message to retrieve. 0 means everything.
 * @return \bctbx_list{LinphoneChatMessage}
 */
LINPHONE_PUBLIC bctbx_list_t *linphone_chat_room_get_history (LinphoneChatRoom *cr,int nb_message);

/**
 * Gets the partial list of messages in the given range, sorted from oldest to most recent.
 * @param[in] cr The #LinphoneChatRoom object corresponding to the conversation for which messages should be retrieved
 * @param[in] begin The first message of the range to be retrieved. History most recent message has index 0.
 * @param[in] end The last message of the range to be retrieved. History oldest message has index of history size - 1 (use #linphone_chat_room_get_history_size to retrieve history size)
 * @return \bctbx_list{LinphoneChatMessage}
 */
LINPHONE_PUBLIC bctbx_list_t *linphone_chat_room_get_history_range(LinphoneChatRoom *cr, int begin, int end);

/**
 * Gets nb_events most recent chat message events from cr chat room, sorted from oldest to most recent.
 * @param[in] cr The #LinphoneChatRoom object corresponding to the conversation for which events should be retrieved
 * @param[in] nb_events Number of events to retrieve. 0 means everything.
 * @return \bctbx_list{LinphoneEventLog}
 */
LINPHONE_PUBLIC bctbx_list_t *linphone_chat_room_get_history_message_events (LinphoneChatRoom *cr, int nb_events);

/**
 * Gets the partial list of chat message events in the given range, sorted from oldest to most recent.
 * @param[in] cr The #LinphoneChatRoom object corresponding to the conversation for which events should be retrieved
 * @param[in] begin The first event of the range to be retrieved. History most recent event has index 0.
 * @param[in] end The last event of the range to be retrieved. History oldest event has index of history size - 1
 * @return \bctbx_list{LinphoneEventLog}
 */
LINPHONE_PUBLIC bctbx_list_t *linphone_chat_room_get_history_range_message_events (LinphoneChatRoom *cr, int begin, int end);

/**
 * Gets nb_events most recent events from cr chat room, sorted from oldest to most recent.
 * @param[in] cr The #LinphoneChatRoom object corresponding to the conversation for which events should be retrieved
 * @param[in] nb_events Number of events to retrieve. 0 means everything.
 * @return \bctbx_list{LinphoneEventLog}
 */
LINPHONE_PUBLIC bctbx_list_t *linphone_chat_room_get_history_events (LinphoneChatRoom *cr, int nb_events);

/**
 * Gets the partial list of events in the given range, sorted from oldest to most recent.
 * @param[in] cr The #LinphoneChatRoom object corresponding to the conversation for which events should be retrieved
 * @param[in] begin The first event of the range to be retrieved. History most recent event has index 0.
 * @param[in] end The last event of the range to be retrieved. History oldest event has index of history size - 1
 * @return \bctbx_list{LinphoneEventLog}
 */
LINPHONE_PUBLIC bctbx_list_t *linphone_chat_room_get_history_range_events (LinphoneChatRoom *cr, int begin, int end);

/**
 * Gets the number of events in a chat room.
 * @param[in] cr The #LinphoneChatRoom object corresponding to the conversation for which size has to be computed
 * @return the number of events.
 */
LINPHONE_PUBLIC int linphone_chat_room_get_history_events_size(LinphoneChatRoom *cr);

/**
 * Gets the last chat message sent or received in this chat room
 * @param[in] cr The #LinphoneChatRoom object corresponding to the conversation for which last message should be retrieved
 * @return the latest #LinphoneChatMessage
 */
LINPHONE_PUBLIC LinphoneChatMessage *linphone_chat_room_get_last_message_in_history(LinphoneChatRoom *cr);

/**
 * Gets the chat message sent or received in this chat room that matches the message_id
 * @param[in] cr The #LinphoneChatRoom object corresponding to the conversation for which the message should be retrieved
 * @param[in] message_id The id of the message to find
 * @return the #LinphoneChatMessage
 */
LINPHONE_PUBLIC LinphoneChatMessage * linphone_chat_room_find_message(LinphoneChatRoom *cr, const char *message_id);

/**
 * Notifies the destination of the chat message being composed that the user is typing a new message.
 * @param[in] cr The #LinphoneChatRoom object corresponding to the conversation for which a new message is being typed.
 */
LINPHONE_PUBLIC void linphone_chat_room_compose(LinphoneChatRoom *cr);

/**
 * Tells whether the remote is currently composing a message.
 * @param[in] cr The #LinphoneChatRoom object corresponding to the conversation.
 * @return TRUE if the remote is currently composing a message, FALSE otherwise.
 */
LINPHONE_PUBLIC bool_t linphone_chat_room_is_remote_composing(const LinphoneChatRoom *cr);

/**
 * Gets the number of unread messages in the chatroom.
 * @param[in] cr The #LinphoneChatRoom object corresponding to the conversation.
 * @return the number of unread messages.
 */
LINPHONE_PUBLIC int linphone_chat_room_get_unread_messages_count(LinphoneChatRoom *cr);

/**
 * Returns back pointer to #LinphoneCore object.
**/
LINPHONE_PUBLIC LinphoneCore* linphone_chat_room_get_core(const LinphoneChatRoom *cr);

/**
 * When realtime text is enabled #linphone_call_params_realtime_text_enabled, #LinphoneCoreIsComposingReceivedCb is call everytime a char is received from peer.
 * At the end of remote typing a regular #LinphoneChatMessage is received with committed data from #LinphoneCoreMessageReceivedCb.
 * @param[in] cr #LinphoneChatRoom object
 * @returns  RFC 4103/T.140 char
 */
LINPHONE_PUBLIC uint32_t linphone_chat_room_get_char(const LinphoneChatRoom *cr);

/**
 * Returns true if lime is available for given peer
 *
 * @return true if zrtp secrets have already been shared and ready to use
 */
LINPHONE_PUBLIC bool_t linphone_chat_room_lime_available(LinphoneChatRoom *cr);

/**
 * get Curent Call associated to this chatroom if any
 * To commit a message, use #linphone_chat_room_send_message
 * @param[in] room #LinphoneChatRomm
 * @returns #LinphoneCall or NULL.
 */
LINPHONE_PUBLIC LinphoneCall *linphone_chat_room_get_call(const LinphoneChatRoom *room);

/**
 * Add a listener in order to be notified of #LinphoneChatRoom events. Once an event is received, registred #LinphoneChatRoomCbs are
 * invoked sequencially.
 * @param[in] call #LinphoneChatRoom object to monitor.
 * @param[in] cbs A #LinphoneChatRoomCbs object holding the callbacks you need. A reference is taken by the #LinphoneChatRoom until you invoke linphone_call_remove_callbacks().
 */
LINPHONE_PUBLIC void linphone_chat_room_add_callbacks(LinphoneChatRoom *cr, LinphoneChatRoomCbs *cbs);

/**
 * Remove a listener from a LinphoneChatRoom
 * @param[in] call LinphoneChatRoom object
 * @param[in] cbs LinphoneChatRoomCbs object to remove.
 */
LINPHONE_PUBLIC void linphone_chat_room_remove_callbacks(LinphoneChatRoom *cr, LinphoneChatRoomCbs *cbs);

/**
 * Gets the current LinphoneChatRoomCbs.
 * This is meant only to be called from a callback to be able to get the user_data associated with the LinphoneChatRoomCbs that is calling the callback.
 * @param[in] call LinphoneChatRoom object
 * @return The LinphoneChatRoomCbs that has called the last callback
 */
LINPHONE_PUBLIC LinphoneChatRoomCbs *linphone_chat_room_get_current_callbacks(const LinphoneChatRoom *cr);

/**
 * Get the state of the chat room.
 * @param[in] cr #LinphoneChatRoom object
 * @return The state of the chat room
 */
LINPHONE_PUBLIC LinphoneChatRoomState linphone_chat_room_get_state (const LinphoneChatRoom *cr);

/**
 * Return whether or not the chat room has been left.
 * @param[in] cr #LinphoneChatRoom object
 * @return whether or not the chat room has been left
 */
LINPHONE_PUBLIC bool_t linphone_chat_room_has_been_left (const LinphoneChatRoom *cr);

/**
 * Return the last updated time for the chat room
 * @param[in] cr LinphoneChatRoom object
 * @return the last updated time
 */
LINPHONE_PUBLIC time_t linphone_chat_room_get_last_update_time(const LinphoneChatRoom *cr);

/**
 * Add a participant to a chat room. This may fail if this type of chat room does not handle participants.
 * Use linphone_chat_room_can_handle_participants() to know if this chat room handles participants.
 * @param[in] cr A #LinphoneChatRoom object
 * @param[in] addr The address of the participant to add to the chat room
 */
LINPHONE_PUBLIC void linphone_chat_room_add_participant (LinphoneChatRoom *cr, const LinphoneAddress *addr);

/**
 * Add several participants to a chat room at once. This may fail if this type of chat room does not handle participants.
 * Use linphone_chat_room_can_handle_participants() to know if this chat room handles participants.
 * @param[in] cr A #LinphoneChatRoom object
 * @param[in] addresses \bctbx_list{LinphoneAddress}
 */
LINPHONE_PUBLIC void linphone_chat_room_add_participants (LinphoneChatRoom *cr, const bctbx_list_t *addresses);

/**
 * Tells whether a chat room is able to handle participants.
 * @param[in] cr A #LinphoneChatRoom object
 * @return A boolean value telling whether the chat room can handle participants or not
 */
LINPHONE_PUBLIC bool_t linphone_chat_room_can_handle_participants (const LinphoneChatRoom *cr);

/**
 * Find a participant of a chat room from its address.
 * @param[in] cr A #LinphoneChatRoom object
 * @param[in] addr The address to search in the list of participants of the chat room
 * @return The participant if found, NULL otherwise.
 */
LINPHONE_PUBLIC LinphoneParticipant *linphone_chat_room_find_participant (const LinphoneChatRoom *cr, const LinphoneAddress *addr);

/**
 * Get the capabilities of a chat room.
 * @param[in] cr A #LinphoneChatRoom object
 * @return The capabilities of the chat room
 */
LINPHONE_PUBLIC LinphoneChatRoomCapabilitiesMask linphone_chat_room_get_capabilities (const LinphoneChatRoom *cr);

/**
 * Check if a chat room has given capabilities.
 * @param[in] cr A #LinphoneChatRoom object
 * @param[in] mask A Capabilities mask
 * @return True if the mask matches, false otherwise
 */
LINPHONE_PUBLIC bool_t linphone_chat_room_has_capability(const LinphoneChatRoom *cr, int mask);

/**
 * Get the conference address of the chat room.
 * @param[in] cr A #LinphoneChatRoom object
 * @return The conference address of the chat room or NULL if this type of chat room is not conference based
 */
LINPHONE_PUBLIC const LinphoneAddress *linphone_chat_room_get_conference_address (const LinphoneChatRoom *cr);

/**
 * Get the participant representing myself in the chat room.
 * @param[in] cr A #LinphoneChatRoom object
 * @return The participant representing myself in the conference.
 */
LINPHONE_PUBLIC LinphoneParticipant *linphone_chat_room_get_me (const LinphoneChatRoom *cr);

/**
 * Get the number of participants in the chat room (that is without ourselves).
 * @param[in] cr A #LinphoneChatRoom object
 * @return The number of participants in the chat room
 */
LINPHONE_PUBLIC int linphone_chat_room_get_nb_participants (const LinphoneChatRoom *cr);

/**
 * Get the list of participants of a chat room.
 * @param[in] cr A #LinphoneChatRoom object
 * @return \bctbx_list{LinphoneParticipant}
 */
LINPHONE_PUBLIC bctbx_list_t * linphone_chat_room_get_participants (const LinphoneChatRoom *cr);

/**
 * Get the subject of a chat room.
 * @param[in] cr A #LinphoneChatRoom object
 * @return The subject of the chat room
 */
LINPHONE_PUBLIC const char * linphone_chat_room_get_subject (const LinphoneChatRoom *cr);

/**
 * Leave a chat room.
 * @param[in] cr A #LinphoneChatRoom object
 */
LINPHONE_PUBLIC void linphone_chat_room_leave (LinphoneChatRoom *cr);

/**
 * Remove a participant of a chat room.
 * @param[in] cr A #LinphoneChatRoom object
 * @param[in] participant The participant to remove from the chat room
 */
LINPHONE_PUBLIC void linphone_chat_room_remove_participant (LinphoneChatRoom *cr, LinphoneParticipant *participant);

/**
 * Remove several participants of a chat room at once.
 * @param[in] cr A #LinphoneChatRoom object
 * @param[in] participants \bctbx_list{LinphoneParticipant}
 */
LINPHONE_PUBLIC void linphone_chat_room_remove_participants (LinphoneChatRoom *cr, const bctbx_list_t *participants);

/**
 * Change the admin status of a participant of a chat room (you need to be an admin yourself to do this).
 * @param[in] cr A #LinphoneChatRoom object
 * @param[in] participant The Participant for which to change the admin status
 * @param[in] isAdmin A boolean value telling whether the participant should now be an admin or not
 */
LINPHONE_PUBLIC void linphone_chat_room_set_participant_admin_status (LinphoneChatRoom *cr, LinphoneParticipant *participant, bool_t isAdmin);

/**
 * Set the subject of a chat room.
 * @param[in] cr A #LinphoneChatRoom object
 * @param[in] subject The new subject to set for the chat room
 */
LINPHONE_PUBLIC void linphone_chat_room_set_subject (LinphoneChatRoom *cr, const char *subject);

/**
 * Gets the list of participants that are currently composing
 * @param[in] cr A #LinphoneChatRoom object
 * @return \bctbx_list{LinphoneAddress} list of addresses that are in the is_composing state
 */
LINPHONE_PUBLIC const bctbx_list_t * linphone_chat_room_get_composing_addresses(LinphoneChatRoom *cr);

/**
 * Set the conference address of a group chat room. This function needs to be called from the
 * #LinphoneChatRoomCbsConferenceAddressGenerationCb callback and only there.
 * @param[in] cr A #LinphoneChatRoom object
 * @param[in] confAddr The conference address to be used by the group chat room
 */
LINPHONE_PUBLIC void linphone_chat_room_set_conference_address (LinphoneChatRoom *cr, const LinphoneAddress *confAddr);

/**
 * Set the participant device. This function needs to be called from the
 * #LinphoneChatRoomCbsParticipantDeviceFetchRequestedCb callback and only there.
 * @param[in] cr A #LinphoneChatRoom object
 * @param[in] partAddr The participant address
 * @param[in] partDevices \bctbx_list{LinphoneAddress} list of the participant devices to be used by the group chat room
 */
LINPHONE_PUBLIC void linphone_chat_room_set_participant_devices (LinphoneChatRoom *cr, const LinphoneAddress *partAddr, const bctbx_list_t *partDevices);

/**
 * Add a participant device.
 * This is to used if a new device registers itself after the chat room creation.
 * @param[in] cr A #LinphoneChatRoom object
 * @param[in] participantAddress The address of the participant for which a new device is to be added
 * @param[in] deviceAddress The address of the new device to be added
 */
LINPHONE_PUBLIC void linphone_chat_room_add_participant_device (LinphoneChatRoom *cr, const LinphoneAddress *participantAddress, const LinphoneAddress *deviceAddress);

/**
 * Set the participant device. This function needs to be called from the
 * #LinphoneChatRoomCbsParticipantsCapabilitiesCheckedCb callback and only there.
 * @param[in] cr A #LinphoneChatRoom object
 * @param[in] deviceAddr The device address
 * @param[in] participantsCompatible \bctbx_list{LinphoneAddress}
 */
LINPHONE_PUBLIC void linphone_chat_room_add_compatible_participants (LinphoneChatRoom *cr, const LinphoneAddress *deviceAddr, const bctbx_list_t *participantsCompatible);

/**
 * Returns back pointer to #LinphoneCore object.
 * @deprecated use linphone_chat_room_get_core()
 * @donotwrap
**/
LINPHONE_PUBLIC LINPHONE_DEPRECATED LinphoneCore* linphone_chat_room_get_lc(const LinphoneChatRoom *cr);

/**
 * @}
 */

#ifdef __cplusplus
	}
#endif // ifdef __cplusplus

#endif // ifndef _L_C_CHAT_ROOM_H_
